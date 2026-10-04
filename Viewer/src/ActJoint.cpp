/*----------------------------------------------------------------------------*\
|
|								NovodeX Technology
|
|							     www.novodex.com
|
\*----------------------------------------------------------------------------*/
#include "ViewerPlatform.h"

#include "Act.h"
#include "ActJoint.h"
#include "ODBlock.h"
#include "ActActor.h"
#include "3DMath.h"

#include "ViewerGraphicsContext.h"

extern void Fatal(const char * m);
#define ACT Act::instance()

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

ActJoint::ActJoint()
	{
	name = 0;
	joint = 0;
	next = previous = 0;
	type = JT_NONE; 
	block = 0;
	}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

ActJoint::~ActJoint()
	{
	//TODO: release joint
	if (joint)
		{
		ACT->scene->releaseJoint(*joint);
		}
	name = 0;
	joint = 0;
	next = previous = 0;
	type = JT_NONE;
	}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

bool ActJoint::load(unsigned jointNum, ODBlock * block, NxScene* scene)
/*
JointType
	{
	bodies { name1; name2; }	//name "WORLD" is fixed world frame.
	//other joint dependent properties
	}
*/
	{
	this->block = block;
	NxActor * b1 = 0, * b2 = 0;
	ActActor * actor1 = 0, * actor2 = 0;

	//get bodies.
	ODBlock * bbs = block->getBlock("bodies");
	if (bbs)
		{
		bbs->reset();
		if(bbs->moreTerminals())
			{
			char * bname = bbs->nextTerminal();
			//find it
			if (0 == strcmp(bname,"WORLD"))
				{
				//b1 already NULL.
				}
			else
				{
				actor1 = ACT->findActor(bname);
				if (!actor1)
					{
					printf("Warning: Unknown body1 reference in joint %d! Joint ignored.\n", jointNum);
					return false;
					}
				if (actor1)
					{
					b1 = actor1->getActor();
					if (!b1->isDynamic())
						b1 = 0;				//drop reference to statics --> map them to world.
					}
				}
			}
		//body 2
		if(bbs->moreTerminals())
			{
			char * bname = bbs->nextTerminal();
			//find it
			if (0 == strcmp(bname,"WORLD"))
				{
				//b2 already NULL.
				}
			else
				{
				actor2 = ACT->findActor(bname);
				if (!actor2)
					{
					printf("Warning: Unknown body2 reference in joint %d! Joint ignored.\n", jointNum);
					return false;
					}
				if (actor2)
					{
					b2 = actor2->getActor();
					if (!b2->isDynamic())
						b2 = 0;				//drop reference to statics --> map them to world.
					}
				}
			}
		}

	if (!b1 && !b2)
		{
		printf("Warning: Joint %d references no dynamic actors. Joint ignored.\n", jointNum);
		return false;
		}

	//the following depends on the joint type
	NxJoint * pjoint = 0;

	const char * type = block->ident();
	if (0 == strcmp(type, "Spherical"))
		{
		Vec3 t, n;
		NxSphericalJointDesc sjd;
		sjd.name = name;
		sjd.actor[0] = b1;
		sjd.actor[1] = b2;

		if (!block->getBlockFloats("anchor",&t.x,3))
			{
			printf("Warning: SphericalJoint %d has no anchor. Joint ignored.\n", jointNum);
			return false;
			}
		sjd.setGlobalAnchor(NxVec3(t.x, t.y, t.z));

		if (block->getBlockFloats("direction",&n.x,3))
			{
			sjd.setGlobalAxis(NxVec3(n.x, n.y, n.z));
			}

		if (block->getBlockFloats("twistLowLimit",&t.x,3))
			{
			sjd.twistLimit.low.value = NxMath::degToRad(t.x);
			sjd.twistLimit.low.restitution = t.y;
			sjd.twistLimit.low.hardness = t.z;
			sjd.flags |= NX_SJF_TWIST_LIMIT_ENABLED;
			}
		if (block->getBlockFloats("twistHighLimit",&t.x,3))
			{
			sjd.twistLimit.high.value = NxMath::degToRad(t.x);
			sjd.twistLimit.high.restitution = t.y;
			sjd.twistLimit.high.hardness = t.z;
			sjd.flags |= NX_SJF_TWIST_LIMIT_ENABLED;
			}
		if (block->getBlockFloats("swingLimit",&t.x,3))
			{
			sjd.swingLimit.value = NxMath::degToRad(t.x);
			sjd.swingLimit.restitution = t.y;
			sjd.swingLimit.hardness = t.z;
			sjd.flags |= NX_SJF_SWING_LIMIT_ENABLED;
			}
		if (block->getBlockFloats("swingAxis",&t.x,3))
			{
			sjd.swingAxis.set(t.x,t.y,t.z);
			}
		if (block->getBlockFloats("swingSpring",&t.x,3))
			{
			sjd.swingSpring.spring = t.x;
			sjd.swingSpring.damper = t.y;
			sjd.swingSpring.targetValue = NxMath::degToRad(t.z);
			sjd.flags |= NX_SJF_SWING_SPRING_ENABLED;
			}
		if (block->getBlockFloats("twistSpring",&t.x,3))
			{
			sjd.twistSpring.spring = t.x;
			sjd.twistSpring.damper = t.y;
			sjd.twistSpring.targetValue = NxMath::degToRad(t.z);
			sjd.flags |= NX_SJF_TWIST_SPRING_ENABLED;
			}

		//TODO: this is default for now until we update the file format:
		sjd.projectionMode = NX_JPM_POINT_MINDIST;
		sjd.projectionDistance = 0.08f;
		NxSphericalJoint * xjoint = scene->createJoint(sjd)->isSphericalJoint(); 
		pjoint = &xjoint->getJoint();
		this->type = JT_SPHERICAL;
		}
	else if (0 == strcmp(type, "PointInPlane"))
		{
		Vec3 t, n;

		if (!block->getBlockFloats("anchor",&t.x,3))
			{
			printf("Warning: PointInPlane %d has no anchor. Joint ignored.\n", jointNum);
			return false;
			}
		if (!block->getBlockFloats("normal",&n.x,3))
			n.Set(0,1,0);
		else
			n.Normalize();

		//create the joint:
		NxPointInPlaneJointDesc pipjd;
		pipjd.name = name;
		pipjd.actor[0] = b1;
		pipjd.actor[1] = b2;
		pipjd.setGlobalAnchor(NxVec3(t.x, t.y, t.z));
		pipjd.setGlobalAxis(NxVec3(n.x, n.y, n.z));
		NxPointInPlaneJoint * xjoint = scene->createJoint(pipjd)->isPointInPlaneJoint();
		pjoint = &xjoint->getJoint();
		this->type = JT_POINT_IN_PLANE;
		}
	else if (0 == strcmp(type, "PointOnLine"))
		{
		Vec3 t, n;

		if (!block->getBlockFloats("anchor",&t.x,3))
			{
			printf("Warning: PointOnLine %d has no anchor. Joint ignored.\n", jointNum);
			return false;
			}
		if (!block->getBlockFloats("direction",&n.x,3))
			n.Set(0,1,0);
		else
			n.Normalize();

		//create the joint:
		NxPointOnLineJointDesc poljd;
		poljd.name = name;
		poljd.actor[0] = b1;
		poljd.actor[1] = b2;
		poljd.setGlobalAnchor(NxVec3(t.x, t.y, t.z));
		poljd.setGlobalAxis(NxVec3(n.x, n.y, n.z));
		NxPointOnLineJoint * xjoint = scene->createJoint(poljd)->isPointOnLineJoint();
		pjoint = &xjoint->getJoint();
		this->type = JT_POINT_ON_LINE;
		}
	else if (0 == strcmp(type, "Prismatic"))
		{
		Vec3 t, n;
		if (!block->getBlockFloats("anchor",&t.x,3))
			{
			printf("Warning: Prismatic %d has no anchor. Joint ignored.\n", jointNum);
			return false;
			}

		if (!block->getBlockFloats("direction",&n.x,3))
			n.Set(0,1,0);
		else
			n.Normalize();

		//create the joint:
		NxPrismaticJointDesc pjd;
		pjd.name = name;
		pjd.actor[0] = b1;
		pjd.actor[1] = b2;
		pjd.setGlobalAnchor(NxVec3(t.x, t.y, t.z));
		pjd.setGlobalAxis(NxVec3(n.x, n.y, n.z));
		NxPrismaticJoint * xjoint = scene->createJoint(pjd)->isPrismaticJoint(); //,Vec3d(t.x, t.y, t.z), Vec3d(n.x, n.y, n.z));
		pjoint = &xjoint->getJoint();
		this->type = JT_PRISMATIC;
		}
	else if (0 == strcmp(type, "Sliding"))
		{
		Vec3 t, n;
		if (!block->getBlockFloats("anchor",&t.x,3))
			{
			printf("Warning: Sliding %d has no anchor. Joint ignored.\n", jointNum);
			return false;
			}
		if (!block->getBlockFloats("direction",&n.x,3))
			n.Set(0,1,0);
		else
			n.Normalize();
		//create the joint:
		NxCylindricalJointDesc sjd;
		sjd.name = name;
		sjd.actor[0] = b1;
		sjd.actor[1] = b2;
		sjd.setGlobalAnchor(NxVec3(t.x, t.y, t.z));
		sjd.setGlobalAxis(NxVec3(n.x, n.y, n.z));
		NxCylindricalJoint * xjoint = scene->createJoint(sjd)->isCylindricalJoint();
		pjoint = &xjoint->getJoint();
		this->type = JT_CYLINDRICAL;
		}
	else if (0 == strcmp(type, "Hinge"))
		{
		Vec3 t, n;
		if (!block->getBlockFloats("anchor",&t.x,3))
			{
			printf("Warning: Hinge %d has no anchor. Joint ignored.\n", jointNum);
			return false;
			}
		if (!block->getBlockFloats("direction",&n.x,3))
			n.Set(0,1,0);
		else
			n.Normalize();

		//create the joint:
		NxRevoluteJointDesc hjd;
		hjd.name = name;
		hjd.actor[0] = b1;
		hjd.actor[1] = b2;
		hjd.setGlobalAnchor(NxVec3(t.x, t.y, t.z));
		hjd.setGlobalAxis(NxVec3(n.x, n.y, n.z));

		if (block->getBlock("limits"))
			{
			Fatal("found joint with obsolete 'limits', please fix the file!");
			}

		if (block->getBlockFloats("lowLimit",&t.x,3))
			{
			hjd.limit.low.value = NxMath::degToRad(t.x);
			hjd.limit.low.restitution = t.y;
			hjd.limit.low.hardness = t.z;
			hjd.flags |= NX_RJF_LIMIT_ENABLED;
			}
		if (block->getBlockFloats("highLimit",&t.x,3))
			{
			hjd.limit.high.value = NxMath::degToRad(t.x);
			hjd.limit.high.restitution = t.y;
			hjd.limit.high.hardness = t.z;
			hjd.flags |= NX_RJF_LIMIT_ENABLED;
			}
		if (block->getBlockFloats("motor",&t.x,3))
			{
			hjd.motor.velTarget = t.x;
			hjd.motor.maxForce = t.y;
			hjd.motor.freeSpin = (t.z != 0);
			hjd.flags |= NX_RJF_MOTOR_ENABLED;
			}
		if (block->getBlockFloats("springDamper",&t.x,3))
			{
			hjd.spring.spring = t.x;
			hjd.spring.damper = t.y;
			hjd.spring.targetValue = NxMath::degToRad(t.z);
			hjd.flags |= NX_RJF_SPRING_ENABLED;
			}

		NxRevoluteJoint * xjoint = scene->createJoint(hjd)->isRevoluteJoint();


		pjoint = &xjoint->getJoint();
		this->type = JT_REVOLUTE;
		}
	else if (0 == strncmp(type, "CollisionVeto", 13))	//legacy: used to be CollisionVetoJoint
		{
		//this is actually not a joint, but a call to the collision SDK:
		if (actor1 && actor2)
			actor1->enablePair(scene, *actor2, false);
		else
			printf("Warning: CollisionVeto %d doesn't have two decent bodies. Joint ignored.\n", jointNum);

		return false;//don't create a joint object.
		}
	else
		{
		printf("Warning: Joint %d has unknown joint type. Joint ignored.\n", jointNum);
		return false;
		}

	Vec3 n, p;
	// joint limits -- same format for all joints:
	if (block->getBlockFloats("limitPoint_2", &p.x, 3))
		pjoint->setLimitPoint(NxVec3(p.x, p.y, p.z), true);
	else if (block->getBlockFloats("limitPoint_1", &p.x, 3))
		pjoint->setLimitPoint(NxVec3(p.x, p.y, p.z), false);

	ODBlock * planes = block->getBlock("limitPlanes");
	if (planes)
		{
		planes->reset();
		while (planes->moreSubBlocks())
			{
			ODBlock * plane = planes->nextSubBlock();
			if (plane->getBlockFloats("n", &n.x, 3))
				{
				if (plane->getBlockFloats("p", &p.x, 3))
					{
					if (!pjoint->addLimitPlane(NxVec3(n.x, n.y, n.z), NxVec3(p.x, p.y, p.z)))
						printf("Warning: One of joint %d's limit planes(%s) is invalid!\n", jointNum, plane->ident());
					}
				else
					printf("Warning: One of joint %d's limit planes (%s) is missing a 'p'. Plane ignored.\n", jointNum, plane->ident());
				}
			else
				printf("Warning: One of joint %d's limit planes (%s) is missing an 'n'. Plane ignored.\n", jointNum, plane->ident());
			}
		}

	block->getBlockString("name", &name);
	if (!name)
		name = type;
	//method:

	// This SDK selects the joint solver internally; the old "Lagrange"
	// script directive no longer has a requestMethod API.

	printf("Read: %s\n", name);

	joint = pjoint;

	//breakability:
	NxReal jointBreakForce = 0;
	if (block->getBlockFloat("breakable",&jointBreakForce))
		{
		joint->setBreakable(jointBreakForce, jointBreakForce);	//TODO: allow setting these separately?
		}
	joint->userData = this;
	return true;
	}

