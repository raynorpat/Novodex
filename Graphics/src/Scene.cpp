// Recovered Scene.obj behavior; see reconstruction/Scene.oracle.cpp.txt.
#include "Graphics.h"
#include "GraphicsGL.h"
#include <algorithm>
#include <cmath>
#include <stdexcept>

namespace {
template<class T> void addUnique(std::vector<T *> & list, T & value) {
    if (std::find(list.begin(),list.end(),&value)==list.end()) list.push_back(&value);
}
template<class T> void removeEntry(std::vector<T *> & list, T & value) {
    const auto it=std::find(list.begin(),list.end(),&value);
    if (it!=list.end()) { *it=list.back(); list.pop_back(); }
}
template<class T> T * findName(const std::vector<T *> & list,const char * name) {
    if (name) for (T * entry:list) if (entry && entry->name==name) return entry;
    return 0;
}
template<class T> void deleteOwned(std::vector<T *> & list) {
    while (!list.empty()) { T * value=list.back(); list.pop_back(); delete value; }
}
}
namespace SceneGraph {
Scene * Scene::scene=0;
Scene & Scene::getInstance() { if (!scene) scene=new Scene; return *scene; }
void Scene::shutDown() {
    // Keep the singleton reachable while object destructors notify it.
    Scene * old=scene; delete old; scene=0;
}
Scene::Scene():camera(0),nearPlane(0.1f),farPlane(1000),FOVAngle(60),aspect(1) {
    std::fill(lights,lights+8,static_cast<Object *>(0));
}
Scene::~Scene() { purge(); if (scene==this) scene=0; }
void Scene::addObject(Object & object) {
    if (object.parentObj) object.parentObj->removeChild(&object);
    object.parentObj=0;
    addUnique(objectList,object);
}
void Scene::addModel(Model & value) { addUnique(modelList,value); }
void Scene::addMesh(Mesh & value) { addUnique(meshList,value); }
void Scene::addMaterial(Material & value) { addUnique(materialList,value); }
void Scene::addTexture(Texture & value) { addUnique(textureList,value); }
void Scene::removeObject(Object & value) { removeEntry(objectList,value); }
void Scene::removeModel(Model & value) { removeEntry(modelList,value); }
void Scene::removeMesh(Mesh & value) { removeEntry(meshList,value); }
void Scene::removeMaterial(Material & value) { removeEntry(materialList,value); }
void Scene::removeTexture(Texture & value) { removeEntry(textureList,value); }
Model * Scene::getModel(const char * name) { return findName(modelList,name); }
Mesh * Scene::getMesh(const char * name) { return findName(meshList,name); }
Material * Scene::getMaterial(const char * name) { return findName(materialList,name); }
Texture * Scene::getTexture(const char * name) { return findName(textureList,name); }
void Scene::deletedObject(Object & object) {
    if (camera==&object) camera=0;
    for (Object * & light:lights) if (light==&object) light=0;
    removeObject(object);
}
void Scene::purge() {
    camera=0; std::fill(lights,lights+8,static_cast<Object *>(0));
    deleteOwned(objectList); deleteOwned(modelList); deleteOwned(meshList);
    deleteOwned(materialList); deleteOwned(textureList);
}
void Scene::clear(bool color,bool depth) {
    if (graphicsHasContext()) glClear((color?GL_COLOR_BUFFER_BIT:0)|(depth?GL_DEPTH_BUFFER_BIT:0));
}
void Scene::render() {
    if (!graphicsHasContext()) return;
    glMatrixMode(GL_MODELVIEW); glLoadIdentity();
    if (camera) camera->renderAsCamera();
    for (int i=0;i<8;++i) {
        if (lights[i]) lights[i]->renderAsLight(i);
    }
    for (Object * object:objectList) if (object) object->render();
}
void Scene::setCamera(Object * object) { camera=object; }
void Scene::setLight(int index,Object * object) {
    if (index<0 || index>=8) throw std::out_of_range("Scene light index");
    lights[index]=object;
    if (!object && graphicsHasContext()) glDisable(GL_LIGHT0+index);
}
void Scene::setProjection(flo nearValue,flo farValue,flo angle,flo ratio) {
    if (!std::isfinite(nearValue)||!std::isfinite(farValue)||!std::isfinite(angle)||
        !std::isfinite(ratio)||ratio<=0||farValue<=nearValue||angle<0||angle>=180||
        (angle!=0 && nearValue<=0)) throw std::invalid_argument("Invalid scene projection");
    nearPlane=nearValue; farPlane=farValue; FOVAngle=angle; aspect=ratio;
    if (!graphicsHasContext()) return;
    glMatrixMode(GL_PROJECTION); glLoadIdentity();
    if (angle==0) glOrtho(-50,50,-50,50,nearValue,farValue);
    else gluPerspective(angle,ratio,nearValue,farValue);
    glMatrixMode(GL_MODELVIEW);
}
}
