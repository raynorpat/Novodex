/*----------------------------------------------------------------------------*\
|
|								NovodeX Technology
|
|							     www.novodex.com
|
\*----------------------------------------------------------------------------*/
#include "ViewerGraphicsContext.h"

SceneGraph::Object * ViewGC::world = 0;
SceneGraph::Object * ViewGC::camera = 0;
SceneGraph::Line * ViewGC::pickLine = 0;
float ViewGC::visualizationScale = 1.0f;
float ViewGC::fov = 45.0f;
Vec3 ViewGC::cameraEulers(0,0,0);

