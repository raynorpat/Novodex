#ifndef __3DCLASSES_H__
#define __3DCLASSES_H__
// Source compatibility for the pre-2003 names. SceneGraph owns the renderer and
// resources; this facade deliberately does not recreate the obsolete binary ABI.
#include "Graphics.h"
#include "FNode.h"
#include <string>

class clSceneObj; class clModel; class clMesh; class clMaterial; class clTexture;

template<class Legacy, class Modern> class clLegacyPointer {
    Modern * & slot;
public:
    explicit clLegacyPointer(Modern * & value):slot(value){}
    operator Legacy *() const { return dynamic_cast<Legacy *>(slot); }
    Legacy * operator->() const { return dynamic_cast<Legacy *>(slot); }
    clLegacyPointer & operator=(Legacy * value) { slot=value; return *this; }
    clLegacyPointer & operator=(const clLegacyPointer & value) { slot=value.slot; return *this; }
};

class clScene {
    friend class clSceneObj;
    friend class clModel;
    friend class clMesh;
    friend class clMaterial;
    friend class clTexture;
public:
    static void Purge();
    static void Clear(bool bRendertarget=true,bool bDepth=true);
    static void Render();
    static void RenderToTexture(float,int,int,int,clTexture &,clSceneObj *,int,clSceneObj * camera=0);
    static void SetCamera(clSceneObj *);
    static void SetLight(int,clSceneObj *);
    static void SetProjection(flo,flo,flo,flo aspect=1);
    static clSceneObj * CreateObject(clModel * model=0);
    static clSceneObj * AddObject(clSceneObj *);
    static clSceneObj * CreatePreCamObject(clModel *);
    static clSceneObj * CreatePreTranslatObject(clModel *);
    static clModel * CreateModel(clMesh *,clMaterial *);
    static clMesh * CreateMesh(const char *,bool TextureCoords=true,float Scale=0,bool bDynamic=false);
    static clMaterial * CreateMaterial(clTexture * texture=0,float * Color=0,float * Specular=0,float SpecularExponent=10);
    static clTexture * CreateTexture(const char *);
    static void DeleteObject(clSceneObj *,bool bOnlyRemove=false);
    static void ObjDeleted(clSceneObj *);
    static void Hint_FullscreenDraw();
protected:
    static clFNodeList<clSceneObj> SceneObjList;
    static clFNodeList<clModel> ModelList;
    static clFNodeList<clMesh> MeshList;
    static clFNodeList<clMaterial> MaterialList;
    static clFNodeList<clTexture> TextureList;
    static clSceneObj * clopCamera,* clopPreCamObj,* clopPreTranslatObj,* clopLights[8];
    static flo NearPlane,FarPlane,FOVAngle,Aspect;
    static bool fullScreenDraw;
};

class clSceneObj : public SceneGraph::Object {
    friend class clScene;
    int specialKind;
public:
    clLegacyPointer<clModel,SceneGraph::Model> clopModel;
    Vec3 & Position;
    clMatrix4x4 & Orientation,& Model2World;
    clLegacyPointer<clSceneObj,SceneGraph::Object> LeftObj,MiddleObj,RightObj,ParentObj;
    clSceneObj(clModel * model=0,bool bNULLXform=false,clSceneObj * parent=0);
    ~clSceneObj() override;
    void render() override;
    virtual void Render();
    void RenderAsCamera(clSceneObj * cameraSpace=0,clSceneObj * preTranslation=0);
    void RenderAsLight(int);
    void SetScale(flo); void SetPosition(flo,flo,flo); void SetPosition(Vec3 *);
    void SetOrientation(Quat *); void SetOrientation(Vec3 *); void LookAt(Vec3 *,Vec3 *); void UpdateMatrix();
    clSceneObj * CreateChild(clModel * model=0);
    void AddChild(clSceneObj *); bool DeleteChild(clSceneObj *);
    void getBoundingBox(Vec3 &);
    clModel * collapseSubtreeToNewModel(bool keepMaterials=true);
    void executeTransform();
    clSceneObj(const clSceneObj &)=delete;
    clSceneObj & operator=(const clSceneObj &)=delete;
};

class clMesh : public SceneGraph::Mesh {
protected:
    unsigned DispListNum;
    float Scale; bool TextureCoords;
    char * FileName;
    void setFileName(const char *);
public:
    clMesh(const char *,bool TextureCoords=true,float Scale=0);
    clMesh(); ~clMesh() override;
    void render(int subMeshIndex=-1) override;
    virtual void Render(); virtual void Create(); virtual void Destroy();
    char * getFName() { return FileName; }
};
struct GLMmodel;
class clDynamicMesh : public clMesh {
public:
    GLMmodel * MyModel; unsigned Mode;
    clDynamicMesh(const char *,bool TextureCoords=true,float Scale=1);
    ~clDynamicMesh() override;
    void render(int subMeshIndex=-1) override;
    void Render() override; void Create() override; void Destroy() override;
};

class clTexture : public SceneGraph::Texture {
public:
    explicit clTexture(const char * filename=0);
    ~clTexture() override;
    void Activate(); void Create(); void Destroy();
    char * getFName() { return name.empty()?0:const_cast<char *>(name.c_str()); }
    unsigned getDispListNum() { return SceneGraph::Texture::getDispListNum(); }
};
class clMaterial : public SceneGraph::Material {
protected:
    bool activated;
    float (& Color)[4],(& Specular)[4];
    float & SpecularExponent;
    clLegacyPointer<clTexture,SceneGraph::Texture> clopTexture,clopPass2Texture;
public:
    clMaterial(clTexture *,float *,float *,float);
    clMaterial(); ~clMaterial() override;
    clMaterial(const clMaterial &);
    clMaterial & operator=(const clMaterial &);
    void RenderAsLight(int); void AddEnvMap(clTexture *);
    virtual void Activate(); void ActivatePass2(); virtual void Deactivate();
    clTexture * getTexture() { return clopTexture; }
    virtual char * getFName() { return name.empty()?0:const_cast<char *>(name.c_str()); }
};
class clModel : public SceneGraph::Model {
protected:
    clMesh * clopMesh; clMaterial * clopMaterial; bool bModel3;
    void synchronize();
public:
    clModel(clMesh * mesh=0,clMaterial * material=0);
    explicit clModel(const SceneGraph::Model &);
    ~clModel() override;
    bool isModel3() { return bModel3; }
    void render() override;
    virtual void Render();
    clMesh * getMesh() { return clopMesh; }
    void RenderAsLight(int);
    virtual void getBoundingBox(Vec3 &);
};

class clGrxPrimitive : public SceneGraph::Primitive {
public:
    ~clGrxPrimitive() override {}
    void render() override { Render(); }
    virtual void Render()=0;
    virtual bool getBounds(Vec3 &,Vec3 &) const { return false; }
};
class clBillboard : public clGrxPrimitive {
    SceneGraph::Billboard billboard;
public:
    clBillboard(clTexture * t,float xmin=-1,float xmax=1,float ymin=-1,float ymax=1):billboard(t,xmin,xmax,ymin,ymax){}
    void Render() override;
    bool getBounds(Vec3 &,Vec3 &) const override;
};
class clDisplayList : public clGrxPrimitive {
    SceneGraph::DisplayList list;
public:
    explicit clDisplayList(int no):list(no){}
    ~clDisplayList() override;
    void Render() override;
};
class clLine3D : public clGrxPrimitive {
    SceneGraph::Line line;
    Vec3 s,e;
public:
    clLine3D(Vec3 & start,Vec3 & end):line(start,end),s(start),e(end){}
    void set(Vec3 & start,Vec3 & end) { line.set(start,end);s=start;e=end; }
    void Render() override;
    bool getBounds(Vec3 &,Vec3 &) const override;
};
class clPlane3D : public clGrxPrimitive {
    SceneGraph::Plane plane;
public:
    Vec3 (& v)[4],(& base)[2];
    Vec3 & point,& normal;
    clPlane3D():v(plane.v),base(plane.base),point(plane.point),normal(plane.normal){}
    clPlane3D(Vec3 & location,Vec3 & normal,float size=1):clPlane3D() { Construct(location,normal,size); }
    clPlane3D(const clPlane3D & other):plane(other.plane),v(plane.v),base(plane.base),point(plane.point),normal(plane.normal){}
    clPlane3D & operator=(const clPlane3D & other) { plane=other.plane;return *this; }
    void Construct(Vec3 & location,Vec3 & normal,float size=1) { plane.construct(location,normal,size); }
    void computeVertices(float size=1) { plane.computeVertices(size); }
    void Render() override;
    bool getBounds(Vec3 &,Vec3 &) const override;
};
#define CMP_NUMPRIMITIVES 20
class clModelPrimitiveContainer : public clModel {
    clGrxPrimitive * primitives[CMP_NUMPRIMITIVES];
public:
    clModelPrimitiveContainer(); ~clModelPrimitiveContainer() override;
    clModelPrimitiveContainer(const clModelPrimitiveContainer &)=delete;
    clModelPrimitiveContainer & operator=(const clModelPrimitiveContainer &)=delete;
    void Render() override;
    bool addPrim(clGrxPrimitive *);
    void purgePrims();
    void getBoundingBox(Vec3 &) override;
};
class clMirrorFloor : public clSceneObj {
public:
    clMirrorFloor(clModel * model=0,bool bNULLXform=false,clSceneObj * parent=0):clSceneObj(model,bNULLXform,parent){}
    void Render() override;
};
#endif
