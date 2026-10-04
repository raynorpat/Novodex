// Texture.obj reconstruction using the stb image adapter.
#include "Texture.h"
#include "gltx.h"
#include "GraphicsGL.h"
#include <memory>
#include <stdexcept>
#include <limits>

namespace SceneGraph {
Texture::Texture():DispListNum(~0u),width(0),height(0),originalAspect(1),pendingFilter(true) {}
Texture::~Texture() { destroy(); }
void Texture::load(const char * filename,bool filter) {
    if (filename) name=filename;
    create(filter);
}
void Texture::activate() {
    if (!graphicsHasContext()) return;
    if (DispListNum==~0u) create(pendingFilter);
    glEnable(GL_TEXTURE_2D); glBindTexture(GL_TEXTURE_2D,DispListNum);
}
void Texture::create(bool filter) {
    pendingFilter=filter;
    std::unique_ptr<GLTXimage,void(*)(GLTXimage *)> image(gltxReadBMP(name.c_str()),gltxDelete);
    if (!image) throw std::runtime_error("Cannot load texture: "+name);
    if (!graphicsHasContext()) {
        width=image->width; height=image->height;
        originalAspect=float(image->origHeight)/image->origWidth;
        return;
    }
    uploadRGBA(image->width,image->height,image->data,filter);
    originalAspect=float(image->origHeight)/image->origWidth;
}
void Texture::uploadRGBA(unsigned w,unsigned h,const unsigned char * pixels,bool filter) {
    if (!graphicsHasContext()) throw std::runtime_error("Texture upload requires a GL context");
    GLint limit=0; glGetIntegerv(GL_MAX_TEXTURE_SIZE,&limit);
    if (!pixels || !w || !h || w>unsigned(limit) || h>unsigned(limit) ||
        size_t(w)>std::numeric_limits<size_t>::max()/4/h)
        throw std::invalid_argument("Invalid texture dimensions or pixels");
    GLuint replacement=0;
    glGenTextures(1,&replacement);
    if (!replacement) throw std::runtime_error("Cannot allocate GL texture: "+name);
    glPushAttrib(GL_PIXEL_MODE_BIT); graphicsResetColorTransfer();
    typedef void (APIENTRY * BindBuffer)(GLenum,GLuint);
    BindBuffer bindBuffer=reinterpret_cast<BindBuffer>(graphicsGLProcedure("glBindBuffer","glBindBufferARB"));
    GLint unpackBuffer=0;
    if (bindBuffer) { glGetIntegerv(0x88ef,&unpackBuffer); bindBuffer(0x88ec,0); }
    GLint binding=0,alignment=0,rowLength=0,skipRows=0,skipPixels=0;
    glGetIntegerv(GL_TEXTURE_BINDING_2D,&binding); glGetIntegerv(GL_UNPACK_ALIGNMENT,&alignment);
    glGetIntegerv(GL_UNPACK_ROW_LENGTH,&rowLength);
    glGetIntegerv(GL_UNPACK_SKIP_ROWS,&skipRows);
    glGetIntegerv(GL_UNPACK_SKIP_PIXELS,&skipPixels);
    glPixelStorei(GL_UNPACK_ROW_LENGTH,0);
    glPixelStorei(GL_UNPACK_SKIP_ROWS,0);
    glPixelStorei(GL_UNPACK_SKIP_PIXELS,0);
    glBindTexture(GL_TEXTURE_2D,replacement); glPixelStorei(GL_UNPACK_ALIGNMENT,1);
    const int status=filter?gluBuild2DMipmaps(GL_TEXTURE_2D,GL_RGBA,w,h,
                                           GL_RGBA,GL_UNSIGNED_BYTE,pixels):0;
    if (!filter) glTexImage2D(GL_TEXTURE_2D,0,GL_RGBA,w,h,0,
                             GL_RGBA,GL_UNSIGNED_BYTE,pixels);
    GLint allocatedWidth=0; glGetTexLevelParameteriv(GL_TEXTURE_2D,0,GL_TEXTURE_WIDTH,&allocatedWidth);
    glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,filter?GL_LINEAR:GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,filter?GL_LINEAR_MIPMAP_LINEAR:GL_NEAREST);
    glPixelStorei(GL_UNPACK_ALIGNMENT,alignment);
    glPixelStorei(GL_UNPACK_ROW_LENGTH,rowLength);
    glPixelStorei(GL_UNPACK_SKIP_ROWS,skipRows);
    glPixelStorei(GL_UNPACK_SKIP_PIXELS,skipPixels);
    glBindTexture(GL_TEXTURE_2D,binding);
    if (bindBuffer) bindBuffer(0x88ec,unpackBuffer);
    glPopAttrib();
    if (status || !allocatedWidth) { glDeleteTextures(1,&replacement); throw std::runtime_error("Cannot upload texture: "+name); }
    destroy(); DispListNum=replacement; width=w; height=h;
    originalAspect=float(h)/w; pendingFilter=filter;
}
void Texture::destroy() {
    if (DispListNum!=~0u && graphicsHasContext()) glDeleteTextures(1,&DispListNum);
    DispListNum=~0u;
}
}
