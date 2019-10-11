
#include "geometry/MeshSweeper.h"
#include "P2.h"

MeshMap P2::_defaultMeshes;
inline void
P2::buildDefaultMeshes()
{
	_defaultMeshes["None"] = nullptr;
	_defaultMeshes["Box"] = GLGraphics3::box();
	_defaultMeshes["Sphere"] = GLGraphics3::sphere();
}

inline Primitive*
makePrimitive(MeshMapIterator mit)
{
	return new Primitive{ mit->second, mit->first };
}

inline void
P2::buildScene()
{
	_current = _scene = new Scene{ "Scene 1" };
	_editor = new SceneEditor{ *_scene };
	_editor->setDefaultView((float)width() / (float)height());
	// **Begin initialization of temporary attributes
	// It should be replaced by your scene initialization
	/*{
		auto o = new SceneObject{"Main Camera", *_scene};
		auto camera = new Camera;

		o->addComponent(camera);
		o->setParent(_scene->root());
		_objects.push_back(o);
		o = new SceneObject{"Box 1", *_scene};
		o->addComponent(makePrimitive(_defaultMeshes.find("Box")));
		o->setParent(_scene->root());
		_objects.push_back(o);
		Camera::setCurrent(camera);
	}*/
	// **End initialization of temporary attributes
	Reference<SceneObject> sceneObject;
	std::string name{ "Camera " + std::to_string(_sceneObjectCounter++) };
	sceneObject = new SceneObject{ name.c_str(), _scene };
	sceneObject->setParent(nullptr, true);
	auto c = new Camera;
	sceneObject->add(c);
	Camera::setCurrent(c);
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
P2::initialize()
{
	Application::loadShaders(_program, "shaders/p2.vs", "shaders/p2.fs");
	Assets::initialize();
	buildDefaultMeshes();
	buildScene();
	_renderer = new GLRenderer{ *_scene };
	_renderer->setProgram(&_program);
	glEnable(GL_DEPTH_TEST);
	glEnable(GL_POLYGON_OFFSET_FILL);
	glPolygonOffset(1.0f, 1.0f);
	glEnable(GL_LINE_SMOOTH);
	_program.use();
}

namespace ImGui
{
	void ShowDemoWindow(bool*);
}

void
P2::dragDrop(SceneNode* sceneObject)
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
			auto o = *(SceneObject * *)payload->Data;
			if (auto s = dynamic_cast<Scene*>(o))
				o->setParent(nullptr);
			else if (auto obj = dynamic_cast<SceneObject*>(o))
				o->setParent(obj);
		}
		ImGui::EndDragDropTarget();
	}
}

void
P2::treeChildren(SceneNode* obj)
{
	auto open = false;
	ImGuiTreeNodeFlags flag{ ImGuiTreeNodeFlags_OpenOnArrow };
	auto s = dynamic_cast<Scene*>(obj);
	auto o = dynamic_cast<SceneObject*>(obj);

	if (s)
	{
		auto open = ImGui::TreeNodeEx(_scene,
			_current == _scene ? flag | ImGuiTreeNodeFlags_Selected : flag,
			_scene->name());
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
			_current = o;

		dragDrop(o);

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
P2::addEmptyCurrent()
{
	std::string name{ "Object " + std::to_string(_sceneObjectCounter++) };
	auto sceneObject = new SceneObject{ name.c_str(), _scene };
	SceneObject* current = dynamic_cast<SceneObject*>(_current);
	sceneObject->setParent(current, true);
}

void
P2::addBoxCurrent()
{
	// TODO: create a new box.
	std::string name{ "Box " + std::to_string(_sceneObjectCounter++) };
	auto sceneObject = new SceneObject{ name.c_str(), _scene };
	SceneObject* current = nullptr;
	current = dynamic_cast<SceneObject*>(_current);
	sceneObject->setParent(current, true);

	Component* primitive = dynamic_cast<Component*>(makePrimitive(_defaultMeshes.find("Box")));
	sceneObject->add(primitive);
}

void
P2::removeCurrent() {
	if (_current != _scene && _current != nullptr)
	{
		SceneObject* sceneObject = dynamic_cast<SceneObject*>(_current);
		auto parent = sceneObject->parent();

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
P2::hierarchyWindow()
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
		ImGui::TextColored({ 0.5,0.5,0.5,1 }, "Shortcut: 'Ctrl + E'");
		if (ImGui::BeginMenu("3D Object"))
		{
			if (ImGui::MenuItem("Box"))
			{
				addBoxCurrent();
			}
			ImGui::SameLine();
			ImGui::TextColored({ 0.5,0.5,0.5,1 }, "Shortcut: 'Ctrl + B'");
			ImGui::EndMenu();
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

	treeChildren(_scene);
	ImGuiTreeNodeFlags flag{ ImGuiTreeNodeFlags_OpenOnArrow };
	auto open = ImGui::TreeNodeEx(_scene,
		_current == _scene ? flag | ImGuiTreeNodeFlags_Selected : flag,
		_scene->name());

	if (ImGui::IsItemClicked())
		_current = _scene;

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
	ImGui::End();
}

namespace ImGui
{ // begin namespace ImGui

	void
		ObjectNameInput(NameableObject* object)
	{
		const int bufferSize{ 128 };
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
		return ImGui::ColorEdit3(label, (float*)& color);
	}

	inline bool
		DragVec3(const char* label, vec3f& v)
	{
		return DragFloat3(label, (float*)& v, 0.1f, 0.0f, 0.0f, "%.2g");
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
P2::sceneGui()
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
P2::inspectPrimitive(Primitive& primitive)
{
	char buffer[16];

	snprintf(buffer, 16, "%s", primitive.meshName());
	ImGui::InputText("Mesh", buffer, 16, ImGuiInputTextFlags_ReadOnly);
	if (ImGui::BeginDragDropTarget())
	{
		if (auto * payload = ImGui::AcceptDragDropPayload("PrimitiveMesh"))
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
	ImGui::ColorEdit3("Mesh Color", (float*)& primitive.color);
}

void
P2::inspectCamera(Camera& camera)
{
	static const char* projectionNames[]{ "Perspective", "Orthographic" };
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
		if (f - n < MIN_DEPTH)
			f = (n + MIN_DEPTH);
		camera.setClippingPlanes(n, f);
	}
}

inline void
P2::addComponentButton(SceneObject& object)
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
		if (ImGui::MenuItem("Camera"))
		{
			// TODONE
			auto c = dynamic_cast<Component*>((Camera*) new Camera);
			object.add(c);
		}
		ImGui::EndPopup();
	}
}

void
P2::removePrimitive(Primitive* p)
{
	p->sceneObject()->remove(dynamic_cast<Component*>((Primitive*)p));
}
inline void
P2::sceneObjectGui()
{
	auto object = (SceneObject*)_current;

	addComponentButton(*object);
	ImGui::Separator();
	ImGui::ObjectNameInput(object);
	ImGui::SameLine();
	ImGui::Checkbox("###visible", &object->visible);
	ImGui::Separator();
	//if (ImGui::CollapsingHeader(object->transform()->typeName()))
	//  ImGui::TransformEdit(object->transform());

	// **Begin inspection of temporary components
	// It should be replaced by your component inspection
	//auto component = object->component();

	//if (auto p = dynamic_cast<Primitive*>(component))
	//{
	//  auto notDelete{true};
	//  auto open = ImGui::CollapsingHeader(p->typeName(), &notDelete);

	//  if (!notDelete)
	//  {
	//    // TODO: delete primitive
	//  }
	//  else if (open)
	//    inspectPrimitive(*p);
	//}
	//else if (auto c = dynamic_cast<Camera*>(component))
	//{
	//  auto notDelete{true};
	//  auto open = ImGui::CollapsingHeader(c->typeName(), &notDelete);

	//  if (!notDelete)
	//  {
	//    // TODO: delete camera
	//  }
	//  else if (open)
	//  {
	//    auto isCurrent = c == Camera::current();

	//    ImGui::Checkbox("Current", &isCurrent);
	//    Camera::setCurrent(isCurrent ? c : nullptr);
	//    inspectCamera(*c);
	//  }
	//}
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
				_scene->remove(p);
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

			preview(c);

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
				inspectCamera(*c);
			}
		}
	}
}

inline void
P2::objectGui()
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
P2::inspectorWindow()
{
	ImGui::Begin("Inspector");
	objectGui();
	ImGui::End();
}

inline void
P2::editorViewGui()
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
	ImGui::Checkbox("Show Ground", &_editor->showGround);
}

inline void
P2::assetsWindow()
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
		// p3
	}
	ImGui::End();
}

inline void
P2::editorView()
{
	if (!_showEditorView)
		return;
	ImGui::Begin("Editor View Settings");
	editorViewGui();
	ImGui::End();
}

inline void
P2::fileMenu()
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
P2::showOptions()
{
	ImGui::PushItemWidth(ImGui::GetWindowWidth() * 0.6f);
	showStyleSelector("Color Theme##Selector");
	ImGui::ColorEdit3("Selected Wireframe", _selectedWireframeColor);
	ImGui::PopItemWidth();
}

inline void
P2::mainMenu()
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
				static const char* viewLabels[]{ "Editor", "Renderer" };

				if (ImGui::BeginCombo("View", viewLabels[_viewMode]))
				{
					for (auto i = 0; i < IM_ARRAYSIZE(viewLabels); ++i)
					{
						if (ImGui::Selectable(viewLabels[i], _viewMode == i))
							_viewMode = (ViewMode)i;
					}
					ImGui::EndCombo();
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
		ImGui::EndMainMenuBar();
	}
}

inline void
P2::preview(Camera* c)
{

	// 1st step: save current viewport (lower left corner = (0, 0))
	// Viewport[0] = x, viewport[1] = y, viewport[2] = width, viewport[3] = height
	GLint oldViewPort[4];
	glGetIntegerv(GL_VIEWPORT, oldViewPort);

	// 2nd step: adjust preview viewport
	GLint viewPortHeight = (oldViewPort[3] / 5);
	GLint viewPortWidth = c->aspectRatio() * viewPortHeight;

	GLint viewPortX = oldViewPort[2] / 2 - viewPortWidth / 2;
	GLint viewPortY = 0;

	glViewport(viewPortX, viewPortY, viewPortWidth, viewPortHeight);

	//3rd step: enable and define scissor
	glScissor(viewPortX, viewPortY, viewPortWidth, viewPortHeight);
	glEnable(GL_SCISSOR_TEST);

	// 4th step: draw primitives
	renderScene();


	// 5th step: desable scissor region
	glDisable(GL_SCISSOR_TEST);

	// 6th step: restore original viewport
	glViewport(oldViewPort[0], oldViewPort[1], oldViewPort[2], oldViewPort[3]);

}

void
P2::gui()
{
	mainMenu();
	hierarchyWindow();
	inspectorWindow();
	assetsWindow();
	editorView();
	//preview();
	/*
	static bool demo = true;
	ImGui::ShowDemoWindow(&demo);
	*/
}

inline void
drawMesh(GLMesh* mesh, GLuint mode)
{
	glPolygonMode(GL_FRONT_AND_BACK, mode);
	glDrawElements(GL_TRIANGLES, mesh->vertexCount(), GL_UNSIGNED_INT, 0);
}

inline void
P2::drawPrimitive(Primitive& primitive)
{
	auto m = glMesh(primitive.mesh());

	if (nullptr == m)
		return;

	auto t = primitive.transform();
	auto normalMatrix = mat3f{ t->worldToLocalMatrix() }.transposed();

	_program.setUniformMat4("transform", t->localToWorldMatrix());
	_program.setUniformMat3("normalMatrix", normalMatrix);
	_program.setUniformVec4("color", primitive.color);
	_program.setUniform("flatMode", (int)0);
	m->bind();
	drawMesh(m, GL_FILL);
	if (primitive.sceneObject() != _current)
		return;
	_program.setUniformVec4("color", _selectedWireframeColor);
	_program.setUniform("flatMode", (int)1);
	drawMesh(m, GL_LINE);
}

inline void
P2::drawCamera(Camera& camera)
{
	float F, B, H, W;
	auto BF = camera.clippingPlanes(F, B);
	//B *= 0.1f;
	auto m = mat4f{ camera.cameraToWorldMatrix() };
	vec3f p1, p2, p3, p4, p5, p6, p7, p8;

	if (camera.projectionType() == Camera::ProjectionType::Perspective)
	{
		auto tanAngle = tanf(camera.viewAngle() / 2 * M_PI / 180);

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
	/*p1 = m.transform(p1);
	p2 = m.transform(p2);
	p3 = m.transform(p3);
	p4 = m.transform(p4);
	p5 = m.transform(p5);
	p6 = m.transform(p6);
	p7 = m.transform(p7);
	p8 = m.transform(p8);*/

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
P2::renderScene()
{
	if (auto camera = Camera::current())
	{
		_renderer->setCamera(camera);
		_renderer->setImageSize(width(), height());
		_renderer->render();
		_program.use();
	}
}

constexpr auto CAMERA_RES = 0.01f;
constexpr auto ZOOM_SCALE = 1.01f;

void
P2::render()
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
	auto ec = _editor->camera();
	const auto& p = ec->transform()->position();
	auto vp = vpMatrix(ec);

	_program.setUniformMat4("vpMatrix", vp);
	_program.setUniformVec4("ambientLight", _scene->ambientLight);
	_program.setUniformVec3("lightPosition", p);
	/*for (const auto& o : _objects)
	{
		if (!o->visible)
			continue;

		auto component = o->component();

		if (auto p = dynamic_cast<Primitive*>(component))
			drawPrimitive(*p);
		else if (auto c = dynamic_cast<Camera*>(component))
			drawCamera(*c);
		if (o == _current)
		{
			auto t = o->transform();
			_editor->drawAxes(t->position(), mat3f{ t->rotation() });
		}
	}*/
	// **End rendering of temporary scene objects
	//GLWindow::render();
	auto it = _scene->getPrimitiveIter();
	auto end = _scene->getPrimitiveEnd();

	// itera nos primitivos
	for (; it != end; it++)
	{
		auto component = (Component*)* it;
		auto o = (*it)->sceneObject();

		if (auto p = dynamic_cast<Primitive*>(component))
			drawPrimitive(*p);
		else if (auto c = dynamic_cast<Camera*>(component))
			if (c == Camera::current())
				drawCamera(*c);
		if (o == _current)
		{
			auto t = o->transform();
			_editor->drawAxes(t->position(), mat3f{ t->rotation() });
		}
	}
}

void
P2::focus()
{
	if (auto o = dynamic_cast<SceneObject*>(_current))
	{
		auto t = o->transform();
		auto localP = t->localPosition();
		localP.z += FOCUS_OFFSET;
		_editor->camera()->transform()->setLocalPosition(localP);
	}
}

bool
P2::windowResizeEvent(int width, int height)
{
	_editor->camera()->setAspectRatio(float(width) / float(height));
	return true;
}

bool
P2::keyInputEvent(int key, int action, int mods)
{
	auto active = action != GLFW_RELEASE && mods == GLFW_MOD_ALT;

	if (key == GLFW_KEY_DELETE && action == GLFW_RELEASE)
		removeCurrent();
	else if (key == GLFW_KEY_E && action == GLFW_RELEASE && mods == GLFW_MOD_CONTROL)
		addEmptyCurrent();
	else if (key == GLFW_KEY_B && action == GLFW_RELEASE && mods == GLFW_MOD_CONTROL)
		addBoxCurrent();
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
		case GLFW_KEY_E:
			_moveFlags.enable(MoveBits::Down, active);
			break;
		}
	return false;
}

bool
P2::scrollEvent(double, double yOffset)
{
	if (ImGui::GetIO().WantCaptureMouse || _viewMode == ViewMode::Renderer)
		return false;
	_editor->zoom(yOffset < 0 ? 1.0f / ZOOM_SCALE : ZOOM_SCALE);
	return true;
}

bool
P2::mouseButtonInputEvent(int button, int actions, int mods)
{
	if (ImGui::GetIO().WantCaptureMouse || _viewMode == ViewMode::Renderer)
		return false;
	(void)mods;

	auto active = actions == GLFW_PRESS;

	if (button == GLFW_MOUSE_BUTTON_RIGHT)
		_dragFlags.enable(DragBits::Rotate, active);
	else if (button == GLFW_MOUSE_BUTTON_MIDDLE)
		_dragFlags.enable(DragBits::Pan, active);
	if (_dragFlags)
		cursorPosition(_pivotX, _pivotY);
	return true;
}

bool
P2::mouseMoveEvent(double xPos, double yPos)
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
