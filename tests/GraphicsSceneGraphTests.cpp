#include "Graphics.h"
#include "Bounds3d.h"
#include <cstdio>
#include <cmath>
#include <set>
#include <sstream>
#include <stdexcept>

static int failures = 0;
#define CHECK(c) do { if (!(c)) { std::fprintf(stderr, "FAIL %d: %s\n", __LINE__, #c); ++failures; } } while (0)
static bool near(float a, float b) { return std::fabs(a-b) < 0.0001f; }
using namespace SceneGraph;
struct CountPrimitive : Primitive {
    explicit CountPrimitive(int & count) : count(count) {}
    ~CountPrimitive() { ++count; }
    void render() override {}
    int & count;
};
struct CountObject : Object {
    explicit CountObject(int & count) : count(count) {}
    ~CountObject() override { ++count; }
    int & count;
};
static SubMesh * triangle(float x) {
    SubMesh * s = new SubMesh;
    s->nVertices=3; s->nTriangles=1;
    s->vertexList = new float[24]{x,0,0,0,0,1,0,0, x+2,0,0,0,0,1,1,0, x,3,0,0,0,1,0,1};
    s->indexList = new unsigned[3]{0,1,2};
    return s;
}
int main() {
    {
        Object root;
        std::set<Object *> leaves;
        for (int n=0;n<20;++n) leaves.insert(root.createChild(0));
        Iterator it(&root);
        std::set<Object *> seen;
        while (Object * o = it.getNextObject()) { CHECK(seen.insert(o).second); if (o!=&root) CHECK(o->parentObj!=0); }
        for (Object * o : leaves) CHECK(seen.count(o)==1);
        CHECK(seen.size()>21); // Overflow uses transparent storage nodes.
        Object other;
        Object * moved=*leaves.begin();
        other.addChild(moved);
        CHECK(moved->parentObj==&other);
        CHECK(!root.removeChild(moved));
        other.addChild(moved);
        CHECK(other.leftObj==moved && !other.middleObj);
        moved->addChild(&other); // A cycle must never take ownership.
        CHECK(other.parentObj==0);
        CHECK(other.removeChild(moved)); CHECK(moved->parentObj==0); delete moved;
        int destroyed=0;
        Object * child=root.createChild(0); child->primitive=new CountPrimitive(destroyed);
        CHECK(root.deleteChild(child)); CHECK(destroyed==1);
        CountObject * derived=new CountObject(destroyed); root.addChild(derived);
        CHECK(root.deleteChild(derived)); CHECK(destroyed==2);
        CHECK(!root.deleteChild(0));
        Object detachRoot;
        Object * detached=detachRoot.createChild(0); detachRoot.removeSubtree(); CHECK(detached->parentObj==0);
        delete detached;
    }
    CHECK(!Scene::existingInstance()); // Object cleanup must not create a scene.
    {
        Object root; root.setPosition(10,0,0); root.setScale(2);
        Object * child=root.createChild(0); child->setPosition(1,2,3);
        Object * sibling=root.createChild(0); sibling->setPosition(-1,0,0);
        Iterator i(&root); i.setCumulateTransforms(true,true);
        CHECK(i.getNextObject()==&root); CHECK(near(i.getCumulatedTransform()->M16[0],2));
        CHECK(i.getNextObject()==child); Vec3 p=*i.getCumulatedTransform()*Vec3(0,0,0);
        CHECK(near(p.x,12)&&near(p.y,4)&&near(p.z,6));
        CHECK(i.getNextObject()==sibling); CHECK(near(i.getCumulatedTransform()->M16[12],8)); CHECK(!i.getNextObject());
        Iterator j(&root); j.setCumulateTransforms(true,false); CHECK(j.getNextObject()==&root); CHECK(!j.getCumulatedTransform());
        CHECK(j.getNextObject()==child); CHECK(near(j.getCumulatedTransform()->M16[12],1));
        Vec3 angles(90,0,0); child->setOrientation(&angles); p=child->orientation*Vec3(1,0,0);
        CHECK(near(p.x,0)&&near(p.y,1));
        Vec3 target(1,2,2),up(0,1,0); child->lookAt(&target,&up);
        p=child->orientation*Vec3(0,0,-1); CHECK(near(p.z,-1));
        child->setVisibility(false); CHECK(child->bHideModel && !child->bHideSubtree);
    }
    {
        Mesh sourceMesh; sourceMesh.subMeshList.push_back(triangle(0)); sourceMesh.subMeshList.push_back(triangle(4));
        Material a,b; Model source; source.mesh=&sourceMesh; source.materialList={&a,&b}; source.name="original";
        Model copy(source); CHECK(copy.mesh==source.mesh && copy.materialList==source.materialList && copy.name==source.name);
        Model destination; clMatrix4x4 transform; transform.SetTranslation(Vec3(5,1,0)); source.merge(true,destination,&transform);
        CHECK(destination.mesh && destination.mesh!=source.mesh); CHECK(destination.mesh->subMeshList.size()==2);
        CHECK(destination.materialList==source.materialList); CHECK(near(destination.mesh->subMeshList[0]->vertexList[0],5));
        CHECK(near(sourceMesh.subMeshList[0]->vertexList[0],0)); Vec3 dims; destination.getBoundingBox(dims);
        CHECK(near(dims.x,6)&&near(dims.y,3));
        Iterator mi(&source); CHECK(mi.getNextModelOfCurrObject()==&source); CHECK(!mi.getNextModelOfCurrObject());
        CHECK(mi.getNextMaterialOfCurrModel()==&a); CHECK(mi.getNextMaterialOfCurrModel()==&b); CHECK(!mi.getNextMaterialOfCurrModel());
        CHECK(mi.getNextMeshOfCurrModel()==&sourceMesh); CHECK(!mi.getNextMeshOfCurrModel());
        CHECK(mi.getNextSubmeshOfCurrMesh()==sourceMesh.subMeshList[0]); CHECK(mi.getNextSubmeshOfCurrMesh()==sourceMesh.subMeshList[1]); CHECK(!mi.getNextSubmeshOfCurrMesh());
        Iterator si(&sourceMesh); CHECK(si.getNextSubmeshOfCurrMesh()==sourceMesh.subMeshList[0]);
        Model packed; source.merge(false,packed); CHECK(packed.mesh->subMeshList.size()==1);
        CHECK(packed.mesh->subMeshList[0]->nVertices==6 && packed.mesh->subMeshList[0]->indexList[3]==3);
        Object object(&source); object.setPosition(7,0,0); Object * child=object.createChild(&source); child->setPosition(2,0,0);
        Model * collapse=object.collapseSubtreeToNewModel(); CHECK(collapse && collapse->mesh->subMeshList.size()==4);
        CHECK(near(collapse->mesh->subMeshList[2]->vertexList[0],2));
        object.executeTransform(); CHECK(near(object.model2World.M16[12],0)); CHECK(near(sourceMesh.subMeshList[0]->vertexList[0],0));
        CHECK(object.model!=&source && child->model!=&source && child->model!=object.model);
        CHECK(near(object.model->mesh->subMeshList[0]->vertexList[0],7));
        CHECK(near(child->model->mesh->subMeshList[0]->vertexList[0],9));
        CHECK(near(child->model2World.M16[12],0));
        int primitiveDestroyed=0;
        child->primitive=new CountPrimitive(primitiveDestroyed); object.setPosition(3,0,0);
        bool rejected=false; Model * prior=object.model;
        try { object.executeTransform(); } catch (const std::invalid_argument &) { rejected=true; }
        CHECK(!rejected && object.model!=prior && near(object.model2World.M16[12],0));
        delete child->primitive; child->primitive=0; CHECK(primitiveDestroyed==1);
        Scene::getInstance().removeMesh(*destination.mesh); delete destination.mesh;
        Scene::getInstance().removeMesh(*packed.mesh); delete packed.mesh;
        Scene::getInstance().removeMesh(*collapse->mesh); delete collapse->mesh;
        Scene::getInstance().removeModel(*collapse); delete collapse;
    }
    {
        Mesh * cached=new Mesh; cached->name="cached.obj"; SubMesh * s=triangle(0); s->name="body"; cached->subMeshList.push_back(s);
        Material * material=new Material; material->name="cached.mat";
        Scene::getInstance().addMesh(*cached); Scene::getInstance().addMaterial(*material);
        std::istringstream script("Model { Mesh { cached.obj; } Groups { body { cached.mat; } } }");
        Model loaded; loaded.load(script); CHECK(loaded.mesh==cached); CHECK(loaded.materialList.size()==1 && loaded.materialList[0]==material);
        std::istringstream invalid("Model { Scale { 2; } }"); loaded.load(invalid); CHECK(!loaded.mesh);
    }
    {
        Bounds3d a,b; CHECK(!a.contain(Vec3d(0,0,0))); a.include(Vec3d(-1,-2,-3)); a.include(Vec3d(1,2,3));
        b.set(1,2,3,4,5,6); CHECK(a.intersects(b)); a.intersect(b); CHECK(a.contain(Vec3d(1,2,3)));
        a.clear(); a.combine(b); CHECK(a.contain(Vec3d(4,5,6)));
        Mat33d rotation; rotation.ID(); rotation.M33[0][0]=0; rotation.M33[0][1]=1; rotation.M33[1][0]=-1; rotation.M33[1][1]=0;
        a.transform(rotation,Vec3d(10,20,30)); CHECK(a.contain(Vec3d(5,24,36))); CHECK(a.contain(Vec3d(8,21,33)));
        Bounds3d empty; a.intersect(empty); CHECK(a.isEmpty());
    }
    Scene::shutDown();
    return failures ? 1 : 0;
}
