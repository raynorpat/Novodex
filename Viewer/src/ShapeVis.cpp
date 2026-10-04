/*----------------------------------------------------------------------------*\
|
|								NovodeX Technology
|
|							     www.novodex.com
|
\*----------------------------------------------------------------------------*/
#include "NxPhysics.h"
#include "ShapeVis.h"

#if defined WIN32
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX				//suppress windows' global min,max macros.
#include <windows.h>	//needed for gl.h
#endif
#include <GL/glu.h>

static int dispListBox;
static int dispListSphere;
static int dispListCylinder;

void ShapeVis::init()
{
	glLoadIdentity();
	dispListBox = glGenLists(1);//get a unique display list ID.
	glNewList(dispListBox, GL_COMPILE);      

	glBegin(GL_QUADS);
	glNormal3f(0,0,1);
	glVertex3f(-1,-1,1);
	glVertex3f(1,-1,1);
	glVertex3f(1,1,1);
	glVertex3f(-1,1,1);

	glNormal3f(0,0,-1);
	glVertex3f(-1,1,-1);
	glVertex3f(1,1,-1);
	glVertex3f(1,-1,-1);
	glVertex3f(-1,-1,-1);

	glNormal3f(0,1,0);
	glVertex3f(-1,1,1);
	glVertex3f(1,1,1);
	glVertex3f(1,1,-1);
	glVertex3f(-1,1,-1);

	glNormal3f(0,-1,0);
	glVertex3f(-1,-1,-1);
	glVertex3f(1,-1,-1);
	glVertex3f(1,-1,1);
	glVertex3f(-1,-1,1);

	glNormal3f(1,0,0);
	glVertex3f(1,-1,-1);
	glVertex3f(1,1,-1);
	glVertex3f(1,1,1);
	glVertex3f(1,-1,1);

	glNormal3f(-1,0,0);
	glVertex3f(-1,-1,1);
	glVertex3f(-1,1,1);
	glVertex3f(-1,1,-1);
	glVertex3f(-1,-1,-1);

	glEnd();
	glEndList();


	dispListSphere = glGenLists(1);
	glNewList(dispListSphere, GL_COMPILE);      
	GLUquadricObj * quadObj = gluNewQuadric ();
	gluQuadricDrawStyle (quadObj, GLU_FILL);
	gluQuadricNormals (quadObj, GLU_SMOOTH); 
	gluQuadricOrientation(quadObj,GLU_OUTSIDE);
	gluSphere (quadObj, 1.0f, 9, 7);	//unit sphere
	glEndList();
	gluDeleteQuadric(quadObj);

	dispListCylinder = glGenLists(1);
	glNewList(dispListCylinder, GL_COMPILE);      
	quadObj = gluNewQuadric ();
	gluQuadricDrawStyle (quadObj, GLU_FILL);
	gluQuadricNormals (quadObj, GLU_SMOOTH); 
	gluQuadricOrientation(quadObj,GLU_OUTSIDE);
	gluCylinder  (quadObj, 1.0f, 1.0f, 1.0f, 18, 1);	//unit cylinder
	glEndList();
	gluDeleteQuadric(quadObj);
}

void ShapeVis::release()
	{
	glDeleteLists(dispListBox,1);
	glDeleteLists(dispListSphere,1);
	glDeleteLists(dispListCylinder,1);
	}

void ShapeVis::setupGLMatrix(const NxVec3& pos, const NxMat33& orient)
	{
	float glmat[16];	//4x4 column major matrix for OpenGL.
	orient.getColumnMajorStride4(&(glmat[0]));
	pos.get(&(glmat[12]));

	//clear the elements we don't need:
	glmat[3] = glmat[7] = glmat[11] = 0.0f;
	glmat[15] = 1.0f;

	glMultMatrixf(&(glmat[0]));
	}

void ShapeVis::render(NxBoxShape & shape)
	{
	NxBox worldBox;
	shape.getWorldOBB(worldBox);

	// Render the OBB
	renderBox(worldBox.center, worldBox.rot, worldBox.extents);
	}

void ShapeVis::render(NxSphereShape & shape)
	{
	//rescale our unit sphere:
	glPushMatrix();
	NxMat34 pose;
	shape.getShape().getGlobalPose(pose);
	setupGLMatrix(pose.t, pose.M);
	NxReal r = shape.getRadius();
	glScaled(r,r,r);
	glCallList(dispListSphere);
	glPopMatrix();
	}

void ShapeVis::render(NxCapsuleShape & capsule)
	{
	//rescale unit spheres:
	NxMat34 pose;
	capsule.getShape().getGlobalPose(pose);

	const NxReal & r = capsule.getRadius();
	const NxReal & h = capsule.getHeight();

	glPushMatrix();

	float glmat[16];	//4x4 column major matrix for OpenGL.
	pose.M.getColumnMajorStride4(&(glmat[0]));
	pose.t.get(&(glmat[12]));

	//clear the elements we don't need:
	glmat[3] = glmat[7] = glmat[11] = 0.0f;
	glmat[15] = 1.0f;

	glMultMatrixf(&(glmat[0]));

	float Red[4] = {1,0.7f,0.7f,0};

	glPushMatrix();
	glTranslated(0.0f, h*0.5f, 0.0f);
	glScaled(r,r,r);
	glCallList(dispListSphere);
	glPopMatrix();

	glPushMatrix();
	glTranslated(0.0f,-h*0.5f, 0.0f);
	glScaled(r,r,r);
	glCallList(dispListSphere);
	glPopMatrix();

	glPushMatrix();
	glTranslated(0.0f,h*0.5f, 0.0f);
	glScaled(r,h,r);
	glRotated(90.0f,1.0f,0.0f,0.0f);
	glCallList(dispListCylinder);
	glPopMatrix();

	glPopMatrix();
	}


bool ShapeVis::renderBox(const NxVec3 & pos, const NxMat33 & orient, const NxVec3  & halfWidths)
	{
	//transform our unit cube:
	glPushMatrix();

	//glTranslated(pos.x(), pos.y(), pos.z());
	setupGLMatrix(pos, orient);

	//glScaled(halfWidths.x(), halfWidths.y(), halfWidths.z());

	//protect against infinitely thin leaf-AABBs, which do not shade correctly:
	NxVec3 mess;
	mess = halfWidths;

	bool bad = false;

	if (mess.x <= 0.001f)
		{
		mess.setx(0.001f);
		bad = true;
		}
	if (mess.y <= 0.001f)
		{
		mess.sety(0.001f);
		bad = true;
		}
	if (mess.z <= 0.001f)
		{
		mess.setz(0.001f);
		bad = true;
		}

	float Red[4] = {1,0.7f,0.7f,0};

	if (bad)
		glMaterialfv(GL_FRONT, GL_DIFFUSE, Red);

	glScaled(mess.x, mess.y, mess.z);

	glCallList(dispListBox);

	glPopMatrix();

	return true;
	}

bool ShapeVis::renderSphere(const NxVec3  & pos, NxReal r)
	{
	//rescale our unit sphere:
	glPushMatrix();

	const NxVec3 * t = &pos;
	glTranslated(t->x, t->y, t->z);

	glScaled(r,r,r);
	glCallList(dispListSphere);
	glPopMatrix();
	return true;
	}


void ShapeVis::renderShape(NxShape* shape)
	{
	if(!shape)	
		return;
	switch (shape->getType())
		{
		case NX_SHAPE_BOX:
			{
			NxBoxShape * box = shape->isBox();
			if (box)
				render(*box);
			}
			break;
		case NX_SHAPE_SPHERE:
			{
			NxSphereShape * sphere = shape->isSphere();
			if (sphere)
				render(*sphere);
			}
			break;
		case NX_SHAPE_CAPSULE:
			{
			NxCapsuleShape * capsule = shape->isCapsule();
			if (capsule)
				render(*capsule);
			}
			break;
		}
	}