#include "3DClasses3.h"
#include "Iterator.h"
#include <cstdio>
#include <cmath>
#include <sstream>
#include <set>
#include <stdexcept>

static int failures=0;
#define CHECK(c) do { if (!(c)) { std::fprintf(stderr,"FAIL %d: %s\n",__LINE__,#c); ++failures; } } while(0)
static bool near(float a,float b) { return std::fabs(a-b)<0.0001f; }
struct PrimitiveCounter : clGrxPrimitive { int & count; PrimitiveCounter(int & c):count(c){} ~PrimitiveCounter(){++count;} void Render() override{++count;} };
struct MaterialCounter : clMaterial {
    unsigned activations=0,deactivations=0;
    void Activate() override { ++activations; }
    void Deactivate() override { ++deactivations; }
};
struct Material3Counter : clMaterial3 {
    unsigned activations=0,deactivations=0;
    Material3Counter():clMaterial3(SceneGraph::Material()){}
    void Activate() override { ++activations; }
    void Deactivate() override { ++deactivations; }
};
struct MeshCounter : clMesh {
    unsigned draws=0;bool throwOnDraw=false;
    MeshCounter(){subMeshList.push_back(new SceneGraph::SubMesh);}
    void render(int) override { ++draws;if(throwOnDraw)throw std::runtime_error("mesh draw failure"); }
    void Render() override { render(-1); }
};
struct LegacyRenderMesh : clMesh {
    unsigned draws=0;bool throwOnDraw=false;
    // Old clients implement only this entry point, without indexed geometry.
    void Render() override { ++draws;if(throwOnDraw)throw std::runtime_error("legacy mesh draw failure"); }
};
int main() {
    const char * path="legacy-mesh.obj";
    FILE * file=std::fopen(path,"wb"); if(!file)return 2;
    std::fputs("v 0 0 0\nv 2 0 0\nv 0 3 0\ng body\nf 1 2 3\n",file); std::fclose(file);
    clMesh * mesh=clScene::CreateMesh(path,false);
    clMaterial * material=clScene::CreateMaterial();
    clModel * model=clScene::CreateModel(mesh,material);
    Vec3 dims; model->getBoundingBox(dims); CHECK(near(dims.x,2)&&near(dims.y,3));
    CHECK(model->getMesh()==mesh && !model->isModel3());
    CHECK(clScene::CreateMesh(path,false)==mesh);
    clSceneObj * root=clScene::CreateObject(model); root->SetPosition(10,0,0); root->SetScale(2);
    CHECK(near(root->Position.x,10)&&near(root->Model2World.M16[12],10));
    std::set<clSceneObj *> leaves;
    for(int i=0;i<10;++i)leaves.insert(root->CreateChild(model));
    SceneGraph::Iterator traversal(root); std::set<SceneGraph::Object *> seen;
    while(SceneGraph::Object * o=traversal.getNextObject()) { CHECK(seen.insert(o).second); CHECK(dynamic_cast<clSceneObj *>(o)!=0); }
    for(clSceneObj * leaf:leaves)CHECK(seen.count(leaf));
    CHECK(root->LeftObj && root->LeftObj->ParentObj==root);
    clSceneObj * moved=*leaves.begin(); clScene::AddObject(moved); CHECK(moved->ParentObj==0);
    root->AddChild(moved); CHECK(SceneGraph::Scene::getInstance().objectList.size()==1);
    clScene::DeleteObject(moved,true); CHECK(moved->ParentObj==0); delete moved;
    clSceneObj * detached=root->CreateChild(model); CHECK(root->DeleteChild(detached));
    clScene::SetCamera(root); clScene::SetLight(0,root); clScene::SetProjection(.1f,100,60); clScene::Hint_FullscreenDraw();
    clSceneObj * cockpit=clScene::CreatePreCamObject(model); clSceneObj * replacement=clScene::CreatePreCamObject(model);
    CHECK(cockpit!=replacement || SceneGraph::Scene::getInstance().objectList.size()==2);
    clScene::CreatePreTranslatObject(model);
    clScene::Clear(); clScene::Render(); // CPU/headless calls remain valid.
    clModel * collapsed=root->collapseSubtreeToNewModel(true); CHECK(collapsed && collapsed->getMesh());
    collapsed->getBoundingBox(dims); CHECK(near(dims.x,2)&&near(dims.y,3));
    clMesh3 * modernMesh=new clMesh3(path,false); clScene3::AddMesh3(*modernMesh);
    CHECK(modernMesh->subMeshList && modernMesh->subMeshList->nVertices==3);
    modernMesh->subMeshList->vertexList[0]=-2; modernMesh->changed(); modernMesh->getBoundingBox(dims); CHECK(near(dims.x,4));
    CHECK(modernMesh->findGroup("body")>=0);
    clScene3::keepMeshesDynamic(true); CHECK(clScene3::keepMeshesDynamic());
    std::istringstream script("Model { Mesh { legacy-mesh.obj; } }"); clModel3 * loaded=clScene3::CreateModel3(script);
    CHECK(loaded->isModel3() && loaded->clopMesh); loaded->getBoundingBox(dims); CHECK(near(dims.x,2)&&near(dims.y,3));
    clModel3 copy(*loaded); CHECK(copy.clopMesh==loaded->clopMesh);
    int destroyed=0; clModelPrimitiveContainer * container=new clModelPrimitiveContainer;
    CHECK(container->addPrim(new PrimitiveCounter(destroyed))); container->Render(); CHECK(destroyed==1);
    delete container; CHECK(destroyed==2);
    clModelPrimitiveContainer bounded; Vec3 start(-1,-2,-3),end(4,5,6); CHECK(bounded.addPrim(new clLine3D(start,end)));
    bounded.getBoundingBox(dims); CHECK(near(dims.x,5)&&near(dims.y,7)&&near(dims.z,9));
    clSceneObj boundedObject(&bounded); boundedObject.getBoundingBox(dims); CHECK(near(dims.x,5));
    Vec3 planeLocation(0,0,0),planeNormal(0,1,0);clPlane3D plane(planeLocation,planeNormal),planeCopy(plane);planeCopy.point.x=10;CHECK(near(plane.point.x,0));
    clDynamicMesh dynamic(path,false,0); CHECK(dynamic.MyModel); dynamic.Destroy(); CHECK(!dynamic.MyModel); dynamic.Create(); CHECK(dynamic.MyModel);
    mesh->Destroy(); model->getBoundingBox(dims); CHECK(near(dims.x,0)); mesh->Create(); model->getBoundingBox(dims); CHECK(near(dims.x,2));
    clScene::Purge(); CHECK(SceneGraph::Scene::getInstance().objectList.empty()); CHECK(SceneGraph::Scene::getInstance().modelList.empty());
    {
        LegacyRenderMesh geometry;MaterialCounter appearance;clModel bridge(&geometry,&appearance);
        bridge.Render();CHECK(geometry.draws==1 && appearance.activations==1 && appearance.deactivations==1);
        SceneGraph::Model * modern=&bridge;modern->render();CHECK(geometry.draws==2 && appearance.activations==2 && appearance.deactivations==2);
        geometry.subMeshList.push_back(new SceneGraph::SubMesh);
        geometry.subMeshList.push_back(new SceneGraph::SubMesh);
        bridge.Render();CHECK(geometry.draws==3 && appearance.activations==3 && appearance.deactivations==3);
        geometry.throwOnDraw=true;bool threw=false;try{bridge.Render();}catch(const std::runtime_error &){threw=true;}
        CHECK(threw && geometry.draws==4 && appearance.activations==4 && appearance.deactivations==4);
    }
    {
        MeshCounter geometry;MaterialCounter appearance;clModel bridge(&geometry,&appearance);
        bridge.Render();CHECK(geometry.draws==1 && appearance.activations==1 && appearance.deactivations==1);
        SceneGraph::Model * modern=&bridge;modern->render();CHECK(geometry.draws==2 && appearance.activations==2 && appearance.deactivations==2);
        geometry.throwOnDraw=true;bool threw=false;try{bridge.Render();}catch(const std::runtime_error &){threw=true;}
        CHECK(threw && appearance.activations==3 && appearance.deactivations==3);
        SceneGraph::Model source;source.mesh=&geometry;source.materialList.push_back(&appearance);
        clModel3 model3(source);model3.Render();CHECK(appearance.activations==4 && appearance.deactivations==4);
        Material3Counter specialized;model3.materialList[0]=&specialized;
        model3.Render();SceneGraph::Model * modern3=&model3;modern3->render();
        CHECK(specialized.activations==2 && specialized.deactivations==2);
    }
    clScene::Purge();
    SceneGraph::Scene::shutDown(); std::remove(path);
    return failures?1:0;
}
