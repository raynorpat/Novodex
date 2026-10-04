#include "ViewerPlatform.h"
#include "Graphics.h"
#include "3DClasses3.h"
#include <cstdio>
#include <vector>
#include <cmath>
#include <stdexcept>

using namespace SceneGraph;
static unsigned failures;
static void check(bool ok,const char * message) {
    if (!ok) { ++failures; std::fprintf(stderr,"%s\n",message); }
}
static const char * title() { return "Graphics offscreen checks"; }
static Object * target;
static Object * camera;
struct FailingPrimitive : Primitive {
    void render() override { throw std::runtime_error("Fixture render failure"); }
};
struct FailingLegacyModel : clModel {
    void Render() override { throw std::runtime_error("Legacy fixture failure"); }
};
static void start(int,char **) {
    Scene & scene=Scene::getInstance();
    Mesh * mesh=new Mesh;
    SubMesh * sub=new SubMesh;
    sub->nVertices=3; sub->nTriangles=1;
    sub->vertexList=new float[24]{-1,-1,0,0,0,1,0,0, 1,-1,0,0,0,1,1,0, 0,1,0,0,0,1,0.5f,1};
    sub->indexList=new unsigned[3]{0,1,2};
    mesh->subMeshList.push_back(sub); scene.addMesh(*mesh);
    Material * material=new Material;
    material->color[0]=1; material->color[1]=material->color[2]=0;
    scene.addMaterial(*material);
    Model * model=new Model; model->mesh=mesh; model->materialList.push_back(material); scene.addModel(*model);
    target=new Object(model); scene.addObject(*target);
    camera=new Object; camera->setPosition(0,0,3); scene.addObject(*camera); scene.setCamera(camera);
    scene.setProjection(0.1f,100,60,1);
}
static void render() {
    Scene & scene=Scene::getInstance();
    Texture destination;
    glClearColor(0,0,0,1);
    glViewport(3,4,701,502);
    GLfloat projection[16],modelview[16];
    glGetFloatv(GL_PROJECTION_MATRIX,projection); glGetFloatv(GL_MODELVIEW_MATRIX,modelview);
    glMatrixMode(GL_TEXTURE);
    typedef void (APIENTRY * ActivateTexture)(GLenum);
    ActivateTexture activate=reinterpret_cast<ActivateTexture>(wglGetProcAddress("glActiveTexture"));
    ActivateTexture activateClient=reinterpret_cast<ActivateTexture>(wglGetProcAddress("glClientActiveTexture"));
    check(activate && activateClient,"Texture unit procedures are available");
    if (activate && activateClient) { activate(0x84c1); activateClient(0x84c1); }
    glPixelTransferf(GL_RED_SCALE,0.5f);
    glPixelStorei(GL_PACK_ROW_LENGTH,7); glPixelStorei(GL_PACK_SKIP_ROWS,1); glPixelStorei(GL_PACK_SKIP_PIXELS,2);
    scene.renderToTexture(60,96,64,48,destination,0,Scene::TextureUnlit);
    check(destination.width==48 && destination.height==32,"Offscreen output preserves requested size/aspect");
    GLint mode,viewport[4],pack;
    glGetIntegerv(GL_MATRIX_MODE,&mode); glGetIntegerv(GL_VIEWPORT,viewport);
    glGetIntegerv(GL_PACK_ROW_LENGTH,&pack);
    check(mode==GL_TEXTURE && viewport[0]==3 && viewport[2]==701 && pack==7,"Offscreen rendering restores caller GL state");
    GLint activeClient,activeServer;
    glGetIntegerv(0x84e1,&activeClient); glGetIntegerv(0x84e0,&activeServer);
    check(activeClient==0x84c1 && activeServer==0x84c1,"Offscreen rendering restores server and client texture units");
    GLfloat transfer; glGetFloatv(GL_RED_SCALE,&transfer);
    check(transfer==0.5f,"Offscreen rendering restores pixel transfer state");
    glPixelTransferf(GL_RED_SCALE,1);
    if (activate && activateClient) { activate(0x84c0); activateClient(0x84c0); }
    GLfloat after[16]; glGetFloatv(GL_PROJECTION_MATRIX,after);
    for (int i=0;i<16;++i) check(after[i]==projection[i],"Offscreen preserves projection matrix");
    glGetFloatv(GL_MODELVIEW_MATRIX,after);
    for (int i=0;i<16;++i) check(after[i]==modelview[i],"Offscreen preserves modelview matrix");
    glPixelStorei(GL_PACK_ROW_LENGTH,0); glPixelStorei(GL_PACK_SKIP_ROWS,0); glPixelStorei(GL_PACK_SKIP_PIXELS,0);
    glPixelStorei(GL_PACK_ALIGNMENT,1);
    destination.activate();
    std::vector<unsigned char> pixels(destination.width*destination.height*4);
    glGetTexImage(GL_TEXTURE_2D,0,GL_RGBA,GL_UNSIGNED_BYTE,pixels.data());
    const unsigned center=4*((destination.height/2)*destination.width+destination.width/2);
    check(pixels.size()>center+3 && pixels[center]>200 && pixels[center+1]<20,"Scene is rendered into the destination texture");
    // Explicit targets use their parent's world transform, and an alternate
    // camera must not replace the scene's camera after the call.
    Object * parent=new Object; parent->setPosition(1,0,0); scene.addObject(*parent);
    parent->addChild(target); target->setPosition(-1,0,0);
    Object alternate; alternate.setPosition(0,0,3);
    scene.renderToTexture(60,64,64,64,destination,target,Scene::TextureUnlit,&alternate);
    destination.activate(); pixels.resize(64*64*4);
    glGetTexImage(GL_TEXTURE_2D,0,GL_RGBA,GL_UNSIGNED_BYTE,pixels.data());
    check(pixels[4*(32*64+32)]>200,"Target subtree includes ancestor transforms");
    bool rejected=false;
    try { scene.renderToTexture(60,0,64,64,destination); }
    catch (const std::invalid_argument &) { rejected=true; }
    check(rejected,"Invalid offscreen dimensions report an error");
    Object failing;
    failing.primitive=new FailingPrimitive;
    GLint stackBefore,stackAfter;
    glGetIntegerv(GL_MODELVIEW_STACK_DEPTH,&stackBefore);
    bool renderFailed=false;
    try { scene.renderToTexture(60,64,64,64,destination,&failing); }
    catch (const std::runtime_error &) { renderFailed=true; }
    glGetIntegerv(GL_MODELVIEW_STACK_DEPTH,&stackAfter);
    check(renderFailed && stackBefore==stackAfter,"Failed target rendering restores matrix stack depth");
    Object * failingSceneObject=new Object;
    failingSceneObject->primitive=new FailingPrimitive; scene.addObject(*failingSceneObject);
    clMirrorFloor mirror(0);
    glGetIntegerv(GL_MODELVIEW_STACK_DEPTH,&stackBefore);
    for (unsigned attempt=0;attempt<2;++attempt) {
        bool mirrorFailed=false;
        try { scene.renderToTexture(60,64,64,64,destination,&mirror); }
        catch (const std::runtime_error &) { mirrorFailed=true; }
        check(mirrorFailed,"A failed reflection does not disable subsequent reflections");
    }
    glGetIntegerv(GL_MODELVIEW_STACK_DEPTH,&stackAfter);
    check(stackBefore==stackAfter,"Mirror exceptions restore the modelview stack");
    scene.removeObject(*failingSceneObject); delete failingSceneObject;
    FailingLegacyModel failingLegacyModel;
    clSceneObj * cameraSpace=clScene::CreatePreCamObject(&failingLegacyModel);
    glGetIntegerv(GL_MODELVIEW_STACK_DEPTH,&stackBefore);
    try { clScene::Render(); } catch (const std::runtime_error &) {}
    glGetIntegerv(GL_MODELVIEW_STACK_DEPTH,&stackAfter);
    check(stackBefore==stackAfter,"Camera-space exceptions restore the modelview stack");
    clScene::DeleteObject(cameraSpace);
    clTexture legacyDestination;
    scene.renderToTexture(60,64,64,64,legacyDestination,target,Scene::TextureUnlit);
    check(legacyDestination.width==64 && legacyDestination.height==64,"Legacy texture overload captures a real target");
    clMaterial legacyMaterial;
    clTexture badTexture;
    badTexture.name="missing-legacy-texture.png"; legacyMaterial.textures[0]=&badTexture;
    GLint attributesBefore,attributesAfter;
    glGetIntegerv(GL_ATTRIB_STACK_DEPTH,&attributesBefore);
    try { legacyMaterial.Activate(); } catch (const std::runtime_error &) {}
    glGetIntegerv(GL_ATTRIB_STACK_DEPTH,&attributesAfter);
    check(attributesBefore==attributesAfter,"Failed legacy material activation restores attributes");
    clMaterial3 legacyOverride(*target->model->materialList[0]);
    try { legacyOverride.Activate(&badTexture); } catch (const std::runtime_error &) {}
    glGetIntegerv(GL_ATTRIB_STACK_DEPTH,&attributesAfter);
    check(attributesBefore==attributesAfter,"Failed legacy texture override restores attributes");
    parent->removeChild(target); scene.addObject(*target); target->setPosition(0,0,0);
    // A plane primitive must keep identical pixels after its object transforms
    // are baked, including subsequent baking under another parent transform.
    Object primitive;
    Vec3 origin(0,0,0),normal(0,0,1);
    primitive.primitive=new Plane(origin,normal,0.75f); primitive.setPosition(0.25f,0,0);
    scene.renderToTexture(60,64,64,64,destination,&primitive,Scene::TextureUnlit);
    destination.activate(); glGetTexImage(GL_TEXTURE_2D,0,GL_RGBA,GL_UNSIGNED_BYTE,pixels.data());
    primitive.executeTransform();
    check(primitive.model2World.M16[12]==0,"Primitive transform baking resets the object");
    Texture baked;
    scene.renderToTexture(60,64,64,64,baked,&primitive,Scene::TextureUnlit);
    baked.activate(); std::vector<unsigned char> bakedPixels(64*64*4);
    glGetTexImage(GL_TEXTURE_2D,0,GL_RGBA,GL_UNSIGNED_BYTE,bakedPixels.data());
    check(pixels==bakedPixels,"Primitive baking preserves rendered pixels");
    clModelPrimitiveContainer legacyPrimitives;
    legacyPrimitives.addPrim(new clPlane3D(origin,normal,0.75f));
    clSceneObj legacyPrimitive(&legacyPrimitives); legacyPrimitive.SetPosition(0.25f,0,0);
    scene.renderToTexture(60,64,64,64,destination,&legacyPrimitive,Scene::TextureUnlit);
    destination.activate(); glGetTexImage(GL_TEXTURE_2D,0,GL_RGBA,GL_UNSIGNED_BYTE,pixels.data());
    legacyPrimitive.executeTransform();
    scene.renderToTexture(60,64,64,64,baked,&legacyPrimitive,Scene::TextureUnlit);
    baked.activate(); glGetTexImage(GL_TEXTURE_2D,0,GL_RGBA,GL_UNSIGNED_BYTE,bakedPixels.data());
    check(pixels==bakedPixels,"Legacy primitive-container baking preserves appearance");
    legacyPrimitive.SetPosition(0.1f,0.1f,0);
    legacyPrimitive.setScale(0.5f);
    Vec3 angles(15,0,0); legacyPrimitive.setOrientation(&angles);
    scene.renderToTexture(60,64,64,64,destination,&legacyPrimitive,Scene::TextureUnlit);
    destination.activate(); glGetTexImage(GL_TEXTURE_2D,0,GL_RGBA,GL_UNSIGNED_BYTE,pixels.data());
    legacyPrimitive.executeTransform();
    scene.renderToTexture(60,64,64,64,baked,&legacyPrimitive,Scene::TextureUnlit);
    baked.activate(); glGetTexImage(GL_TEXTURE_2D,0,GL_RGBA,GL_UNSIGNED_BYTE,bakedPixels.data());
    check(pixels==bakedPixels,"Repeated legacy baking composes translation, rotation and scale");
    primitive.setPosition(0.25f,0,0); primitive.executeTransform();
    scene.renderToTexture(60,64,64,64,baked,&primitive,Scene::TextureUnlit);
    check(glGetError()==GL_NO_ERROR,"Offscreen and primitive rendering have no GL errors");
}
static void stop() { Scene::shutDown(); }
int main(int argc,char ** argv) {
    ViewerCallbacks callbacks={title,start,stop,0,0,render,0,0,0};
    if (viewerRun(argc,argv,callbacks,true,1)) return 1;
    return failures?1:0;
}
