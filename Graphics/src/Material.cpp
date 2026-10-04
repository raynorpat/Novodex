// Material.obj reconstruction; defaults/directives in reconstruction evidence.
#include "Graphics.h"
#include "ODBlock.h"
#include "glm.h"
#include "GraphicsGL.h"
#include <algorithm>
#include <fstream>
#include <memory>
#include <stdexcept>
#include <cmath>

namespace SceneGraph {
Material::Material():specularExponent(1),shaderID(0),smooth(1),bDualPass(false) {
    std::fill(color,color+4,1.0f); std::fill(specular,specular+4,1.0f);
    std::fill(textures,textures+3,static_cast<Texture *>(0));
}
Material::~Material() {}
void Material::load(const char * filename) {
    if (!filename) throw std::invalid_argument("Null material filename");
    std::ifstream file(filename,std::ios::binary);
    if (!file) throw std::runtime_error(std::string("Cannot load material: ")+filename);
    std::unique_ptr<ODBlock> script(ODBlock::loadScript(file));
    if (!script) throw std::runtime_error(std::string("Invalid material: ")+filename+": "+(ODBlock::lastError?ODBlock::lastError:"syntax error"));
    name=filename;
    script->getBlockInt("Smooth",&smooth);
    const char * shader=0;
    shaderID=script->getBlockString("Shader",&shader)?getShaderID(shader):0;
    glmSetShader(shaderID);
    std::fill(color,color+4,0.3f); std::fill(specular,specular+4,0.3f);
    script->getBlockFloats("Color",color,4); script->getBlockFloats("Specular",specular,4);
    specularExponent=1; script->getBlockFloat("SpecExp",&specularExponent);
    std::fill(textures,textures+3,static_cast<Texture *>(0));
    Scene & scene=Scene::getInstance();
    const char * keys[]={"Tex1","Tex2","Tex3"};
    for (unsigned i=0;i<3;++i) {
        const char * textureName=0;
        if (!script->getBlockString(keys[i],&textureName)) continue;
        textures[i]=scene.getTexture(textureName);
        if (!textures[i]) {
            std::unique_ptr<Texture> texture(new Texture);
            texture->load(textureName); scene.addTexture(*texture);
            textures[i]=texture.release();
        }
    }
    bDualPass=textures[1] && glmxNumTexUnits()<2;
}
void Material::activate(Texture * overrideTexture) {
    if (!graphicsHasContext()) return;
    glmSetShader(shaderID);
    Texture * saved=textures[0];
    if (overrideTexture) textures[0]=overrideTexture;
    try { glmxActivateTexUnit(0); activatePass0(); }
    catch (...) { textures[0]=saved; throw; }
    textures[0]=saved;
    const unsigned units=std::min(3u,glmxNumTexUnits());
    for (unsigned i=1;i<units;++i) {
        glmxActivateTexUnit(i);
        if (textures[i]) {
            textures[i]->activate();
            glTexGeni(GL_S,GL_TEXTURE_GEN_MODE,GL_SPHERE_MAP);
            glTexGeni(GL_T,GL_TEXTURE_GEN_MODE,GL_SPHERE_MAP);
            glEnable(GL_TEXTURE_GEN_S); glEnable(GL_TEXTURE_GEN_T);
        } else glDisable(GL_TEXTURE_2D);
    }
    glmxActivateTexUnit(0); glShadeModel(smooth?GL_SMOOTH:GL_FLAT);
}
void Material::deactivate() {
    if (!graphicsHasContext()) return;
    for (unsigned i=0;i<std::min(3u,glmxNumTexUnits());++i) {
        glmxActivateTexUnit(i); glDisable(GL_TEXTURE_2D);
        glDisable(GL_TEXTURE_GEN_S); glDisable(GL_TEXTURE_GEN_T);
    }
    glmxActivateTexUnit(0);
}
void Material::renderAsLight(int lightNo) {
    if (!graphicsHasContext() || lightNo<0 || lightNo>=8) return;
    glLightfv(GL_LIGHT0+lightNo,GL_DIFFUSE,color);
    glLightfv(GL_LIGHT0+lightNo,GL_SPECULAR,specular);
    glLightf(GL_LIGHT0+lightNo,GL_SPOT_EXPONENT,std::max(0.0f,std::min(128.0f,specularExponent)));
}
void Material::activatePass0() {
    glMaterialfv(GL_FRONT,GL_DIFFUSE,color); glMaterialfv(GL_FRONT,GL_AMBIENT,color);
    glColor4fv(color);
    const float exponent=std::max(0.0f,std::min(128.0f,specularExponent));
    glMaterialf(GL_FRONT,GL_SHININESS,exponent);
    const float black[]={0,0,0,1};
    glMaterialfv(GL_FRONT,GL_SPECULAR,exponent==0?black:specular);
    if (textures[0]) textures[0]->activate(); else glDisable(GL_TEXTURE_2D);
}
void Material::activatePass1() {
    if (!graphicsHasContext() || !textures[1]) return;
    glDisable(GL_LIGHTING); glEnable(GL_BLEND); glBlendFunc(GL_ONE,GL_SRC_COLOR);
    glTexGeni(GL_S,GL_TEXTURE_GEN_MODE,GL_SPHERE_MAP);
    glTexGeni(GL_T,GL_TEXTURE_GEN_MODE,GL_SPHERE_MAP);
    glEnable(GL_TEXTURE_GEN_S); glEnable(GL_TEXTURE_GEN_T); glDepthMask(GL_FALSE);
    textures[1]->activate();
}
}
