#include "geometry/MeshSweeper.h"
#include "P4.h"
#include <iostream>
#include <fstream>
#include <string>
#include <sstream>

enum KernelType {
	kCPU = 0,
	kOPENMP = 1,
	kTBB = 2,
	kCUDA = 3,
	kCL = 4,
	kGLSL = 5,
	kGLSLCompute = 6
};

enum DisplayStyle {
	kDisplayStyleWire,
	kDisplayStyleShaded,
	kDisplayStyleWireOnShaded
};

enum ShadingMode {
	kShadingMaterial,
	kShadingVaryingColor,
	kShadingInterleavedVaryingColor,
	kShadingFaceVaryingColor,
	kShadingPatchType,
	kShadingPatchDepth,
	kShadingPatchCoord,
	kShadingNormal
};

enum EndCap {
	kEndCapBilinearBasis = 0,
	kEndCapBSplineBasis,
	kEndCapGregoryBasis,
	kEndCapLegacyGregory
};

enum HudCheckBox {
	kHUD_CB_DISPLAY_CONTROL_MESH_EDGES,
	kHUD_CB_DISPLAY_CONTROL_MESH_VERTS,
	kHUD_CB_ANIMATE_VERTICES,
	kHUD_CB_DISPLAY_PATCH_COLOR,
	kHUD_CB_VIEW_LOD,
	kHUD_CB_FRACTIONAL_SPACING,
	kHUD_CB_PATCH_CULL,
	kHUD_CB_FREEZE,
	kHUD_CB_DISPLAY_PATCH_COUNTS,
	kHUD_CB_ADAPTIVE,
	kHUD_CB_SMOOTH_CORNER_PATCH,
	kHUD_CB_SINGLE_CREASE_PATCH,
	kHUD_CB_INF_SHARP_PATCH
};

OpenSubdiv::Osd::GLMeshInterface* g_mesh = NULL;

int g_currentShape = 0;

ObjAnim const* g_objAnim = 0;

int   g_frame = 0,
g_repeatCount = 0;
float g_animTime = 0;

// GUI variables
int   g_freeze = 0,
g_shadingMode = kShadingMaterial,
g_displayStyle = kDisplayStyleWireOnShaded;
bool g_adaptive = 1,g_dispVert,g_dispEdge;
int
g_endCap = kEndCapBSplineBasis,
g_smoothCornerPatch = 1,
g_singleCreasePatch = 1,
g_infSharpPatch = 1,
g_mbutton[3] = { 0, 0, 0 },
g_running = 1;

int   g_screenSpaceTess = 0,
g_fractionalSpacing = 0,
g_patchCull = 0,
g_displayPatchCounts = 0;

float g_rotate[2] = { 0, 0 },
g_dolly = 5,
g_pan[2] = { 0, 0 },
g_center[3] = { 0, 0, 0 },
g_size = 0;

bool  g_yup = false;

int   g_prev_x = 0,
g_prev_y = 0;

int   g_width = 1024,
g_height = 1024;

GLControlMeshDisplay g_controlMeshDisplay;

// performance
float g_cpuTime = 0;
float g_gpuTime = 0;
Stopwatch g_fpsTimer;

// geometry
std::vector<float> g_orgPositions;

int g_level = 2;
int g_tessLevel = 1;
int g_tessLevelMin = 1;
int g_kernel = kCPU;
float g_moveScale = 0.0f;

GLuint g_queries[2] = { 0, 0 };

GLuint g_transformUB = 0,
g_transformBinding = 0,
g_tessellationUB = 0,
g_tessellationBinding = 1,
g_lightingUB = 0,
g_lightingBinding = 2,
g_fvarArrayDataUB = 0,
g_fvarArrayDataBinding = 3;

struct TransformOsd {
	float ModelViewMatrix[16];
	float ProjectionMatrix[16];
	float ModelViewProjectionMatrix[16];
	float ModelViewInverseMatrix[16];
} g_transformData;

GLuint g_vao = 0;
int numLights = 0;

// XXX:
// this struct meant to be used as a stopgap entity until we fully implement
// face-varying stuffs into patch table.
//
struct FVarData
{
	FVarData() :
		textureBuffer(0), textureParamBuffer(0) {
	}
	~FVarData() {
		Release();
	}
	void Release() {
		if (textureBuffer)
			glDeleteTextures(1, &textureBuffer);
		textureBuffer = 0;
		if (textureParamBuffer)
			glDeleteTextures(1, &textureParamBuffer);
		textureParamBuffer = 0;
	}
	void Create(OpenSubdiv::Far::PatchTable const* patchTable,
		int fvarWidth, std::vector<float> const& fvarSrcData) {

		using namespace OpenSubdiv;

		Release();
		Far::ConstIndexArray indices = patchTable->GetFVarValues();

		// expand fvardata to per-patch array
		std::vector<float> data;
		data.reserve(indices.size() * fvarWidth);

		for (int fvert = 0; fvert < (int)indices.size(); ++fvert) {
			int index = indices[fvert] * fvarWidth;
			for (int i = 0; i < fvarWidth; ++i) {
				data.push_back(fvarSrcData[index++]);
			}
		}
		GLuint buffer;
		glGenBuffers(1, &buffer);
		glBindBuffer(GL_ARRAY_BUFFER, buffer);
		glBufferData(GL_ARRAY_BUFFER, data.size() * sizeof(float),
			&data[0], GL_STATIC_DRAW);

		glGenTextures(1, &textureBuffer);
		glBindTexture(GL_TEXTURE_BUFFER, textureBuffer);
		glTexBuffer(GL_TEXTURE_BUFFER, GL_R32F, buffer);
		glBindTexture(GL_TEXTURE_BUFFER, 0);
		glBindBuffer(GL_ARRAY_BUFFER, 0);

		glDeleteBuffers(1, &buffer);

		Far::ConstPatchParamArray fvarParam = patchTable->GetFVarPatchParams();
		glGenBuffers(1, &buffer);
		glBindBuffer(GL_ARRAY_BUFFER, buffer);
		glBufferData(GL_ARRAY_BUFFER, fvarParam.size() * sizeof(Far::PatchParam),
			&fvarParam[0], GL_STATIC_DRAW);

		glGenTextures(1, &textureParamBuffer);
		glBindTexture(GL_TEXTURE_BUFFER, textureParamBuffer);
		glTexBuffer(GL_TEXTURE_BUFFER, GL_RG32I, buffer);
		glBindTexture(GL_TEXTURE_BUFFER, 0);
		glBindBuffer(GL_ARRAY_BUFFER, 0);

		glDeleteBuffers(1, &buffer);
	}
	GLuint textureBuffer, textureParamBuffer;
} g_fvarData;

//------------------------------------------------------------------------------
static void
updateGeom() {

	std::vector<float> vertex, varying;

	int nverts = 0;
	int stride = (g_shadingMode == kShadingInterleavedVaryingColor ? 7 : 3);

	if (g_objAnim && g_currentShape == 0) {

		nverts = g_objAnim->GetShape()->GetNumVertices(),

			vertex.resize(nverts * stride);

		if (g_shadingMode == kShadingVaryingColor) {
			varying.resize(nverts * 4);
		}

		g_objAnim->InterpolatePositions(g_animTime, &vertex[0], stride);

		if (g_shadingMode == kShadingVaryingColor ||
			g_shadingMode == kShadingInterleavedVaryingColor) {

			const float* p = &g_objAnim->GetShape()->verts[0];
			for (int i = 0; i < nverts; ++i) {
				if (g_shadingMode == kShadingInterleavedVaryingColor) {
					int ofs = i * stride;
					vertex[ofs + 0] = p[1];
					vertex[ofs + 1] = p[2];
					vertex[ofs + 2] = p[0];
					vertex[ofs + 3] = 0.0f;
					p += 3;
				}
				if (g_shadingMode == kShadingVaryingColor) {
					varying.push_back(p[2]);
					varying.push_back(p[1]);
					varying.push_back(p[0]);
					varying.push_back(1);
					p += 3;
				}
			}
		}
	}
	else {

		nverts = (int)g_orgPositions.size() / 3;

		vertex.reserve(nverts * stride);

		if (g_shadingMode == kShadingVaryingColor) {
			varying.reserve(nverts * 4);
		}

		const float* p = &g_orgPositions[0];
		float r = sin(g_frame * 0.001f) * g_moveScale;
		for (int i = 0; i < nverts; ++i) {
			float ct = cos(p[2] * r);
			float st = sin(p[2] * r);
			vertex.push_back(p[0] * ct + p[1] * st);
			vertex.push_back(-p[0] * st + p[1] * ct);
			vertex.push_back(p[2]);
			if (g_shadingMode == kShadingInterleavedVaryingColor) {
				vertex.push_back(p[1]);
				vertex.push_back(p[2]);
				vertex.push_back(p[0]);
				vertex.push_back(1.0f);
			}
			else if (g_shadingMode == kShadingVaryingColor) {
				varying.push_back(p[2]);
				varying.push_back(p[1]);
				varying.push_back(p[0]);
				varying.push_back(1);
			}
			p += 3;
		}
	}

	g_mesh->UpdateVertexBuffer(&vertex[0], 0, nverts);

	if (g_shadingMode == kShadingVaryingColor)
		g_mesh->UpdateVaryingBuffer(&varying[0], 0, nverts);
	

	g_mesh->Refine();
	

	g_mesh->Synchronize();

}

void APIENTRY
MessageCallback(GLenum source,
	GLenum type,
	GLuint id,
	GLenum severity,
	GLsizei length,
	const GLchar* message,
	const void* userParam)
{
	// ignore non-significant error/warning codes
	if (id == 131169 || id == 131185 || id == 131218 || id == 131204) return;

	std::cout << "---------------" << std::endl;
	std::cout << "Debug message (" << id << "): " << message << std::endl;

	switch (source)
	{
	case GL_DEBUG_SOURCE_API:             std::cout << "Source: API"; break;
	case GL_DEBUG_SOURCE_WINDOW_SYSTEM:   std::cout << "Source: Window System"; break;
	case GL_DEBUG_SOURCE_SHADER_COMPILER: std::cout << "Source: Shader Compiler"; break;
	case GL_DEBUG_SOURCE_THIRD_PARTY:     std::cout << "Source: Third Party"; break;
	case GL_DEBUG_SOURCE_APPLICATION:     std::cout << "Source: Application"; break;
	case GL_DEBUG_SOURCE_OTHER:           std::cout << "Source: Other"; break;
	} std::cout << std::endl;

	switch (type)
	{
	case GL_DEBUG_TYPE_ERROR:               std::cout << "Type: Error"; break;
	case GL_DEBUG_TYPE_DEPRECATED_BEHAVIOR: std::cout << "Type: Deprecated Behaviour"; break;
	case GL_DEBUG_TYPE_UNDEFINED_BEHAVIOR:  std::cout << "Type: Undefined Behaviour"; break;
	case GL_DEBUG_TYPE_PORTABILITY:         std::cout << "Type: Portability"; break;
	case GL_DEBUG_TYPE_PERFORMANCE:         std::cout << "Type: Performance"; break;
	case GL_DEBUG_TYPE_MARKER:              std::cout << "Type: Marker"; break;
	case GL_DEBUG_TYPE_PUSH_GROUP:          std::cout << "Type: Push Group"; break;
	case GL_DEBUG_TYPE_POP_GROUP:           std::cout << "Type: Pop Group"; break;
	case GL_DEBUG_TYPE_OTHER:               std::cout << "Type: Other"; break;
	} std::cout << std::endl;

	switch (severity)
	{
	case GL_DEBUG_SEVERITY_HIGH:         std::cout << "Severity: high"; break;
	case GL_DEBUG_SEVERITY_MEDIUM:       std::cout << "Severity: medium"; break;
	case GL_DEBUG_SEVERITY_LOW:          std::cout << "Severity: low"; break;
	case GL_DEBUG_SEVERITY_NOTIFICATION: std::cout << "Severity: notification"; break;
	} std::cout << std::endl;
	std::cout << std::endl;
}
MeshMap P4::_defaultMeshes;

inline auto
normalize(const vec4f& p)
{
  return vec3f{p} * math::inverse(p.w);
}

inline auto
viewportToNDC(int x, int y)
{
  GLint v[4];

  glGetIntegerv(GL_VIEWPORT, v);

  const auto xn = float(x - v[0]) * 2.0f / float(v[2]) - 1.0f;
  const auto yn = float(y - v[1]) * 2.0f / float(v[3]) - 1.0f;

  return vec4f{xn, yn, -1, 1};
}

inline Ray
P4::makeRay(int x, int y) const
{
  auto c = _editor->camera();
  mat4f m{vpMatrix(c)};

  m.invert();

  auto p = normalize(m * viewportToNDC(x, height() - y));
  auto t = c->transform();
  Ray r{t->position(), -t->forward()};

  if (c->projectionType() == Camera::Perspective)
    r.direction = (p - r.origin).versor();
  else
    r.origin = p - r.direction * c->nearPlane();
  return r;
}

inline void
P4::buildDefaultMeshes()
{
  _defaultMeshes["None"] = nullptr;
  _defaultMeshes["Box"] = GLGraphics3::box();
  _defaultMeshes["Sphere"] = GLGraphics3::sphere();
}

inline Primitive*
makePrimitive(MeshMapIterator mit)
{
  return new Primitive(mit->second, mit->first);
}

inline void
P4::buildScene()
{
	_current = _scene = new Scene{ "Scene 1" };
	_editor = new SceneEditor{ *_scene };
	_editor->setDefaultView((float)width() / (float)height());
	Reference<SceneObject> sceneObject;
	std::string name{ "Camera " + std::to_string(_sceneObjectCounter++) };
	sceneObject = new SceneObject{ name.c_str(), _scene };
	sceneObject->setParent(nullptr, true);
	auto c = new Camera;
	sceneObject->add(c);
	c->transform()->translate(vec3f{ 0,0,3 });
	Camera::setCurrent(c);
	auto o = new SceneObject{ "Directional Light", _scene };
	o->add(new Light);
	o->transform()->translate({ 0,10,0 });
	o->setParent(nullptr, true);
	for (int i = 0; i < 5; i++) {
		std::string name{ "Box " + std::to_string(_sceneObjectCounter++) };
		sceneObject = new SceneObject{ name.c_str(), _scene };
		sceneObject->setParent(nullptr, true);
		sceneObject->add(makePrimitive(_defaultMeshes.find("Box")));
		for (int j = 0; j < 5; j++) {
			std::string name{ "Object " + std::to_string(_sceneObjectCounter++) };
			sceneObject->add(new SceneObject{ name.c_str(), _scene });
		}
	}
}

void
P4::initialize()
{
	//Application::loadShaders(_program, "shaders/p3.vs", "shaders/p3.fs");
	Application::loadShaders(_programG, "shaders/p3G.vert", "shaders/p3G.frag");
	Application::loadShaders(_programP, "shaders/p3F.vert", "shaders/p3F.frag");
	Assets::initialize();
	initShapes();
  buildDefaultMeshes();
  buildScene();
  _renderer = new GLRenderer{*_scene};
	_renderer->setProgram(&_programP);
  _rayTracer = new RayTracer{*_scene};
  glEnable(GL_DEPTH_TEST);
  glEnable(GL_POLYGON_OFFSET_FILL);
  glPolygonOffset(1.0f, 1.0f);
  glEnable(GL_LINE_SMOOTH);
	//init gl
	glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
	glCullFace(GL_BACK);
	glEnable(GL_CULL_FACE);
	// During init, enable debug output
	glEnable(GL_DEBUG_OUTPUT);
	glDebugMessageCallback(MessageCallback, 0);
	glGenQueries(2, g_queries);
	glGenVertexArrays(1, &g_vao);
  _programG.use();
}

void
P4::dragDrop(SceneNode* sceneObject)
{
	if (ImGui::BeginDragDropSource())
	{
		ImGui::SetDragDropPayload("SceneNode", &sceneObject, sizeof(&sceneObject));
		ImGui::EndDragDropSource();
	}

	if (ImGui::BeginDragDropTarget())
	{
		if (auto * payload = ImGui::AcceptDragDropPayload("SceneNode"))
		{
			auto data = *(SceneNode * *)payload->Data;
			if (auto source = dynamic_cast<SceneObject*>(data))
			{
				if (dynamic_cast<Scene*>(sceneObject))
					source->setParent(nullptr);
				else if (auto target = dynamic_cast<SceneObject*>(sceneObject))
					if (!source->childrenContain(target))
						source->setParent(target);
			}
		}
		ImGui::EndDragDropTarget();
	}
}

void
P4::treeChildren(SceneNode* obj)
{
	auto open = false;
	ImGuiTreeNodeFlags flag{ ImGuiTreeNodeFlags_OpenOnArrow };
	auto s = dynamic_cast<Scene*>(obj);
	auto o = dynamic_cast<SceneObject*>(obj);

	if (s)
	{
		open = ImGui::TreeNodeEx(_scene,
			_current == _scene ? flag | ImGuiTreeNodeFlags_Selected : flag,
			_scene->name());

		if (ImGui::IsItemClicked())
			_current = obj;

		dragDrop(obj);

		if (open)
		{
			auto it = _scene->getRootIt();
			auto end = _scene->getRootEnd();
			auto size = _scene->getRootSize();
			for (; it != end; it++)
			{
				treeChildren(*it);
				if (_scene->getRootSize() != size) break; //Desculpa, eu não sou capaz de fazer melhor
			}
			ImGui::TreePop();
		}
	}
	else if (o)
	{
		if (o->isChildrenEmpty())
		{
			flag = ImGuiTreeNodeFlags_Leaf | ImGuiTreeNodeFlags_NoTreePushOnOpen;
			ImGui::TreeNodeEx(o,
				_current == o ? flag | ImGuiTreeNodeFlags_Selected : flag,
				o->name());

		}
		else
		{
			open = ImGui::TreeNodeEx(o,
				_current == o ? flag | ImGuiTreeNodeFlags_Selected : flag,
				o->name());
		}

		if (ImGui::IsItemClicked())
			_current = obj;

		dragDrop(obj);
		if (open)
		{
			auto cIt = o->getChildrenIter();
			auto cEnd = o->getChildrenEnd();
			auto size = o->getChildrenSize();
			for (; cIt != cEnd; cIt++)
			{
				treeChildren(*cIt);
				if (o->getChildrenSize() != size) break; //Desculpa, eu não sou capaz de fazer melhor
			}
			ImGui::TreePop();
		}
	}
}

void
P4::addEmptyCurrent()
{
	std::string name{ "Object " + std::to_string(_sceneObjectCounter++) };
	auto sceneObject = new SceneObject{ name.c_str(), _scene };
	SceneObject* current = dynamic_cast<SceneObject*>(_current);
	sceneObject->setParent(current, true);
}

void
P4::addSphereCurrent()
{
	// TODONE: create a new sphere.
	std::string name{ "Sphere " + std::to_string(_sceneObjectCounter++) };
	auto sceneObject = new SceneObject{ name.c_str(), _scene };
	SceneObject* current = nullptr;
	current = dynamic_cast<SceneObject*>(_current);
	sceneObject->setParent(current, true);

	Component* primitive = dynamic_cast<Component*>(makePrimitive(_defaultMeshes.find("Sphere")));
	sceneObject->add(primitive);
}

void
P4::addBoxCurrent()
{
	// TODONE: create a new box.
	std::string name{ "Box " + std::to_string(_sceneObjectCounter++) };
	auto sceneObject = new SceneObject{ name.c_str(), _scene };
	SceneObject* current = nullptr;
	current = dynamic_cast<SceneObject*>(_current);
	sceneObject->setParent(current, true);

	Component* primitive = dynamic_cast<Component*>(makePrimitive(_defaultMeshes.find("Box")));
	sceneObject->add(primitive);
}

void
P4::addLightCurrent(Light::Type T)
{
	std::string name{};
	if(T == Light::Type::Directional)
		name = { "Directional Light " + std::to_string(_dirLightCounter++) };
	if (T == Light::Type::Spot)
		name = { "Spot Light " + std::to_string(_spotLightCounter++) };
	if (T == Light::Type::Point)
		name = { "Point Light " + std::to_string(_pointLightCounter++) };

	auto object = new SceneObject{ name.c_str(), _scene };
	SceneObject* current = dynamic_cast<SceneObject*>(_current);
	object->setParent(current, true);

	Light* light = new Light();
	light->setType(T);

	auto l = dynamic_cast<Component*>(light);
	object->add(l);
}
void
P4::removeCurrent() {
	if (_current != _scene && _current != nullptr)
	{
		SceneObject* sceneObject = dynamic_cast<SceneObject*>(_current);
		auto parent = sceneObject->parent();

		sceneObject->removeComponentRenderable();

		if (parent == nullptr) {
			_current = _scene;
			_scene->remove(sceneObject);
		}
		else
		{
			_current = parent;
			parent->remove(sceneObject);
		}
	}
}
inline void
P4::hierarchyWindow()
{
	ImGui::Begin("Hierarchy");
	ImGui::Text("Application average %.3f ms/frame (%.1f FPS)",
		1000.0 / (ImGui::GetIO().Framerate), (ImGui::GetIO().Framerate));
	ImGui::Separator();
	if (ImGui::Button("Create###object"))
		ImGui::OpenPopup("CreateObjectPopup");
	if (ImGui::BeginPopup("CreateObjectPopup"))
	{
		if (ImGui::MenuItem("Empty Object"))
		{
			addEmptyCurrent();
		}
		ImGui::SameLine();
		ImGui::TextColored({ 0.5,0.5,0.5,1 }, "Shortcut: 'Shift + E'");
		if (ImGui::BeginMenu("3D Object"))
		{
			if (ImGui::MenuItem("Box"))
			{
				addBoxCurrent();
			}
			ImGui::SameLine();
			ImGui::TextColored({ 0.5,0.5,0.5,1 }, "Shortcut: 'Shift + B'");
			if (ImGui::MenuItem("Sphere"))
			{
				addSphereCurrent();
			}
			ImGui::SameLine();
			ImGui::TextColored({ 0.5,0.5,0.5,1 }, "Shortcut: 'Shift + S'");
			ImGui::EndMenu();
		}
		if (ImGui::BeginMenu("Light"))
		{
			if (ImGui::MenuItem("Directional Light"))
			{
				// TODO: create a new directional light.
				addLightCurrent(Light::Type::Directional);
			}
			if (ImGui::MenuItem("Point Light"))
			{
				// TODO: create a new pontual light.
				addLightCurrent(Light::Type::Point);
			}
			if (ImGui::MenuItem("Spotlight"))
			{
				// TODO: create a new spotlight.
				addLightCurrent(Light::Type::Spot);
			}
			ImGui::EndMenu();
		}
		if (ImGui::MenuItem("Camera"))
		{
			// create an empty object to add the camera 
			std::string name{ "Camera " + std::to_string(_cameraCounter++) };
			auto object = new SceneObject{ name.c_str(), _scene };
			SceneObject* current = dynamic_cast<SceneObject*>(_current);
			object->setParent(current, true);

			auto c = dynamic_cast<Component*>((Camera*) new Camera);
			object->add(c);

		}
		ImGui::EndPopup();
	}
	ImGui::SameLine();
	if (ImGui::Button("Delete"))
	{
		removeCurrent();
	}
	ImGui::SameLine();
	ImGui::TextColored({ 0.5,0.5,0.5,1 }, "Shortcut: 'Del'");
	ImGui::Separator();

	treeChildren((SceneNode*)_scene);
	ImGui::End();
}


namespace ImGui
{ // begin namespace ImGui

void
ObjectNameInput(NameableObject* object)
{
  const int bufferSize{128};
  static NameableObject* current;
  static char buffer[bufferSize];

  if (object != current)
  {
    strcpy_s(buffer, bufferSize, object->name());
    current = object;
  }
  if (ImGui::InputText("Name", buffer, bufferSize))
    object->setName(buffer);
}

inline bool
ColorEdit3(const char* label, Color& color)
{
  return ImGui::ColorEdit3(label, (float*)&color);
}

inline bool
DragVec3(const char* label, vec3f& v)
{
  return DragFloat3(label, (float*)&v, 0.1f, 0.0f, 0.0f, "%.2g");
}

void inline
limitValues(vec3f& t)
{
	t.x = t.x < 0.001 ? 0.001f : t.x;
	t.y = t.y < 0.001 ? 0.001f : t.y;
	t.z = t.z < 0.001 ? 0.001f : t.z;
}

void
TransformEdit(Transform* transform)
{
  vec3f temp;

  temp = transform->localPosition();
  if (ImGui::DragVec3("Position", temp))
    transform->setLocalPosition(temp);
  temp = transform->localEulerAngles();
  if (ImGui::DragVec3("Rotation", temp))
    transform->setLocalEulerAngles(temp);
  temp = transform->localScale();
	if (ImGui::DragVec3("Scale", temp))
	{
		limitValues(temp);
		transform->setLocalScale(temp);
	}
}

} // end namespace ImGui

inline void
P4::sceneGui()
{
  auto scene = (Scene*)_current;

  ImGui::ObjectNameInput(_current);
  ImGui::Separator();
  if (ImGui::CollapsingHeader("Colors"))
  {
    ImGui::ColorEdit3("Background", scene->backgroundColor);
    ImGui::ColorEdit3("Ambient Light", scene->ambientLight);
  }
}

inline void
P4::inspectShape(Primitive& primitive)
{
  char buffer[16];

  snprintf(buffer, 16, "%s", primitive.meshName());
  ImGui::InputText("Mesh", buffer, 16, ImGuiInputTextFlags_ReadOnly);
  if (ImGui::BeginDragDropTarget())
  {
    if (auto* payload = ImGui::AcceptDragDropPayload("PrimitiveMesh"))
    {
      auto mit = *(MeshMapIterator*)payload->Data;
      primitive.setMesh(mit->second, mit->first);
    }
    ImGui::EndDragDropTarget();
  }
  ImGui::SameLine();
  if (ImGui::Button("...###PrimitiveMesh"))
    ImGui::OpenPopup("PrimitiveMeshPopup");
  if (ImGui::BeginPopup("PrimitiveMeshPopup"))
  {
    auto& meshes = Assets::meshes();

    if (!meshes.empty())
    {
      for (auto mit = meshes.begin(); mit != meshes.end(); ++mit)
        if (ImGui::Selectable(mit->first.c_str()))
          primitive.setMesh(Assets::loadMesh(mit), mit->first);
      ImGui::Separator();
    }
    for (auto mit = _defaultMeshes.begin(); mit != _defaultMeshes.end(); ++mit)
      if (ImGui::Selectable(mit->first.c_str()))
        primitive.setMesh(mit->second, mit->first);
    ImGui::EndPopup();
  }
}

inline void
P4::inspectShape(OsdPrimitive& primitive)
{
	char buffer[16];

	snprintf(buffer, 16, "%s", primitive.meshName());
	ImGui::InputText("Mesh", buffer, 16, ImGuiInputTextFlags_ReadOnly);
	ImGui::SameLine();
	if (ImGui::Button("...###PrimitiveMesh"))
		ImGui::OpenPopup("PrimitiveMeshPopup");
	if (ImGui::BeginPopup("PrimitiveMeshPopup"))
	{
		auto& meshes = g_defaultShapes;

		if (!meshes.empty())
		{
			int i = 0;
			for (auto mit = meshes.begin(); mit != meshes.end(); ++mit, i++)
				if (ImGui::Selectable(mit->name.c_str()))
				{
					//rebuiild mesh
					g_currentShape = i;
					rebuildOsdMesh();
				}
			ImGui::Separator();
		}
		ImGui::EndPopup();
	}
}


inline void
P4::inspectMaterial(Material& material)
{
  ImGui::ColorEdit3("Ambient", material.ambient);
  ImGui::ColorEdit3("Diffuse", material.diffuse);
  ImGui::ColorEdit3("Spot", material.spot);
  ImGui::DragFloat("Shine", &material.shine, 1, 0, 1000.0f);
  ImGui::ColorEdit3("Specular", material.specular);
}

const char* items[] = {"0","1","2","3","4","5","6","7","8","9","10"};
static const char* current_item = items[g_level];
inline void
P4::inspectOsdParam(OsdPrimitive& primitive)
{
	auto prev = current_item;
	g_dispEdge = g_controlMeshDisplay.GetEdgesDisplay();
	g_dispVert = g_controlMeshDisplay.GetVerticesDisplay();
	if (ImGui::Checkbox("Control Edges", &g_dispEdge))
		g_controlMeshDisplay.SetEdgesDisplay(g_dispEdge);
	ImGui::SameLine();
	if (ImGui::Checkbox("Control Vert", &g_dispVert))
		g_controlMeshDisplay.SetVerticesDisplay(g_dispVert);
	ImGui::SameLine(); 
	if (ImGui::Checkbox("Adaptative", &g_adaptive))
		rebuildOsdMesh();
	if (ImGui::BeginCombo("Level", current_item))
	{
		for (int i = 0; i < 11; i++)
		{
			bool is_selected = (current_item == items[i]);
			if (ImGui::Selectable(items[i], is_selected))
				current_item = items[i];
			if (is_selected)
				ImGui::SetItemDefaultFocus();
		}
		ImGui::EndCombo();
		if (strcmp(current_item, prev) != 0) {
			g_level = atoi(current_item);
			rebuildOsdMesh();
		}
	}
}

inline void
P4::inspectPrimitive(Primitive& primitive)
{
  inspectShape(primitive);
  inspectMaterial(primitive.material);
}

void P4::inspectPrimitive(OsdPrimitive& primitive)
{
	inspectShape(primitive);
	inspectOsdParam(primitive);
}

inline void
P4::inspectLight(Light& light)
{
  static const char* lightTypes[]{"Directional", "Point", "Spot"};
  auto lt = light.type();

  if (ImGui::BeginCombo("Type", lightTypes[lt]))
  {
    for (auto i = 0; i < IM_ARRAYSIZE(lightTypes); ++i)
    {
      bool selected = lt == i;

      if (ImGui::Selectable(lightTypes[i], selected))
        lt = (Light::Type)i;
      if (selected)
        ImGui::SetItemDefaultFocus();
    }
    ImGui::EndCombo();
  }
  light.setType(lt);

	auto ed = light.decayExponent();
	auto oa = light.openningAngle();
	auto fl = light.decayValue();

	if (light.type() == Light::Spot || light.type() == Light::Point)
	{

		static const char* decayValues[]{ "None", "Linear", "Quadratic" };
		auto fl = light.decayValue();

		if (ImGui::BeginCombo("Fallof", decayValues[fl]))
		{
			for (auto i = 0; i < IM_ARRAYSIZE(decayValues); ++i)
			{
				bool selected = fl == i;

				if (ImGui::Selectable(decayValues[i], selected))
					fl = i;
				if (selected)
					ImGui::SetItemDefaultFocus();
			}
			ImGui::EndCombo();
		}
		light.setDecayValue(fl);

		if (light.type() == Light::Spot)
		{
			if (ImGui::DragInt("Decay exponent", &ed, 0.3f, 0))
				light.setDecayExponent(ed);

			if (ImGui::DragFloat("Opening angle", &oa, 1.0f, 0.0f, 90.0f))
				light.setOpeningAngle(oa);
		}
	}

  ImGui::ColorEdit3("Color", light.color);
}

void
P4::inspectCamera(Camera& camera)
{
  static const char* projectionNames[]{"Perspective", "Orthographic"};
  auto cp = camera.projectionType();

  if (ImGui::BeginCombo("Projection", projectionNames[cp]))
  {
    for (auto i = 0; i < IM_ARRAYSIZE(projectionNames); ++i)
    {
      auto selected = cp == i;

      if (ImGui::Selectable(projectionNames[i], selected))
        cp = (Camera::ProjectionType)i;
      if (selected)
        ImGui::SetItemDefaultFocus();
    }
    ImGui::EndCombo();
  }
  camera.setProjectionType(cp);
  if (cp == View3::Perspective)
  {
    auto fov = camera.viewAngle();

    if (ImGui::SliderFloat("View Angle",
      &fov,
      MIN_ANGLE,
      MAX_ANGLE,
      "%.0f deg",
      1.0f))
      camera.setViewAngle(fov <= MIN_ANGLE ? MIN_ANGLE : fov);
  }
  else
  {
    auto h = camera.height();

    if (ImGui::DragFloat("Height",
      &h,
      MIN_HEIGHT * 10.0f,
      MIN_HEIGHT,
      math::Limits<float>::inf()))
      camera.setHeight(h <= 0 ? MIN_HEIGHT : h);
  }

  float n;
  float f;

  camera.clippingPlanes(n, f);

  if (ImGui::DragFloatRange2("Clipping Planes",
    &n,
    &f,
    MIN_DEPTH,
    MIN_DEPTH,
    math::Limits<float>::inf(),
    "Near: %.2f",
    "Far: %.2f"))
  {
    if (n <= 0)
      n = MIN_DEPTH;
		if (f * 0.7f - n < MIN_DEPTH)
			f = (n + MIN_DEPTH) / 0.7f;
    camera.setClippingPlanes(n, f);
  }
}

inline void
P4::addComponentButton(SceneObject& object)
{
	if (ImGui::Button("Add Component"))
		ImGui::OpenPopup("AddComponentPopup");
	if (ImGui::BeginPopup("AddComponentPopup"))
	{
		if (ImGui::BeginMenu("Primitives"))
		{
			// TODONE

			if (ImGui::MenuItem("Box"))
			{
				Component* primitive = dynamic_cast<Component*>(makePrimitive(_defaultMeshes.find("Box")));
				object.add(primitive);
			}
			if (ImGui::MenuItem("Sphere"))
			{
				Component* primitive = dynamic_cast<Component*>(makePrimitive(_defaultMeshes.find("Sphere")));
				object.add(primitive);
			}
			ImGui::EndMenu();

		}
		if (ImGui::BeginMenu("Lights"))
		{

			if (ImGui::MenuItem("Point"))
			{
				// TODONE
				Light* light = new Light();
				light->setType(Light::Type::Point);
				Component* lightComponent = dynamic_cast<Component*>(light);
				object.add(lightComponent);
			}
			if (ImGui::MenuItem("Spot"))
			{
				// TODONE
				Light* light = new Light();
				light->setType(Light::Type::Spot);
				Component* lightComponent = dynamic_cast<Component*>(light);
				object.add(lightComponent);
			}
			if (ImGui::MenuItem("Directional"))
			{
				// TODONE
				Light* light = new Light();
				light->setType(Light::Type::Directional);
				Component* lightComponent = dynamic_cast<Component*>(light);
				object.add(lightComponent);
			}
			ImGui::EndMenu();
		}
		if (ImGui::MenuItem("Camera"))
		{
			// TODONE
			auto c = dynamic_cast<Component*>((Camera*) new Camera);
			object.add(c);
		}
		ImGui::EndPopup();
	}
}

inline void
P4::sceneObjectGui()
{
	auto object = (SceneObject*)_current;

	addComponentButton(*object);
	ImGui::Separator();
	ImGui::ObjectNameInput(object);
	ImGui::SameLine();
	ImGui::Checkbox("###visible", &object->visible);
	ImGui::Separator();

	// **End inspection of temporary components
	auto it = object->getComponentIter();
	auto end = object->getComponentEnd();

	for (; it != end; it++)
	{
		//Comparar com dynamic cast
		//auto casted = dynamic_cast<Transform*>((Component*)(*it));
		if (auto c = dynamic_cast<Transform*>((Component*)(*it)))//Se for Transform
		{
			if (ImGui::CollapsingHeader(object->transform()->typeName()))
			{
				auto t = object->transform();

				ImGui::TransformEdit(t);
				/*_transform = t->localToWorldMatrix();*/
			}
		}
		else if (auto p = dynamic_cast<Primitive*>((Component*)(*it)))//Se for primitive
		{
			auto notDelete{ true };
			auto open = ImGui::CollapsingHeader(p->typeName(), &notDelete);

			if (!notDelete)
			{
				// TODONE: delete primitive
				object->remove(p);
				it = object->getComponentIter();
				end = object->getComponentEnd();
			}
			else if (open)
				inspectPrimitive(*p);
		}
		else if (auto p = dynamic_cast<OsdPrimitive*>((Component*)(*it)))//Se for OsdPrimitive
		{
			auto notDelete{ true };
			auto open = ImGui::CollapsingHeader(p->typeName(), &notDelete);

			if (!notDelete)
			{
				// TODONE: delete primitive
				object->remove(p);
				it = object->getComponentIter();
				end = object->getComponentEnd();
			}
			else if (open)
				inspectPrimitive(*p);
		}
		else if (auto c = dynamic_cast<Camera*>((Component*)* it))//Se for camera
		{
			auto notDelete{ true };
			auto open = ImGui::CollapsingHeader(c->typeName(), &notDelete);

			if (!notDelete)
			{
				// TODONE: delete camera
				c->setCurrent(nullptr);
				object->remove(c);
				it = object->getComponentIter();
				end = object->getComponentEnd();
			}


			else if (open)
			{
				auto isCurrent = c == Camera::current();

				ImGui::Checkbox("Current", &isCurrent);
				Camera::setCurrent(isCurrent ? c : nullptr);
				if (Camera::current() == nullptr)
					_viewMode = ViewMode::Editor;
				inspectCamera(*c);
			}
		}
		else if (auto l = dynamic_cast<Light*>((Component*)* it))//Se for light
		{
			auto notDelete{ true };
			auto open = ImGui::CollapsingHeader(l->typeName(), &notDelete);

			if (!notDelete)
			{
				// TODONE: delete Light
				object->remove(l);
				it = object->getComponentIter();
				end = object->getComponentEnd();
			}


			else if (open)
			{
				inspectLight(*l);
			}
		}
	}
}

inline void
P4::objectGui()
{
  if (_current == nullptr)
    return;
  if (dynamic_cast<SceneObject*>(_current))
  {
    sceneObjectGui();
    return;
  }
  if (dynamic_cast<Scene*>(_current))
    sceneGui();
}

inline void
P4::inspectorWindow()
{
  ImGui::Begin("Inspector");
  objectGui();
  ImGui::End();
}

inline void
P4::editorViewGui()
{
  if (ImGui::Button("Set Default View"))
    _editor->setDefaultView(float(width()) / float(height()));
  ImGui::Separator();

  auto t = _editor->camera()->transform();
  vec3f temp;

  temp = t->localPosition();
  if (ImGui::DragVec3("Position", temp))
    t->setLocalPosition(temp);
  temp = t->localEulerAngles();
  if (ImGui::DragVec3("Rotation", temp))
    t->setLocalEulerAngles(temp);
  inspectCamera(*_editor->camera());
  ImGui::Separator();
  {
    static int sm;

    ImGui::Combo("Shading Mode", &sm, "None\0Flat\0Gouraud\0\0");
    // TODO

    static Color edgeColor;
    static bool showEdges;

    ImGui::ColorEdit3("Edges", edgeColor);
    ImGui::SameLine();
    ImGui::Checkbox("###showEdges", &showEdges);
  }
  ImGui::Separator();
  ImGui::Checkbox("Show Ground", &_editor->showGround);
}

inline void
P4::assetsWindow()
{
	if (!_showAssets)
		return;
  ImGui::Begin("Assets");
  if (ImGui::CollapsingHeader("Meshes"))
  {
    auto& meshes = Assets::meshes();

    for (auto mit = meshes.begin(); mit != meshes.end(); ++mit)
    {
      auto meshName = mit->first.c_str();
      auto selected = false;

      ImGui::Selectable(meshName, &selected);
      if (ImGui::BeginDragDropSource())
      {
        Assets::loadMesh(mit);
        ImGui::Text(meshName);
        ImGui::SetDragDropPayload("PrimitiveMesh", &mit, sizeof(mit));
        ImGui::EndDragDropSource();
      }
    }
  }
  ImGui::Separator();
  if (ImGui::CollapsingHeader("Textures"))
  {
    // next semester
  }
  ImGui::End();
}

inline void
P4::editorView()
{
  if (!_showEditorView)
    return;
  ImGui::Begin("Editor View Settings");
  editorViewGui();
  ImGui::End();
}

inline void
P4::fileMenu()
{
  if (ImGui::MenuItem("New"))
  {
    // TODO
  }
  if (ImGui::MenuItem("Open...", "Ctrl+O"))
  {
    // TODO
  }
  ImGui::Separator();
  if (ImGui::MenuItem("Save", "Ctrl+S"))
  {
    // TODO
  }
  if (ImGui::MenuItem("Save As..."))
  {
    // TODO
  }
  ImGui::Separator();
  if (ImGui::MenuItem("Exit", "Alt+F4"))
  {
    shutdown();
  }
}

inline bool
showStyleSelector(const char* label)
{
  static int style = 1;

  if (!ImGui::Combo(label, &style, "Classic\0Dark\0Light\0"))
    return false;
  switch (style)
  {
    case 0: ImGui::StyleColorsClassic();
      break;
    case 1: ImGui::StyleColorsDark();
      break;
    case 2: ImGui::StyleColorsLight();
      break;
  }
  return true;
}

inline void
P4::showOptions()
{
  ImGui::PushItemWidth(ImGui::GetWindowWidth() * 0.6f);
  showStyleSelector("Color Theme##Selector");
  ImGui::ColorEdit3("Selected Wireframe", _selectedWireframeColor);
  ImGui::PopItemWidth();
}

inline void
P4::mainMenu()
{
  if (ImGui::BeginMainMenuBar())
  {
    if (ImGui::BeginMenu("File"))
    {
      fileMenu();
      ImGui::EndMenu();
    }
    if (ImGui::BeginMenu("View"))
    {
      if (Camera::current() == 0)
        ImGui::MenuItem("Edit View", nullptr, true, false);
      else
      {
        static const char* viewLabels[]{"Editor", "Renderer"};

        if (ImGui::BeginCombo("View", viewLabels[_viewMode]))
        {
          for (auto i = 0; i < IM_ARRAYSIZE(viewLabels); ++i)
          {
            if (ImGui::Selectable(viewLabels[i], _viewMode == i))
              _viewMode = (ViewMode)i;
          }
          ImGui::EndCombo();
          // TODO: change mode only if scene has changed
          if (_viewMode == ViewMode::Editor)
            _image = nullptr;
        }
      }
      ImGui::Separator();
      ImGui::MenuItem("Assets Window", nullptr, &_showAssets);
      ImGui::MenuItem("Editor View Settings", nullptr, &_showEditorView);
      ImGui::EndMenu();
    }
    if (ImGui::BeginMenu("Tools"))
    {
      if (ImGui::BeginMenu("Options"))
      {
        showOptions();
        ImGui::EndMenu();
      }
      ImGui::EndMenu();
    }
		if (ImGui::BeginMenu("Scene Selector"))
		{

			if (ImGui::MenuItem("Original Scene"))
			{
				_sceneObjectCounter = 0;
				initOriginalScene();
				_viewMode = Editor;

			}
			if (ImGui::MenuItem("Scene 2"))
			{
				_sceneObjectCounter = 0;
				initScene2();
				_viewMode = Editor;

			}
			if (ImGui::MenuItem("Scene 3"))
			{
				_sceneObjectCounter = 0;
				initScene3();
				_viewMode = Editor;
			}
			if (ImGui::BeginMenu("RayTracer Focused"))
			{
				if (ImGui::MenuItem("Scene 1"))
				{
					_sceneObjectCounter = 0;
					initRayScene1();
					_viewMode = Editor;
				}
				if (ImGui::MenuItem("Scene 2 (191s to Render)"))
				{
					_sceneObjectCounter = 0;
					initRayScene2();
					_viewMode = Editor;
				}
				if (ImGui::MenuItem("Bat Paulo (5s to Render)"))
				{
					_sceneObjectCounter = 0;
					initRayScene0();
					_viewMode = Editor;
				}
				if (ImGui::MenuItem("Iron Paulo (85s to Render)"))
				{
					_sceneObjectCounter = 0;
					initRayScene();
					_viewMode = Editor;
				}
				ImGui::EndMenu();
			}
			ImGui::EndMenu();
			
		}
		if (ImGui::BeginMenu("OSD DEMO"))
		{
			if (ImGui::MenuItem("Scene 1"))
			{
				//TODO call osd scene 1
				_sceneObjectCounter = 0;
				initOsdScene1();
				_viewMode = Editor;
			}
			if (ImGui::MenuItem("Scene 2"))
			{
				//TODO call osd scene 2
				_sceneObjectCounter = 0;
				initOsdScene2();
				_viewMode = Editor;
			}
			ImGui::EndMenu();
		}
    ImGui::EndMainMenuBar();
  }
}

inline void
P4::initOsdScene1()
{
	//Initialize Osd and refine mesh
	{
		// Cube geometry from catmark_cube.h
		static float g_verts[8][3] = { { -0.5f, -0.5f,  0.5f },
																	{  0.5f, -0.5f,  0.5f },
																	{ -0.5f,  0.5f,  0.5f },
																	{  0.5f,  0.5f,  0.5f },
																	{ -0.5f,  0.5f, -0.5f },
																	{  0.5f,  0.5f, -0.5f },
																	{ -0.5f, -0.5f, -0.5f },
																	{  0.5f, -0.5f, -0.5f } };

		static int g_nverts = 8,
			g_nfaces = 6;

		static int g_vertsperface[6] = { 4, 4, 4, 4, 4, 4 };

		static int g_vertIndices[24] = { 0, 1, 3, 2,
																		 2, 3, 5, 4,
																		 4, 5, 7, 6,
																		 6, 7, 1, 0,
																		 1, 7, 5, 3,
																		 6, 0, 2, 4 };
		using namespace OpenSubdiv;
		// Populate a topology descriptor with our raw data

		typedef Far::TopologyDescriptor Descriptor;

		Sdc::SchemeType type = OpenSubdiv::Sdc::SCHEME_CATMARK;

		Sdc::Options options;
		options.SetVtxBoundaryInterpolation(Sdc::Options::VTX_BOUNDARY_EDGE_ONLY);

		Descriptor desc;
		desc.numVertices = g_nverts;
		desc.numFaces = g_nfaces;
		desc.numVertsPerFace = g_vertsperface;
		desc.vertIndicesPerFace = g_vertIndices;


		// Instantiate a Far::TopologyRefiner from the descriptor
		Far::TopologyRefiner* refiner = Far::TopologyRefinerFactory<Descriptor>::Create(desc,
			Far::TopologyRefinerFactory<Descriptor>::Options(type, options));

		// Uniformly refine the topology up to 'g_level'
		refiner->RefineUniform(Far::TopologyRefiner::UniformOptions(g_level));


		// Allocate a buffer for vertex primvar data. The buffer length is set to
		// be the sum of all children vertices up to the highest level of refinement.
		std::vector<OsdVertex> vbuffer(refiner->GetNumVerticesTotal());
		OsdVertex* verts = &vbuffer[0];


		// Initialize coarse mesh positions
		int nCoarseVerts = g_nverts;
		for (int i = 0; i < nCoarseVerts; ++i) {
			verts[i].SetPosition(g_verts[i][0], g_verts[i][1], g_verts[i][2]);
		}


		// Interpolate vertex primvar data
		Far::PrimvarRefiner primvarRefiner(*refiner);

		OsdVertex* src = verts;
		for (int level = 1; level <= g_level; ++level) {
			OsdVertex* dst = src + refiner->GetLevel(level - 1).GetNumVertices();
			primvarRefiner.Interpolate(level, src, dst);
			src = dst;
		}


		{ // Output OBJ for max level refined -----------

			std::ofstream file;
			std::stringstream path;
			path << "../../assets/meshes/testmeshLevel" << g_level << ".obj";
			file.open(path.str());
			Far::TopologyLevel const& refLastLevel = refiner->GetLevel(g_level);

			int nverts = refLastLevel.GetNumVertices();
			int nfaces = refLastLevel.GetNumFaces();

			// Print vertex positions
			int firstOfLastVerts = refiner->GetNumVerticesTotal() - nverts;

			for (int vert = 0; vert < nverts; ++vert) {
				float const* pos = verts[firstOfLastVerts + vert].GetPosition();
				printf("v %f %f %f\n", pos[0], pos[1], pos[2]);
				file << "v " << pos[0] << " " << pos[1] << " " << pos[2] << "\n";
			}

			// Print faces
			for (int face = 0; face < nfaces; ++face) {

				Far::ConstIndexArray fverts = refLastLevel.GetFaceVertices(face);

				// all refined Catmark faces should be quads
				assert(fverts.size() == 4);

				printf("f ");
				file << "f ";
				for (int vert = 0; vert < fverts.size(); ++vert) {
					printf("%d ", fverts[vert] + 1); // OBJ uses 1-based arrays...
					file << fverts[vert] + 1 << " ";
				}
				printf("\n");
				file << "\n";
			}
			file.close();

		}//end namespace opensubdiv
	}
	Assets::initialize();

	auto s = new Scene{ "Osd ToyExample" };
	_rayTracer = new RayTracer{ *s };
	_renderer = new GLRenderer{ *s };
	_renderer->setProgram(&_programP);
	glEnable(GL_DEPTH_TEST);
	glEnable(GL_POLYGON_OFFSET_FILL);
	glPolygonOffset(1.0f, 1.0f);
	glEnable(GL_LINE_SMOOTH);
	_programG.use();
	_current = s;
	_scene = s;

	// camera 0
	Reference<SceneObject> sceneObject;
	std::string name{ "Camera " + std::to_string(_sceneObjectCounter++) };
	sceneObject = new SceneObject{ name.c_str(), _scene };
	sceneObject->setParent(nullptr, true);
	auto c = new Camera;
	sceneObject->add(c);
	Camera::setCurrent(c);
	c->transform()->translate(vec3f{ -0.8f,1.3f,1.4f });
	c->transform()->rotate(vec3f{ -24,-31,0 });

	// directional light
	auto dl = new SceneObject{ "Directional Light", _scene };
	dl->setParent(nullptr, true);
	auto l1 = new Light();
	dl->transform()->setLocalPosition(vec3f{ 0,10,0 });
	dl->transform()->rotate(vec3f{ 50,30,0 });
	l1->setType(Light::Type::Directional);
	l1->setColor(Color::white);
	dl->add(l1);

	Reference<SceneObject> OsdMesh = new SceneObject{ "Sphere", _scene };
	OsdMesh->setParent(nullptr, true);

	auto& meshes = Assets::meshes();
	if (!meshes.empty())
	{
		for (auto mit = meshes.begin(); mit != meshes.end(); ++mit)
		{
			if (std::strcmp(mit->first.c_str(), "testmeshLevel1.obj") == 0)
			{
				Assets::loadMesh(mit);
				auto p = makePrimitive(mit);
				p->material.ambient.setRGB(51, 51, 51);
				p->material.diffuse.setRGB(199, 88, 88);
				p->material.spot.setRGB(255, 255, 0);
				p->material.specular.setRGB(150, 150, 150);
				OsdMesh->add(p);
			}
		}
	}
}

inline void
P4::rebuildOsdMesh()
{
	using namespace OpenSubdiv;
	ShapeDesc const& shapeDesc = g_defaultShapes[g_currentShape];
	auto it = _scene->getPrimitiveIter();
	auto end = _scene->getPrimitiveEnd();
	// itera nos primitivos
	for (; it != end; it++)
	
		 if (auto p = dynamic_cast<OsdPrimitive*>((Component*)*it))
			 p->setMeshName(g_defaultShapes[g_currentShape].name.c_str());

	int level = g_level;
	int kernel = g_kernel;
	bool doAnim = g_objAnim && g_currentShape == 0;

	Shape const* shape = 0;
	if (doAnim) {
		shape = g_objAnim->GetShape();
	}
	else {
		shape = Shape::parseObj(shapeDesc);
	}

	// create Far mesh (topology)
	Sdc::SchemeType sdctype = GetSdcType(*shape);
	Sdc::Options sdcoptions = GetSdcOptions(*shape);

	Far::TopologyRefiner* refiner =
		Far::TopologyRefinerFactory<Shape>::Create(*shape,
			Far::TopologyRefinerFactory<Shape>::Options(sdctype, sdcoptions));

	g_orgPositions = shape->verts;

	Osd::MeshBitset bits;
	bits.set(Osd::MeshAdaptive, g_adaptive != 0);
	bits.set(Osd::MeshUseSmoothCornerPatch, g_smoothCornerPatch != 0);
	bits.set(Osd::MeshUseSingleCreasePatch, g_singleCreasePatch != 0);
	bits.set(Osd::MeshUseInfSharpPatch, g_infSharpPatch != 0);
	bits.set(Osd::MeshInterleaveVarying, g_shadingMode == kShadingInterleavedVaryingColor);
	bits.set(Osd::MeshFVarData, g_shadingMode == kShadingFaceVaryingColor);
	bits.set(Osd::MeshEndCapBilinearBasis, g_endCap == kEndCapBilinearBasis);
	bits.set(Osd::MeshEndCapBSplineBasis, g_endCap == kEndCapBSplineBasis);
	bits.set(Osd::MeshEndCapGregoryBasis, g_endCap == kEndCapGregoryBasis);
	bits.set(Osd::MeshEndCapLegacyGregory, g_endCap == kEndCapLegacyGregory);

	int numVertexElements = 3;
	int numVaryingElements =
		(g_shadingMode == kShadingVaryingColor || bits.test(Osd::MeshInterleaveVarying)) ? 4 : 0;

	delete g_mesh;
	g_mesh = NULL;
	
	g_mesh = new Osd::Mesh<Osd::CpuGLVertexBuffer,
		Far::StencilTable,
		Osd::CpuEvaluator,
		Osd::GLPatchTable>(
			refiner,
			numVertexElements,
			numVaryingElements,
			level, bits);
	//// save coarse topology (used for coarse mesh drawing)
	g_controlMeshDisplay.SetTopology(refiner->GetLevel(0));
	if (g_shadingMode == kShadingFaceVaryingColor && shape->HasUV()) {

		std::vector<float> fvarData;

		InterpolateFVarData(*refiner, *shape, fvarData);

		// set fvardata to texture buffer
		g_fvarData.Create(g_mesh->GetFarPatchTable(),
			shape->GetFVarWidth(), fvarData);
	}

	if (!doAnim) {
		delete shape;
	}

	// compute model bounding
	float min[3] = { FLT_MAX,  FLT_MAX,  FLT_MAX };
	float max[3] = { -FLT_MAX, -FLT_MAX, -FLT_MAX };
	for (size_t i = 0; i < g_orgPositions.size() / 3; ++i) {
		for (int j = 0; j < 3; ++j) {
			float v = g_orgPositions[i * 3 + j];
			min[j] = std::min(min[j], v);
			max[j] = std::max(max[j], v);
		}
	}
	for (int j = 0; j < 3; ++j) {
		g_center[j] = (min[j] + max[j]) * 0.5f;
		g_size += (max[j] - min[j]) * (max[j] - min[j]);
	}
	g_size = sqrtf(g_size);

	g_tessLevelMin = 1;

	g_tessLevel = std::max(g_tessLevel, g_tessLevelMin);

	updateGeom();

	// -------- VAO
	glBindVertexArray(g_vao);

	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, g_mesh->GetPatchTable()->GetPatchIndexBuffer());
	glBindBuffer(GL_ARRAY_BUFFER, g_mesh->BindVertexBuffer());

	glEnableVertexAttribArray(0);

	if (g_shadingMode == kShadingVaryingColor) {
		glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(GLfloat) * 3, 0);
		glBindBuffer(GL_ARRAY_BUFFER, g_mesh->BindVaryingBuffer());
		glEnableVertexAttribArray(1);
		glVertexAttribPointer(1, 4, GL_FLOAT, GL_FALSE, sizeof(GLfloat) * 4, 0);
	}
	else if (g_shadingMode == kShadingInterleavedVaryingColor) {
		glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(GLfloat) * 7, 0);
		glEnableVertexAttribArray(1);
		glVertexAttribPointer(1, 4, GL_FLOAT, GL_FALSE, sizeof(GLfloat) * 7, (void*)(sizeof(GLfloat) * 3));
	}
	else {
		glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(GLfloat) * 3, 0);
		glDisableVertexAttribArray(1);
	}

	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glBindVertexArray(0);
}
inline void
P4::initOsdScene2()
{
	auto s = new Scene{ "Osd ToyExample2" };
	_rayTracer = new RayTracer{ *s };
	_renderer = new GLRenderer{ *s };
	_renderer->setProgram(&_programP);
	glEnable(GL_DEPTH_TEST);
	glEnable(GL_POLYGON_OFFSET_FILL);
	glPolygonOffset(1.0f, 1.0f);
	glEnable(GL_LINE_SMOOTH);
	_programG.use();
	_current = s;
	_scene = s;

	// camera 0
	Reference<SceneObject> sceneObject;
	std::string name{ "Camera " + std::to_string(_sceneObjectCounter++) };
	sceneObject = new SceneObject{ name.c_str(), _scene };
	sceneObject->setParent(nullptr, true);
	auto c = new Camera;
	sceneObject->add(c);
	Camera::setCurrent(c);
	c->transform()->translate(vec3f{ -0.8f,1.3f,1.4f });
	c->transform()->rotate(vec3f{ -24,-31,0 });

	// directional light
	auto dl = new SceneObject{ "Directional Light", _scene };
	dl->setParent(nullptr, true);
	auto l1 = new Light();
	dl->transform()->setLocalPosition(vec3f{ 0,10,0 });
	l1->setType(Light::Type::Directional);
	l1->setColor(Color::white);
	dl->add(l1);

	Reference<SceneObject> OsdMesh = new SceneObject{ "OsdMesh", _scene };
	OsdMesh->setParent(nullptr, true);
	rebuildOsdMesh();
	auto p = new OsdPrimitive{g_mesh,g_defaultShapes[g_currentShape].name.c_str()};
	OsdMesh->add(p);
	OsdMesh->transform()->rotate({ -90,0,0 });

	
}

inline void
P4::initOriginalScene()
{
	buildScene();
	_rayTracer = new RayTracer{ *_scene };
	_renderer = new GLRenderer{ *_scene };
	_renderer->setProgram(&_programP);
	glEnable(GL_DEPTH_TEST);
	glEnable(GL_POLYGON_OFFSET_FILL);
	glPolygonOffset(1.0f, 1.0f);
	glEnable(GL_LINE_SMOOTH);
	_programG.use();
}

inline void
P4::initScene2()
{
	auto s = new Scene{ "Scene 2" };
	_rayTracer = new RayTracer{ *s };
	_renderer = new GLRenderer{ *s };
	_renderer->setProgram(&_programP);
	glEnable(GL_DEPTH_TEST);
	glEnable(GL_POLYGON_OFFSET_FILL);
	glPolygonOffset(1.0f, 1.0f);
	glEnable(GL_LINE_SMOOTH);
	_programG.use();
	_current = s;
	_scene = s;

	Reference<SceneObject> sceneObject;
	std::string name{ "Camera " + std::to_string(_sceneObjectCounter++) };
	sceneObject = new SceneObject{ name.c_str(), _scene };
	sceneObject->setParent(nullptr, true);
	auto c = new Camera;
	sceneObject->add(c);
	Camera::setCurrent(c);
	c->transform()->translate(vec3f{ -5,2,4 });
	c->transform()->rotate(vec3f{ 0,-90,0 });

	name = { "Spot Light " + std::to_string(_spotLightCounter++) };
	auto o = new SceneObject{ name.c_str(), _scene };
	auto l = new Light;
	o->add(l);
	o->setParent(nullptr, true);
	l->setType(Light::Type::Spot);
	l->setOpeningAngle(13);
	l->sceneObject()->transform()->translate(vec3f{ 0,15,0 });
	l->setColor(Color::cyan);

	name = { "Spot Light " + std::to_string(_spotLightCounter++) };
	o = new SceneObject{ name.c_str(), _scene };
	l = new Light;
	o->add(l);
	o->setParent(nullptr, true);
	l->setType(Light::Type::Spot);
	l->sceneObject()->transform()->translate(vec3f{ 0,-15,0 });
	l->sceneObject()->transform()->rotate(vec3f{ 0,0,180 });
	l->setColor(Color::magenta);

	name = { "Spot Light " + std::to_string(_spotLightCounter++) };
	o = new SceneObject{ name.c_str(), _scene };
	l = new Light;
	o->add(l);
	o->setParent(nullptr, true);
	l->setType(Light::Type::Spot);
	l->sceneObject()->transform()->translate(vec3f{ -5,2,4 });
	l->sceneObject()->transform()->rotate(vec3f{ 0,0,90 });
	l->setColor(Color::yellow);
	for (int i = 0; i < 5; i++) {
		std::string name{ "Sphere " + std::to_string(_sceneObjectCounter++) };
		sceneObject = new SceneObject{ name.c_str(), _scene };
		sceneObject->setParent(nullptr, true);
		sceneObject->add(makePrimitive(_defaultMeshes.find("Sphere")));
		sceneObject->transform()->translate(vec3f{ (float)(rand() % 5),(float)(rand() % 5),(float)(rand() % 5) });
	}
}

inline void
P4::initScene3()
{
	auto s = new Scene{ "Scene 3" };
	_rayTracer = new RayTracer{ *s };
	_renderer = new GLRenderer{ *s };
	_renderer->setProgram(&_programP);
	glEnable(GL_DEPTH_TEST);
	glEnable(GL_POLYGON_OFFSET_FILL);
	glPolygonOffset(1.0f, 1.0f);
	glEnable(GL_LINE_SMOOTH);
	_programG.use();
	_current = s;
	_scene = s;

	Reference<SceneObject> sceneObject;
	std::string name{ "Camera " + std::to_string(_sceneObjectCounter++) };
	sceneObject = new SceneObject{ name.c_str(), _scene };
	sceneObject->setParent(nullptr, true);
	auto c = new Camera;
	sceneObject->add(c);
	Camera::setCurrent(c);
	c->transform()->translate(vec3f{ -0.4f,0.9f,5.5f });
	c->transform()->rotate(vec3f{ 17,0,0 });

	name = { "Directional Light " + std::to_string(_dirLightCounter++) };
	auto o = new SceneObject{ name.c_str(), _scene };
	auto l = new Light;
	o->add(l);
	o->setParent(nullptr, true);
	l->setType(Light::Type::Directional);
	l->setColor(Color::white);

	name = { "Directional Light " + std::to_string(_dirLightCounter++) };
	o = new SceneObject{ name.c_str(), _scene };
	l = new Light;
	o->add(l);
	o->setParent(nullptr, true);
	l->setType(Light::Type::Directional);
	l->sceneObject()->transform()->rotate(vec3f{ 0,0,180 });
	l->setColor(Color::white);

	auto deer = new SceneObject{ "Deer", _scene };
	deer->setParent(nullptr, true);
	deer->transform()->setLocalScale(0.002f);
	deer->transform()->translate(vec3f{ -0.9f,0,2.2f });
	deer->transform()->rotate(vec3f{ 0,-47,0 });

	auto pagli = new SceneObject{ "Pagli", _scene };
	pagli->setParent(deer, true);
	pagli->transform()->reset();
	pagli->transform()->setLocalScale(100);
	pagli->transform()->setLocalPosition(vec3f{ 0,-60,110 });
	pagli->transform()->rotate(vec3f{ 0,0,180 });
	auto& meshes = Assets::meshes();
	if (!meshes.empty())
	{
		for (auto mit = meshes.begin(); mit != meshes.end(); ++mit)
		{
			if (std::strcmp(mit->first.c_str(), "deer.obj") == 0)
			{
				Assets::loadMesh(mit);
				auto p = makePrimitive(mit);
				p->material.diffuse.setRGB(155, 101, 41);
				p->material.spot.setRGB(214, 161, 12);
				deer->add(p);
			}
			if (std::strcmp(mit->first.c_str(), "paglijon.obj") == 0)
			{
				Assets::loadMesh(mit);
				auto pag = makePrimitive(mit);
				pagli->add(pag);
			}
		}
	}

	auto eye = new SceneObject{ "Eye", _scene };
	eye->setParent(deer, true);
	eye->transform()->setLocalScale(50);
	eye->transform()->setLocalPosition(vec3f{ 420,1010,7 });
	eye->transform()->rotate(vec3f{ 0,44,0 });
	auto p = makePrimitive(_defaultMeshes.find("Sphere"));
	p->material.diffuse.setRGB(0, 0, 0);
	eye->add(p);

	auto iris = new SceneObject{ "Iris", _scene };
	iris->setParent(eye, true);
	iris->transform()->setLocalScale(0.1f);
	iris->transform()->setLocalPosition(vec3f{ -0.69f,0.08f,0.6f });
	p = makePrimitive(_defaultMeshes.find("Sphere"));
	p->material.diffuse.setRGB(22, 68, 114);
	p->material.spot.setRGB(0, 0, 255);
	iris->add(p);

	name = { "Spot Light " + std::to_string(_spotLightCounter++) };
	o = new SceneObject{ name.c_str(), _scene };
	l = new Light;
	o->add(l);
	o->setParent(iris, true);
	l->setType(Light::Type::Spot);
	l->sceneObject()->transform()->setLocalPosition(vec3f{ 3.2f,-0.2f,32 });
	l->sceneObject()->transform()->rotate(vec3f{ 91,6.1f,0 });
	l->setOpeningAngle(2);
	l->setColor(Color::white);

	name = { "Point Light " + std::to_string(_pointLightCounter++) };
	o = new SceneObject{ name.c_str(), _scene };
	l = new Light;
	o->add(l);
	o->setParent(nullptr, true);
	l->setType(Light::Type::Point);
	l->sceneObject()->transform()->translate(vec3f{ -2,1.8f,3.7f });
	l->setDecayValue(2);
	l->setColor(Color::red);

	o = new SceneObject{ "Ground", _scene };
	o->setParent(nullptr, true);
	o->transform()->setLocalScale(vec3f{ 5.8f,0.001f,13 });
	p = makePrimitive(_defaultMeshes.find("Box"));
	p->material.diffuse.setRGB(49, 140, 10);
	p->material.spot.setRGB(142, 255, 0);
	p->material.shine = 3;
	o->add(p);

}

inline void
P4::initRayScene()
{
	auto s = new Scene{ "Paulo de Ferro" };
	_rayTracer = new RayTracer{ *s };
	_renderer = new GLRenderer{ *s };
	_renderer->setProgram(&_programP);
	glEnable(GL_DEPTH_TEST);
	glEnable(GL_POLYGON_OFFSET_FILL);
	glPolygonOffset(1.0f, 1.0f);
	glEnable(GL_LINE_SMOOTH);
	_programG.use();
	_current = s;
	_scene = s;

	// camera 0
	Reference<SceneObject> sceneObject;
	std::string name{ "Camera " + std::to_string(_sceneObjectCounter++) };
	sceneObject = new SceneObject{ name.c_str(), _scene };
	sceneObject->setParent(nullptr, true);
	auto c = new Camera;
	sceneObject->add(c);
	Camera::setCurrent(c);
	c->transform()->translate(vec3f{ -3.9f,9.7f,8.9f });
	c->transform()->rotate(vec3f{ -14,-21,0 });

	// directional light
	auto dl = new SceneObject{ "Directional Light", _scene };
	dl->setParent(nullptr, true);
	auto l1 = new Light();
	dl->transform()->setLocalPosition(vec3f{0,10,0});
	dl->transform()->rotate(vec3f{ 50,30,0 });
	l1->setType(Light::Type::Directional);
	l1->setColor(Color::white);
	dl->add(l1);

	// iron man
	Reference<SceneObject> ironMan = new SceneObject{ "Iron Man", _scene };
	ironMan->setParent(nullptr, true);

	// body
	Reference<SceneObject> body = new SceneObject{ "Body", _scene };
	body->setParent(ironMan, true);
	body->transform()->setLocalPosition(vec3f{ -4.2f, 0.0f, 0.0f });
	body->transform()->rotate(vec3f{ -84.0, -20.0, -12.0 });
	body->transform()->setLocalScale(0.01f);

	// right arm
	Reference<SceneObject> rightArm = new SceneObject{ "Right arm", _scene };
	rightArm->setParent(ironMan, true);
	rightArm->transform()->setLocalPosition(vec3f{ -4.6f, 6, -5.5f });
	rightArm->transform()->rotate(vec3f{ 0.4f, -24.0f, -2.6f });
	rightArm->transform()->setLocalScale(0.01f);

	// left arm
	Reference<SceneObject> leftArm = new SceneObject{ "Left arm", _scene };
	leftArm->setParent(ironMan, true);
	leftArm->transform()->setLocalPosition(vec3f{ 2.6f, 0, -5.0f });
	leftArm->transform()->rotate(vec3f{ 0, -21.0f, 3.2f });
	leftArm->transform()->setLocalScale(0.01f);

	// pagliosa
	Reference<SceneObject> pagliosa = new SceneObject{ "Pagliosa", _scene };
	pagliosa->setParent(ironMan, true);
	pagliosa->transform()->setLocalPosition(vec3f{ 0.4f, 6.3f, -2.9f });
	pagliosa->transform()->rotate(vec3f{ -90.0f, 170.0f, 0.0 });
	pagliosa->transform()->setLocalScale(0.5f);

	// core chest light
	auto cc = new SceneObject{ "Core Chest Light", _scene };
	cc->setParent(nullptr, true);
	auto l2 = new Light();
	cc->transform()->setLocalPosition(vec3f{ 0.2f,5.9f,0.9f });
	cc->transform()->rotate(vec3f{ 91,1,-0.2f });
	l2->setType(Light::Type::Spot);
	l2->setOpeningAngle(9);
	l2->setColor(Color::white);
	cc->add(l2);

	// spot light 1
	auto spt1 = new SceneObject{ "Spot Light 1", _scene };
	spt1->setParent(cc, true);
	auto l3 = new Light();
	spt1->add(l3);
	l3->setType(Light::Type::Spot);
	l3->setOpeningAngle(9);
	l3->setColor(Color::white);
	spt1->transform()->setLocalPosition(vec3f{ 0.0, 0.0, 0.0 });
	spt1->transform()->setRotation(l2->transform()->rotation());

	// spot light 2
	auto spt2 = new SceneObject{ "Spot Light 2", _scene };
	spt2->setParent(cc, true);
	auto l4 = new Light();
	spt2->add(l4);
	l4->setType(Light::Type::Spot);
	l4->setOpeningAngle(9);
	l4->setColor(Color::white);
	spt2->transform()->setLocalPosition(vec3f{ 0.0, 0.0, 0.0 });
	spt2->transform()->setRotation(l2->transform()->rotation());

	// spot light 3
	auto spt3 = new SceneObject{ "Hand Light", _scene };
	spt3->setParent(nullptr, true);
	auto l5 = new Light();
	spt3->add(l5);
	spt3->transform()->setLocalPosition(vec3f{ -6.3f,7.5f,3.1f });
	spt3->transform()->rotate(vec3f{ 91,1.5f,0 });
	l5->setType(Light::Type::Spot);
	l5->setOpeningAngle(7);
	l5->setColor(Color::white);

	// spot light 4
	auto spt4 = new SceneObject{ "Spot Light 4", _scene };
	spt4->setParent(spt3, true);
	auto l6 = new Light();
	spt4->add(l6);
	l6->setType(Light::Type::Spot);
	l6->setOpeningAngle(7);
	l6->setColor(Color::white);
	spt4->transform()->setLocalPosition(vec3f{ 0.0, 0.0, 0.0 });
	spt4->transform()->setRotation(l5->transform()->rotation());

	// spot light 5
	auto spt5 = new SceneObject{ "Spot Light 5", _scene };
	spt5->setParent(spt3, true);
	auto l7 = new Light();
	spt5->add(l7);
	l7->setType(Light::Type::Spot);
	l7->setOpeningAngle(7);
	l7->setColor(Color::white);
	spt5->transform()->setLocalPosition(vec3f{ 0.0, 0.0, 0.0 });
	spt5->transform()->setRotation(l5->transform()->rotation());


	Reference<SceneObject> thor = new SceneObject{ "Thor Hammer", _scene };
	thor->setParent(nullptr, true);
	thor->transform()->setLocalPosition(vec3f{ 1.7f, 5.4f, 2.2f });
	spt4->transform()->rotate({ 0,70,0 });
	thor->transform()->setLocalScale(0.3f);
	//// directional light 0
	//auto dl0 = new SceneObject{ "Directional Light 0", _scene };
	//dl0->setParent(nullptr, true);
	//auto l8 = new Light();
	//dl0->transform()->setLocalPosition(vec3f{ 0.0, 0.0, 0.0 });
	//dl0->transform()->rotate(vec3f{ 180,0,0 });
	//l8->setType(Light::Type::Directional);
	//l8->setColor(Color::white);
	//dl0->add(l8);

	auto& meshes = Assets::meshes();
	if (!meshes.empty())
	{
		for (auto mit = meshes.begin(); mit != meshes.end(); ++mit)
		{
			if (std::strcmp(mit->first.c_str(), "bodyv1.obj") == 0)
			{
				Assets::loadMesh(mit);
				auto p = makePrimitive(mit);
				p->material.ambient.setRGB(51, 51, 51);
				p->material.diffuse.setRGB(199, 88, 88);
				p->material.spot.setRGB(255, 255, 0);
				p->material.specular.setRGB(150, 150, 150);
				body->add(p);
			}
			if (std::strcmp(mit->first.c_str(), "right+handv1.obj") == 0)
			{
				Assets::loadMesh(mit);
				auto p = makePrimitive(mit);
				p->material.ambient.setRGB(51, 51, 51);
				p->material.diffuse.setRGB(199, 88, 88);
				p->material.spot.setRGB(255, 255, 0);
				p->material.specular.setRGB(150, 150, 150);
				rightArm->add(p);
			}
			if (std::strcmp(mit->first.c_str(), "left+handv1.obj") == 0)
			{
				Assets::loadMesh(mit);
				auto p = makePrimitive(mit);
				p->material.ambient.setRGB(51, 51, 51);
				p->material.diffuse.setRGB(206, 83, 83);
				p->material.spot.setRGB(255, 255, 0);
				p->material.specular.setRGB(150, 150, 150);
				leftArm->add(p);
			}
			if (std::strcmp(mit->first.c_str(), "paglijon.obj") == 0)
			{
				Assets::loadMesh(mit);
				auto p = makePrimitive(mit);
				p->material.ambient.setRGB(51, 51, 51);
				p->material.diffuse.setRGB(215, 185, 185);
				p->material.spot.setRGB(0, 0, 0);
				p->material.specular.setRGB(10, 10, 10);
				pagliosa->add(p);
			}
			if (std::strcmp(mit->first.c_str(), "thor.obj") == 0)
			{
				Assets::loadMesh(mit);
				auto p = makePrimitive(mit);
				p->material.ambient.setRGB(51, 51, 51);
				p->material.diffuse.setRGB(200, 200, 200);
				p->material.spot.setRGB(0, 210, 255);
				p->material.specular.setRGB(255,255,255);
				thor->add(p);
			}
		}
	}

}

inline void
P4::initRayScene0()
{
	auto s = new Scene{ "Batpaulo" };
	_rayTracer = new RayTracer{ *s };
	_renderer = new GLRenderer{ *s };
	_renderer->setProgram(&_programP);
	glEnable(GL_DEPTH_TEST);
	glEnable(GL_POLYGON_OFFSET_FILL);
	glPolygonOffset(1.0f, 1.0f);
	glEnable(GL_LINE_SMOOTH);
	_programG.use();
	_current = s;
	_scene = s;

	Reference<SceneObject> sceneObject;
	std::string name{ "Camera " + std::to_string(_sceneObjectCounter++) };
	sceneObject = new SceneObject{ name.c_str(), _scene };
	sceneObject->setParent(nullptr, true);
	auto c = new Camera;
	sceneObject->add(c);
	Camera::setCurrent(c);
	c->transform()->translate(vec3f{ 0.9f,1.1f,2.4f });
	c->transform()->rotate(vec3f{ 0,0,0 });

	auto dk = new SceneObject{ "Batman Pagli", _scene }; //você sabe o que é dk ?
	dk->setParent(nullptr, true);
	dk->transform()->rotate(vec3f{ 0,26,0 });
	dk->transform()->setLocalScale({ 0.01f,0.005f,0.01f });

	auto pp = new SceneObject{ "Pagliosa", _scene };
	pp->setParent(dk, true);
	pp->transform()->setLocalScale(vec3f{ 4.f,4.f,4.f });
	pp->transform()->setLocalPosition(vec3f{ 0.0f, 164.0f, 5.6f });
	pp->transform()->rotate(vec3f{ -76.0f, 210.0f, 0.0f });


	auto dk2 = new SceneObject{ "Batman", _scene }; //você sabe o que é dk ?
	dk2->setParent(nullptr, true);
	dk2->transform()->rotate(vec3f{ 0,-35,0 });
	dk2->transform()->setLocalPosition(vec3f{ 1.9f, 0.0f, 0.0f });
	dk2->transform()->setLocalScale(0.01f);

	auto& meshes = Assets::meshes();
	if (!meshes.empty())
	{
		for (auto mit = meshes.begin(); mit != meshes.end(); ++mit)
		{
			if (std::strcmp(mit->first.c_str(), "DarkKnight.obj") == 0)
			{
				Assets::loadMesh(mit);
				auto p = makePrimitive(mit);
				p->material.diffuse.setRGB(64,60,55);
				p->material.spot.setRGB(255, 240,	0);
				dk->add(p);
				p = makePrimitive(mit);
				p->material.diffuse.setRGB(64, 60, 55);
				p->material.spot.setRGB(255, 240, 0);
				dk2->add(p);
			}
			if (std::strcmp(mit->first.c_str(), "paglijon.obj") == 0)
			{
				Assets::loadMesh(mit);
				auto pag = makePrimitive(mit);
				pag->material.diffuse.setRGB(225, 185, 185);
				pp->add(pag);
			}
		}
	}

	auto dl = new SceneObject{ "Directional Light", _scene };
	dl->setParent(nullptr, true);
	auto l = new Light;
	dl->add(l);
	l->sceneObject()->transform()->rotate(vec3f{ 40,0,0 });
	
}

inline void
P4::initRayScene1()
{
	auto s = new Scene{ "RayScene 1" };
	_rayTracer = new RayTracer{ *s };
	_renderer = new GLRenderer{ *s };
	_renderer->setProgram(&_programP);
	glEnable(GL_DEPTH_TEST);
	glEnable(GL_POLYGON_OFFSET_FILL);
	glPolygonOffset(1.0f, 1.0f);
	glEnable(GL_LINE_SMOOTH);
	_programG.use();
	_current = s;
	_scene = s;

	Reference<SceneObject> sceneObject;
	std::string name{ "Camera " + std::to_string(_sceneObjectCounter++) };
	sceneObject = new SceneObject{ name.c_str(), _scene };
	sceneObject->setParent(nullptr, true);
	auto c = new Camera;
	sceneObject->add(c);
	Camera::setCurrent(c);
	c->transform()->translate(vec3f{ 0,0,12 });

	auto o = new SceneObject{ "Mirror", _scene };
	o->setParent(nullptr, true);
	o->transform()->rotate(vec3f{ -45,0,0 });
	o->transform()->setLocalScale(vec3f{ 5.f,5.f,1 });
	auto p = makePrimitive(_defaultMeshes.find("Box"));
	p->material.diffuse.setRGB(Color::white*0.8f);
	p->material.specular.setRGB(Color::white);
	o->add(p);

	o = new SceneObject{ "Directional Light", _scene };
	o->setParent(nullptr, true);
	o->add(new Light);

	o = new SceneObject{ "Sphere", _scene };
	o->setParent(nullptr, true);
	o->transform()->translate(vec3f{ 0, 10, 0 });
	p = makePrimitive(_defaultMeshes.find("Sphere"));
	p->material.diffuse.setRGB(255, 0, 0);
	o->add(p);

	name = { "Spot Light " + std::to_string(_spotLightCounter++) };
	auto sl = new SceneObject{ name.c_str(), _scene };
	auto l = new Light;
	sl->add(l);
	sl->setParent(o, true);
	l->setType(Light::Type::Spot);
	l->sceneObject()->transform()->setLocalPosition(vec3f{ 0,-4,0 });
	l->sceneObject()->transform()->rotate(vec3f{ 180,0,0 });
	l->setOpeningAngle(8);
	l->setColor(Color::white);
}

inline void
P4::initRayScene2()
{
	auto s = new Scene{ "RayScene 2" };
	_rayTracer = new RayTracer{ *s };
	_renderer = new GLRenderer{ *s };
	_renderer->setProgram(&_programP);
	glEnable(GL_DEPTH_TEST);
	glEnable(GL_POLYGON_OFFSET_FILL);
	glPolygonOffset(1.0f, 1.0f);
	glEnable(GL_LINE_SMOOTH);
	_programG.use();
	_current = s;
	_scene = s;

	Reference<SceneObject> sceneObject;
	std::string name{ "Camera " + std::to_string(_sceneObjectCounter++) };
	sceneObject = new SceneObject{ name.c_str(), _scene };
	sceneObject->setParent(nullptr, true);
	auto c = new Camera;
	sceneObject->add(c);
	Camera::setCurrent(c);
	sceneObject->transform()->translate(vec3f{ 4.7f, 5.3f, 6 });
	sceneObject->transform()->rotate(vec3f{ -35, 38, 6 });

	auto o = new SceneObject{ "Spot Light", _scene };
	o->setParent(nullptr, true);
	auto l = new Light;
	o->add(l);
	l->transform()->setLocalPosition({8.2f,6.3f,8.5f});
	l->transform()->rotate({9,-42,-69});
	l->setType(Light::Type::Spot);
	l->setOpeningAngle(30);


	o = new SceneObject{ "Ground", _scene };
	o->setParent(nullptr, true);
	o->transform()->setLocalScale(vec3f{ 10,0.01f,10 });
	auto p = makePrimitive(_defaultMeshes.find("Box"));
	p->material.diffuse.setRGB(50, 50, 50);
	o->add(p);
	
	auto numOfBalls = 3;
	for (int i = -numOfBalls; i < numOfBalls; i++)
	{
		for (int j = -numOfBalls; j < numOfBalls; j++)
		{
			float x = 2.f * i + 1;
			float z = 2.f * j + 1;
			if (abs(x) != 1 || abs(z) != 1)
			{
				name = { "Sphere " + std::to_string(_sceneObjectCounter++) };
				o = new SceneObject{ name.c_str(), _scene };
				o->setParent(nullptr, true);
				o->transform()->translate(vec3f{ x, 1, z });
				p = makePrimitive(_defaultMeshes.find("Sphere"));
				p->material.diffuse.setRGB(rand() % 265 + rand() % 100, rand() % 265 + rand() % 100, rand() % 265 + rand() % 100);
				p->material.specular.setRGB(255, 255, 255);
				o->add(p);
			}
		}
	}

	o = new SceneObject{ "Wall 1", _scene };
	o->setParent(nullptr, true);
	o->transform()->setLocalScale(vec3f{ 10,10,0.01f });
	o->transform()->translate(vec3f{ 0, 0,-10});
	p = makePrimitive(_defaultMeshes.find("Box"));
	p->material.diffuse.setRGB(100, 100, 100);
	p->material.specular.setRGB(255, 255, 255);
	o->add(p);

	o = new SceneObject{ "Wall 2", _scene };
	o->setParent(nullptr, true);
	o->transform()->setLocalScale(vec3f{ 0.01f,10,10});
	o->transform()->translate(vec3f{ -10, 0,0 });
	p = makePrimitive(_defaultMeshes.find("Box"));
	p->material.diffuse.setRGB(100, 100, 100);
	p->material.specular.setRGB(255, 255, 255);
	o->add(p);

	o = new SceneObject{ "Wall 3", _scene };
	o->setParent(nullptr, true);
	o->transform()->setLocalScale(vec3f{ 10,10,0.01f });
	o->transform()->translate(vec3f{ 0, 0,10 });
	p = makePrimitive(_defaultMeshes.find("Box"));
	p->material.diffuse.setRGB(100, 100, 100);
	p->material.specular.setRGB(255, 255, 255);
	o->add(p);

	o = new SceneObject{ "Wall 4", _scene };
	o->setParent(nullptr, true);
	o->transform()->setLocalScale(vec3f{ 0.01f,10,10 });
	o->transform()->translate(vec3f{ 10, 0,0 });
	p = makePrimitive(_defaultMeshes.find("Box"));
	p->material.diffuse.setRGB(100, 100, 100);
	p->material.specular.setRGB(255, 255, 255);
	o->add(p);

	o = new SceneObject{ "Wall 4", _scene };
	o->setParent(nullptr, true);
	o->transform()->setLocalScale(vec3f{ 10,0.01f,10 });
	o->transform()->translate(vec3f{ 0, 10,0 });
	p = makePrimitive(_defaultMeshes.find("Box"));
	p->material.diffuse.setRGB(100, 100, 100);
	p->material.specular.setRGB(255, 255, 255);
	o->add(p);

	o = new SceneObject{ "Mirror Sphere", _scene };
	o->setParent(nullptr, true);
	o->transform()->setLocalScale(vec3f{ 2,2,2 });
	o->transform()->translate(vec3f{ 0, 2, 0 });
	p = makePrimitive(_defaultMeshes.find("Sphere"));
	p->material.diffuse.setRGB(100, 100, 100);
	p->material.specular.setRGB(255, 255, 255);
	o->add(p);


}
inline void
P4::preview(int x, int y, int width, int height)
{

	// 1st step: save current viewport (lower left corner = (0, 0))
	GLint oldViewPort[4];
	glGetIntegerv(GL_VIEWPORT, oldViewPort);

	// 2nd step: adjust preview viewport
	GLint viewPortX = x;
	GLint viewPortY = y;
	GLint viewPortWidth = width;
	GLint viewPortHeight = height;

	glViewport(viewPortX - 8, viewPortY - 8, viewPortWidth + 16, viewPortHeight + 16);
	glEnable(GL_SCISSOR_TEST);
	glScissor(viewPortX - 8, viewPortY - 8, viewPortWidth + 16, viewPortHeight + 16);
	glClearColor(0, 0, 0, 1.0f);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	glViewport(viewPortX, viewPortY, viewPortWidth, viewPortHeight);
	//3rd step: enable and define scissor
	glScissor(viewPortX, viewPortY, viewPortWidth, viewPortHeight);
	// 4th step: draw primitives

	_renderer->render();

	_programG.use();

	// 5th step: disable scissor region
	glDisable(GL_SCISSOR_TEST);

	// 6th step: restore original viewport
	glViewport(oldViewPort[0], oldViewPort[1], oldViewPort[2], oldViewPort[3]);

}

void
P4::gui()
{
  mainMenu();
  if (_viewMode == ViewMode::Renderer)
    return;
  hierarchyWindow();
  inspectorWindow();
  assetsWindow();
  editorView();
}

inline void
P4::drawMesh(GLMesh* mesh, GLuint mode)
{
  glPolygonMode(GL_FRONT_AND_BACK, mode);
  glDrawElements(GL_TRIANGLES, mesh->vertexCount(), GL_UNSIGNED_INT, 0);
}

union Effect {
	Effect(int displayStyle_, int shadingMode_, int screenSpaceTess_,
		int fractionalSpacing_, int patchCull_, int singleCreasePatch_)
		: value(0) {
		displayStyle = displayStyle_;
		shadingMode = shadingMode_;
		screenSpaceTess = screenSpaceTess_;
		fractionalSpacing = fractionalSpacing_;
		patchCull = patchCull_;
		singleCreasePatch = singleCreasePatch_;
	}

	struct {
		unsigned int displayStyle : 2;
		unsigned int shadingMode : 4;
		unsigned int screenSpaceTess : 1;
		unsigned int fractionalSpacing : 1;
		unsigned int patchCull : 1;
		unsigned int singleCreasePatch : 1;
	};
	int value;

	bool operator < (const Effect& e) const {
		return value < e.value;
	}
};

static Effect
GetEffect()
{
	return Effect(g_displayStyle,
		g_shadingMode,
		g_screenSpaceTess,
		g_fractionalSpacing,
		g_patchCull,
		g_singleCreasePatch);
}

struct EffectDesc {
	EffectDesc(OpenSubdiv::Far::PatchDescriptor desc,
		Effect effect) : desc(desc), effect(effect),
		maxValence(0), numElements(0) { }

	OpenSubdiv::Far::PatchDescriptor desc;
	Effect effect;
	int maxValence;
	int numElements;

	bool operator < (const EffectDesc& e) const {
		return
			(desc < e.desc || ((desc == e.desc &&
				(maxValence < e.maxValence || ((maxValence == e.maxValence) &&
					(numElements < e.numElements || ((numElements == e.numElements) &&
						(effect < e.effect))))))));
	}
};

static const char* shaderSource() {
	static const char* res = NULL;
	if (!res) {
		static const char* gen =
#include "assets/shaders/osd.shader"
			;
		static const char* gen3 =
#include "opensubdiv/../build/examples/glViewer/shader_gl3.gen.h"
			;
		if (GLUtils::SupportsAdaptiveTessellation()) {
			res = gen;
		}
		else {
			res = gen3;
		}
	}
	return res;
}

class ShaderCache : public GLShaderCache<EffectDesc> {
public:
	virtual GLDrawConfig* CreateDrawConfig(EffectDesc const& effectDesc) {

		using namespace OpenSubdiv;

		// compile shader program

		GLDrawConfig* config = new GLDrawConfig(GLUtils::GetShaderVersionInclude().c_str());

		Far::PatchDescriptor::Type type = effectDesc.desc.GetType();

		// common defines
		std::stringstream ss;

		if (type == Far::PatchDescriptor::QUADS) {
			ss << "#define PRIM_QUAD\n";
		}
		else {
			ss << "#define PRIM_TRI\n";
		}

		// OSD tessellation controls
		if (effectDesc.effect.screenSpaceTess) {
			ss << "#define OSD_ENABLE_SCREENSPACE_TESSELLATION\n";
		}
		if (effectDesc.effect.fractionalSpacing) {
			ss << "#define OSD_FRACTIONAL_ODD_SPACING\n";
		}
		if (effectDesc.effect.patchCull) {
			ss << "#define OSD_ENABLE_PATCH_CULL\n";
		}
		if (effectDesc.effect.singleCreasePatch &&
			type == Far::PatchDescriptor::REGULAR) {
			ss << "#define OSD_PATCH_ENABLE_SINGLE_CREASE\n";
		}
		// for legacy gregory
		ss << "#define OSD_MAX_VALENCE " << effectDesc.maxValence << "\n";
		ss << "#define OSD_NUM_ELEMENTS " << effectDesc.numElements << "\n";

		// display styles
		switch (effectDesc.effect.displayStyle) {
		case kDisplayStyleWire:
			ss << "#define GEOMETRY_OUT_WIRE\n";
			break;
		case kDisplayStyleWireOnShaded:
			ss << "#define GEOMETRY_OUT_LINE\n";
			break;
		case kDisplayStyleShaded:
			ss << "#define GEOMETRY_OUT_FILL\n";
			break;
		}

		// shading mode
		switch (effectDesc.effect.shadingMode) {
		case kShadingMaterial:
			ss << "#define SHADING_MATERIAL\n";
			break;
		case kShadingVaryingColor:
			ss << "#define SHADING_VARYING_COLOR\n";
			break;
		case kShadingInterleavedVaryingColor:
			ss << "#define SHADING_VARYING_COLOR\n";
			break;
		case kShadingFaceVaryingColor:
			ss << "#define OSD_FVAR_WIDTH 2\n";
			ss << "#define SHADING_FACEVARYING_COLOR\n";
			if (!effectDesc.desc.IsAdaptive()) {
				ss << "#define SHADING_FACEVARYING_UNIFORM_SUBDIVISION\n";
			}
			break;
		case kShadingPatchType:
			ss << "#define SHADING_PATCH_TYPE\n";
			break;
		case kShadingPatchDepth:
			ss << "#define SHADING_PATCH_DEPTH\n";
			break;
		case kShadingPatchCoord:
			ss << "#define SHADING_PATCH_COORD\n";
			break;
		case kShadingNormal:
			ss << "#define SHADING_NORMAL\n";
			break;
		}

		if (type != Far::PatchDescriptor::TRIANGLES &&
			type != Far::PatchDescriptor::QUADS) {
			ss << "#define SMOOTH_NORMALS\n";
		}

		// need for patch color-coding : we need these defines in the fragment shader
		if (type == Far::PatchDescriptor::GREGORY) {
			ss << "#define OSD_PATCH_GREGORY\n";
		}
		else if (type == Far::PatchDescriptor::GREGORY_BOUNDARY) {
			ss << "#define OSD_PATCH_GREGORY_BOUNDARY\n";
		}
		else if (type == Far::PatchDescriptor::GREGORY_BASIS) {
			ss << "#define OSD_PATCH_GREGORY_BASIS\n";
		}
		else if (type == Far::PatchDescriptor::LOOP) {
			ss << "#define OSD_PATCH_LOOP\n";
		}
		else if (type == Far::PatchDescriptor::GREGORY_TRIANGLE) {
			ss << "#define OSD_PATCH_GREGORY_TRIANGLE\n";
		}

		// include osd PatchCommon
		ss << "#define OSD_PATCH_BASIS_GLSL\n";
		ss << Osd::GLSLPatchShaderSource::GetPatchBasisShaderSource();
		ss << Osd::GLSLPatchShaderSource::GetCommonShaderSource();
		std::string common = ss.str();
		ss.str("");

		// vertex shader
		ss << common
			// enable local vertex shader
			<< (effectDesc.desc.IsAdaptive() ? "" : "#define VERTEX_SHADER\n")
			<< shaderSource()
			<< Osd::GLSLPatchShaderSource::GetVertexShaderSource(type);
		config->CompileAndAttachShader(GL_VERTEX_SHADER, ss.str());
		ss.str("");

		if (effectDesc.desc.IsAdaptive()) {
			// tess control shader
			ss << common
				<< shaderSource()
				<< Osd::GLSLPatchShaderSource::GetTessControlShaderSource(type);
			config->CompileAndAttachShader(GL_TESS_CONTROL_SHADER, ss.str());
			ss.str("");

			// tess eval shader
			ss << common
				<< shaderSource()
				<< Osd::GLSLPatchShaderSource::GetTessEvalShaderSource(type);
			config->CompileAndAttachShader(GL_TESS_EVALUATION_SHADER, ss.str());
			ss.str("");
		}

		// geometry shader
		ss << common
			<< "#define GEOMETRY_SHADER\n"
			<< shaderSource();
		config->CompileAndAttachShader(GL_GEOMETRY_SHADER, ss.str());
		ss.str("");

		// fragment shader
		ss << common
			<< "#define FRAGMENT_SHADER\n"
			<< shaderSource();
		config->CompileAndAttachShader(GL_FRAGMENT_SHADER, ss.str());
		ss.str("");

		if (!config->Link()) {
			delete config;
			return NULL;
		}

		// assign uniform locations
		GLuint uboIndex;
		GLuint program = config->GetProgram();
		uboIndex = glGetUniformBlockIndex(program, "Transform");
		if (uboIndex != GL_INVALID_INDEX)
			glUniformBlockBinding(program, uboIndex, g_transformBinding);

		uboIndex = glGetUniformBlockIndex(program, "Tessellation");
		if (uboIndex != GL_INVALID_INDEX)
			glUniformBlockBinding(program, uboIndex, g_tessellationBinding);

		uboIndex = glGetUniformBlockIndex(program, "Lighting");
		if (uboIndex != GL_INVALID_INDEX)
			glUniformBlockBinding(program, uboIndex, g_lightingBinding);

		uboIndex = glGetUniformBlockIndex(program, "OsdFVarArrayData");
		if (uboIndex != GL_INVALID_INDEX)
			glUniformBlockBinding(program, uboIndex, g_fvarArrayDataBinding);

		// assign texture locations
		GLint loc;
		
		glUseProgram(program);
		if ((loc = glGetUniformLocation(program, "OsdPatchParamBuffer")) != -1) {
			glUniform1i(loc, 0); // GL_TEXTURE0
		}
		if ((loc = glGetUniformLocation(program, "OsdFVarDataBuffer")) != -1) {
			glUniform1i(loc, 1); // GL_TEXTURE1
		}
		if ((loc = glGetUniformLocation(program, "OsdFVarParamBuffer")) != -1) {
			glUniform1i(loc, 2); // GL_TEXTURE2
		}
		// for legacy gregory patches
		if ((loc = glGetUniformLocation(program, "OsdVertexBuffer")) != -1) {
			glUniform1i(loc, 3); // GL_TEXTURE3
		}
		if ((loc = glGetUniformLocation(program, "OsdValenceBuffer")) != -1) {
			glUniform1i(loc, 4); // GL_TEXTURE4
		}
		if ((loc = glGetUniformLocation(program, "OsdQuadOffsetBuffer")) != -1) {
			glUniform1i(loc, 5); // GL_TEXTURE5
		}
		glUseProgram(0);

		return config;
	}
};

ShaderCache g_shaderCache;

static GLenum
bindProgram(Effect effect,
	OpenSubdiv::Osd::PatchArray const& patch, GLuint &program) {
	EffectDesc effectDesc(patch.GetDescriptor(), effect);

	// only legacy gregory needs maxValence and numElements
	// neither legacy gregory nor gregory basis need single crease
	typedef OpenSubdiv::Far::PatchDescriptor Descriptor;
	if (patch.GetDescriptor().GetType() == Descriptor::GREGORY ||
		patch.GetDescriptor().GetType() == Descriptor::GREGORY_BOUNDARY) {
		int maxValence = g_mesh->GetMaxValence();
		int numElements = (g_shadingMode == kShadingInterleavedVaryingColor ? 7 : 3);
		effectDesc.maxValence = maxValence;
		effectDesc.numElements = numElements;
		effectDesc.effect.singleCreasePatch = 0;
	}
	if (patch.GetDescriptor().GetType() == Descriptor::GREGORY_BASIS) {
		effectDesc.effect.singleCreasePatch = 0;
	}

	// lookup shader cache (compile the shader if needed)
	GLDrawConfig* config = g_shaderCache.GetDrawConfig(effectDesc);
	if (!config) return 0;

	program = config->GetProgram();
	glUseProgram(program);

	// bind standalone uniforms
	GLint uniformPrimitiveIdBase =
		glGetUniformLocation(program, "PrimitiveIdBase");
	if (uniformPrimitiveIdBase >= 0)
		glUniform1i(uniformPrimitiveIdBase, patch.GetPrimitiveIdBase());

	// update uniform
	GLint uniformDiffuseColor =
		glGetUniformLocation(program, "diffuseColor");
	if (uniformDiffuseColor >= 0)
		glUniform4f(uniformDiffuseColor, 0.4f, 0.4f, 0.8f, 1);

	// return primtype
	GLenum primType;
	switch (effectDesc.desc.GetType()) {
	case Descriptor::QUADS:
		primType = GL_LINES_ADJACENCY;
		break;
	case Descriptor::TRIANGLES:
		primType = GL_TRIANGLES;
		break;
	default:
#if defined(GL_ARB_tessellation_shader) || defined(GL_VERSION_4_0)
		primType = GL_PATCHES;
		glPatchParameteri(GL_PATCH_VERTICES, effectDesc.desc.GetNumControlVertices());
#else
		primType = GL_POINTS;
#endif
		break;
	}

	return primType;
}
static void
bindTextures() {
	// bind patch textures
	if (g_mesh->GetPatchTable()->GetPatchParamTextureBuffer()) {
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_BUFFER,
			g_mesh->GetPatchTable()->GetPatchParamTextureBuffer());
	}

	if (true) {
		glActiveTexture(GL_TEXTURE1);
		glBindTexture(GL_TEXTURE_BUFFER,
			g_fvarData.textureBuffer);
		glActiveTexture(GL_TEXTURE2);
		glBindTexture(GL_TEXTURE_BUFFER,
			g_fvarData.textureParamBuffer);
	}
	glActiveTexture(GL_TEXTURE0);
}
void
P4::updateUniformBlocks() {

	using namespace OpenSubdiv;

	if (!g_transformUB) {
		glGenBuffers(1, &g_transformUB);
		glBindBuffer(GL_UNIFORM_BUFFER, g_transformUB);
		glBufferData(GL_UNIFORM_BUFFER,
			sizeof(g_transformData), NULL, GL_STATIC_DRAW);
	};
	glBindBuffer(GL_UNIFORM_BUFFER, g_transformUB);
	glBufferSubData(GL_UNIFORM_BUFFER,
		0, sizeof(g_transformData), &g_transformData);
	glBindBuffer(GL_UNIFORM_BUFFER, 0);

	glBindBufferBase(GL_UNIFORM_BUFFER, g_transformBinding, g_transformUB);

	// Update and bind tessellation state
	struct Tessellation {
		float TessLevel;
	} tessellationData;

	tessellationData.TessLevel = static_cast<float>(1 << g_tessLevel);

	if (!g_tessellationUB) {
		glGenBuffers(1, &g_tessellationUB);
		glBindBuffer(GL_UNIFORM_BUFFER, g_tessellationUB);
		glBufferData(GL_UNIFORM_BUFFER,
			sizeof(tessellationData), NULL, GL_STATIC_DRAW);
	};
	glBindBuffer(GL_UNIFORM_BUFFER, g_tessellationUB);
	glBufferSubData(GL_UNIFORM_BUFFER,
		0, sizeof(tessellationData), &tessellationData);
	glBindBuffer(GL_UNIFORM_BUFFER, 0);

	glBindBufferBase(GL_UNIFORM_BUFFER, g_tessellationBinding, g_tessellationUB);

	// Update and bind fvar patch array state
	if (g_mesh->GetPatchTable()->GetNumFVarChannels() > 0) {
		Osd::PatchArrayVector const& fvarPatchArrays =
			g_mesh->GetPatchTable()->GetFVarPatchArrays();

		// bind patch arrays UBO (std140 struct size padded to vec4 alignment)
		int patchArraySize =
			sizeof(GLint) * ((sizeof(Osd::PatchArray) / sizeof(GLint) + 3) & ~3);
		if (!g_fvarArrayDataUB) {
			glGenBuffers(1, &g_fvarArrayDataUB);
		}
		glBindBuffer(GL_UNIFORM_BUFFER, g_fvarArrayDataUB);
		glBufferData(GL_UNIFORM_BUFFER,
			fvarPatchArrays.size() * patchArraySize, NULL, GL_STATIC_DRAW);
		for (int i = 0; i < (int)fvarPatchArrays.size(); ++i) {
			glBufferSubData(GL_UNIFORM_BUFFER,
				i * patchArraySize, sizeof(Osd::PatchArray), &fvarPatchArrays[i]);
		}

		glBindBufferBase(GL_UNIFORM_BUFFER,
			g_fvarArrayDataBinding, g_fvarArrayDataUB);
	}

	// Update and bind lighting state
	OsdLight::Lighting lightingData;
	lightingData = loadOsdLights();
	if (!g_lightingUB) {
		glGenBuffers(1, &g_lightingUB);
		glBindBuffer(GL_UNIFORM_BUFFER, g_lightingUB);
		glBufferData(GL_UNIFORM_BUFFER,
			sizeof(lightingData), NULL, GL_STATIC_DRAW);
	};
	glBindBuffer(GL_UNIFORM_BUFFER, g_lightingUB);
	glBufferSubData(GL_UNIFORM_BUFFER,
		0, sizeof(lightingData), &lightingData);
	glBindBuffer(GL_UNIFORM_BUFFER, 0);

	glBindBufferBase(GL_UNIFORM_BUFFER, g_lightingBinding, g_lightingUB);
}

inline OsdLight::Lighting 
P4::loadOsdLights()
{
	// percorrer a lista de lights e passar os parametros de luz
	auto itL = _scene->getPrimitiveIter();
	auto endL = _scene->getPrimitiveEnd();
	numLights = 0;
	std::string name;
	OsdLight::Lighting::Light light[10] = { 0 };
	for (; itL != endL && numLights < 10; itL++)
	{
		if (auto l = dynamic_cast<Light*>((Component*)*itL))
		{
			auto p = l->sceneObject()->transform()->position();
			auto d = l->sceneObject()->transform()->rotation() * vec3f(0, 1, 0);
			auto c = l->color;
			light[numLights] = {
				{p.x, p.y, p.z, 0},
				{d.x, d.y, d.z, 0},
				{ 0.1f, 0.1f, 0.1f, 1.0f },
				{ 0.8f, 0.8f, 0.8f, 1.0f },
				{ 1.0f, 1.0f, 1.0f, 1.0f },
				{ c.x, c.y, c.z ,1.0f},
				{ (float)l->type(), (float)l->decayValue(), (float)l->decayExponent(), l->openningAngle()}
			};
			numLights++;
		}

		/*program->setUniform((name + "type").c_str(), l->type());
		program->setUniform((name + "fallof").c_str(), l->decayValue());
		program->setUniform((name + "decayExponent").c_str(), l->decayExponent());
		program->setUniform((name + "openningAngle").c_str(), l->openningAngle());
		program->setUniformVec3((name + "lightPosition").c_str(), l->sceneObject()->transform()->position());
		program->setUniformVec4((name + "lightColor").c_str(), l->color);
		program->setUniformVec3((name + "Ldirection").c_str(), l->sceneObject()->transform()->rotation() * vec3f(0, 1, 0));
	*/
	}
	OsdLight::Lighting lighting;
	auto pos = _editor->camera()->sceneObject()->transform()->position();
	float camPos[3] = { pos.x, pos.y, pos.z };
	memcpy(lighting.lightSource, light, sizeof(OsdLight::Lighting::Light) * 10);
	return lighting;
}

inline void
P4::drawPrimitive(OsdPrimitive& primitive)
{
	g_width = width();
	g_height = height();
	auto t = primitive.transform();
	auto c = _editor->camera();
	
	mat4f modelMatrix{ t->localToWorldMatrix() };
	mat4f viewMatrix{ c->worldToCameraMatrix() };
	mat4f projMatrix{ c->projectionMatrix() };
	mat4f mv = viewMatrix * modelMatrix;
	mat4f imv;
	mv.inverse(imv);

	Stopwatch s;
	s.Start();
	glViewport(0, 0, g_width, g_height);
	// prepare view matrix
	double aspect = g_width / (double)g_height;
	for (int i = 0; i < 4; i++)
	{
		for (int j = 0; j < 4; j++)
		{
			g_transformData.ModelViewMatrix[4 * i + j] = mv[i][j];
			g_transformData.ModelViewInverseMatrix[4 * i + j] = imv[i][j];
			g_transformData.ModelViewProjectionMatrix[4 * i + j] = (projMatrix * mv)[i][j];
			g_transformData.ProjectionMatrix[4 * i + j] = projMatrix[i][j];
		}
	}
	// make sure that the vertex buffer is interoped back as a GL resource.
	GLuint vbo = g_mesh->BindVertexBuffer();


	if (g_shadingMode == kShadingVaryingColor)
		g_mesh->BindVaryingBuffer();

	// update transform and lighting uniform blocks
	updateUniformBlocks();

	// also bind patch related textures
	bindTextures();

	if (g_displayStyle == kDisplayStyleWire)
		glDisable(GL_CULL_FACE);

	glEnable(GL_DEPTH_TEST);
	glBindVertexArray(g_vao);
	OpenSubdiv::Osd::PatchArrayVector const& patches =
		g_mesh->GetPatchTable()->GetPatchArrays();
	// patch drawing
	int patchCount[13]; // [Type] (see far/patchTable.h)
	int numTotalPatches = 0;
	int numDrawCalls = 0;
	memset(patchCount, 0, sizeof(patchCount));

	// primitive counting
	glBeginQuery(GL_PRIMITIVES_GENERATED, g_queries[0]);
	// core draw-calls
	for (int i = 0; i < (int)patches.size(); ++i) {
		OpenSubdiv::Osd::PatchArray const& patch = patches[i];

		OpenSubdiv::Far::PatchDescriptor desc = patch.GetDescriptor();
		OpenSubdiv::Far::PatchDescriptor::Type patchType = desc.GetType();

		patchCount[patchType] += patch.GetNumPatches();
		numTotalPatches += patch.GetNumPatches();
		GLuint program;
		GLenum primType = bindProgram(GetEffect(), patch, program);
		auto loc = glGetUniformLocation(program, "NumLights");
		glUniform1i(loc, numLights);
		loc = glGetUniformLocation(program, "CamPos");
		glUniform4fv(loc, 1, (float*)&_editor->camera()->sceneObject()->transform()->position());
		glDrawElements(primType,
			patch.GetNumPatches() * desc.GetNumControlVertices(),
			GL_UNSIGNED_INT,
			(void*)(patch.GetIndexBase() * sizeof(unsigned int)));
		++numDrawCalls;
	}
	s.Stop();
	float drawCpuTime = float(s.GetElapsed() * 1000.0f);

	glEndQuery(GL_PRIMITIVES_GENERATED);
	glBindVertexArray(0);
	glUseProgram(0);

	if (g_displayStyle == kDisplayStyleWire)
		glEnable(GL_CULL_FACE);
	// draw the control mesh
	int stride = g_shadingMode == kShadingInterleavedVaryingColor ? 7 : 3;
	g_controlMeshDisplay.Draw(vbo, stride * sizeof(float),
		g_transformData.ModelViewProjectionMatrix);
	glBindBuffer(GL_ARRAY_BUFFER, 0);
	GLuint numPrimsGenerated = 0;
	GLuint timeElapsed = 0;
	glGetQueryObjectuiv(g_queries[0], GL_QUERY_RESULT, &numPrimsGenerated);

	float drawGpuTime = timeElapsed / 1000.0f / 1000.0f;

	g_fpsTimer.Stop();
	float elapsed = (float)g_fpsTimer.GetElapsed();
	if (!g_freeze) {
		g_animTime += elapsed;
	}
	g_fpsTimer.Start();

	/*if (g_hud.IsVisible()) {

		typedef OpenSubdiv::Far::PatchDescriptor Descriptor;

		double fps = 1.0 / elapsed;

		if (g_displayPatchCounts) {
			int x = -420;
			int y = -140;
			g_hud.DrawString(x, y, "Quads            : %d",
				patchCount[Descriptor::QUADS]); y += 20;
			g_hud.DrawString(x, y, "Triangles        : %d",
				patchCount[Descriptor::TRIANGLES]); y += 20;
			g_hud.DrawString(x, y, "Regular          : %d",
				patchCount[Descriptor::REGULAR]); y += 20;
			g_hud.DrawString(x, y, "Loop             : %d",
				patchCount[Descriptor::LOOP]); y += 20;
			g_hud.DrawString(x, y, "Gregory Basis    : %d",
				patchCount[Descriptor::GREGORY_BASIS]); y += 20;
			g_hud.DrawString(x, y, "Gregory Triangle : %d",
				patchCount[Descriptor::GREGORY_TRIANGLE]); y += 20;
		}

		int y = -220;
		g_hud.DrawString(10, y, "Tess level : %d", g_tessLevel); y += 20;
		g_hud.DrawString(10, y, "Patches    : %d", numTotalPatches); y += 20;
		g_hud.DrawString(10, y, "Draw calls : %d", numDrawCalls); y += 20;
		g_hud.DrawString(10, y, "Primitives : %d", numPrimsGenerated); y += 20;
		g_hud.DrawString(10, y, "Vertices   : %d", g_mesh->GetNumVertices()); y += 20;
		g_hud.DrawString(10, y, "GPU Kernel : %.3f ms", g_gpuTime); y += 20;
		g_hud.DrawString(10, y, "CPU Kernel : %.3f ms", g_cpuTime); y += 20;
		g_hud.DrawString(10, y, "GPU Draw   : %.3f ms", drawGpuTime); y += 20;
		g_hud.DrawString(10, y, "CPU Draw   : %.3f ms", drawCpuTime); y += 20;
		g_hud.DrawString(10, y, "FPS        : %3.1f", fps); y += 20;

		g_hud.Flush();
	}*/

	glFinish();
}

inline void
P4::drawPrimitive(Primitive& primitive)
{
	auto mesh = primitive.mesh();
	auto m = glMesh(mesh);

	if (nullptr == m)
		return;

	auto t = primitive.transform();
	auto normalMatrix = mat3f{ t->worldToLocalMatrix() }.transposed();
	_programG.disuse();
	_programG.use();
	_programG.setUniformMat4("transform", t->localToWorldMatrix());
	_programG.setUniformMat3("normalMatrix", normalMatrix);
	_programG.setUniformVec4("material.ambient", primitive.material.ambient);
	_programG.setUniformVec4("material.diffuse", primitive.material.diffuse);
	_programG.setUniformVec4("material.spot", primitive.material.spot);
	_programG.setUniform("material.shine", primitive.material.shine);
	_programG.setUniform("flatMode", (int)0);

	m->bind();
	drawMesh(m, GL_FILL);
	// **Begin BVH test
	auto bvh = bvhMap[mesh];

	if (bvh == nullptr)
		bvhMap[mesh] = bvh = new BVH{ *mesh, 16 };
	
	// stores a reference to the bvh related to the primitive being drawn
	primitive.setBVH(bvh);
	
	// **End BVH test

	if (primitive.sceneObject() != _current)
		return;

  /*_editor->setLineColor(_selectedWireframeColor);
  _editor->drawBounds(mesh->bounds(), t->localToWorldMatrix());*/

	//_program.setUniformVec4("color", _selectedWireframeColor);
	//drawMesh(m, GL_LINE);
	//_editor->setVectorColor(Color::white);
	//_editor->drawNormals(*mesh, t->localToWorldMatrix(), normalMatrix);
	//_editor->setLineColor(_selectedWireframeColor);
	//_editor->drawBounds(mesh->bounds(), t->localToWorldMatrix());
//#ifndef _DEBUG
//
//
//	bvh->iterate([this, t](const BVHNodeInfo& node)
//		{
//			_editor->setLineColor(node.isLeaf ? Color::yellow : Color::magenta);
//			_editor->drawBounds(node.bounds, t->localToWorldMatrix());
//		});
//#endif // !_DEBUG

}

inline void
P4::drawLight(Light& light)
{

	auto m = mat4f{ light.sceneObject()->transform()->localToWorldMatrix() };
	vec3f normal, points[16];

	if (light.type() == Light::Directional)
	{
		// normal vector
		normal = { 0, -1, 0 };

		// initial points of the vectors
		points[0] = { -0.5, 0, 0 };
		points[1] = { 0   , 0, 0 };
		points[2] = { 0.5 , 0, 0 };

		// tranforming the points and the vector to local coord
		normal = m.transformVector(normal);

		for (int i = 0; i < 3; i++)
			points[i] = m.transform(points[i]);

		_editor->setVectorColor(Color::yellow);

		// drawing 
		for (int i = 0; i < 3; i++)
			_editor->drawVector(points[i], normal, 1.0);

	}

	else if (light.type() == Light::Point)
	{
		// setting line points 
		points[0] = { 1, 0, 0 };
		points[1] = { -1, 0, 0 };

		points[2] = { 0, 1, 0 };
		points[3] = { 0, -1, 0 };

		points[4] = { 0, 0, 1 };
		points[5] = { 0, 0, -1 };

		points[6] = { 0, 0, 1 };
		points[7] = { 0, 0, -1 };

		points[8] = { 1, 1, 1 };
		points[9] = { -1, 1, 1 };

		points[10] = { 1, -1, 1 };
		points[11] = { -1, -1, 1 };

		points[12] = { 1, 1, -1 };
		points[13] = { -1, 1, -1 };

		points[14] = { -1, -1, -1 };
		points[15] = { 1, -1, -1 };

		// transforming points to local coord
		for (int i = 0; i < 16; i++)
			points[i] = m.transform(points[i]);

		// drawing
		_editor->setLineColor(Color::yellow);

		for (int i = 0; i < 8; i += 2)
			_editor->drawLine(points[i], points[i + 1]);

		for (int i = 8; i < 16; i++)
			_editor->drawLine(m.transform(vec3f{ 0,0,0 }), points[i]);

	}

	else
	{
		// we need to get the opening angle in order to calculate the radius
		float openningAngle = light.openningAngle();

		// unsing a arbitrary value for the cone height, whe use pythagoras' theorem to get the radius
		float height = 2;
		float radius = std::tan(math::toRadians(openningAngle)) * height;


		vec3f center = m.transform(vec3f{ 0, -2, 0 });
		vec3f origin = m.transform(vec3f{ 0.0, 0.0, 0.0 });

		// cone side lines
		points[0] = { radius, -2.0, 0.0 };
		points[1] = { -radius, -2.0, 0.0 };

		points[2] = { 0.0, -2.0,  radius };
		points[3] = { 0.0, -2.0, -radius };

		for (int i = 0; i < 4; i++)
			points[i] = m.transform(points[i]);

		// normal vector used to draw the circle
		vec3f normal = { 0.0,-2.0,0.0 };

		// normal vector transformation
		m.transpose();
		m.invert();

		normal = m.transformVector(normal);

		// drawing
		_editor->setLineColor(Color::yellow);

		_editor->drawCircle(center, radius, normal);

		for (int i = 0; i < 4; i++)
			_editor->drawLine(origin, points[i]);

	}
}

inline void
P4::drawCamera(Camera& camera)
{
	float F, B, H, W;
	auto BF = camera.clippingPlanes(F, B);
	B *= 0.7f;
	auto m = mat4f{ camera.cameraToWorldMatrix() };
	vec3f p1, p2, p3, p4, p5, p6, p7, p8;

	if (camera.projectionType() == Camera::ProjectionType::Perspective)
	{
		auto tanAngle = tanf(camera.viewAngle() / 2 * (float)M_PI / 180.0f);

		//p5					p6
		//		p1	p2
		//		p3	p4
		//p7					p8

		H = 2 * F * tanAngle;
		W = H * camera.aspectRatio();

		p1 = { -W / 2,  H / 2, -F };
		p2 = { W / 2,  H / 2, -F };
		p3 = { -W / 2, -H / 2, -F };
		p4 = { W / 2, -H / 2, -F };

		p1 = m.transform(p1);
		p2 = m.transform(p2);
		p3 = m.transform(p3);
		p4 = m.transform(p4);

		H = 2 * B * tanAngle;
		W = H * camera.aspectRatio();

		p5 = { -W / 2,  H / 2, -B };
		p6 = { W / 2,  H / 2, -B };
		p7 = { -W / 2, -H / 2, -B };
		p8 = { W / 2, -H / 2, -B };

		p5 = m.transform(p5);
		p6 = m.transform(p6);
		p7 = m.transform(p7);
		p8 = m.transform(p8);

	}
	else if (camera.projectionType() == Camera::ProjectionType::Parallel)
	{
		H = camera.height();
		W = H * camera.aspectRatio();

		p1 = { -W / 2, H / 2, -F };
		p2 = { W / 2, H / 2, -F };
		p3 = { -W / 2, -H / 2, -F };
		p4 = { W / 2, -H / 2, -F };

		p5 = { -W / 2, H / 2, -B };
		p6 = { W / 2, H / 2, -B };
		p7 = { -W / 2, -H / 2, -B };
		p8 = { W / 2, -H / 2, -B };

		p1 = m.transform(p1);
		p2 = m.transform(p2);
		p3 = m.transform(p3);
		p4 = m.transform(p4);
		p5 = m.transform(p5);
		p6 = m.transform(p6);
		p7 = m.transform(p7);
		p8 = m.transform(p8);

	}

	_editor->setLineColor(Color::green);

	_editor->drawLine(p1, p2);
	_editor->drawLine(p2, p4);
	_editor->drawLine(p4, p3);
	_editor->drawLine(p3, p1);

	_editor->drawLine(p5, p6);
	_editor->drawLine(p6, p8);
	_editor->drawLine(p8, p7);
	_editor->drawLine(p7, p5);

	_editor->drawLine(p1, p5);
	_editor->drawLine(p2, p6);
	_editor->drawLine(p3, p7);
	_editor->drawLine(p4, p8);
}

inline void
P4::renderScene()
{
  if (auto camera = Camera::current())
  {
    if (_image == nullptr)
    {
      const auto w = width(), h = height();

      _image = new GLImage{w, h};
      _rayTracer->setImageSize(w, h);
      _rayTracer->setCamera(camera);
      _rayTracer->renderImage(*_image);
    }
    _image->draw(0, 0);
  }
}

constexpr auto CAMERA_RES = 0.01f;
constexpr auto ZOOM_SCALE = 1.01f;

void
P4::loadLights(GLSL::Program* program, Camera* ec)
{

	const auto& p = ec->transform()->position();
	auto vp = vpMatrix(ec);

	program->setUniformMat4("vpMatrix", vp);
	program->setUniformVec4("ambientLight", _scene->ambientLight);


	// percorrer a lista de lights e passar os parametros de luz
	auto itL = _scene->getPrimitiveIter();
	auto endL = _scene->getPrimitiveEnd();
	int numLights = 0;
	std::string name;

	program->setUniformVec3("camPos", p);

	for (; itL != endL && numLights < 10; itL++)
	{
		if (auto l = dynamic_cast<Light*>((Component*)* itL))
		{
			name = "lights[" + std::to_string(numLights) + "].";
			program->setUniform((name + "type").c_str(), l->type());
			program->setUniform((name + "fallof").c_str(), l->decayValue());
			program->setUniform((name + "decayExponent").c_str(), l->decayExponent());
			program->setUniform((name + "openningAngle").c_str(), l->openningAngle());
			program->setUniformVec3((name + "lightPosition").c_str(), l->sceneObject()->transform()->position());
			program->setUniformVec4((name + "lightColor").c_str(), l->color);
			program->setUniformVec3((name + "Ldirection").c_str(), l->sceneObject()->transform()->rotation() * vec3f(0, 1, 0));
			numLights++;
		}
	}

	program->setUniform("numLights", numLights);
}

void
P4::render()
{
	if (_viewMode == ViewMode::Renderer)
	{
		renderScene();
		return;
	}
	if (_moveFlags)
	{
		const auto delta = _editor->orbitDistance() * CAMERA_RES;
		auto d = vec3f::null();

		if (_moveFlags.isSet(MoveBits::Forward))
			d.z -= delta;
		if (_moveFlags.isSet(MoveBits::Back))
			d.z += delta;
		if (_moveFlags.isSet(MoveBits::Left))
			d.x -= delta;
		if (_moveFlags.isSet(MoveBits::Right))
			d.x += delta;
		if (_moveFlags.isSet(MoveBits::Up))
			d.y += delta;
		if (_moveFlags.isSet(MoveBits::Down))
			d.y -= delta;
		_editor->pan(d);
	}
	_editor->newFrame();

	// **Begin rendering of temporary scene objects
	// It should be replaced by your rendering code (and moved to scene editor?)
	loadLights(&_programG, _editor->camera());

	auto it = _scene->getPrimitiveIter();
	auto end = _scene->getPrimitiveEnd();
	Camera* cam = nullptr;
	// itera nos primitivos
	for (; it != end; it++)
	{
		auto component = (Component*)* it;
		auto o = (*it)->sceneObject();

		if (auto p = dynamic_cast<Primitive*>(component))
			drawPrimitive(*p);
		else if (auto p = dynamic_cast<OsdPrimitive*>(component))
			drawPrimitive(*p);
		else if (auto c = dynamic_cast<Camera*>(component))
		{
			if (c->sceneObject() == dynamic_cast<SceneObject*>(_current))
			{
				cam = c;
				drawCamera(*c);
			}
		}
		else if (auto l = dynamic_cast<Light*>(component))
		{
			if (l->sceneObject() == dynamic_cast<SceneObject*>(_current))
			{
				drawLight(*l);
			}
		}
		if (o == _current)
		{
			auto t = o->transform();
			_editor->setPolygonMode(GLGraphicsBase::PolygonMode::FILL);
			_editor->drawAxes(t->position(), mat3f{ t->rotation() });
		}
	}
	if (cam)
		previewWindow(cam);
}

void
P4::previewWindow(Camera * c)
{
	auto h = height();
	auto w = width();

	ImGui::SetNextWindowSize(ImVec2{ w / 4.0f,h / 4.0f });
	ImGui::SetNextWindowBgAlpha(0);
	if (ImGui::Begin("Preview", NULL, ImGuiWindowFlags_NoResize))
	{
		ImVec2 vMin = ImGui::GetWindowContentRegionMin();
		ImVec2 vMax = ImGui::GetWindowContentRegionMax();

		int x = (int)(ImGui::GetWindowPos().x + vMin.x);
		int y = (int)(h - (ImGui::GetWindowPos().y + vMax.y));
		int width = (int)(vMax.x - vMin.x);
		int height = (int)(vMax.y - vMin.y);

		_programP.use();
		loadLights(&_programP, c);
		_renderer->setCamera(c);
		_renderer->setImageSize(w, h);
		preview(x, y, width, height);
	}
	ImGui::End();
}
void
P4::focus()
{
	if (auto o = dynamic_cast<SceneObject*>(_current))
	{
		auto t = o->transform();
		auto localP = t->position();
		_editor->camera()->transform()->setPosition(localP);
		auto scale = t->lossyScale();
		auto ang = _editor->camera()->viewAngle();
		auto x = scale.x / tanf(math::toRadians(ang / 2));//pitágoras
		auto y = scale.y / tanf(math::toRadians(ang / 2));
		_editor->camera()->transform()->translate(vec3f{ 0,0,x + y + scale.z });
	}
}
bool
P4::windowResizeEvent(int width, int height)
{
  _editor->camera()->setAspectRatio(float(width) / float(height));
  _viewMode = ViewMode::Editor;
  _image = nullptr;
  return true;
}

bool
P4::keyInputEvent(int key, int action, int mods)
{
  auto active = action != GLFW_RELEASE && mods == GLFW_MOD_ALT;

	if (key == GLFW_KEY_DELETE && action == GLFW_RELEASE)
		removeCurrent();
	else if (key == GLFW_KEY_E && action == GLFW_RELEASE && mods == GLFW_MOD_SHIFT)
		addEmptyCurrent();
	else if (key == GLFW_KEY_B && action == GLFW_RELEASE && mods == GLFW_MOD_SHIFT)
		addBoxCurrent();
	else if (key == GLFW_KEY_S && action == GLFW_RELEASE && mods == GLFW_MOD_SHIFT)
		addSphereCurrent();
	else if (key == GLFW_KEY_F && action == GLFW_RELEASE && mods == GLFW_MOD_ALT)
		focus();
	else
  switch (key)
  {
    case GLFW_KEY_W:
      _moveFlags.enable(MoveBits::Forward, active);
      break;
    case GLFW_KEY_S:
      _moveFlags.enable(MoveBits::Back, active);
      break;
    case GLFW_KEY_A:
      _moveFlags.enable(MoveBits::Left, active);
      break;
    case GLFW_KEY_D:
      _moveFlags.enable(MoveBits::Right, active);
      break;
    case GLFW_KEY_Q:
      _moveFlags.enable(MoveBits::Up, active);
      break;
    case GLFW_KEY_Z:
      _moveFlags.enable(MoveBits::Down, active);
      break;
  }
  return false;
}

bool
P4::scrollEvent(double, double yOffset)
{
  if (ImGui::GetIO().WantCaptureMouse)
    return false;
  _editor->zoom(yOffset < 0 ? 1.0f / ZOOM_SCALE : ZOOM_SCALE);
  return true;
}

bool
P4::mouseButtonInputEvent(int button, int actions, int mods)
{
  if (ImGui::GetIO().WantCaptureMouse)
    return false;
  (void)mods;

  auto active = actions == GLFW_PRESS;

  if (button == GLFW_MOUSE_BUTTON_LEFT)
  {
    if (active)
    {
      cursorPosition(_pivotX, _pivotY);

      const auto ray = makeRay(_pivotX, _pivotY);

      // **Begin picking of temporary scene objects
      // It should be replaced by your picking code
			auto it = _scene->getPrimitiveIter();
			auto end = _scene->getPrimitiveEnd();
      for (; it != end; it++)
      {
				auto o = (*it)->sceneObject();
        if (!o->visible)
          continue;

        auto component = *it;
				Intersection hit;
				hit.distance = ray.tMax;
				auto minDist = math::Limits<float>::inf();
        if (auto p = dynamic_cast<Primitive*>((Component*)component))
          if (p->intersect(ray, hit) && hit.distance < minDist)
          {
						minDist = hit.distance;
            _current = o;
          }
      }
      // **End picking of temporary scene objects
    }
    return true;
  }
  if (button == GLFW_MOUSE_BUTTON_RIGHT)
    _dragFlags.enable(DragBits::Rotate, active);
  else if (button == GLFW_MOUSE_BUTTON_MIDDLE)
    _dragFlags.enable(DragBits::Pan, active);
  if (_dragFlags)
    cursorPosition(_pivotX, _pivotY);
  return true;
}

bool
P4::mouseMoveEvent(double xPos, double yPos)
{
  if (!_dragFlags)
    return false;
  _mouseX = (int)xPos;
  _mouseY = (int)yPos;

  const auto dx = (_pivotX - _mouseX);
  const auto dy = (_pivotY - _mouseY);

  _pivotX = _mouseX;
  _pivotY = _mouseY;
  if (dx != 0 || dy != 0)
  {
    if (_dragFlags.isSet(DragBits::Rotate))
    {
      const auto da = -_editor->camera()->viewAngle() * CAMERA_RES;
      isKeyPressed(GLFW_KEY_LEFT_ALT) ?
        _editor->orbit(dy * da, dx * da) :
        _editor->rotateView(dy * da, dx * da);
    }
    if (_dragFlags.isSet(DragBits::Pan))
    {
      const auto dt = -_editor->orbitDistance() * CAMERA_RES;
      _editor->pan(-dt * math::sign(dx), dt * math::sign(dy), 0);
    }
  }
  return true;
}