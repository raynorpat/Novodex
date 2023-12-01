#ifndef NX_COLLISION_NXSHAPEDESC
#define NX_COLLISION_NXSHAPEDESC
/*----------------------------------------------------------------------------*\
|
|						Public Interface to NovodeX Technology
|
|							     www.novodex.com
|
\*----------------------------------------------------------------------------*/

#include "NxShape.h"

/**
Descriptor for NxShape class. Used for saving and loading the shape state.
See the derived classes for the different shape types.
*/
class NxShapeDesc
	{
	protected:
	const NxShapeType		type;			//!< The type of the shape (see NxShape.h). This gets set by the derived class' ctor, the user should not have to change it.
	public:
	NxMat34					localPose;		//!< The pose of the shape in the coordinate frame of the owning actor.
	NxU32					triggerFlags;	//!< A combination of NxShape::TriggerFlag values.
	NxCollisionGroup		group;			//!< See the documentation for NxShape::setGroup().
	NxMaterialIndex			materialIndex;	//!< The material index of the shape.  See NxPhysicsSDK::addMaterial().
	void*					userData;		//!< Will be copied to NxShape::userData.
	const char*				name;			//!< Possible debug name. The string is not copied by the SDK, only the pointer is stored.

	NX_INLINE virtual		~NxShapeDesc();
	/**
	(re)sets the structure to the default.	
	*/
	NX_INLINE virtual	void setToDefault();
	/**
	returns true if the current settings are valid
	*/
	NX_INLINE virtual	bool isValid() const;

	NX_INLINE			NxShapeType	getType()	const	{ return type; }

	protected:
	/**
	constructor sets to default.
	*/
	NX_INLINE				NxShapeDesc(NxShapeType);
	};

NX_INLINE NxShapeDesc::NxShapeDesc(NxShapeType t) : type(t)
	{
	setToDefault();
	}

NX_INLINE NxShapeDesc::~NxShapeDesc()
	{
	}

NX_INLINE void NxShapeDesc::setToDefault()
	{
	localPose.id();
	triggerFlags	= 0;
	group			= 0;
	materialIndex	= 0;
	userData		= NULL;
	name			= NULL;
	}

NX_INLINE bool NxShapeDesc::isValid() const
	{
	if(!localPose.isFinite())
		return false;
	if(group>=32)
		return false;	// We only support 32 different groups
#ifdef WIN32
	if(triggerFlags&~NX_TRIGGER_ENABLE)
		return false;
#elif LINUX
	if(triggerFlags&~NX_TRIGGER_ENABLE)
		return false;
#endif
	if (type >= NX_SHAPE_COUNT)
		return false;
	return true;
	}


#endif
