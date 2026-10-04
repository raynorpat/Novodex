#ifndef _SG_OBJECT_H__
#define _SG_OBJECT_H__
/*----------------------------------------------------------------------------*\
|                               NovodeX Technology
|                                  www.novodex.com
\*----------------------------------------------------------------------------*/
#include <3DMath.h>

namespace SceneGraph {
class Model;
class Primitive;

// Scene objects form an owned forest. Models are shared, nonowned resources;
// primitives and descendants belong to their object.
class Object {
public:
    Object(Model * m=0,Object * parent=0);
    virtual ~Object();
    virtual void render();
    void renderAsCamera();
    void renderAsLight(int lightNo);
    void setScale(flo s);
    void setPosition(flo x,flo y,flo z);
    void setPosition(Vec3 * position);
    void setOrientation(Quat * quat);
    void setOrientation(Vec3 * yawPitchRoll);
    void setVisibility(bool visible);
    void lookAt(Vec3 * target,Vec3 * up);
    void updateMatrix();

    Object * createChild(Model * model);
    void addChild(Object * child);
    bool deleteChild(Object * child);
    bool removeChild(Object * child);
    // Returns model box edge lengths, including baked and object scale.
    void getBoundingBox(Vec3 & dims);
    Model * collapseSubtreeToNewModel(Model * model=0,bool keepMaterials=true,
        bool meshesInParentsSpace=false,clMatrix4x4 * transform=0);
    void deleteSubtree();
    void removeSubtree();
    // Move subtree drawing into our parent's space and reset node transforms.
    void executeTransform();

    Model * model; // Nonowned.
    Primitive * primitive; // Owned.
    float scale;
    Vec3 position;
    Quat qOrientation;
    clMatrix4x4 orientation;
    clMatrix4x4 model2World; // Does not contain scaling.
    bool bNullXform; // Storage node: ignore transforms and model.
    Object * leftObj,*middleObj,*rightObj; // Owned descendants.
    Object * parentObj;
    bool bHideModel;
    bool bHideSubtree;
    bool bWireFrame;
    bool bTwoSided;
private:
    clMatrix4x4 bakedModelTransform;
};
}
#endif
