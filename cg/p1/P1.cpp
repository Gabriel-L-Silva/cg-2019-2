#include "P1.h"
#include <iostream>

namespace cg
{ // begin namespace cg

inline Primitive*
makeBoxMesh()
{
  const vec4f p1{-0.5, -0.5, -0.5, 1};
  const vec4f p2{+0.5, -0.5, -0.5, 1};
  const vec4f p3{+0.5, +0.5, -0.5, 1};
  const vec4f p4{-0.5, +0.5, -0.5, 1};
  const vec4f p5{-0.5, -0.5, +0.5, 1};
  const vec4f p6{+0.5, -0.5, +0.5, 1};
  const vec4f p7{+0.5, +0.5, +0.5, 1};
  const vec4f p8{-0.5, +0.5, +0.5, 1};
  const Color c1{Color::black};
  const Color c2{Color::red};
  const Color c3{Color::yellow};
  const Color c4{Color::green};
  const Color c5{Color::blue};
  const Color c6{Color::magenta};
  const Color c7{Color::cyan};
  const Color c8{Color::white};

  // Box vertices
  static const vec4f v[]
  {
    p1, p5, p8, p4, // x = -0.5
    p2, p3, p7, p6, // x = +0.5
    p1, p2, p6, p5, // y = -0.5
    p4, p8, p7, p3, // y = +0.5
    p1, p4, p3, p2, // z = -0.5
    p5, p6, p7, p8  // z = +0.5
  };

  // Box vertex colors
  static const Color c[]
  {
    c1, c5, c8, c4, // x = -0.5
    c2, c3, c7, c6, // x = +0.5
    c1, c2, c6, c5, // y = -0.5
    c4, c8, c7, c3, // y = +0.5
    c1, c4, c3, c2, // z = -0.5
    c5, c6, c7, c8  // z = +0.5
  };

  // Box triangles
  static const GLMeshArray::Triangle t[]
  {
    { 0,  1,  2}, { 2,  3,  0},
    { 4,  5,  7}, { 5,  6,  7},
    { 8,  9, 11}, { 9, 10, 11},
    {12, 13, 14}, {14, 15, 12},
    {16, 17, 19}, {17, 18, 19},
    {20, 21, 22}, {22, 23, 20}
  };

  return new Primitive{new GLMeshArray{24, {v, 0}, {c, 1}, 12, t}};
}

} // end namespace cg

inline void
P1::buildScene()
{
	_scene = new Scene{"Scene 1"};
	Reference<SceneObject> sceneObject;

	for (int i = 0; i < 5; i++) {
		std::string name{ "Box " + std::to_string(_sceneObjectCounter++) };
		sceneObject = new SceneObject{ name.c_str(), _scene };
		sceneObject->setParent(nullptr);
		sceneObject->add((Reference<Component>)makeBoxMesh());
		for (int j = 0; j < 5; j++) {
			std::string name{ "Object " + std::to_string(_sceneObjectCounter++) };
			sceneObject->add(new SceneObject{ name.c_str(), _scene });
			
		}
	}
}

void
P1::initialize()
{
  Application::loadShaders(_program, "p1.vs", "p1.fs");
  buildScene();
  glEnable(GL_DEPTH_TEST);
  glEnable(GL_POLYGON_OFFSET_FILL);
  glPolygonOffset(1.0f, 1.0f);
  glLineWidth(2.0f);
  glEnable(GL_LINE_SMOOTH);
  _program.use();
}

namespace ImGui
{
  void ShowDemoWindow(bool*);
}

void 
P1::treeChildren(bool open, std::vector<Reference<SceneObject>>::iterator it, std::vector<Reference<SceneObject>>::iterator end)
{
	if (open)
	{
		for (; it != end; it++)
		{
			if ((*it)->isChildrenEmpty())
			{
				auto flag = ImGuiTreeNodeFlags_Leaf | ImGuiTreeNodeFlags_NoTreePushOnOpen;
				ImGui::TreeNodeEx((*it),
					_current == (*it) ? flag | ImGuiTreeNodeFlags_Selected : flag,
					(*it)->name());
				if (ImGui::IsItemClicked()) 
					_current = (*it);
				
			}
			else
			{
				auto flag = ImGuiTreeNodeFlags_OpenOnArrow;
				auto open = ImGui::TreeNodeEx((*it),
					_current == (*it) ? flag | ImGuiTreeNodeFlags_Selected : flag,
					(*it)->name());

				if (ImGui::IsItemClicked())
					_current = (*it);
				auto cIt = (*it)->getChildrenIter();
				auto cEnd = (*it)->getChildrenEnd();
				treeChildren(open, cIt, cEnd);
			}
		}
		ImGui::TreePop();
	}
}


void
P1::remove() {
	if (_current != _scene)
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

bool
P1::keyInputEvent(int key, int action, int mods) {
	remove();
	return true;
}
inline void
P1::hierarchyWindow()
{
  ImGui::Begin("Hierarchy");
  if (ImGui::Button("Create###object"))
    ImGui::OpenPopup("CreateObjectPopup");
  if (ImGui::BeginPopup("CreateObjectPopup"))
  {
		if (ImGui::MenuItem("Empty Object"))
		{
			std::string name{ "Object " + std::to_string(_sceneObjectCounter++) };
			auto sceneObject = new SceneObject{ name.c_str(), _scene };
			SceneObject* current  = dynamic_cast<SceneObject*>(_current);
			sceneObject->setParent(current);
		}
    if (ImGui::BeginMenu("3D Object"))
    {
      if (ImGui::MenuItem("Box"))
      {
        // TODO: create a new box.
				std::string name{ "Box " + std::to_string(_sceneObjectCounter++) };
				auto sceneObject = new SceneObject{ name.c_str(), _scene };
				SceneObject* current = nullptr;
				current = dynamic_cast<SceneObject*>(_current);
				sceneObject->setParent(current);

				Component* primitive = nullptr;
				primitive = dynamic_cast<Component*>(makeBoxMesh());
				sceneObject->add(primitive);
      }
      ImGui::EndMenu();
    }
    ImGui::EndPopup();
  }
	if (ImGui::Button("Delete"))
	{
		remove();
	}
	
  ImGui::Separator();

  ImGuiTreeNodeFlags flag{ImGuiTreeNodeFlags_OpenOnArrow};
  auto open = ImGui::TreeNodeEx(_scene,
    _current == _scene ? flag | ImGuiTreeNodeFlags_Selected : flag,
    _scene->name());

  if (ImGui::IsItemClicked())
    _current = _scene;
	auto it = _scene->getRootIt();
	auto end = _scene->getRootEnd();
	treeChildren(open, it, end);
	ImGui::End();
}


namespace ImGui
{ // begin namespace ImGui

void
ObjectNameInput(SceneNode* object)
{
  const int bufferSize{128};
  static SceneNode* current;
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
    transform->setLocalScale(temp);
}

} // end namespace ImGui

inline void
P1::sceneGui()
{
  auto scene = (Scene*)_current;

  ImGui::ObjectNameInput(_current);
  ImGui::Separator();
  if (ImGui::CollapsingHeader("Colors"))
  {
    ImGui::ColorEdit3("Background", backgroundColor);
    ImGui::ColorEdit3("Selected Wireframe", selectedWireframeColor);
  }
}

inline void
P1::sceneObjectGui()
{
  auto object = (SceneObject*)_current;

  ImGui::ObjectNameInput(object);
  ImGui::SameLine();
  ImGui::Checkbox("visible", &object->visible);
  ImGui::Separator();

	auto it = object->getComponentIter();
	auto end = object->getComponentEnd();

	for (; it != end; it++)
	{
		//Comparar com dynamic cast
		auto casted = dynamic_cast<Transform*>((Component*)(*it));
		if (casted != nullptr)//Se for Transform
		{
			if (ImGui::CollapsingHeader(object->transform()->typeName()))
			{
				auto t = object->transform();

				ImGui::TransformEdit(t);
				_transform = t->localToWorldMatrix();
			}
		}
		else//Se for primitive
		{
			auto casted = dynamic_cast<Primitive*>((Component*)(*it));
			if (ImGui::CollapsingHeader(casted->typeName()))
			{
				// TODO: show primitive properties.
				auto nVert = std::to_string(casted->mesh()->vertexCount());
				ImGui::Text((std::string{ "Vertices: " } +nVert).c_str());
			}
		}
	}
	
  
}

inline void
P1::objectGui()
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
P1::inspectorWindow()
{
  ImGui::Begin("Inspector");
  objectGui();
  ImGui::End();
}

void
P1::gui()
{
  hierarchyWindow();
  inspectorWindow();
  /*
  static bool demo = true;
  ImGui::ShowDemoWindow(&demo);
  */
}

void
P1::renderPrim(Reference<Primitive> p)
{
	if (p->sceneObject()->visible) {
		_program.setUniformMat4("transform", p->sceneObject()->transform()->localToWorldMatrix());
		//_program.setUniformMat4("transform", _transform);

		auto m = p->mesh();

		m->bind();
		glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
		glDrawElements(GL_TRIANGLES, m->vertexCount(), GL_UNSIGNED_INT, 0);

		// adiciona fios de arame no polígono
		if (_current == p->sceneObject()) {
			m->setVertexColor(selectedWireframeColor);
			glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
			glDrawElements(GL_TRIANGLES, m->vertexCount(), GL_UNSIGNED_INT, 0);
			m->useVertexColors();
		}
	}
	
}


//GLWindow::render();


void
P1::render() {
	GLWindow::render();
	auto it = _scene->getPrimitiveIter();
	auto end = _scene->getPrimitiveEnd();

	// itera nos primitivos
	for (; it != end; it++)
	{
		renderPrim(*it);
	}

	/*if (!_box->visible)
		return;

	_program.setUniformMat4("transform", _transform);

	auto m = _primitive->mesh();

	m->bind();
	glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
	glDrawElements(GL_TRIANGLES, m->vertexCount(), GL_UNSIGNED_INT, 0);
	if (_current != _box)
		return;
	m->setVertexColor(selectedWireframeColor);
	glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
	glDrawElements(GL_TRIANGLES, m->vertexCount(), GL_UNSIGNED_INT, 0);
	m->useVertexColors(); */
	
}


