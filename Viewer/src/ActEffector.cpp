/*----------------------------------------------------------------------------*\
|
|								NovodeX Technology
|
|							     www.novodex.com
|
\*----------------------------------------------------------------------------*/
#include "ActEffector.h"

#include "ODBlock.h"
#include "glm.h"

#include "Act.h"
#include "ActActor.h"

#define ACT Act::instance()


ActEffector::ActEffector()
	{
	effector = 0;
	next = previous = 0;
	}

ActEffector::~ActEffector()
	{
	effector = 0;
	next = previous = 0;
	}


bool ActEffector::load(unsigned effNum, ODBlock * block, NxScene* scene)
	{
	//only spring-damper support for now
	NxActor * b1 = 0, * b2 = 0;

	const char * type = block->ident();
	if (0 != strcmp(type, "SpringAndDamper"))
		{
		printf("Warning: Unknown Effector %s! Effector ignored.\n", type);
		return false;
		}

		Vec3 a1,a2;

		if (!block->getBlockFloats("anchor1",&a1.x,3))
			{
			printf("Warning: SpringAndDamper has no anchor1. Effector ignored.\n");
			return false;
			}
		if (!block->getBlockFloats("anchor2",&a2.x,3))
			{
			printf("Warning: SpringAndDamper has no anchor2. Effector ignored.\n");
			return false;
			}
		float spring[5];
		float damper[4];
		if (!block->getBlockFloats("spring",spring,5))
			{
			printf("Warning: SpringAndDamper has no spring params. Effector ignored.\n");
			return false;
			}
		if (!block->getBlockFloats("damper",damper,4))
			{
			printf("Warning: SpringAndDamper has no damper params. Effector ignored.\n");
			return false;
			}

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
					ActActor * actor = ACT->findActor(bname);
					if (!actor)
						{
						printf("Warning: Unknown body1 reference in Effector! Effector ignored.\n");
						return false;
						}

					if (actor)
						b1 = actor->getActor();
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
					ActActor * actor = ACT->findActor(bname);
					if (!actor)
						{
						printf("Warning: Unknown body2 reference in Effector! Effector ignored.\n");
						return false;
						}

					if (actor)
						b2 = actor->getActor();
					}
				}
			}

		if (!b1 && !b2)
			{
			printf("Warning: Effector references no bodies. Effector ignored.\n");
			return false;
			}

		NxSpringAndDamperEffectorDesc desc;
		NxSpringAndDamperEffector * e = scene->createSpringAndDamperEffector(desc);

		e->setBodies(b1, NxVec3(a1.x,a1.y,a1.z), b2, NxVec3(a2.x,a2.y,a2.z));
		e->setLinearSpring(spring[0],spring[1],spring[2],spring[3],spring[4]);
		e->setLinearDamper(damper[0],damper[1],damper[2],damper[3]);

		effector = &e->getEffector();

	return true;
	}

void ActEffector::visualize()
	{
	}

//#endif