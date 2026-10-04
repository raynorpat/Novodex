#include "ViewerPlatform.h"
#include "Graphics.h"
#include "gltx.h"
#include "Font.h"
#include "SysTime.h"
#include <cstdio>
#include <fstream>
#include <stdexcept>
#include <cmath>
#include <cstring>

using namespace SceneGraph;
static unsigned failures;
static void check(bool condition, const char * message) {
    if (!condition) { ++failures; std::fprintf(stderr, "%s\n", message); }
}
static const char * title() { return "Graphics rendering test"; }
static Texture * texture;
static unsigned oldTexture;
static Mesh * meshUnderTest;
static Texture deferredTexture;
static void start(int, char **) {
    check(glIsEnabled(GL_DEPTH_TEST), "Viewer enables depth testing before scene startup");
    Scene & scene = Scene::getInstance();
    deferredTexture.activate();
    GLint filtering;
    glGetTexParameteriv(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, &filtering);
    check(filtering == GL_NEAREST, "Deferred texture creation retains filtering choice");
    deferredTexture.destroy();
    Material overrideMaterial;
    Texture badOverride;
    badOverride.name = "nonexistent-override.png";
    try { overrideMaterial.activate(&badOverride); }
    catch (const std::exception &) {}
    check(overrideMaterial.textures[0] == 0, "Failed override leaves material texture unchanged");
    unsigned char rows[] = {255,0,0,255, 0,255,0,255, 0,0,255,255, 255,255,255,255};
    GLTXimage rowImage = {2,2,2,2,4,GL_RGBA,rows};
    check(gltxWriteBMP("render-rows.bmp", &rowImage), "Write row fixture");
    glPixelStorei(GL_UNPACK_ROW_LENGTH,1);
    Texture rowTexture;
    rowTexture.load("render-rows.bmp",false);
    rowTexture.activate();
    unsigned char rowPixels[16] = {};
    glGetTexImage(GL_TEXTURE_2D,0,GL_RGBA,GL_UNSIGNED_BYTE,rowPixels);
    check(std::memcmp(rows,rowPixels,sizeof(rows)) == 0, "Texture ignores inherited unpack row length");
    GLint rowLength;
    glGetIntegerv(GL_UNPACK_ROW_LENGTH,&rowLength);
    check(rowLength == 1, "Texture restores unpack row length");
    glPixelStorei(GL_UNPACK_ROW_LENGTH,0);
    Material defaults;
    check(defaults.color[0] == 1 && defaults.specular[3] == 1 && defaults.smooth == 1,
          "Recovered material defaults");
    unsigned char pixels[] = { 0,255,0,255, 0,255,0,255 };
    GLTXimage image = { 2,1,2,1,4,GL_RGBA,pixels };
    check(gltxWriteBMP("render-green.bmp", &image), "Write texture fixture");
    texture = new Texture;
    texture->load("render-green.bmp", false);
    check(texture->width == 2 && texture->height == 1 && texture->originalAspect == 0.5f,
          "Texture dimensions and legacy height/width aspect");
    oldTexture = texture->DispListNum;
    texture->create(false);
    check(glIsTexture(oldTexture) == GL_FALSE, "Texture reload releases previous GL object");
    texture->activate();
    unsigned char downloaded[8] = {};
    glGetTexImage(GL_TEXTURE_2D, 0, GL_RGBA, GL_UNSIGNED_BYTE, downloaded);
    check(downloaded[0] == 0 && downloaded[1] == 255 && downloaded[3] == 255,
          "stb pixels reach OpenGL texture");
    scene.addTexture(*texture);
    {
        std::ofstream file("render-material.ods");
        file << "Material3 { Smooth {0;} Color {1;0;0;1;} Specular {0;0;0;1;} SpecExp {0;} }";
    }
    Material * material = new Material;
    material->load("render-material.ods");
    check(material->smooth == 0 && material->color[0] == 1 && material->color[1] == 0,
          "Material script directives");
    scene.addMaterial(*material);
    Mesh * mesh = new Mesh;
    meshUnderTest = mesh;
    SubMesh * sub = new SubMesh;
    sub->nVertices = 3; sub->nTriangles = 1;
    sub->vertexList = new float[24] {
        -1,-1,0, 0,0,1, 0,0, 1,-1,0, 0,0,1, 1,0, 0,1,0, 0,0,1, 0.5f,1 };
    sub->indexList = new unsigned[3] {0,1,2};
    mesh->subMeshList.push_back(sub); scene.addMesh(*mesh);
    Model * model = new Model;
    model->mesh = mesh; model->materialList.push_back(material); scene.addModel(*model);
    scene.addObject(*new Object(model));
    Object * camera = new Object;
    camera->setPosition(0,0,3); scene.addObject(*camera); scene.setCamera(camera);
    scene.setProjection(0.1f, 100, 60, 1024.0f/768);
    bool failed = false;
    try { Texture missing; missing.load("nonexistent-image.png"); }
    catch (const std::exception &) { failed = true; }
    check(failed, "Missing texture reports recoverable failure");
    clWinTime clock;
    ::Sleep(5);
    check(clock.PeekElapsedSeconds() > 0 && clWinTime::GetClockFrequency() > 0,
          "Legacy QPC timer is usable");
}
static void resize(unsigned w, unsigned h) {
    Scene::getInstance().setProjection(0.1f,100,60,float(w)/h);
}
static void render() {
    Scene & scene = Scene::getInstance();
    glClearColor(0,0,0,1); scene.clear();
    glDisable(GL_LIGHTING); glDisable(GL_CULL_FACE);
    scene.render();
    check(glIsEnabled(GL_LIGHT0), "Scene retains the default light when no light object is supplied");
    unsigned char pixel[4] = {};
    glReadPixels(512,384,1,1,GL_RGBA,GL_UNSIGNED_BYTE,pixel);
    check(pixel[0] > 200 && pixel[1] < 20, "Scene/camera/material indexed triangle renders");
    scene.clear();
    glColor3f(0,0,1); glPushMatrix(); glTranslatef(0,0,0.5f);
    meshUnderTest->render(); glPopMatrix();
    glColor3f(1,0,0); meshUnderTest->render();
    glReadPixels(512,384,1,1,GL_RGBA,GL_UNSIGNED_BYTE,pixel);
    check(pixel[2] > 200 && pixel[0] < 20, "Near geometry occludes later far geometry");
    GLint mode;
    glGetIntegerv(GL_MATRIX_MODE,&mode);
    Billboard billboard(texture);
    billboard.render();
    glReadPixels(512,384,1,1,GL_RGBA,GL_UNSIGNED_BYTE,pixel);
    check(pixel[0] < 20 && pixel[1] > 200, "Textured billboard renders");
    GLint after;
    glGetIntegerv(GL_MATRIX_MODE,&after);
    check(after == mode && glIsEnabled(GL_DEPTH_TEST), "Billboard restores rendering state");
    Line line(Vec3(-1,0,0),Vec3(1,0,0)); line.render();
    Vec3 p(0,0,0), n(0,1,0);
    Plane plane(p,n); plane.render();
    clText::start();
    char label[] = "Graphics library";
    clText::draw(1,24,-0.8f,0.8f,label);
    clText::end();
    check(glGetError() == GL_NO_ERROR, "Rendering has no OpenGL errors");
}
static void stop() {
    oldTexture = texture->DispListNum;
    Scene::shutDown();
    check(glIsTexture(oldTexture) == GL_FALSE, "Scene releases its GL texture resources");
    clText::shutDown();
    std::remove("render-green.bmp"); std::remove("render-material.ods");
    std::remove("render-rows.bmp"); std::remove("render-deferred.bmp");
}
int main(int argc, char ** argv) {
    unsigned char pixel[] = {255,255,255,255};
    GLTXimage image = {1,1,1,1,4,GL_RGBA,pixel};
    check(gltxWriteBMP("render-deferred.bmp", &image), "Write deferred fixture");
    deferredTexture.load("render-deferred.bmp",false);
    const ViewerCallbacks callbacks = { title,start,stop,resize,0,render,0,0,0 };
    if (viewerRun(argc,argv,callbacks,true,2)) return 1;
    if (failures) return 1;
    std::puts("GraphicsRenderingTests: scene, material, texture, primitive and cleanup passed");
    return 0;
}
