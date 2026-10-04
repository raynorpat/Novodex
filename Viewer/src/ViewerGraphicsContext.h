#ifndef VIEWERGRAPHICSCONTEXT_H
#define VIEWERGRAPHICSCONTEXT_H
/*----------------------------------------------------------------------------*\
|
|								NovodeX Technology
|
|							     www.novodex.com
|
\*----------------------------------------------------------------------------*/
#include "Graphics.h"
#include "3dmath.h"


struct ViewerGraphicsContext
	{
	static SceneGraph::Object * world;
	static SceneGraph::Object * camera;
	static SceneGraph::Line * pickLine;
	static float visualizationScale;
	static float fov;
	static Vec3 cameraEulers;
	};

typedef ViewerGraphicsContext ViewGC;

#endif //VIEWERGRAPHICSCONTEXT_H
