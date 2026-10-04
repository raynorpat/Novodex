#include "3DClasses3.h"
#include "FNode.cpp"
#include "GeometryPrivate.h"
#include "GraphicsGL.h"
#include <algorithm>
#include <cstring>
#include <fstream>
#include <limits>
#include <memory>
#include <stdexcept>
#include <vector>

namespace {
char * duplicate(const char * text) {
    if(!text)return 0;
    char * result=new char[std::strlen(text)+1]; std::strcpy(result,text); return result;
}
void copyMesh(const SceneGraph::Mesh & source,SceneGraph::Mesh & destination) {
    SceneGraph::Model from,to; from.mesh=const_cast<SceneGraph::Mesh *>(&source); to.mesh=&destination;
    from.merge(true,to); destination.name=source.name;
}
clMesh3 * legacyMesh(const SceneGraph::Mesh * source) {
    if(!source)return 0;
    if(clMesh3 * result=dynamic_cast<clMesh3 *>(const_cast<SceneGraph::Mesh *>(source)))return result;
    std::unique_ptr<clMesh3> result(new clMesh3(*source)); clScene3::AddMesh3(*result); return result.release();
}
clMaterial * legacyMaterial(SceneGraph::Material * source) {
    if(!source)return 0;
    if(clMaterial * result=dynamic_cast<clMaterial *>(source))return result;
    std::unique_ptr<clMaterial3> result(new clMaterial3(*source));
    SceneGraph::Scene::getInstance().addMaterial(*result); return result.release();
}
void adaptMaterials(SceneGraph::Model & model) {
    for(SceneGraph::Material * & material:model.materialList)material=legacyMaterial(material);
}
class ModelViewScope {
    GLint mode;
public:
    ModelViewScope():mode(GL_MODELVIEW) { glGetIntegerv(GL_MATRIX_MODE,&mode);glMatrixMode(GL_MODELVIEW); }
    ~ModelViewScope() { glMatrixMode(mode); }
};
class MatrixAttribScope {
public:
    MatrixAttribScope() { glPushMatrix();glPushAttrib(GL_ALL_ATTRIB_BITS); }
    ~MatrixAttribScope() { glPopAttrib();glMatrixMode(GL_MODELVIEW);glPopMatrix(); }
};
class ReflectionScope {
    bool & flag;
    bool previous;
public:
    explicit ReflectionScope(bool & value):flag(value),previous(value) { flag=true; }
    ~ReflectionScope() { flag=previous; }
};
class LegacyMaterialScope {
    SceneGraph::Material * material;
    clMaterial * legacy;
    bool active;
    void deactivate() { if(legacy)legacy->Deactivate();else if(material)material->SceneGraph::Material::deactivate(); }
    void cleanup() noexcept {
        try { deactivate(); }
        catch(...) {
            // A client override may throw after the base activation acquired a frame.
            if(legacy)try { legacy->clMaterial::Deactivate(); }catch(...){}
        }
    }
public:
    explicit LegacyMaterialScope(SceneGraph::Material * value):material(value),legacy(dynamic_cast<clMaterial *>(value)),active(false) {
        try {
            if(legacy)legacy->Activate();else if(material)material->SceneGraph::Material::activate();
            active=material!=0;
        }catch(...) { cleanup();throw; }
    }
    ~LegacyMaterialScope() { if(active)cleanup(); }
    void finish() {
        if(!active)return;
        active=false;
        try { deactivate(); }
        catch(...) { if(legacy)try { legacy->clMaterial::Deactivate(); }catch(...){} throw; }
    }
};
void drawCameraSpaces(clSceneObj * camera,clSceneObj * preTranslation,clSceneObj * cameraSpace,bool beforeScene) {
    if(!graphicsHasContext())return;
    ModelViewScope mode;MatrixAttribScope frame;
    glLoadIdentity();
    if(beforeScene && preTranslation) {
        if(camera)camera->renderAsCamera();
        GLfloat matrix[16]; glGetFloatv(GL_MODELVIEW_MATRIX,matrix); matrix[12]=matrix[13]=matrix[14]=0; glLoadMatrixf(matrix);
        preTranslation->Render();
    }
    if(!beforeScene && cameraSpace) { glLoadIdentity(); cameraSpace->Render(); }
}
void freeChain(clMesh3::SubMesh * node) {
    while(node) { clMesh3::SubMesh * next=node->next; node->next=0; delete node; node=next; }
}
clMatrix4x4 objectWorld(const SceneGraph::Object * object) {
    std::vector<const SceneGraph::Object *> path;
    for(const SceneGraph::Object * node=object;node;node=node->parentObj)path.push_back(node);
    clMatrix4x4 matrix;
    for(auto node=path.rbegin();node!=path.rend();++node)if(!(*node)->bNullXform) {
        clMatrix4x4 local=(*node)->model2World;
        if((*node)->scale!=0)for(int c=0;c<3;++c)for(int r=0;r<3;++r)local.M44[c][r]*=(*node)->scale;
        matrix=matrix*local;
    }
    return matrix;
}
}

clFNodeList<clSceneObj> clScene::SceneObjList;
clFNodeList<clModel> clScene::ModelList;
clFNodeList<clMesh> clScene::MeshList;
clFNodeList<clMaterial> clScene::MaterialList;
clFNodeList<clTexture> clScene::TextureList;
clSceneObj * clScene::clopCamera=0,* clScene::clopPreCamObj=0,* clScene::clopPreTranslatObj=0;
clSceneObj * clScene::clopLights[8]={};
flo clScene::NearPlane=.1f,clScene::FarPlane=1000,clScene::FOVAngle=60,clScene::Aspect=1;
bool clScene::fullScreenDraw=false;
bool clScene3::dynamesh=false;
void clScene::Purge() {
    SceneObjList.Clear(); ModelList.Clear(); MeshList.Clear(); MaterialList.Clear(); TextureList.Clear();
    clopCamera=clopPreCamObj=clopPreTranslatObj=0; std::fill(clopLights,clopLights+8,static_cast<clSceneObj *>(0));
    if(SceneGraph::Scene * scene=SceneGraph::Scene::existingInstance())scene->purge(); fullScreenDraw=false;
}
void clScene::Clear(bool color,bool depth) { SceneGraph::Scene::getInstance().clear(color&&!fullScreenDraw,depth); }
void clScene::Render() {
    drawCameraSpaces(clopCamera,clopPreTranslatObj,0,true);
    SceneGraph::Scene::getInstance().render();
    drawCameraSpaces(clopCamera,0,clopPreCamObj,false);
}
void clScene::RenderToTexture(float fov,int width,int height,int destinationWidth,clTexture & texture,clSceneObj * target,int mode,clSceneObj * camera) {
    SceneGraph::Scene::getInstance().renderToTexture(fov,width,height,destinationWidth,static_cast<SceneGraph::Texture &>(texture),target,mode,camera);
}
void SceneGraph::Scene::renderToTexture(float fov,int width,int height,int destinationWidth,clTexture & texture,Object * target,int mode,Object * camera) {
    renderToTexture(fov,width,height,destinationWidth,static_cast<Texture &>(texture),target,mode,camera);
}
void clScene::SetCamera(clSceneObj * camera) { clopCamera=camera; SceneGraph::Scene::getInstance().setCamera(camera); }
void clScene::SetLight(int index,clSceneObj * light) {
    SceneGraph::Scene::getInstance().setLight(index,light); clopLights[index]=light;
}
void clScene::SetProjection(flo nearValue,flo farValue,flo fov,flo aspect) {
    SceneGraph::Scene::getInstance().setProjection(nearValue,farValue,fov,aspect);
    NearPlane=nearValue; FarPlane=farValue; FOVAngle=fov; Aspect=aspect;
}
clSceneObj * clScene::CreateObject(clModel * model) { std::unique_ptr<clSceneObj> result(new clSceneObj(model)); AddObject(result.get()); return result.release(); }
clSceneObj * clScene::AddObject(clSceneObj * object) {
    if(!object)return 0;
    SceneGraph::Scene & scene=SceneGraph::Scene::getInstance();
    const bool already=std::find(scene.objectList.begin(),scene.objectList.end(),object)!=scene.objectList.end();
    scene.addObject(*object); if(!already)SceneObjList.Push(object,false); return object;
}
clSceneObj * clScene::CreatePreCamObject(clModel * model) {
    DeleteObject(clopPreCamObj); clopPreCamObj=CreateObject(model); clopPreCamObj->specialKind=1; return clopPreCamObj;
}
clSceneObj * clScene::CreatePreTranslatObject(clModel * model) {
    DeleteObject(clopPreTranslatObj); clopPreTranslatObj=CreateObject(model); clopPreTranslatObj->specialKind=2; return clopPreTranslatObj;
}
clModel * clScene::CreateModel(clMesh * mesh,clMaterial * material) {
    std::unique_ptr<clModel> result(new clModel(mesh,material));
    SceneGraph::Scene::getInstance().addModel(*result); ModelList.Push(result.get(),false); return result.release();
}
clMesh * clScene::CreateMesh(const char * path,bool texCoords,float scale,bool dynamic) {
    SceneGraph::Scene & scene=SceneGraph::Scene::getInstance();
    for(SceneGraph::Mesh * mesh:scene.meshList) if(mesh && path && mesh->name==path) {
        clMesh * old=dynamic_cast<clMesh *>(mesh);
        if(old && (dynamic_cast<clDynamicMesh *>(old)!=0)==dynamic)return old;
    }
    std::unique_ptr<clMesh> result(dynamic?static_cast<clMesh *>(new clDynamicMesh(path,texCoords,scale)):new clMesh(path,texCoords,scale));
    scene.addMesh(*result); MeshList.Push(result.get(),false); return result.release();
}
clMaterial * clScene::CreateMaterial(clTexture * texture,float * color,float * specular,float exponent) {
    std::unique_ptr<clMaterial> result(new clMaterial(texture,color,specular,exponent));
    SceneGraph::Scene::getInstance().addMaterial(*result); MaterialList.Push(result.get(),false); return result.release();
}
clTexture * clScene::CreateTexture(const char * path) {
    SceneGraph::Scene & scene=SceneGraph::Scene::getInstance();
    for(SceneGraph::Texture * texture:scene.textureList) if(texture && path && texture->name==path)
        if(clTexture * old=dynamic_cast<clTexture *>(texture))return old;
    std::unique_ptr<clTexture> result(new clTexture(path)); scene.addTexture(*result); TextureList.Push(result.get(),false); return result.release();
}
void clScene::DeleteObject(clSceneObj * object,bool onlyRemove) {
    if(!object)return;
    if(object->parentObj)object->parentObj->removeChild(object);
    ObjDeleted(object);
    if(!onlyRemove)delete object;
}
void clScene::ObjDeleted(clSceneObj * object) {
    if(!object)return;
    SceneObjList.Remove(object);
    if(clopCamera==object)clopCamera=0;
    if(clopPreCamObj==object)clopPreCamObj=0;
    if(clopPreTranslatObj==object)clopPreTranslatObj=0;
    for(clSceneObj * & light:clopLights)if(light==object)light=0;
    if(SceneGraph::Scene * scene=SceneGraph::Scene::existingInstance())scene->deletedObject(*object);
}
void clScene::Hint_FullscreenDraw() { fullScreenDraw=true; }

clSceneObj::clSceneObj(clModel * appearance,bool transparent,clSceneObj * parent)
    :SceneGraph::Object(appearance,parent),specialKind(0),clopModel(model),Position(position),Orientation(orientation),Model2World(model2World),
     LeftObj(leftObj),MiddleObj(middleObj),RightObj(rightObj),ParentObj(parentObj) { bNullXform=transparent; }
clSceneObj::~clSceneObj() { clScene::ObjDeleted(this); }
void clSceneObj::render() { if(!specialKind)Render(); }
void clSceneObj::Render() { SceneGraph::Object::render(); }
void clSceneObj::RenderAsCamera(clSceneObj * cameraSpace,clSceneObj * preTranslation) {
    drawCameraSpaces(this,preTranslation,0,true); renderAsCamera(); drawCameraSpaces(this,0,cameraSpace,false);
}
void clSceneObj::RenderAsLight(int light) { renderAsLight(light); if(clModel * m=clopModel)m->RenderAsLight(light); }
void clSceneObj::SetScale(flo value) { setScale(value); }
void clSceneObj::SetPosition(flo x,flo y,flo z) { setPosition(x,y,z); }
void clSceneObj::SetPosition(Vec3 * value) { setPosition(value); }
void clSceneObj::SetOrientation(Quat * value) { setOrientation(value); }
void clSceneObj::SetOrientation(Vec3 * value) { setOrientation(value); }
void clSceneObj::LookAt(Vec3 * target,Vec3 * up) { lookAt(target,up); }
void clSceneObj::UpdateMatrix() { orientation.FromQuat(qOrientation); updateMatrix(); }
clSceneObj * clSceneObj::CreateChild(clModel * appearance) { std::unique_ptr<clSceneObj> child(new clSceneObj(appearance)); AddChild(child.get()); return child.release(); }
void clSceneObj::AddChild(clSceneObj * child) {
    if(!child)return;
    for(SceneGraph::Object * p=this;p;p=p->parentObj)if(p==child)return;
    if(child->parentObj==this && (leftObj==child || middleObj==child || rightObj==child))return;
    if(child->parentObj)child->parentObj->removeChild(child);
    if(SceneGraph::Scene * scene=SceneGraph::Scene::existingInstance())scene->removeObject(*child);
    clScene::SceneObjList.Remove(child);
    SceneGraph::Object * storage=this;
    while(storage->leftObj && storage->middleObj && storage->rightObj) {
        if(!storage->leftObj->bNullXform) {
            std::unique_ptr<clSceneObj> branch(new clSceneObj(0,true,dynamic_cast<clSceneObj *>(storage)));
            branch->leftObj=storage->leftObj; branch->leftObj->parentObj=branch.get(); storage->leftObj=branch.release();
        }
        storage=storage->leftObj;
    }
    child->parentObj=storage;
    if(!storage->leftObj)storage->leftObj=child;
    else if(!storage->middleObj)storage->middleObj=child;
    else storage->rightObj=child;
}
bool clSceneObj::DeleteChild(clSceneObj * child) { return deleteChild(child); }
void clSceneObj::getBoundingBox(Vec3 & dims) { SceneGraph::Object::getBoundingBox(dims); }
clModel * clSceneObj::collapseSubtreeToNewModel(bool keepMaterials) {
    SceneGraph::Model * merged=SceneGraph::Object::collapseSubtreeToNewModel(0,keepMaterials);
    std::unique_ptr<clModel3> result(new clModel3(*merged)); SceneGraph::Scene & scene=SceneGraph::Scene::getInstance();
    scene.addModel(*result); scene.removeModel(*merged); delete merged; return result.release();
}
void clSceneObj::executeTransform() {
    SceneGraph::Object::executeTransform();
    SceneGraph::Iterator iterator(this);
    while(SceneGraph::Object * object=iterator.getNextObject())if(object->model && !dynamic_cast<clModel *>(object->model)) {
        std::unique_ptr<clModel3> adapted(new clModel3(*object->model)); SceneGraph::Scene::getInstance().addModel(*adapted); object->model=adapted.release();
    }
}

clMesh::clMesh():DispListNum(0),Scale(0),TextureCoords(true),FileName(0){}
clMesh::clMesh(const char * filename,bool textureCoords,float scale):clMesh() { TextureCoords=textureCoords; Scale=scale; setFileName(filename); Create(); }
clMesh::~clMesh() {
    clScene::MeshList.Remove(this); if(SceneGraph::Scene * scene=SceneGraph::Scene::existingInstance())scene->removeMesh(*this); delete[] FileName;
}
void clMesh::setFileName(const char * text) { std::unique_ptr<char[]> next(duplicate(text)); delete[] FileName; FileName=next.release(); name=text?text:""; }
void clMesh::render(int index) { if(index==-1)Render(); else SceneGraph::Mesh::render(index); }
void clMesh::Render() { SceneGraph::Mesh::render(-1); }
void clMesh::Create() { Destroy(); if(FileName)SceneGraph::Mesh::load(FileName,TextureCoords,Scale); }
void clMesh::Destroy() { for(SceneGraph::SubMesh * sub:SceneGraph::Mesh::subMeshList)delete sub; SceneGraph::Mesh::subMeshList.clear(); SceneGraph::Mesh::changed(); DispListNum=0; }
clDynamicMesh::clDynamicMesh(const char * filename,bool textureCoords,float scale):clMesh(),MyModel(0),Mode(GLM_SMOOTH|(textureCoords?GLM_TEXTURE:0)) {
    TextureCoords=textureCoords; Scale=scale; setFileName(filename); Create();
}
clDynamicMesh::~clDynamicMesh() { Destroy(); }
void clDynamicMesh::Render() { if(MyModel)glmDrawList(MyModel,0,Mode); }
void clDynamicMesh::render(int index) {
    if(!MyModel)return;
    if(index<0){Render();return;}
    if(static_cast<size_t>(index)>=SceneGraph::Mesh::subMeshList.size())return;
    GLMgroup * group=glmFindGroup(MyModel,SceneGraph::Mesh::subMeshList[index]->name.c_str());
    if(group)glmDrawList(MyModel,0,Mode,group);
}
void clDynamicMesh::Create() {
    Destroy(); if(!FileName)return;
    SceneGraph::Mesh::load(FileName,TextureCoords,Scale);
    std::ifstream input(FileName,std::ios::binary); char magic[4]={}; input.read(magic,4);
    MyModel=std::memcmp(magic,"MESH",4)?glmReadOBJ(FileName):glmReadBinMesh(FileName);
    if(!MyModel)return;
    if(Scale!=0) { glmUnitize(MyModel); glmScale(MyModel,Scale); }
    if(!MyModel->facetnorms)glmFacetNormals(MyModel); if(!MyModel->normals)glmVertexNormals(MyModel,90);
}
void clDynamicMesh::Destroy() { if(MyModel)glmDelete(MyModel); MyModel=0; clMesh::Destroy(); }
clTexture::clTexture(const char * filename) { if(filename)load(filename); }
clTexture::~clTexture() { clScene::TextureList.Remove(this); if(SceneGraph::Scene * scene=SceneGraph::Scene::existingInstance())scene->removeTexture(*this); }
void clTexture::Activate() { activate(); } void clTexture::Create() { create(); } void clTexture::Destroy() { destroy(); }

clMaterial::clMaterial():activated(false),Color(color),Specular(specular),SpecularExponent(specularExponent),clopTexture(textures[0]),clopPass2Texture(textures[1]){}
clMaterial::clMaterial(clTexture * texture,float * col,float * spec,flo exponent):clMaterial() {
    textures[0]=texture; if(col)std::copy(col,col+4,color); if(spec)std::copy(spec,spec+4,specular); specularExponent=exponent;
}
clMaterial::clMaterial(const clMaterial & source):clMaterial() { SceneGraph::Material::operator=(source); }
clMaterial & clMaterial::operator=(const clMaterial & source) {
    if(this!=&source){if(activated)Deactivate();SceneGraph::Material::operator=(source);}return *this;
}
clMaterial::~clMaterial() {
    if(activated)Deactivate(); clScene::MaterialList.Remove(this);
    if(SceneGraph::Scene * scene=SceneGraph::Scene::existingInstance())scene->removeMaterial(*this);
}
void clMaterial::RenderAsLight(int light) { renderAsLight(light); }
void clMaterial::AddEnvMap(clTexture * texture) { textures[1]=texture; bDualPass=texture && glmxNumTexUnits()<2; }
void clMaterial::Activate() {
    if(graphicsHasContext() && !activated){glPushAttrib(GL_ALL_ATTRIB_BITS);activated=true;}
    try { SceneGraph::Material::activate(); }
    catch(...) { if(activated && graphicsHasContext())glPopAttrib();activated=false;throw; }
}
void clMaterial::ActivatePass2() {
    if(graphicsHasContext() && !activated){glPushAttrib(GL_ALL_ATTRIB_BITS);activated=true;}
    try { SceneGraph::Material::activatePass1(); }
    catch(...) { if(activated && graphicsHasContext())glPopAttrib();activated=false;throw; }
}
void clMaterial::Deactivate() { SceneGraph::Material::deactivate(); if(activated && graphicsHasContext())glPopAttrib(); activated=false; }
clModel::clModel(clMesh * geometry,clMaterial * material):clopMesh(geometry),clopMaterial(material),bModel3(false) { synchronize(); }
clModel::clModel(const SceneGraph::Model & source):SceneGraph::Model(source),clopMesh(legacyMesh(source.mesh)),clopMaterial(0),bModel3(true) {
    mesh=clopMesh; adaptMaterials(*this); if(!materialList.empty())clopMaterial=dynamic_cast<clMaterial *>(materialList[0]);
}
clModel::~clModel() { clScene::ModelList.Remove(this); if(SceneGraph::Scene * scene=SceneGraph::Scene::existingInstance())scene->removeModel(*this); }
void clModel::synchronize() {
    mesh=clopMesh;
    if(!bModel3)materialList.assign(mesh?mesh->subMeshList.size():0,clopMaterial);
}
void clModel::render() { synchronize(); Render(); }
void clModel::Render() {
    synchronize();if(!mesh)return;
    if(!bModel3) {
        LegacyMaterialScope material(clopMaterial);
        clopMesh->Render();material.finish();return;
    }
    for(unsigned i=0;i<mesh->subMeshList.size();++i) {
        LegacyMaterialScope material(i<materialList.size()?materialList[i]:0);
        mesh->render(static_cast<int>(i));material.finish();
    }
}
void clModel::RenderAsLight(int light) { if(clopMaterial)clopMaterial->RenderAsLight(light); }
void clModel::getBoundingBox(Vec3 & dims) {
    synchronize();
    if(clDynamicMesh * dynamic=dynamic_cast<clDynamicMesh *>(clopMesh))if(dynamic->MyModel){flo values[3];glmDimensions(dynamic->MyModel,values);dims.Set(values[0],values[1],values[2]);return;}
    SceneGraph::Model::getBoundingBox(dims);
}

void clBillboard::Render() { billboard.render(); }
bool clBillboard::getBounds(Vec3 & minimum,Vec3 & maximum) const { minimum.Set(billboard.xmin,billboard.ymin,-1);maximum.Set(billboard.xmax,billboard.ymax,-1);return true; }
clDisplayList::~clDisplayList() {}
void clDisplayList::Render() { list.render(); }
void clLine3D::Render() { line.render(); }
bool clLine3D::getBounds(Vec3 & minimum,Vec3 & maximum) const {
    minimum.Set(std::min(s.x,e.x),std::min(s.y,e.y),std::min(s.z,e.z));maximum.Set(std::max(s.x,e.x),std::max(s.y,e.y),std::max(s.z,e.z));return true;
}
void clPlane3D::Render() { plane.render(); }
bool clPlane3D::getBounds(Vec3 & minimum,Vec3 & maximum) const {
    minimum=maximum=v[0];for(unsigned i=1;i<4;++i){minimum.x=std::min(minimum.x,v[i].x);minimum.y=std::min(minimum.y,v[i].y);minimum.z=std::min(minimum.z,v[i].z);maximum.x=std::max(maximum.x,v[i].x);maximum.y=std::max(maximum.y,v[i].y);maximum.z=std::max(maximum.z,v[i].z);}return true;
}
clModelPrimitiveContainer::clModelPrimitiveContainer() { std::fill(primitives,primitives+CMP_NUMPRIMITIVES,static_cast<clGrxPrimitive *>(0)); }
clModelPrimitiveContainer::~clModelPrimitiveContainer() { purgePrims(); }
void clModelPrimitiveContainer::Render() { for(clGrxPrimitive * p:primitives)if(p)p->Render(); }
bool clModelPrimitiveContainer::addPrim(clGrxPrimitive * primitive) {
    if(!primitive)return false;
    for(clGrxPrimitive * p:primitives)if(p==primitive)return false;
    for(clGrxPrimitive * & slot:primitives)if(!slot){slot=primitive;return true;} return false;
}
void clModelPrimitiveContainer::purgePrims() { for(clGrxPrimitive * & p:primitives){delete p;p=0;} }
void clModelPrimitiveContainer::getBoundingBox(Vec3 & dims) {
    bool found=false; Vec3 minimum(0,0,0),maximum(0,0,0);
    for(clGrxPrimitive * p:primitives)if(p) {
        Vec3 lo,hi;if(!p->getBounds(lo,hi))throw std::logic_error("Legacy primitive has no geometric bounds provider");
        if(!found){minimum=lo;maximum=hi;found=true;} else {
            minimum.x=std::min(minimum.x,lo.x);minimum.y=std::min(minimum.y,lo.y);minimum.z=std::min(minimum.z,lo.z);
            maximum.x=std::max(maximum.x,hi.x);maximum.y=std::max(maximum.y,hi.y);maximum.z=std::max(maximum.z,hi.z);
        }
    }
    dims=maximum-minimum;
}
void clMirrorFloor::Render() {
    static bool reflecting=false;
    if(!graphicsHasContext() || reflecting || bHideModel){clSceneObj::Render();return;}
    ModelViewScope mode;
    {
    MatrixAttribScope frame;
    glClearStencil(0);glClear(GL_STENCIL_BUFFER_BIT);glEnable(GL_STENCIL_TEST);
    glStencilFunc(GL_ALWAYS,1,~0u);glStencilOp(GL_KEEP,GL_KEEP,GL_REPLACE);
    glColorMask(GL_FALSE,GL_FALSE,GL_FALSE,GL_FALSE);glDepthMask(GL_FALSE);clSceneObj::Render();
    glColorMask(GL_TRUE,GL_TRUE,GL_TRUE,GL_TRUE);glDepthMask(GL_TRUE);
    glStencilFunc(GL_EQUAL,1,~0u);glStencilOp(GL_KEEP,GL_KEEP,GL_KEEP);
    // Reflection is in world space, even when the floor is parented/transformed.
    clMatrix4x4 parent=objectWorld(parentObj);parent.Invert3x4();glMultMatrixf(parent.M16);
    clMatrix4x4 world=objectWorld(this);Vec3 normal=world.Rotate(Vec3(0,1,0));normal.Normalize();
    Vec3 point=world*Vec3(0,0,0);const flo distance=-normal.Dot(point);
    const GLdouble clip[]={-normal.x,-normal.y,-normal.z,-distance};glClipPlane(GL_CLIP_PLANE0,clip);glEnable(GL_CLIP_PLANE0);
    clMatrix4x4 reflection;
    for(int c=0;c<3;++c)for(int r=0;r<3;++r)reflection.M44[c][r]=(c==r?1.0f:0.0f)-2*normal.GetElem(c)*normal.GetElem(r);
    reflection.M16[12]=-2*distance*normal.x;reflection.M16[13]=-2*distance*normal.y;reflection.M16[14]=-2*distance*normal.z;
    glMultMatrixf(reflection.M16);GLint front=GL_CCW;glGetIntegerv(GL_FRONT_FACE,&front);glFrontFace(front==GL_CCW?GL_CW:GL_CCW);
    ReflectionScope reflectionInProgress(reflecting);
    for(SceneGraph::Object * object:SceneGraph::Scene::getInstance().objectList)if(object && object!=this)object->render();
    }
    clSceneObj::Render();
}

clMesh3::SubMesh::~SubMesh() { delete[] vertexList;delete[] indexList;delete[] name;delete next; }
clMesh3::clMesh3():clMesh(),forceNormals_(true),forceTexCoords_(false),subMeshList(0){}
clMesh3::clMesh3(const char * filename,bool textureCoords,float scale,bool forceNormals,bool forceTexCoords):clMesh3() {
    TextureCoords=textureCoords;Scale=scale;forceNormals_=forceNormals;forceTexCoords_=forceTexCoords;setFileName(filename);Create();
}
clMesh3::clMesh3(const SceneGraph::Mesh & source):clMesh3() { setFileName(source.name.c_str());copyMesh(source,*this);captureModern(); }
clMesh3::~clMesh3() { freeChain(subMeshList);subMeshList=0; }
void clMesh3::captureModern() {
    SubMesh * head=0,* tail=0;
    try { for(SceneGraph::SubMesh * modern:SceneGraph::Mesh::subMeshList)if(modern) {
        std::unique_ptr<SubMesh> old(new SubMesh);old->nVertices=modern->nVertices;old->nTriangles=modern->nTriangles;old->name=duplicate(modern->name.c_str());
        if(old->nVertices){old->vertexList=new float[size_t(old->nVertices)*8];std::copy(modern->vertexList,modern->vertexList+size_t(old->nVertices)*8,old->vertexList);}
        if(old->nTriangles){old->indexList=new unsigned[size_t(old->nTriangles)*3];std::copy(modern->indexList,modern->indexList+size_t(old->nTriangles)*3,old->indexList);}
        if(tail)tail->next=old.get();else head=old.get();tail=old.release();
    }}catch(...){freeChain(head);throw;}
    freeChain(subMeshList);subMeshList=head;
}
void clMesh3::Render() { Render(~0u); }
void clMesh3::Render(unsigned index) {
    if(!clScene3::keepMeshesDynamic())SceneGraph::Mesh::render(index==~0u?-1:static_cast<int>(index));
    else for(size_t i=0;i<SceneGraph::Mesh::subMeshList.size();++i)if(index==~0u || index==i) {
        SceneGraph::SubMesh * sub=SceneGraph::Mesh::subMeshList[i];if(sub)graphicsDrawIndexed(sub->vertexList,sub->nVertices,sub->indexList,sub->nTriangles*3);
    }
    SubMesh * old=subMeshList;
    for(SceneGraph::SubMesh * modern:SceneGraph::Mesh::subMeshList)if(old){old->displayList=modern?modern->displayList:~0u;old=old->next;}
}
void clMesh3::render(int index) { Render(index<0?~0u:static_cast<unsigned>(index)); }
void clMesh3::Create() { Destroy();if(FileName)SceneGraph::Mesh::load(FileName,TextureCoords,Scale,forceNormals_,forceTexCoords_?1:0);captureModern(); }
void clMesh3::Destroy() { freeChain(subMeshList);subMeshList=0;clMesh::Destroy(); }
int clMesh3::findGroup(const char * group) { return SceneGraph::Mesh::findGroup(group); }
void clMesh3::getBoundingBox(Vec3 & dims) { SceneGraph::Mesh::getBoundingBox(dims); }
void clMesh3::changed() {
    std::vector<std::unique_ptr<SceneGraph::SubMesh>> copies;
    for(SubMesh * old=subMeshList;old;old=old->next) {
        if((old->nVertices&&!old->vertexList)||(old->nTriangles&&!old->indexList))throw std::invalid_argument("Legacy submesh missing buffer");
        std::unique_ptr<SceneGraph::SubMesh> modern(new SceneGraph::SubMesh);modern->nVertices=old->nVertices;modern->nTriangles=old->nTriangles;modern->name=old->name?old->name:"";
        if(modern->nVertices){modern->vertexList=new float[size_t(modern->nVertices)*8];std::copy(old->vertexList,old->vertexList+size_t(modern->nVertices)*8,modern->vertexList);}
        if(modern->nTriangles){modern->indexList=new unsigned[size_t(modern->nTriangles)*3];std::copy(old->indexList,old->indexList+size_t(modern->nTriangles)*3,modern->indexList);}
        for(size_t i=0;i<size_t(modern->nTriangles)*3;++i)if(modern->indexList[i]>=modern->nVertices)throw std::invalid_argument("Legacy submesh invalid vertex index");
        copies.push_back(std::move(modern));
    }
    SceneGraph::Mesh::SubMeshList next;next.reserve(copies.size());for(auto & copy:copies)next.push_back(copy.release());
    for(SceneGraph::SubMesh * old:SceneGraph::Mesh::subMeshList)delete old;SceneGraph::Mesh::subMeshList.swap(next);SceneGraph::Mesh::changed();
    for(SubMesh * old=subMeshList;old;old=old->next)old->displayList=~0u;
}
void clMesh3::glmGroupToIndexedList(GLMmodel * model,GLMgroup * group,unsigned & vertices,unsigned & triangles,float * & vertexList,unsigned * & indexList) {
    GraphicsIndexedGroup indexed=graphicsIndexGroup(model,group);vertices=static_cast<unsigned>(indexed.vertices.size()/8);triangles=static_cast<unsigned>(indexed.indices.size()/3);
    std::unique_ptr<float[]> v(vertices?new float[size_t(vertices)*8]:0);std::unique_ptr<unsigned[]> i(triangles?new unsigned[size_t(triangles)*3]:0);
    std::copy(indexed.vertices.begin(),indexed.vertices.end(),v.get());std::copy(indexed.indices.begin(),indexed.indices.end(),i.get());vertexList=v.release();indexList=i.release();
}
clModel3::clModel3(std::istream & stream):clModel(),clopMesh(0) {
    bModel3=true;SceneGraph::Model::load(stream);clopMesh=legacyMesh(mesh);clModel::clopMesh=clopMesh;mesh=clopMesh;adaptMaterials(*this);
    if(!materialList.empty())clopMaterial=dynamic_cast<clMaterial *>(materialList[0]);
}
clModel3::clModel3(const clModel3 & source):clModel(static_cast<const SceneGraph::Model &>(source)),clopMesh(source.clopMesh) { clModel::clopMesh=clopMesh; }
clModel3::clModel3(const SceneGraph::Model & source):clModel(source),clopMesh(dynamic_cast<clMesh3 *>(clModel::clopMesh)){}
clModel3::~clModel3() {}
void clModel3::Render() { clModel::clopMesh=clopMesh;clModel::Render(); }
void clModel3::getBoundingBox(Vec3 & dims) { clModel::clopMesh=clopMesh;mesh=clopMesh;SceneGraph::Model::getBoundingBox(dims); }
clMaterial3::clMaterial3(const char * filename):clMaterial() {
    SceneGraph::Material::load(filename);
    for(SceneGraph::Texture * & texture:textures)if(texture && !dynamic_cast<clTexture *>(texture))texture=clScene::CreateTexture(texture->name.c_str());
}
clMaterial3::clMaterial3(const SceneGraph::Material & source):clMaterial() {
    SceneGraph::Material::operator=(source);
    for(SceneGraph::Texture * & texture:textures)if(texture && !dynamic_cast<clTexture *>(texture))texture=clScene::CreateTexture(texture->name.c_str());
}
clMaterial3::~clMaterial3() {}
void clMaterial3::Activate() { clMaterial::Activate(); }
void clMaterial3::Activate(clTexture * texture) {
    if(graphicsHasContext()&&!activated){glPushAttrib(GL_ALL_ATTRIB_BITS);activated=true;}
    try { SceneGraph::Material::activate(texture); }
    catch(...) { if(activated && graphicsHasContext())glPopAttrib();activated=false;throw; }
}
void clMaterial3::Deactivate() { clMaterial::Deactivate(); }
clModel3 * clScene3::CreateModel3(std::istream & stream) {
    std::unique_ptr<clModel3> result(new clModel3(stream));SceneGraph::Scene::getInstance().addModel(*result);ModelList.Push(result.get(),false);return result.release();
}
void clScene3::AddMesh3(clMesh3 & mesh) {
    SceneGraph::Scene & scene=SceneGraph::Scene::getInstance();
    if(std::find(scene.meshList.begin(),scene.meshList.end(),&mesh)==scene.meshList.end()){scene.addMesh(mesh);MeshList.Push(&mesh,false);}
}
clMaterial3 * clScene3::CreateMaterial3(const char * filename) {
    SceneGraph::Scene & scene=SceneGraph::Scene::getInstance();
    for(SceneGraph::Material * material:scene.materialList)if(material && filename && material->name==filename)
        if(clMaterial3 * old=dynamic_cast<clMaterial3 *>(material))return old;
    std::unique_ptr<clMaterial3> result(new clMaterial3(filename));scene.addMaterial(*result);MaterialList.Push(result.get(),false);return result.release();
}
