/*----------------------------------------------------------------------------*\
|
|								NovodeX Technology
|
|							     www.novodex.com
|
\*----------------------------------------------------------------------------*/
#ifndef SHAPE_VIS_H
#define SHAPE_VIS_H
/*-------------------------------*\
|
| Shape vis helper class
| draws pix of Colldet shapes.
|
\*-------------------------------*/
class NxShape;

class ShapeVis
	{
	public:

	static void renderShape(NxShape* shape);
	static void init();
	static void release();

	private:

	static void setupGLMatrix(const NxVec3& pos, const NxMat33& orient);
	static void render(NxBoxShape & shape);
	static void render(NxSphereShape & shape);
	static void render(NxCapsuleShape & shape);

	static bool renderBox(const NxVec3 & pos, const NxMat33 & orient, const NxVec3  & halfWidths);
	static bool renderSphere(const NxVec3  & pos, NxReal r);
	};

#endif