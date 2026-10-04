// Modern completion of the originally empty Scene renderToTexture entry point.
// FBO contracts: https://registry.khronos.org/OpenGL/extensions/EXT/EXT_framebuffer_object.txt
#include "Graphics.h"
#include "GraphicsGL.h"
#include <vector>
#include <cmath>
#include <stdexcept>
#include <limits>
#include <algorithm>
#include <cstring>

namespace {
const GLenum Framebuffer=0x8d40, Renderbuffer=0x8d41, ColorAttachment=0x8ce0;
struct FramebufferAPI {
    typedef void (APIENTRY * Bind)(GLenum,GLuint);
    typedef void (APIENTRY * Gen)(GLsizei,GLuint *);
    typedef void (APIENTRY * Delete)(GLsizei,const GLuint *);
    typedef void (APIENTRY * Storage)(GLenum,GLenum,GLsizei,GLsizei);
    typedef void (APIENTRY * AttachTexture)(GLenum,GLenum,GLenum,GLuint,GLint);
    typedef void (APIENTRY * AttachBuffer)(GLenum,GLenum,GLenum,GLuint);
    typedef GLenum (APIENTRY * Status)(GLenum);
    Bind bind,bindRenderbuffer;
    Gen gen,genRenderbuffer;
    Delete remove,removeRenderbuffer;
    Storage storage;
    AttachTexture attachTexture;
    AttachBuffer attachBuffer;
    Status status;
    bool core;
    FramebufferAPI() {
        core=graphicsGLProcedure("glBindFramebuffer")!=0;
        bind=reinterpret_cast<Bind>(graphicsGLProcedure("glBindFramebuffer","glBindFramebufferEXT"));
        bindRenderbuffer=reinterpret_cast<Bind>(graphicsGLProcedure("glBindRenderbuffer","glBindRenderbufferEXT"));
        gen=reinterpret_cast<Gen>(graphicsGLProcedure("glGenFramebuffers","glGenFramebuffersEXT"));
        genRenderbuffer=reinterpret_cast<Gen>(graphicsGLProcedure("glGenRenderbuffers","glGenRenderbuffersEXT"));
        remove=reinterpret_cast<Delete>(graphicsGLProcedure("glDeleteFramebuffers","glDeleteFramebuffersEXT"));
        removeRenderbuffer=reinterpret_cast<Delete>(graphicsGLProcedure("glDeleteRenderbuffers","glDeleteRenderbuffersEXT"));
        storage=reinterpret_cast<Storage>(graphicsGLProcedure("glRenderbufferStorage","glRenderbufferStorageEXT"));
        attachTexture=reinterpret_cast<AttachTexture>(graphicsGLProcedure("glFramebufferTexture2D","glFramebufferTexture2DEXT"));
        attachBuffer=reinterpret_cast<AttachBuffer>(graphicsGLProcedure("glFramebufferRenderbuffer","glFramebufferRenderbufferEXT"));
        status=reinterpret_cast<Status>(graphicsGLProcedure("glCheckFramebufferStatus","glCheckFramebufferStatusEXT"));
        if (!bind || !bindRenderbuffer || !gen || !genRenderbuffer || !remove ||
            !removeRenderbuffer || !storage || !attachTexture || !attachBuffer || !status)
            throw std::runtime_error("Render-to-texture requires framebuffer object support");
    }
};
struct RenderState {
    FramebufferAPI & api;
    GLuint framebuffer=0,depth=0,stencil=0;
    GLint previousDraw=0,previousRead=0,previousDepth=0,mode=GL_MODELVIEW,packBuffer=0;
    FramebufferAPI::Bind bindBuffer;
    explicit RenderState(FramebufferAPI & value):api(value) {
        glGetIntegerv(0x8ca6,&previousDraw);
        if (api.core) glGetIntegerv(0x8caa,&previousRead); else previousRead=previousDraw;
        glGetIntegerv(0x8ca7,&previousDepth); glGetIntegerv(GL_MATRIX_MODE,&mode);
        bindBuffer=reinterpret_cast<FramebufferAPI::Bind>(graphicsGLProcedure("glBindBuffer","glBindBufferARB"));
        if (bindBuffer) glGetIntegerv(0x88ed,&packBuffer);
        glPushAttrib(GL_ALL_ATTRIB_BITS); glPushClientAttrib(GL_CLIENT_PIXEL_STORE_BIT|GL_CLIENT_VERTEX_ARRAY_BIT);
        glMatrixMode(GL_PROJECTION); glPushMatrix(); glMatrixMode(GL_MODELVIEW); glPushMatrix();
    }
    ~RenderState() {
        if (api.core) { api.bind(0x8ca9,previousDraw); api.bind(0x8ca8,previousRead); }
        else api.bind(Framebuffer,previousDraw);
        api.bindRenderbuffer(Renderbuffer,previousDepth);
        if (bindBuffer) bindBuffer(0x88eb,packBuffer);
        glMatrixMode(GL_MODELVIEW); glPopMatrix(); glMatrixMode(GL_PROJECTION); glPopMatrix();
        glPopClientAttrib(); glPopAttrib(); glMatrixMode(mode);
        if (depth) api.removeRenderbuffer(1,&depth);
        if (stencil) api.removeRenderbuffer(1,&stencil);
        if (framebuffer) api.remove(1,&framebuffer);
    }
};
clMatrix4x4 objectTransform(const SceneGraph::Object & object) {
    clMatrix4x4 result;
    if (object.bNullXform) return result;
    result=object.model2World;
    if (object.scale!=0) for (int c=0;c<3;++c) for (int r=0;r<3;++r) result.M44[c][r]*=object.scale;
    return result;
}
}
namespace SceneGraph {
void Scene::renderToTexture(float fov,int w,int h,int destinationWidth,Texture & destination,
                            Object * target,int renderMode,Object * selectedCamera) {
    if (!graphicsHasContext()) throw std::runtime_error("Render-to-texture requires a GL context");
    if (w<=0 || h<=0 || destinationWidth<=0 || !std::isfinite(fov) || fov<0 || fov>=180 ||
        (renderMode & ~(TextureWireframe|TextureUnlit)))
        throw std::invalid_argument("Invalid render-to-texture arguments");
    const double outputHeight=double(destinationWidth)*h/w;
    if (outputHeight<1 || outputHeight>std::numeric_limits<int>::max())
        throw std::invalid_argument("Invalid render-to-texture output aspect");
    const unsigned destinationHeight=static_cast<unsigned>(std::floor(outputHeight+0.5));
    FramebufferAPI api;
    GLint maxTexture=0,maxBuffer=0;
    glGetIntegerv(GL_MAX_TEXTURE_SIZE,&maxTexture); glGetIntegerv(0x84e8,&maxBuffer);
    if (w>maxTexture || h>maxTexture || w>maxBuffer || h>maxBuffer ||
        destinationWidth>maxTexture || destinationHeight>unsigned(maxTexture) ||
        size_t(w)>std::numeric_limits<size_t>::max()/4/h ||
        size_t(destinationWidth)>std::numeric_limits<size_t>::max()/4/destinationHeight)
        throw std::invalid_argument("Render-to-texture dimensions exceed GL limits");
    // Allocate all CPU storage before changing caller GL state.
    std::vector<unsigned char> pixels(size_t(w)*h*4,0);
    std::vector<unsigned char> resized;
    if (destinationWidth!=w || destinationHeight!=unsigned(h))
        resized.resize(size_t(destinationWidth)*destinationHeight*4);
    RenderState state(api);
    graphicsResetColorTransfer();
    Texture color; color.uploadRGBA(w,h,pixels.data(),false);
    api.gen(1,&state.framebuffer); api.genRenderbuffer(1,&state.depth);
    if (!state.framebuffer || !state.depth) throw std::runtime_error("Cannot allocate framebuffer objects");
    api.bind(Framebuffer,state.framebuffer);
    api.attachTexture(Framebuffer,ColorAttachment,GL_TEXTURE_2D,color.DispListNum,0);
    const char * extensions=reinterpret_cast<const char *>(glGetString(GL_EXTENSIONS));
    const bool packed=api.core || (extensions && std::strstr(extensions,"GL_EXT_packed_depth_stencil"));
    api.bindRenderbuffer(Renderbuffer,state.depth); api.storage(Renderbuffer,packed?0x88f0:0x81a6,w,h);
    api.attachBuffer(Framebuffer,0x8d00,Renderbuffer,state.depth);
    if (packed) api.attachBuffer(Framebuffer,0x8d20,Renderbuffer,state.depth);
    else {
        api.genRenderbuffer(1,&state.stencil);
        if (!state.stencil) throw std::runtime_error("Cannot allocate stencil renderbuffer");
        api.bindRenderbuffer(Renderbuffer,state.stencil); api.storage(Renderbuffer,0x8d48,w,h);
        api.attachBuffer(Framebuffer,0x8d20,Renderbuffer,state.stencil);
    }
    glDrawBuffer(ColorAttachment); glReadBuffer(ColorAttachment);
    if (api.status(Framebuffer)!=0x8cd5) throw std::runtime_error("Incomplete offscreen framebuffer");
    glViewport(0,0,w,h); glDisable(GL_SCISSOR_TEST); glColorMask(GL_TRUE,GL_TRUE,GL_TRUE,GL_TRUE);
    glDepthMask(GL_TRUE); glClearDepth(1); glEnable(GL_DEPTH_TEST); glDepthFunc(GL_LEQUAL);
    glStencilMask(~0u); glClearStencil(0);
    glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT|GL_STENCIL_BUFFER_BIT);
    glMatrixMode(GL_PROJECTION); glLoadIdentity();
    if (fov==0) glOrtho(-50,50,-50,50,nearPlane,farPlane);
    else {
        const double vertical=2*std::atan(std::tan(fov*0.008726646259971648)*h/w)*57.29577951308232;
        gluPerspective(vertical,double(w)/h,nearPlane,farPlane);
    }
    glMatrixMode(GL_MODELVIEW); glLoadIdentity();
    Object * view=selectedCamera?selectedCamera:camera;
    if (view) view->renderAsCamera();
    for (int i=0;i<8;++i) if (lights[i]) lights[i]->renderAsLight(i);
    if (renderMode & TextureUnlit) glDisable(GL_LIGHTING);
    if (renderMode & TextureWireframe) glPolygonMode(GL_FRONT_AND_BACK,GL_LINE);
    if (target) {
        std::vector<Object *> ancestors;
        for (Object * parent=target->parentObj;parent;parent=parent->parentObj) ancestors.push_back(parent);
        for (auto parent=ancestors.rbegin();parent!=ancestors.rend();++parent) {
            clMatrix4x4 transform=objectTransform(**parent); glMultMatrixf(transform.M16);
        }
        target->render();
    } else for (Object * object:objectList) if (object) object->render();
    if (state.bindBuffer) state.bindBuffer(0x88eb,0);
    glPixelStorei(GL_PACK_ALIGNMENT,1); glPixelStorei(GL_PACK_ROW_LENGTH,0);
    glPixelStorei(GL_PACK_SKIP_ROWS,0); glPixelStorei(GL_PACK_SKIP_PIXELS,0);
    glReadPixels(0,0,w,h,GL_RGBA,GL_UNSIGNED_BYTE,pixels.data());
    if (!resized.empty()) {
        // A CPU bilinear resample avoids coupling scaling to caller pixel-store
        // state or changing the requested destination to a power of two.
        for (unsigned y=0;y<destinationHeight;++y) for (int x=0;x<destinationWidth;++x) {
            const double sx=std::max(0.0,std::min(double(w-1),(x+0.5)*w/destinationWidth-0.5));
            const double sy=std::max(0.0,std::min(double(h-1),(y+0.5)*h/destinationHeight-0.5));
            const unsigned x0=unsigned(sx),y0=unsigned(sy),x1=std::min(x0+1,unsigned(w-1)),y1=std::min(y0+1,unsigned(h-1));
            const double fx=sx-x0,fy=sy-y0;
            for (int c=0;c<4;++c) {
                const double a=pixels[4*(size_t(y0)*w+x0)+c]*(1-fx)+pixels[4*(size_t(y0)*w+x1)+c]*fx;
                const double b=pixels[4*(size_t(y1)*w+x0)+c]*(1-fx)+pixels[4*(size_t(y1)*w+x1)+c]*fx;
                resized[4*(size_t(y)*destinationWidth+x)+c]=static_cast<unsigned char>(a*(1-fy)+b*fy+0.5);
            }
        }
    }
    destination.uploadRGBA(destinationWidth,destinationHeight,resized.empty()?pixels.data():resized.data(),false);
}
}
