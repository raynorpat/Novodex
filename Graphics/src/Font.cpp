// Compatibility API recovered from Font.obj. The viewer HUD uses Dear ImGui.
#include "Font.h"
#include "GraphicsGL.h"
#include <array>
#include <cstring>
#include <stdexcept>
#include <algorithm>
namespace {
const char * faces[]={"Times New Roman","Arial","Courier New","Lucida Console",
                     "Bookman Old Style","Century Schoolbook","Arial Narrow","Garamond"};
struct Font { GLuint list=0; HGLRC context=0; clText::FontStyle style=clText::GFONT_NORMAL;
              std::array<GLYPHMETRICSFLOAT,128> metrics; };
std::array<Font,8> fonts;
unsigned textDepth=0;
GLint previousMode=GL_MODELVIEW;
}
void clText::createFont(int id,int size,FontStyle style) {
    if (!graphicsHasContext() || id<0 || id>=int(fonts.size())) return;
    Font & font=fonts[id];
    if (font.list && font.context==wglGetCurrentContext()) glDeleteLists(font.list,128);
    font.list=0;
    LOGFONTA description={}; description.lfHeight=-std::max(1,size);
    description.lfWeight=(style&GFONT_BOLD)?FW_BOLD:FW_NORMAL;
    description.lfItalic=(style&GFONT_ITALIC)!=0;
    description.lfUnderline=(style&GFONT_UNDERLINE)!=0;
    description.lfOutPrecision=OUT_TT_PRECIS;
    std::strcpy(description.lfFaceName,faces[id]);
    HFONT native=CreateFontIndirectA(&description);
    if (!native) throw std::runtime_error("Cannot create Graphics font");
    const HDC dc=wglGetCurrentDC(); HGDIOBJ previous=SelectObject(dc,native);
    const GLuint list=glGenLists(128);
    const bool ok=list && wglUseFontOutlinesA(dc,0,128,list,0,0,WGL_FONT_POLYGONS,font.metrics.data());
    SelectObject(dc,previous); DeleteObject(native);
    if (!ok) { if (list) glDeleteLists(list,128); throw std::runtime_error("Cannot create Graphics font outlines"); }
    font.list=list; font.context=wglGetCurrentContext(); font.style=style;
}
void clText::draw(unsigned id,float size,float x,float y,char * text,float * color,FontStyle style) {
    if (!text || !graphicsHasContext() || size<=0 || id>=fonts.size()) return;
    Font & font=fonts[id];
    if (!font.list || font.context!=wglGetCurrentContext() || font.style!=style)
        createFont(id,12,style);
    const float height=font.metrics['A'].gmfBlackBoxY;
    if (height<=0) return;
    const float scale=1.0f/(size*height);
    if (x==1) {
        float width=0; for (const unsigned char * p=(const unsigned char *)text;*p;++p)
            width+=font.metrics[*p<128?*p:'?'].gmfCellIncX;
        x=-scale*width*0.5f;
    }
    if (y==1) y=-1.0f/(2*size);
    GLint mode; glGetIntegerv(GL_MATRIX_MODE,&mode);
    glPushAttrib(GL_CURRENT_BIT|GL_LIST_BIT|GL_ENABLE_BIT);
    glDisable(GL_TEXTURE_2D); glDisable(GL_LIGHTING);
    if (color) glColor3fv(color); else glColor3f(0.7f,0.7f,0.7f);
    glMatrixMode(GL_MODELVIEW); glPushMatrix(); glLoadIdentity();
    glTranslatef(x,y,0); glScalef(scale,scale,1); glListBase(font.list);
    for (const unsigned char * p=(const unsigned char *)text;*p;++p) {
        const GLubyte glyph=*p<128?*p:'?'; glCallLists(1,GL_UNSIGNED_BYTE,&glyph);
    }
    glPopMatrix(); glMatrixMode(mode); glPopAttrib();
}
void clText::start() {
    if (!graphicsHasContext() || textDepth++) return;
    glGetIntegerv(GL_MATRIX_MODE,&previousMode);
    glPushAttrib(GL_ENABLE_BIT|GL_POLYGON_BIT); glDisable(GL_LIGHTING); glDisable(GL_DEPTH_TEST);
    glMatrixMode(GL_PROJECTION); glPushMatrix(); glLoadIdentity(); gluOrtho2D(-1,1,-1,1);
    glMatrixMode(GL_MODELVIEW); glCullFace(GL_BACK); glEnable(GL_CULL_FACE);
}
void clText::end() {
    if (!graphicsHasContext() || !textDepth || --textDepth) return;
    glMatrixMode(GL_PROJECTION); glPopMatrix(); glMatrixMode(previousMode); glPopAttrib();
}
void clText::shutDown() {
    for (Font & font:fonts) {
        if (font.list && graphicsHasContext() && font.context==wglGetCurrentContext())
            glDeleteLists(font.list,128);
        font.list=0; font.context=0;
    }
    while (textDepth && graphicsHasContext()) end();
    textDepth=0;
}
