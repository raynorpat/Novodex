#include "Graphics.h"
#include "GraphicsGL.h"
#include <memory>
#include <stdexcept>
#include <utility>
#include <vector>

namespace SceneGraph {
namespace {
clMatrix4x4 localTransform(const Object & object)
{
    clMatrix4x4 matrix;
    if (object.bNullXform) return matrix;
    matrix=object.model2World;
    if (object.scale!=0) for (int c=0;c<3;++c) for (int r=0;r<3;++r) matrix.M44[c][r]*=object.scale;
    return matrix;
}
clMatrix4x4 worldTransform(const Object & object)
{
    std::vector<const Object *> path;
    for (const Object * node=&object;node;node=node->parentObj) path.push_back(node);
    clMatrix4x4 matrix;
    for (auto node=path.rbegin();node!=path.rend();++node) matrix=matrix*localTransform(**node);
    return matrix;
}
// Keep primitive-local drawing intact (including opaque display lists/custom
// primitives) while moving the object's transform into the owned draw wrapper.
class BakedPrimitive : public Primitive {
public:
    explicit BakedPrimitive(const clMatrix4x4 & value):transform(value) {}
    void adopt(Primitive * value) { content.reset(value); }
    void render() override {
        if (!content) return;
        if (!graphicsHasContext()) { content->render(); return; }
        GLint mode; glGetIntegerv(GL_MATRIX_MODE,&mode);
        glMatrixMode(GL_MODELVIEW); glPushMatrix(); glMultMatrixf(transform.M16);
        try { content->render(); }
        catch (...) { glMatrixMode(GL_MODELVIEW); glPopMatrix(); glMatrixMode(mode); throw; }
        glMatrixMode(GL_MODELVIEW); glPopMatrix(); glMatrixMode(mode);
    }
private:
    clMatrix4x4 transform;
    std::unique_ptr<Primitive> content;
};
struct ObjectRenderScope {
    bool context,matrix,attributes;
    GLint mode=GL_MODELVIEW;
    explicit ObjectRenderScope(const Object & object):context(graphicsHasContext()),
        matrix(context&&!object.bNullXform),attributes(context&&(object.bWireFrame||object.bTwoSided)) {
        if (!context) return;
        glGetIntegerv(GL_MATRIX_MODE,&mode); glMatrixMode(GL_MODELVIEW);
        if (matrix) {
            glPushMatrix(); glMultMatrixf(object.model2World.M16);
            if (object.scale!=0) glScalef(object.scale,object.scale,object.scale);
        }
        if (attributes) {
            glPushAttrib(GL_POLYGON_BIT|GL_ENABLE_BIT|GL_LIGHTING_BIT);
            if (object.bWireFrame) glPolygonMode(GL_FRONT_AND_BACK,GL_LINE);
            if (object.bTwoSided) { glDisable(GL_CULL_FACE); glLightModelf(GL_LIGHT_MODEL_TWO_SIDE,1); }
        }
    }
    ~ObjectRenderScope() {
        if (!context) return;
        if (attributes) glPopAttrib();
        glMatrixMode(GL_MODELVIEW); if (matrix) glPopMatrix(); glMatrixMode(mode);
    }
};
struct ModelRenderScope {
    bool context=graphicsHasContext();
    GLint mode=GL_MODELVIEW;
    explicit ModelRenderScope(const clMatrix4x4 & transform) {
        if (!context) return;
        glGetIntegerv(GL_MATRIX_MODE,&mode);
        glMatrixMode(GL_MODELVIEW); glPushMatrix(); glMultMatrixf(transform.M16);
    }
    ~ModelRenderScope() {
        if (!context) return;
        glMatrixMode(GL_MODELVIEW); glPopMatrix(); glMatrixMode(mode);
    }
};
}
Object::Object(Model * m, Object * parent)
    : model(m), primitive(0), scale(0), position(0,0,0), bNullXform(false),
      leftObj(0), middleObj(0), rightObj(0), parentObj(parent),
      bHideModel(false), bHideSubtree(false), bWireFrame(false), bTwoSided(false)
{
    qOrientation.Zero();
}
Object::~Object()
{
    if (parentObj) parentObj->removeChild(this);
    if (Scene * scene=Scene::existingInstance()) scene->deletedObject(*this);
    deleteSubtree(); delete primitive;
}
void Object::render()
{
    const bool hasChildren=!bHideSubtree && (leftObj || middleObj || rightObj);
    const bool renderModel=!bNullXform && !bHideModel && model;
    if (!primitive && !renderModel && !hasChildren) return;
    ObjectRenderScope state(*this);
    if (renderModel) {
        ModelRenderScope modelState(bakedModelTransform);
        model->render();
    }
    if (primitive && !bHideModel) primitive->render();
    if (hasChildren) {
        if (leftObj) leftObj->render();
        if (middleObj) middleObj->render();
        if (rightObj) rightObj->render();
    }
}
void Object::renderAsCamera()
{
    if (!graphicsHasContext()) return;
    clMatrix4x4 view=worldTransform(*this); view.Invert3x4(); glLoadMatrixf(view.M16);
}
void Object::renderAsLight(int lightNo)
{
    if (!graphicsHasContext() || lightNo<0 || lightNo>=8) return;
    clMatrix4x4 world=worldTransform(*this);
    const GLfloat diffuse[]={0.8f,0.8f,0.8f,1};
    const GLfloat specular[]={0.1f,0.1f,0.1f,1};
    const GLfloat point[]={world.M16[12],world.M16[13],world.M16[14],1};
    const GLenum light=GL_LIGHT0+lightNo;
    glLightfv(light,GL_DIFFUSE,diffuse); glLightfv(light,GL_SPECULAR,specular);
    glLightf(light,GL_SPOT_EXPONENT,8); glLightfv(light,GL_POSITION,point); glEnable(light);
}
void Object::setScale(flo value) { bNullXform=false; scale=value; }
void Object::setPosition(flo x, flo y, flo z)
{
    bNullXform=false; position.Set(x,y,z); model2World.SetTranslation(position);
}
void Object::setPosition(Vec3 * value) { if (value) setPosition(value->x,value->y,value->z); }
void Object::setOrientation(Quat * value)
{
    if (!value) return;
    bNullXform=false; qOrientation=*value; orientation.FromQuat(qOrientation); updateMatrix();
}
void Object::setOrientation(Vec3 * angles)
{
    if (!angles) return;
    const flo yaw=angles->x*0.008726646259971648f;
    const flo pitch=angles->y*0.008726646259971648f;
    const flo roll=angles->z*0.008726646259971648f;
    const flo cy=cosf(yaw),sy=sinf(yaw),cp=cosf(pitch),sp=sinf(pitch),cr=cosf(roll),sr=sinf(roll);
    Quat q; q.Set(cy*cp*cr+sy*sp*sr, cy*cp*sr-sy*sp*cr,cy*sp*cr+sy*cp*sr,sy*cp*cr-cy*sp*sr);
    setOrientation(&q);
}
void Object::setVisibility(bool visible) { bHideModel=!visible; }
void Object::lookAt(Vec3 * target, Vec3 * up)
{
    if (!target || !up) return;
    Vec3 forward=*target-position; forward.Normalize();
    Vec3 right=forward.Cross(*up); right.Normalize();
    Vec3 correctedUp=right.Cross(forward); correctedUp.Normalize();
    if (forward.isZero() || right.isZero()) return;
    bNullXform=false; orientation.ID();
    orientation.M16[0]=right.x; orientation.M16[1]=right.y; orientation.M16[2]=right.z;
    orientation.M16[4]=correctedUp.x; orientation.M16[5]=correctedUp.y; orientation.M16[6]=correctedUp.z;
    orientation.M16[8]=-forward.x; orientation.M16[9]=-forward.y; orientation.M16[10]=-forward.z;
    Mat33 rotation; for (int c=0;c<3;++c) for (int r=0;r<3;++r) rotation.M33[c][r]=orientation.M44[c][r];
    qOrientation.fromMatrix(rotation); updateMatrix();
}
void Object::updateMatrix()
{
    if (bNullXform) return;
    model2World.ID(); model2World.Copy3x3(orientation); model2World.SetTranslation(position);
}
Object * Object::createChild(Model * m)
{
    std::unique_ptr<Object> child(new Object(m)); addChild(child.get()); return child.release();
}
void Object::addChild(Object * child)
{
    if (!child) return;
    for (Object * node=this;node;node=node->parentObj) if (node==child) return;
    if (child->parentObj==this && (leftObj==child || middleObj==child || rightObj==child)) return;
    if (child->parentObj) child->parentObj->removeChild(child);
    if (Scene * scene=Scene::existingInstance()) scene->removeObject(*child);
    Object * storage=this;
    while (storage->leftObj && storage->middleObj && storage->rightObj) {
        if (!storage->leftObj->bNullXform) {
            std::unique_ptr<Object> branch(new Object(0,storage)); branch->bNullXform=true;
            Object * original=storage->leftObj;
            // Publish only after allocation succeeds; keep exactly one owner.
            branch->leftObj=original; original->parentObj=branch.get();
            storage->leftObj=branch.release();
        }
        storage=storage->leftObj;
    }
    child->parentObj=storage;
    if (!storage->leftObj) storage->leftObj=child;
    else if (!storage->middleObj) storage->middleObj=child;
    else storage->rightObj=child;
}
bool Object::deleteChild(Object * child)
{
    if (!removeChild(child)) return false;
    delete child; return true;
}
bool Object::removeChild(Object * child)
{
    if (!child) return false;
    Object ** slots[]={&leftObj,&middleObj,&rightObj};
    for (Object ** slot : slots) if (*slot==child) { *slot=0; child->parentObj=0; return true; }
    for (Object ** slot : slots) if (*slot && (*slot)->removeChild(child)) return true;
    return false;
}
void Object::getBoundingBox(Vec3 & dims)
{
    if (model) model->getBoundingBox(dims); else dims.Set(1,1,1);
    const Vec3 original=dims;
    for (int r=0;r<3;++r)
        dims[r]=fabsf(bakedModelTransform.M44[0][r])*original.x
            +fabsf(bakedModelTransform.M44[1][r])*original.y
            +fabsf(bakedModelTransform.M44[2][r])*original.z;
    if (scale!=0) dims*=scale;
}
Model * Object::collapseSubtreeToNewModel(Model * destination, bool keepMaterials, bool meshesInParentsSpace, clMatrix4x4 * transform)
{
    if (!destination) {
        std::unique_ptr<Model> created(new Model);
        Scene::getInstance().addModel(*created); destination=created.release();
    }
    Iterator iterator(this); iterator.setCumulateTransforms(true,meshesInParentsSpace);
    while (Object * object=iterator.getNextObject()) {
        if (object->bNullXform || !object->model) continue;
        clMatrix4x4 cumulative;
        if (clMatrix4x4 * current=iterator.getCumulatedTransform()) cumulative=*current;
        cumulative=cumulative*object->bakedModelTransform;
        if (transform) cumulative=*transform*cumulative;
        object->model->merge(keepMaterials,*destination,&cumulative);
    }
    if (!destination->mesh) {
        std::unique_ptr<Mesh> created(new Mesh); Scene::getInstance().addMesh(*created); destination->mesh=created.release();
    }
    return destination;
}
void Object::deleteSubtree()
{
    Object * children[]={leftObj,middleObj,rightObj}; removeSubtree();
    for (Object * child : children) delete child;
}
void Object::removeSubtree()
{
    if (leftObj) leftObj->parentObj=0;
    if (middleObj) middleObj->parentObj=0;
    if (rightObj) rightObj->parentObj=0;
    leftObj=middleObj=rightObj=0;
}
void Object::executeTransform()
{
    // Snapshot before resetting any transform. Per-node geometry copies preserve
    // shared nonowned resources referenced elsewhere in the scene.
    std::vector<std::pair<Object *,clMatrix4x4>> nodes;
    Iterator iterator(this); iterator.setCumulateTransforms(true,true);
    while (Object * object=iterator.getNextObject()) {
        nodes.emplace_back(object,*iterator.getCumulatedTransform());
    }
    std::vector<std::pair<Object *,std::unique_ptr<Model>>> bakedModels;
    std::vector<std::pair<Object *,std::unique_ptr<BakedPrimitive>>> bakedPrimitives;
    for (auto & entry : nodes) {
        Object * object=entry.first;
        if (object->primitive)
            bakedPrimitives.emplace_back(object,std::unique_ptr<BakedPrimitive>(new BakedPrimitive(entry.second)));
        if (!object->bNullXform && object->model && object->model->mesh) {
            std::unique_ptr<Model> baked(new Model);
            baked->name=object->model->name; baked->userData=object->model->userData;
            clMatrix4x4 transform=entry.second*object->bakedModelTransform;
            object->model->merge(true,*baked,&transform);
            bakedModels.emplace_back(object,std::move(baked));
        }
    }
    if (!bakedModels.empty()) {
        Scene & scene=Scene::getInstance();
        scene.modelList.reserve(scene.modelList.size()+bakedModels.size());
        for (auto & entry : bakedModels) {
            scene.addModel(*entry.second); entry.first->model=entry.second.release();
            entry.first->bakedModelTransform.ID();
        }
    }
    for (auto & entry : bakedPrimitives) {
        entry.second->adopt(entry.first->primitive);
        entry.first->primitive=entry.second.release();
    }
    for (auto & entry : nodes) {
        Object * object=entry.first;
        // Preserve virtual rendering for custom models with no mesh to rewrite.
        if (!object->bNullXform && object->model && !object->model->mesh)
            object->bakedModelTransform=entry.second*object->bakedModelTransform;
        object->scale=0; object->position.Zero(); object->qOrientation.Zero();
        object->orientation.ID(); object->model2World.ID();
    }
}
}
