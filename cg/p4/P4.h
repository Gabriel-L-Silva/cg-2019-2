#ifndef __P4_h
#define __P4_h

#include "Assets.h"
#include "BVH.h"
#include "GLRenderer.h"
#include "Light.h"
#include "Primitive.h"
#include "SceneEditor.h"
#include "RayTracer.h"
#include "core/Flags.h"
#include "graphics/Application.h"
#include "graphics/GLImage.h"
#include <vector>

using namespace cg;

class P4: public GLWindow
{
public:
  P4(int width, int height):
		GLWindow{"cg2019 - P4", width, height},
		_programP{"P4 Phong"},
		_programG{"P4 Gouraud"}
  {
    // do nothing
  }

  /// Initialize the app.
  void initialize() override;

  /// Update the GUI.
  void gui() override;

	/// Render the scene.
	void loadLights(GLSL::Program* program, Camera* cam);
	void render() override;

	void dragDrop(SceneNode* sceneObject);
	void treeChildren(SceneNode*);
	void removeCurrent();
	void addEmptyCurrent();
	void addBoxCurrent();
	void addSphereCurrent();
	void addLightCurrent(Light::Type);
	void focus();

private:
  enum ViewMode
  {
    Editor = 0,
    Renderer = 1
  };

  enum class MoveBits
  {
    Left = 1,
    Right = 2,
    Forward = 4,
    Back = 8,
    Up = 16,
    Down = 32
  };

  enum class DragBits
  {
    Rotate = 1,
    Pan = 2
  };

	using BVHRef = Reference<BVH>;
	using BVHMap = std::map<TriangleMesh*, BVHRef>;

	GLSL::Program _programP, _programG;
  Reference<Scene> _scene;
  Reference<SceneEditor> _editor;
  Reference<GLRenderer> _renderer;
  // **Begin temporary attributes
  // Those are just to show some geometry
  // They should be replaced by your scene hierarchy
  std::vector<Reference<SceneObject>> _objects;
  // **End temporary attributes
  SceneNode* _current{};
  Color _selectedWireframeColor{255, 102, 0};
  Flags<MoveBits> _moveFlags{};
  Flags<DragBits> _dragFlags{};
  int _pivotX;
  int _pivotY;
  int _mouseX;
  int _mouseY;
  bool _showAssets{true};
  bool _showEditorView{true};
  ViewMode _viewMode{ViewMode::Editor};
  Reference<RayTracer> _rayTracer;
  Reference<GLImage> _image;
	BVHMap bvhMap;

  static MeshMap _defaultMeshes;

  void buildScene();
  void renderScene();

	void initOriginalScene();
  void mainMenu();
  void fileMenu();
  void showOptions();

  void hierarchyWindow();
  void inspectorWindow();
  void assetsWindow();
  void editorView();
  void sceneGui();
  void sceneObjectGui();
  void objectGui();
  void editorViewGui();
	void previewWindow(Camera* c);
	void initScene2();
	void initScene3();
	void initRayScene();
	void initRayScene0();
	void initRayScene1();
	void initRayScene2();
	void slenderScene();
	//void initRayScene3();
	void preview(int, int, int, int);
  void inspectPrimitive(Primitive&);
  void inspectShape(Primitive&);
  void inspectMaterial(Material&);
  void inspectLight(Light&);
  void inspectCamera(Camera&);
  void addComponentButton(SceneObject&);

  void drawPrimitive(Primitive&);
  void drawLight(Light&);
  void drawCamera(Camera&);

  bool windowResizeEvent(int, int) override;
  bool keyInputEvent(int, int, int) override;
  bool scrollEvent(double, double) override;
  bool mouseButtonInputEvent(int, int, int) override;
  bool mouseMoveEvent(double, double) override;

  Ray makeRay(int, int) const;

  static void buildDefaultMeshes();
	int _sceneObjectCounter = 0;
	int _cameraCounter = 0;
	int _pointLightCounter = 0;
	int _spotLightCounter = 0;
	int _dirLightCounter = 0;
}; // P4

#endif // __P4_h
