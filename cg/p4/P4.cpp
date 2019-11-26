#include "geometry/MeshSweeper.h"
#include "P4.h"

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
  buildDefaultMeshes();
  buildScene();
  _renderer = new GLRenderer{*_scene};
	_renderer->setProgram(&_programP);
  _rayTracer = new RayTracer{*_scene};
  glEnable(GL_DEPTH_TEST);
  glEnable(GL_POLYGON_OFFSET_FILL);
  glPolygonOffset(1.0f, 1.0f);
  glEnable(GL_LINE_SMOOTH);
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
P4::inspectMaterial(Material& material)
{
  ImGui::ColorEdit3("Ambient", material.ambient);
  ImGui::ColorEdit3("Diffuse", material.diffuse);
  ImGui::ColorEdit3("Spot", material.spot);
  ImGui::DragFloat("Shine", &material.shine, 1, 0, 1000.0f);
  ImGui::ColorEdit3("Specular", material.specular);
}

inline void
P4::inspectPrimitive(Primitive& primitive)
{
  //const auto flag = ImGuiTreeNodeFlags_NoTreePushOnOpen;

  //if (ImGui::TreeNodeEx("Shape", flag))
    inspectShape(primitive);
  //if (ImGui::TreeNodeEx("Material", flag))
    inspectMaterial(primitive.material);
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

			}
			if (ImGui::MenuItem("Scene 2"))
			{
				_sceneObjectCounter = 0;
				initScene2();

			}
			if (ImGui::MenuItem("Scene 3"))
			{
				_sceneObjectCounter = 0;
				initScene3();
			}
			ImGui::EndMenu();
		}

    ImGui::EndMainMenuBar();
  }
}
inline void
P4::initOriginalScene()
{
	buildScene();
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
drawMesh(GLMesh* mesh, GLuint mode)
{
  glPolygonMode(GL_FRONT_AND_BACK, mode);
  glDrawElements(GL_TRIANGLES, mesh->vertexCount(), GL_UNSIGNED_INT, 0);
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
	bvh->iterate([this, t](const BVHNodeInfo& node)
		{
			_editor->setLineColor(node.isLeaf ? Color::yellow : Color::magenta);
			_editor->drawBounds(node.bounds, t->localToWorldMatrix());
		});

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
      auto minDistance = math::Limits<float>::inf();

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

        if (auto p = dynamic_cast<Primitive*>((Component*)component))
          if (p->intersect(ray, hit) && hit.distance < minDistance)
          {
            minDistance = hit.distance;
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
