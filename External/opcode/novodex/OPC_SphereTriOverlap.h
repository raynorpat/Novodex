/*
 * NOVODEX LOCAL MODIFICATION -- BUILD PARITY, not a NovodeX source change
 * upstream: External/opcode/upstream/Opcode/OPC_SphereTriOverlap.h
 *
 * [1] The edge-region distances `u = -fB0/fA00; SqrDist = fB0*u+fC` (and the
 *     fB1/fA11 twins) are evaluated the way the 2003 compiler emitted them: a
 *     reciprocal of -1.0 multiplied in, (-1/fA)*fB*fB + fC, with nothing rounded
 *     to float on the way. The six source sites fold into two blocks in the
 *     image, one per edge. A 2026 /fp:precise build divides (fchs; fdivrp) and
 *     rounds u to float before the product, which differs in the low bits.
 *     established at 0x000de623..0x000de635 (fld -1.0 from .rdata:0x0010687c;
 *     fdiv [fA00]; fmul [fB0]; fmul [fB0]; faddp fC) and 0x000de710..0x000de722
 *     (the same over fA11/fB1); the candidate's stock form was at 0x000a1c0a
 *     and 0x000a1cf1.
 * [2] SqrDist lives in a register to the final compare: every arm jumps with the
 *     value on the x87 stack to 0x000de7c1 `fabs; fcomp [ecx+0x40]` (mRadius2),
 *     so it is never rounded to float. Declared double here, the project's x87
 *     convention for a register lifetime, and compared through fabs, not fabsf.
 */

// This is collision detection. If you do another distance test for collision *response*,
// if might be useful to simply *skip* the test below completely, and report a collision.
// - if sphere-triangle overlap, result is ok
// - if they don't, we'll discard them during collision response with a similar test anyway
// Overall this approach should run faster.

// Original code by David Eberly in Magic.
BOOL SphereCollider::SphereTriOverlap(const Point& vert0, const Point& vert1, const Point& vert2)
{
	// Stats
	mNbVolumePrimTests++;

	// Early exit if one of the vertices is inside the sphere
	Point kDiff = vert2 - mCenter;
	float fC = kDiff.SquareMagnitude();
	if(fC <= mRadius2)	return TRUE;

	kDiff = vert1 - mCenter;
	fC = kDiff.SquareMagnitude();
	if(fC <= mRadius2)	return TRUE;

	kDiff = vert0 - mCenter;
	fC = kDiff.SquareMagnitude();
	if(fC <= mRadius2)	return TRUE;

	// Else do the full distance test
	Point TriEdge0	= vert1 - vert0;
	Point TriEdge1	= vert2 - vert0;

//Point kDiff	= vert0 - mCenter;
	float fA00	= TriEdge0.SquareMagnitude();
	float fA01	= TriEdge0 | TriEdge1;
	float fA11	= TriEdge1.SquareMagnitude();
	float fB0	= kDiff | TriEdge0;
	float fB1	= kDiff | TriEdge1;
//float fC	= kDiff.SquareMagnitude();
	float fDet	= fabsf(fA00*fA11 - fA01*fA01);
	float u		= fA01*fB1-fA11*fB0;
	float v		= fA01*fB0-fA00*fB1;
	double SqrDist;	// NOVODEX [2]: a register lifetime, never spilled as float

	if(u + v <= fDet)
	{
		if(u < 0.0f)
		{
			if(v < 0.0f)  // region 4
			{
				if(fB0 < 0.0f)
				{
//					v = 0.0f;
					if(-fB0>=fA00)			{ /*u = 1.0f;*/		SqrDist = fA00+2.0f*fB0+fC;	}
					else					{ SqrDist = (-1.0/double(fA00))*fB0*fB0+fC;	}	/* NOVODEX [1]: u = -fB0/fA00 */
				}
				else
				{
//					u = 0.0f;
					if(fB1>=0.0f)			{ /*v = 0.0f;*/		SqrDist = fC;				}
					else if(-fB1>=fA11)		{ /*v = 1.0f;*/		SqrDist = fA11+2.0f*fB1+fC;	}
					else					{ SqrDist = (-1.0/double(fA11))*fB1*fB1+fC;	}	/* NOVODEX [1]: v = -fB1/fA11 */
				}
			}
			else  // region 3
			{
//				u = 0.0f;
				if(fB1>=0.0f)				{ /*v = 0.0f;*/		SqrDist = fC;				}
				else if(-fB1>=fA11)			{ /*v = 1.0f;*/		SqrDist = fA11+2.0f*fB1+fC;	}
				else						{ SqrDist = (-1.0/double(fA11))*fB1*fB1+fC;	}	/* NOVODEX [1]: v = -fB1/fA11 */
			}
		}
		else if(v < 0.0f)  // region 5
		{
//			v = 0.0f;
			if(fB0>=0.0f)					{ /*u = 0.0f;*/		SqrDist = fC;				}
			else if(-fB0>=fA00)				{ /*u = 1.0f;*/		SqrDist = fA00+2.0f*fB0+fC;	}
			else							{ SqrDist = (-1.0/double(fA00))*fB0*fB0+fC;	}	/* NOVODEX [1]: u = -fB0/fA00 */
		}
		else  // region 0
		{
			// minimum at interior point
			if(fDet==0.0f)
			{
//				u = 0.0f;
//				v = 0.0f;
				SqrDist = MAX_FLOAT;
			}
			else
			{
				float fInvDet = 1.0f/fDet;
				u *= fInvDet;
				v *= fInvDet;
				SqrDist = u*(fA00*u+fA01*v+2.0f*fB0) + v*(fA01*u+fA11*v+2.0f*fB1)+fC;
			}
		}
	}
	else
	{
		float fTmp0, fTmp1, fNumer, fDenom;

		if(u < 0.0f)  // region 2
		{
			fTmp0 = fA01 + fB0;
			fTmp1 = fA11 + fB1;
			if(fTmp1 > fTmp0)
			{
				fNumer = fTmp1 - fTmp0;
				fDenom = fA00-2.0f*fA01+fA11;
				if(fNumer >= fDenom)
				{
//					u = 1.0f;
//					v = 0.0f;
					SqrDist = fA00+2.0f*fB0+fC;
				}
				else
				{
					u = fNumer/fDenom;
					v = 1.0f - u;
					SqrDist = u*(fA00*u+fA01*v+2.0f*fB0) + v*(fA01*u+fA11*v+2.0f*fB1)+fC;
				}
			}
			else
			{
//				u = 0.0f;
				if(fTmp1 <= 0.0f)		{ /*v = 1.0f;*/		SqrDist = fA11+2.0f*fB1+fC;	}
				else if(fB1 >= 0.0f)	{ /*v = 0.0f;*/		SqrDist = fC;				}
				else					{ SqrDist = (-1.0/double(fA11))*fB1*fB1+fC;	}	/* NOVODEX [1]: v = -fB1/fA11 */
			}
		}
		else if(v < 0.0f)  // region 6
		{
			fTmp0 = fA01 + fB1;
			fTmp1 = fA00 + fB0;
			if(fTmp1 > fTmp0)
			{
				fNumer = fTmp1 - fTmp0;
				fDenom = fA00-2.0f*fA01+fA11;
				if(fNumer >= fDenom)
				{
//					v = 1.0f;
//					u = 0.0f;
					SqrDist = fA11+2.0f*fB1+fC;
				}
				else
				{
					v = fNumer/fDenom;
					u = 1.0f - v;
					SqrDist = u*(fA00*u+fA01*v+2.0f*fB0) + v*(fA01*u+fA11*v+2.0f*fB1)+fC;
				}
			}
			else
			{
//				v = 0.0f;
				if(fTmp1 <= 0.0f)		{ /*u = 1.0f;*/		SqrDist = fA00+2.0f*fB0+fC;	}
				else if(fB0 >= 0.0f)	{ /*u = 0.0f;*/		SqrDist = fC;				}
				else					{ SqrDist = (-1.0/double(fA00))*fB0*fB0+fC;	}	/* NOVODEX [1]: u = -fB0/fA00 */
			}
		}
		else  // region 1
		{
			fNumer = fA11 + fB1 - fA01 - fB0;
			if(fNumer <= 0.0f)
			{
//				u = 0.0f;
//				v = 1.0f;
				SqrDist = fA11+2.0f*fB1+fC;
			}
			else
			{
				fDenom = fA00-2.0f*fA01+fA11;
				if(fNumer >= fDenom)
				{
//					u = 1.0f;
//					v = 0.0f;
					SqrDist = fA00+2.0f*fB0+fC;
				}
				else
				{
					u = fNumer/fDenom;
					v = 1.0f - u;
					SqrDist = u*(fA00*u+fA01*v+2.0f*fB0) + v*(fA01*u+fA11*v+2.0f*fB1)+fC;
				}
			}
		}
	}

	return fabs(SqrDist) < mRadius2;	// NOVODEX [2]
}
