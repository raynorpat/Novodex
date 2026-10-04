#include "Graphics.h"
#include "glm.h"
#include "Terrain.h"
#include <cmath>
#include <cstdio>
#include <cstdint>
#include <cstring>
#include <fstream>
#include <sstream>
#include <stdexcept>
#include <vector>

static int failures;
#define CHECK(x) do {if(!(x)){std::fprintf(stderr,"FAIL %d: %s\n",__LINE__,#x);++failures;}}while(0)
static bool closeEnough(float a,float b){return std::fabs(a-b)<.0001f;}
static void u16(std::vector<unsigned char>& b,unsigned n){b.push_back(n&255);b.push_back((n>>8)&255);}
static void u32(std::vector<unsigned char>& b,unsigned n){u16(b,n);u16(b,n>>16);}
static void f32(std::vector<unsigned char>& b,float f){unsigned n;std::memcpy(&n,&f,4);u32(b,n);}
static std::vector<unsigned char> fixture(unsigned version=9){
  std::vector<unsigned char> b={'C','H','U',0};u16(b,version);u16(b,1);f32(b,1);f32(b,.5f);f32(b,2);u32(b,1);
  u32(b,0);for(unsigned i=0;i<4;++i)u32(b,~0u);b.push_back(0);u16(b,0);u16(b,0);u16(b,0);u16(b,6);u32(b,version==9?57:50);
  u16(b,4);const int xyz[4][3]={{-16384,0,-16384},{-16384,2,16384},{16384,4,-16384},{16384,6,16384}};
  for(auto& v:xyz){for(int x:v)u16(b,unsigned(x));u16(b,100);}u32(b,4);for(unsigned i=0;i<4;++i)u16(b,i);u32(b,2);return b;
}
static void write(const char* path,const std::vector<unsigned char>& bytes){std::ofstream out(path,std::ios::binary);out.write(reinterpret_cast<const char*>(bytes.data()),bytes.size());}
static std::vector<unsigned char> hierarchy(){
  auto one=fixture();std::vector<unsigned char> b(one.begin(),one.begin()+24);b[6]=2;b[20]=5;
  for(unsigned i=0;i<5;++i){u32(b,i);for(unsigned n=0;n<4;++n)u32(b,~0u);b.push_back(i?1:0);u16(b,i?(i-1)&1:0);u16(b,i?(i-1)>>1:0);u16(b,i?0:100);u16(b,i?6:100);u32(b,189+i*50);}
  for(unsigned i=0;i<5;++i){std::vector<unsigned char> mesh(one.begin()+57,one.end());if(!i)for(unsigned v=0;v<4;++v){mesh[2+v*8+2]=100;mesh[2+v*8+3]=0;}b.insert(b.end(),mesh.begin(),mesh.end());}return b;
}
static void load(SceneGraph::Model& m,const char* path,const char* suffix=""){
 std::ostringstream text;text<<"Model3 { Terrain { \""<<path<<"\"; } "<<suffix<<" }";std::istringstream in(text.str());m.load(in);
}
int main(int argc,char** argv){
  auto data=fixture();write("terrain-v9.chu",data);
  std::ofstream("terrain-material.ods")<<"Material3 { Color { .2; .4; .6; 1; } }";
  SceneGraph::Model m;load(m,"terrain-v9.chu","Groups { default { terrain-material.ods; } }");CHECK(m.mesh);
  if(m.mesh){CHECK(m.mesh->name=="terrain-v9.chu"&&m.mesh->subMeshList.size()==1&&m.mesh->findGroup("default")==0);auto* s=m.mesh->subMeshList[0];CHECK(s->nVertices==4&&s->nTriangles==2);CHECK(closeEnough(s->vertexList[0],-.001f)&&closeEnough(s->vertexList[2],-.001f));CHECK(closeEnough(s->vertexList[3*8+1],3));CHECK(s->indexList[3]==2&&s->indexList[4]==1&&s->indexList[5]==3);CHECK(s->vertexList[4]>0);Vec3 bounds;m.mesh->getBoundingBox(bounds);CHECK(closeEnough(bounds.x,2.002f)&&closeEnough(bounds.y,3)&&closeEnough(bounds.z,2.002f));CHECK(m.materialList.size()==1&&m.materialList[0]);}
  write("terrain-v8.chu",fixture(8));SceneGraph::Model v8;load(v8,"terrain-v8.chu");CHECK(v8.mesh&&v8.mesh->subMeshList[0]->nTriangles==2);
  write("terrain-tree.chu",hierarchy());SceneGraph::Model tree;load(tree,"terrain-tree.chu");CHECK(tree.mesh);if(tree.mesh){auto* s=tree.mesh->subMeshList[0];CHECK(s->nVertices==16&&s->nTriangles==8);Vec3 bounds;tree.mesh->getBoundingBox(bounds);CHECK(closeEnough(bounds.x,4.002f)&&closeEnough(bounds.z,4.002f)&&closeEnough(bounds.y,3));for(unsigned i=0;i<s->nVertices;++i)CHECK(s->vertexList[i*8+1]<=3);}
  std::ofstream("terrain-alias.obj")<<"v 0 0 0\nv 1 0 0\nv 0 1 0\ng cliff\nf 1 2 3\n";
  SceneGraph::Model alias;load(alias,"terrain-alias.obj");CHECK(alias.mesh&&alias.mesh->findGroup("cliff")==0);
  GLMmodel* source=glmReadOBJ("terrain-alias.obj");CHECK(source);if(source){glmWriteBinMesh(source,"terrain-alias.mesh",GLM_NONE);glmDelete(source);}
  SceneGraph::Model binary;load(binary,"terrain-alias.mesh");CHECK(binary.mesh&&binary.mesh->findGroup("cliff")==0&&binary.mesh->subMeshList[0]->nTriangles==1);
  SceneGraph::Mesh direct;SceneGraph::loadTerrain(direct,"terrain-v9.chu",false,.5f,false);CHECK(direct.subMeshList.size()==1);if(!direct.subMeshList.empty()){Vec3 bounds;direct.getBoundingBox(bounds);CHECK(closeEnough(bounds.y,1));auto* s=direct.subMeshList[0];for(unsigned i=0;i<s->nVertices;++i)CHECK(s->vertexList[i*8+3]==0&&s->vertexList[i*8+6]==0);}
  // No synthetic replacement for the missing city.chu asset.
  bool missing=false;try{SceneGraph::Model city;load(city,"missing-city.chu");}catch(const std::runtime_error& e){missing=std::strstr(e.what(),"missing-city.chu")!=nullptr;}CHECK(missing);
  for(size_t length=0;length<data.size();++length){write("terrain-bad.chu",std::vector<unsigned char>(data.begin(),data.begin()+length));bool rejected=false;try{SceneGraph::Model bad;load(bad,"terrain-bad.chu");}catch(const std::runtime_error&){rejected=true;}CHECK(rejected);}
  auto bad=data;bad[4]=10;write("terrain-bad.chu",bad);bool rejected=false;try{SceneGraph::Model b;load(b,"terrain-bad.chu");}catch(const std::runtime_error&){rejected=true;}CHECK(rejected);
  bad=data;for(unsigned i=91;i<95;++i)bad[i]=255;write("terrain-bad.chu",bad);rejected=false;try{SceneGraph::Model b;load(b,"terrain-bad.chu");}catch(const std::runtime_error&){rejected=true;}CHECK(rejected);
  if(argc>1){SceneGraph::Model real;load(real,argv[1]);CHECK(real.mesh&&real.mesh->subMeshList.size()==1&&real.mesh->subMeshList[0]->nTriangles>1000);if(real.mesh){Vec3 bounds;real.mesh->getBoundingBox(bounds);CHECK(bounds.x>1000&&bounds.z>1000&&bounds.y>100);}}
  SceneGraph::Scene::shutDown();const char* files[]={"terrain-v9.chu","terrain-v8.chu","terrain-bad.chu","terrain-material.ods","terrain-alias.obj","terrain-alias.mesh","terrain-tree.chu"};for(auto f:files)std::remove(f);return failures?1:0;
}
