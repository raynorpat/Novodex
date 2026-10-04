#include "Graphics.h"
#include <vector>

namespace SceneGraph {
Iterator::Iterator(Object * object) { init(); o=starto=object; }
Iterator::Iterator(Model * model) { init(); mo=model; }
Iterator::Iterator(Mesh * mesh) { init(); me=mesh; visit(mesh); }
Iterator::~Iterator() {}
void Iterator::init()
{
    o=starto=0; mo=currentVisitModel=0; me=currentVisitMesh=0;
    subMeshIndex=materialIndex=numDelayedPops=0;
    cumulateTransforms=cumulateRoot=false;
}
void Iterator::setCumulateTransforms(bool cumulate, bool root)
{
    cumulateTransforms=cumulate; cumulateRoot=root;
    if (!cumulate) xformStack.clear();
}
clMatrix4x4 * Iterator::getCumulatedTransform() { return xformStack.empty() ? 0 : &xformStack.back(); }
Object * Iterator::getNextObject()
{
    if (!o) return 0;
    Object * result=o;
    if (o->leftObj) o=o->leftObj;
    else if (o->middleObj) o=o->middleObj;
    else if (o->rightObj) o=o->rightObj;
    else {
        Object * node=o; o=0;
        while (node!=starto && node->parentObj) {
            Object * parent=node->parentObj;
            if (node==parent->leftObj && parent->middleObj) { o=parent->middleObj; break; }
            if ((node==parent->leftObj || node==parent->middleObj) && parent->rightObj) { o=parent->rightObj; break; }
            node=parent;
        }
    }
    visit(result); descend(result);
    return result;
}
Model * Iterator::getNextModelOfCurrObject()
{
    Model * result=mo; mo=0; if (result) visit(result); return result;
}
Material * Iterator::getNextMaterialOfCurrModel()
{
    if (!currentVisitModel || materialIndex>=currentVisitModel->materialList.size()) return 0;
    return currentVisitModel->materialList[materialIndex++];
}
Mesh * Iterator::getNextMeshOfCurrModel()
{
    Mesh * result=me; me=0; if (result) visit(result); return result;
}
SubMesh * Iterator::getNextSubmeshOfCurrMesh()
{
    if (!currentVisitMesh || subMeshIndex>=currentVisitMesh->subMeshList.size()) return 0;
    return currentVisitMesh->subMeshList[subMeshIndex++];
}
void Iterator::visit(Object * object)
{
    mo=object->bNullXform ? 0 : object->model;
    currentVisitModel=0; me=currentVisitMesh=0; subMeshIndex=materialIndex=0;
}
void Iterator::visit(Model * model)
{
    currentVisitModel=model; me=model ? model->mesh : 0; currentVisitMesh=0;
    materialIndex=subMeshIndex=0;
}
void Iterator::visit(Mesh * mesh) { currentVisitMesh=mesh; subMeshIndex=0; }
void Iterator::descend(Object * object)
{
    xformStack.clear();
    if (!cumulateTransforms) return;
    std::vector<Object *> path;
    for (Object * node=object; node; node=node->parentObj) {
        if (node!=starto || cumulateRoot) path.push_back(node);
        if (node==starto) break;
    }
    if (path.empty()) return;
    clMatrix4x4 transform;
    for (auto node=path.rbegin(); node!=path.rend(); ++node) {
        if ((*node)->bNullXform) continue;
        clMatrix4x4 local=(*node)->model2World;
        if ((*node)->scale!=0) for (int c=0;c<3;++c) for (int r=0;r<3;++r) local.M44[c][r]*=(*node)->scale;
        transform=transform*local;
    }
    xformStack.push_back(transform);
}
void Iterator::ascend(Object *) { ++numDelayedPops; }
}
