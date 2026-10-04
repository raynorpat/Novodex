#ifndef __3DCLASSES3_H__
#define __3DCLASSES3_H__
#include "3DClasses2.h"
#include <istream>
class clModel3; class clMesh3; class clMaterial3;
struct GLMmodel; struct GLMgroup;
class clScene3 : public clScene {
    static bool dynamesh;
public:
    static clModel3 * CreateModel3(std::istream &);
    static void AddMesh3(clMesh3 &);
    static clMaterial3 * CreateMaterial3(const char *);
    static bool keepMeshesDynamic() { return dynamesh; }
    static void keepMeshesDynamic(bool value) { dynamesh=value; }
};
class clMesh3 : public clMesh {
    void captureModern();
    bool forceNormals_,forceTexCoords_;
public:
    struct SubMesh {
        float * vertexList; unsigned * indexList;
        unsigned nVertices,nTriangles; char * name; unsigned displayList;
        SubMesh * next;
        SubMesh():vertexList(0),indexList(0),nVertices(0),nTriangles(0),name(0),displayList(0),next(0){}
        ~SubMesh();
        SubMesh(const SubMesh &)=delete;
        SubMesh & operator=(const SubMesh &)=delete;
    };
    SubMesh * subMeshList;
    clMesh3(const char *,bool TextureCoords=true,float Scale=0,bool forceNormals=true,bool forceTexCoords=false);
    clMesh3();
    explicit clMesh3(const SceneGraph::Mesh &);
    ~clMesh3() override;
    void Render() override;
    virtual void Render(unsigned subMeshIndex);
    void render(int subMeshIndex=-1) override;
    void Create() override; void Destroy() override;
    int findGroup(const char * name) override;
    void getBoundingBox(Vec3 &);
    void changed();
protected:
    static void glmGroupToIndexedList(GLMmodel *,GLMgroup *,unsigned &,unsigned &,float * &,unsigned * &);
};
class clModel3 : public clModel {
public:
    clMesh3 * clopMesh;
    explicit clModel3(std::istream &);
    clModel3(const clModel3 &);
    explicit clModel3(const SceneGraph::Model &);
    ~clModel3() override;
    void Render() override;
    void getBoundingBox(Vec3 &) override;
};
class clMaterial3 : public clMaterial {
public:
    explicit clMaterial3(const char *);
    explicit clMaterial3(const SceneGraph::Material &);
    ~clMaterial3() override;
    void Activate() override;
    void Activate(clTexture * overrideTexture);
    void Deactivate() override;
    char * getFName() override { return name.empty()?0:const_cast<char *>(name.c_str()); }
};
#endif
