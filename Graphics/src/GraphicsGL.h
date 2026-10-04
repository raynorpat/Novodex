#pragma once
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#include <GL/gl.h>
#include <GL/glu.h>

inline PROC graphicsGLProcedure(const char * name,const char * fallback = 0) {
    PROC result=wglGetProcAddress(name);
    if (!result || result==reinterpret_cast<PROC>(1) || result==reinterpret_cast<PROC>(2) ||
        result==reinterpret_cast<PROC>(3) || result==reinterpret_cast<PROC>(-1))
        result=fallback?wglGetProcAddress(fallback):0;
    if (result==reinterpret_cast<PROC>(1) || result==reinterpret_cast<PROC>(2) ||
        result==reinterpret_cast<PROC>(3) || result==reinterpret_cast<PROC>(-1)) return 0;
    return result;
}
inline bool graphicsHasContext() { return wglGetCurrentContext() != 0; }
inline void graphicsResetColorTransfer() {
    glPixelTransferi(GL_MAP_COLOR,GL_FALSE);
    const GLenum scales[]={GL_RED_SCALE,GL_GREEN_SCALE,GL_BLUE_SCALE,GL_ALPHA_SCALE};
    const GLenum biases[]={GL_RED_BIAS,GL_GREEN_BIAS,GL_BLUE_BIAS,GL_ALPHA_BIAS};
    for (unsigned i=0;i<4;++i) { glPixelTransferf(scales[i],1); glPixelTransferf(biases[i],0); }
}
