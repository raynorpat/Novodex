#include "DebugRenderer.h"
#include "NxDebugRenderable.h"
#include "ViewerPlatform.h"


	NX_INLINE void DebugRenderer::setupColor(NxU32 color) const
		{
			NxF32 Blue	= NxF32((color)&0xff)/255.0f;
			NxF32 Green	= NxF32((color>>8)&0xff)/255.0f;
			NxF32 Red	= NxF32((color>>16)&0xff)/255.0f;
			glColor3f(Red, Green, Blue);
		}

	void DebugRenderer::renderData(const NxDebugRenderable& data) const
		{
			//glPushMatrix();
			//ViewerUi::start();
			//glColor4f(1,0.7f,0.7f,1);
			//ViewerUi::draw(1, 32, -1.0, 0.7, "Blah");
			//ViewerUi::end();
			//glPopMatrix();

			glLineWidth(1.0f);
			glPushMatrix();

			glDisable(GL_LIGHTING);
			
			// Render points
			{
				NxU32 NbPoints = data.getNbPoints();
				const NxDebugPoint* Points = data.getPoints();

				glBegin(GL_POINTS);
				while(NbPoints--)
					{
					setupColor(Points->color);
					glVertex3fv(&Points->p.x);
					Points++;
					}
				glEnd();
			}

			// Render lines
			{
				NxU32 NbLines = data.getNbLines();
				const NxDebugLine* Lines = data.getLines();

				glBegin(GL_LINES);
				while(NbLines--)
					{
					setupColor(Lines->color);
					glVertex3fv(&Lines->p0.x);
					glVertex3fv(&Lines->p1.x);
					Lines++;
					}
				glEnd();
			}

			// Render triangles
			{
				NxU32 NbTris = data.getNbTriangles();
				const NxDebugTriangle* Triangles = data.getTriangles();

				glBegin(GL_TRIANGLES);
				while(NbTris--)
					{
					setupColor(Triangles->color);
					glVertex3fv(&Triangles->p0.x);
					glVertex3fv(&Triangles->p1.x);
					glVertex3fv(&Triangles->p2.x);
					Triangles++;
					}
				glEnd();
			}
			glEnable(GL_LIGHTING);
			glPopMatrix();
		}
