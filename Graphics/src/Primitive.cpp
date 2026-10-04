// Primitive.obj reconstruction with scoped OpenGL state restoration.
#include "Primitive.h"
#include "Texture.h"
#include "GraphicsGL.h"
namespace SceneGraph {
void Billboard::render() {
    if (bHide || !texture || !graphicsHasContext()) return;
    GLint mode=GL_MODELVIEW; glGetIntegerv(GL_MATRIX_MODE,&mode);
    glPushAttrib(GL_ENABLE_BIT|GL_COLOR_BUFFER_BIT|GL_CURRENT_BIT|GL_TEXTURE_BIT|GL_DEPTH_BUFFER_BIT);
    texture->activate(); glMatrixMode(GL_MODELVIEW); glPushMatrix(); glLoadIdentity();
    glMatrixMode(GL_PROJECTION); glPushMatrix(); glLoadIdentity();
    glDisable(GL_LIGHTING); glDisable(GL_CULL_FACE); glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA,GL_ONE_MINUS_SRC_ALPHA); glColor4f(1,1,1,1);
    glBegin(GL_QUADS);
    glTexCoord2f(0,1); glVertex3f(xmin,ymin,-1);
    glTexCoord2f(1,1); glVertex3f(xmax,ymin,-1);
    glTexCoord2f(1,0); glVertex3f(xmax,ymax,-1);
    glTexCoord2f(0,0); glVertex3f(xmin,ymax,-1);
    glEnd(); glPopMatrix(); glMatrixMode(GL_MODELVIEW); glPopMatrix();
    glMatrixMode(mode); glPopAttrib();
}
DisplayList::~DisplayList() {
    if (dispList>0 && graphicsHasContext()) glDeleteLists(dispList,1);
}
void DisplayList::render() {
    if (dispList<=0 || !graphicsHasContext()) return;
    glPushAttrib(GL_ENABLE_BIT); glDisable(GL_TEXTURE_2D); glDisable(GL_LIGHTING);
    glCallList(dispList); glPopAttrib();
}
void Line::render() {
    if (!graphicsHasContext()) return;
    glPushAttrib(GL_ENABLE_BIT|GL_CURRENT_BIT); glDisable(GL_TEXTURE_2D); glDisable(GL_LIGHTING);
    glColor3f(0,1,1); glBegin(GL_LINES);
    glVertex3f(s.x,s.y,s.z); glVertex3f(e.x,e.y,e.z); glEnd(); glPopAttrib();
}
void Plane::render() {
    if (!graphicsHasContext()) return;
    glPushAttrib(GL_ENABLE_BIT|GL_CURRENT_BIT|GL_COLOR_BUFFER_BIT);
    glDisable(GL_TEXTURE_2D); glDisable(GL_LIGHTING); glEnable(GL_BLEND);
    glBlendFunc(GL_ONE,GL_SRC_COLOR); glColor4f(0,0.5f,1,0.5f); glBegin(GL_QUADS);
    for (const Vec3 & point:v) glVertex3f(point.x,point.y,point.z);
    glEnd(); glPopAttrib();
}
}
