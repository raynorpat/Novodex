#include "Act.h"
#include "ActActor.h"
#include "ActRecorder.h"
#include "PhysicsDemo.h"
#include "ViewerGraphicsContext.h"

//interaction
void Act::linePick(const Vec3 & start, const Vec3 & end)	//pick an object (stab)
	{
	NxVec3 cstart; cstart.set(&start.x);
	NxVec3 cend;   cend.set(&end.x);

	if (recorder)
		recorder->recordPick(cstart, cend, true, frameNumber);

	linePick(cstart, cend);
	}

ActActor * Act::linePick(const NxVec3 & cstart, const NxVec3 & cend)	//pick an object (stab)
	{
	NxVec3 cworldStab;
	
	pickedActor = 0;

//	NxShape* pickedShape = scene->pickShape(cstart, cend, cworldStab);
	NxRay worldRay;
	worldRay.orig	= cstart;
	worldRay.dir	= cend - cstart;
	worldRay.dir.normalize();

	NxRaycastHit hit;
	NxShape* pickedShape = scene->raycastClosestShape(worldRay, NX_ALL_SHAPES, hit);
	if (!pickedShape) return pickedActor;
	cworldStab = hit.worldImpact;
		pickedActor = (ActActor *)(pickedShape->getActor().userData);
	if (!pickedActor) return pickedActor;
			
	if (pickedActor->getName())
		printf("picked: %s (%p)\n", pickedActor->getName(), static_cast<void *>(pickedActor));
	else
		printf("picked: [noname] (%p)\n", static_cast<void *>(pickedActor));
			
	//does this body have a controller that can be activated?
	if (pickedActor->getController())
		{
		container->setActController(pickedActor->getController());
		pickedActor = 0;	//cancel dragging
		}
	else
		{ //otherwise just drag it:
		const NxMat34 * mp = pickedActor->getActor2World();
		if (mp)
			{	
			mp->multiplyByInverseRT(cworldStab, stabPointActorSpace);
			lineDrag(cstart, cend);	// that way we can pick moving objects without dragging
			NxVec3 delta = cworldStab - lineStart;
			stabPointCameraDist = delta.magnitude();
			}

		}
	return pickedActor;
	}

void Act::lineDrag(const Vec3 & start, const Vec3 & end)	//drag object around.
	{
	NxVec3 s(start.x, start.y, start.z);
	NxVec3 e(end.x, end.y, end.z);

	lineDrag(s,e);

	if (recorder)
		recorder->recordPick(lineStart, lineEnd, false, frameNumber);
	}


void Act::lineDrag(const NxVec3 & s, const NxVec3 & e)
	{
	lineStart = s;
	lineEnd = e;
	drag = true;
	}

void Act::activatePicked(unsigned code)	//this is just for testing
	{
	if (pickedActor)
		{
		switch (code)
			{
			case 0://delete
			deleteActor(*pickedActor);
			pickedActor = 0;
			break;
			case 1://nudge
				{
				NxActor* b = pickedActor->getActor();
				if (b)
					{
					b->addForceAtLocalPos(NxVec3(0.0f,-1000.0f,0.0f), NxVec3(10.0f,0.0f,0.0f));
					pickedActor = 0;
					}
				}
			break;
			}
		}
	}

void Act::nearestPointOnLine(const NxVec3 & point, const NxVec3 & start, const NxVec3 & end, NxVec3 & nearest)
//Helper for lineDrag -- based on 3d point - line distance by David Eberly
	{
    NxVec3 kDiff = point - start;
	NxVec3 dir = end - start;
    double fT = kDiff.dot(dir);

    if ( fT <= 0.0f )
        fT = 0.0f;
    else
    {
        double  fSqrLen= dir.magnitudeSquared();
        if ( fT >= fSqrLen )
        {
            fT = 1.0f;
            kDiff -= dir;
        }
        else
        {
            fT /= fSqrLen;
            kDiff -= dir*fT;
        }
    }

   nearest = dir * fT + start;
    //distance from point to line segment is: kDiff.MagnitudeSq();
	}

void Act::unpick()
	{
	Vec3 z(0,0,0);
	ViewGC::pickLine->set(z, z);
	pickedActor = NULL;
	drag = false;

	if (recorder)
		recorder->recordUnpick(frameNumber);
	}

void Act::tickDrag()
	{
	NxVec3 nearest;
	NxVec3 worldStab;

	if (recorder)
		recorder->playback(frameNumber);

	if (pickedActor)
		{
		//1) get the point of attachment in world space (where worldStab needs to be moved)
		nearest = lineEnd - lineStart;
		nearest.normalize();
		nearest *= stabPointCameraDist;
		nearest += lineStart;

		//0) track mouse, and find moved line L  -- obj could have moved -- retransform localStab to worldStab:
		const NxMat34 * mp = pickedActor->getActor2World();
		if (mp)
			mp->multiply(stabPointActorSpace, worldStab);

		//1) get the point of attachment in world space (where worldStab needs to be moved)
		nearest = lineEnd - lineStart;
		nearest.normalize();
		nearest *= stabPointCameraDist;
		nearest += lineStart;

		//2) apply a force to the picked object at X, with direction along segment, and magnitude proportional to length of the segment.
		NxVec3 force = nearest - worldStab;

		double stretchSq = force.magnitudeSquared();
		if (stretchSq > maxStretch*maxStretch)
			{
			force *= maxStretch*(1/sqrt(stretchSq));	//vector may maximally be maxStretch sized.
			}

		force *= dragForce;

		//damping:
		NxActor* b = pickedActor->getActor();

		if (b && b->isDynamic())
			{
			NxVec3 pointVel;
			pointVel = b->getPointVelocity(worldStab);

			force += pointVel * -1.0f; //0.1f;

			force *= b->getMass();			//force is mass invariant
			b->addForceAtPos(force, worldStab);
			}

		//visualize dragging with a line somehow.
		ViewGC::pickLine->set(Vec3((float)worldStab.x,(float)worldStab.y,(float)worldStab.z), Vec3((float)nearest.x, (float)nearest.y, (float)nearest.z));
		}
	}

