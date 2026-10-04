#include "Graphics.h"
#include "ODBlock.h"
#include "Terrain.h"
#include <algorithm>
#include <cstring>
#include <limits>
#include <memory>
#include <stdexcept>

namespace SceneGraph {
Model::Model() : mesh(0), userData(0) {}
Model::Model(const Model & other) : materialList(other.materialList), mesh(other.mesh), name(other.name), userData(other.userData) {}
Model::~Model() {}
void Model::load(std::istream & input)
{
    mesh=0; materialList.clear();
    std::unique_ptr<ODBlock> script(ODBlock::loadScript(input));
    if (!script) return;
    const char * filename=0;
    const bool terrain=!script->getBlockString("Mesh",&filename);
    if (terrain && !script->getBlockString("Terrain",&filename)) return;
    Scene & scene=Scene::getInstance();
    mesh=scene.getMesh(filename);
    if (!mesh) {
        float scale=0; int texCoords=1,forceNormals=1,forceTexCoords=0;
        script->getBlockFloat("Scale",&scale); script->getBlockInt("TexCoords",&texCoords);
        script->getBlockInt("ForceNormals",&forceNormals); script->getBlockInt("ForceTexCoords",&forceTexCoords);
        std::unique_ptr<Mesh> loaded(new Mesh);
        if(terrain)loadTerrain(*loaded,filename,texCoords!=0,scale,forceNormals!=0,static_cast<unsigned>(forceTexCoords));
        else loaded->load(filename,texCoords!=0,scale,forceNormals!=0,static_cast<unsigned>(forceTexCoords));
        scene.addMesh(*loaded); mesh=loaded.release();
    }
    ODBlock * groups=script->getBlock("Groups");
    if (!groups) return;
    groups->reset();
    while (groups->moreSubBlocks()) {
        ODBlock * group=groups->nextSubBlock();
        const int index=mesh->findGroup(group->ident());
        group->reset();
        if (index<0 || !group->moreTerminals()) continue;
        const char * materialFile=group->nextTerminal();
        if (!materialFile) continue;
        Material * material=scene.getMaterial(materialFile);
        if (!material) {
            std::unique_ptr<Material> loaded(new Material);
            loaded->load(materialFile); scene.addMaterial(*loaded); material=loaded.release();
        }
        if (materialList.size()<=static_cast<unsigned>(index)) materialList.resize(index+1,0);
        materialList[index]=material;
    }
}
void Model::render()
{
    if (!mesh) return;
    for (unsigned i=0;i<mesh->subMeshList.size();++i) {
        Material * material=i<materialList.size() ? materialList[i] : 0;
        if (material) material->activate();
        mesh->render(static_cast<int>(i));
        if (material) material->deactivate();
    }
}
void Model::getBoundingBox(Vec3 & dims) { if (mesh) mesh->getBoundingBox(dims); else dims.Zero(); }

static Vec3 transformNormal(const clMatrix4x4 & transform, const Vec3 & normal)
{
    const Vec3 a(transform.M16[0],transform.M16[1],transform.M16[2]);
    const Vec3 b(transform.M16[4],transform.M16[5],transform.M16[6]);
    const Vec3 c(transform.M16[8],transform.M16[9],transform.M16[10]);
    const flo determinant=a.Dot(b.Cross(c));
    Vec3 result;
    if (determinant!=0) result=(b.Cross(c)*normal.x+c.Cross(a)*normal.y+a.Cross(b)*normal.z)*(1/determinant);
    else result=transform.Rotate(normal);
    result.Normalize(); return result;
}
void Model::merge(bool keepMaterials, Model & destination, clMatrix4x4 * transform)
{
    if (!mesh) return;
    // Snapshot references before appending: self-merge is a supported operation.
    const Mesh::SubMeshList sources=mesh->subMeshList;
    const MaterialList materials=materialList;
    std::vector<std::unique_ptr<SubMesh>> copies;
    copies.reserve(sources.size());
    for (SubMesh * source : sources) {
        if (!source) { copies.emplace_back(new SubMesh); continue; }
        if ((source->nVertices && !source->vertexList) || (source->nTriangles && !source->indexList))
            throw std::invalid_argument("Model::merge: missing geometry buffers");
        if (source->nVertices>std::numeric_limits<unsigned>::max()/8 || source->nTriangles>std::numeric_limits<unsigned>::max()/3)
            throw std::length_error("Model::merge: geometry too large");
        std::unique_ptr<SubMesh> copy(new SubMesh);
        copy->name=source->name; copy->userData=source->userData;
        copy->nVertices=source->nVertices; copy->nTriangles=source->nTriangles;
        if (copy->nVertices) {
            copy->vertexList=new float[static_cast<size_t>(copy->nVertices)*8];
            std::copy(source->vertexList,source->vertexList+static_cast<size_t>(copy->nVertices)*8,copy->vertexList);
        }
        if (copy->nTriangles) {
            copy->indexList=new unsigned[static_cast<size_t>(copy->nTriangles)*3];
            std::copy(source->indexList,source->indexList+static_cast<size_t>(copy->nTriangles)*3,copy->indexList);
            for (size_t i=0;i<static_cast<size_t>(copy->nTriangles)*3;++i)
                if (copy->indexList[i]>=copy->nVertices) throw std::invalid_argument("Model::merge: invalid vertex index");
        }
        if (transform) for (unsigned i=0;i<copy->nVertices;++i) {
            float * v=copy->vertexList+static_cast<size_t>(i)*8;
            Vec3 point=*transform*Vec3(v[0],v[1],v[2]);
            Vec3 normal=transformNormal(*transform,Vec3(v[3],v[4],v[5]));
            v[0]=point.x; v[1]=point.y; v[2]=point.z;
            v[3]=normal.x; v[4]=normal.y; v[5]=normal.z;
        }
        copies.push_back(std::move(copy));
    }
    if (!destination.mesh) {
        std::unique_ptr<Mesh> created(new Mesh);
        Scene::getInstance().addMesh(*created); destination.mesh=created.release();
    }
    const size_t offset=destination.mesh->subMeshList.size();
    destination.mesh->subMeshList.reserve(offset+copies.size());
    destination.materialList.resize(offset+copies.size(),0);
    for (size_t i=0;i<copies.size();++i) {
        destination.mesh->subMeshList.push_back(copies[i].release());
        destination.materialList[offset+i]=i<materials.size() ? materials[i] : 0;
    }
    if (!keepMaterials && !destination.mesh->subMeshList.empty()) {
        destination.mesh->collapseToSingleSubmesh();
        destination.materialList.resize(1);
    }
    destination.mesh->changed();
}
}
