// Single production definition shared with isolated kernel tests.
double __cdecl NxSegmentSegmentSquareDistance(const NxSegment* segment0,
	const NxSegment* segment1, NxReal* parameter0, NxReal* parameter1)
	{
	const double dirX = (double) segment0->p1.x - segment0->p0.x;
	const double dirY = (double) segment0->p1.y - segment0->p0.y;
	const double dirZ = (double) segment0->p1.z - segment0->p0.z;

	const NxReal otherX = (NxReal) ((double) segment1->p1.x - segment1->p0.x);
	const NxReal otherY = (NxReal) ((double) segment1->p1.y - segment1->p0.y);
	const NxReal otherZ = (NxReal) ((double) segment1->p1.z - segment1->p0.z);

	const NxReal offsetX = (NxReal) ((double) segment0->p0.x - segment1->p0.x);
	const NxReal offsetY = (NxReal) ((double) segment0->p0.y - segment1->p0.y);
	const NxReal offsetZ = (NxReal) ((double) segment0->p0.z - segment1->p0.z);

	const NxReal a = (NxReal) ((dirZ * dirZ + dirX * dirX) + dirY * dirY);
	const NxReal b = (NxReal) ((-dirZ * otherZ + -dirY * otherY) + -dirX * otherX);
	const NxReal c = (NxReal) (((double) otherZ * otherZ + (double) otherY * otherY)
		+ (double) otherX * otherX);
	const NxReal d = (NxReal) (((double) offsetZ * dirZ + (double) offsetY * dirY)
		+ (double) offsetX * dirX);
	// f stays in st(0) from 0x00033f67 to the faddp at 0x00034589 or one of the
	// fadd chains that end at 0x0003458b. Every leaf adds into it in place
	// rather than reloading it.
	const double f = ((double) offsetZ * offsetZ + (double) offsetY * offsetY)
		+ (double) offsetX * offsetX;

	// fabs at 0x00033f7b, then fst a narrowed copy and compare the *wide* one
	// against the epsilon at 0x00107938, which is 1e-5f. A NaN determinant
	// takes the near-parallel branch, because test ah,1 after fcomp reads C0
	// and the unordered result sets it.
	const double product = ((double) c * a) - ((double) b * b);
	const double wideDeterminant = product < 0.0 ? -product : product;
	const NxReal determinant = (NxReal) wideDeterminant;

	NxReal s = 0.0f;
	NxReal t = 0.0f;
	double result;

	if(wideDeterminant >= 1.0e-5f)
		{
		// e is computed here and not beside the other five: the near-parallel
		// branch at 0x000343cf skips it entirely and recomputes it in the two
		// of its own leaves that need it.
		const double wideE = (-(double) offsetZ * otherZ + -(double) offsetY * otherY)
			+ -(double) offsetX * otherX;
		const NxReal e = (NxReal) wideE;

		s = (NxReal) ((wideE * b) - ((double) d * c));
		t = (NxReal) (((double) d * b) - ((double) e * a));

		if(s >= 0.0f && s <= determinant)
			{
			if(t >= 0.0f && t <= determinant)
				{
				// 0x0003402d, the interior. The only leaf that divides both
				// parameters down and the only one that evaluates the full
				// quadratic. t is used wide for its first product and narrowed
				// for the two that follow.
				const double inverse = (double) 1.0f / determinant;
				s = (NxReal) ((double) s * inverse);
				const double wideT = inverse * t;
				t = (NxReal) wideT;
				const double half1 = ((wideT * c + (double) s * b) + ((double) e + e)) * t;
				const double half2 = (((double) t * b + (double) s * a) + ((double) d + d)) * s;
				result = f + (half1 + half2);
				}
			else if(t >= 0.0f)
				{
				// 0x00034083: t clamped to 1, s solved along segment0.
				const double edge = (double) d + b;
				t = 1.0f;
				if(edge >= 0.0)
					{
					s = 0.0f;
					result = (f + ((double) e + e)) + c;
					}
				else if(-edge >= a)
					{
					s = 1.0f;
					result = ((f + ((edge + e) + (edge + e))) + c) + a;
					}
				else
					{
					s = (NxReal) -(edge / a);
					result = (f + (edge * s + ((double) e + e))) + c;
					}
				}
			else
				{
				// 0x000340f3: t clamped to 0.
				t = 0.0f;
				if(d >= 0.0f)
					{
					s = 0.0f;
					result = f;
					}
				else if(-(double) d >= a)
					{
					s = 1.0f;
					result = (f + ((double) d + d)) + a;
					}
				else
					{
					// 0x0003425c keeps the quotient wide for the product and
					// stores only the narrowed copy as the parameter.
					const double quotient = -((double) d / a);
					s = (NxReal) quotient;
					result = f + quotient * d;
					}
				}
			}
		else if(s >= 0.0f)
			{
			// 0x0003411c: s past the far end. The fcomp qword at 0x00034120
			// compares t against a *double* zero where every other comparison
			// in this function uses a float one; the value is the same and the
			// operand size is not.
			bool clampS = false;
			if(t >= 0.0f && t <= determinant)
				clampS = true;
			else if(t >= 0.0f)
				{
				// 0x0003417b. The comparison against a uses the wide edge and
				// everything after it reloads the narrowed copy the fst at
				// 0x00034183 left in the determinant's own slot.
				const double wideEdge = (double) d + b;
				const NxReal edge = (NxReal) wideEdge;
				if(-wideEdge <= a)
					{
					t = 1.0f;
					if(edge >= 0.0f)
						{
						s = 0.0f;
						result = (f + ((double) e + e)) + c;
						}
					else
						{
						s = (NxReal) -((double) edge / a);
						result = (f + ((double) edge * s + ((double) e + e))) + c;
						}
					}
				else
					clampS = true;
				}
			else if(-(double) d < a)
				{
				// 0x0003422d
				t = 0.0f;
				if(d >= 0.0f)
					{
					s = 0.0f;
					result = f;
					}
				else
					{
					const double quotient = -((double) d / a);
					s = (NxReal) quotient;
					result = f + quotient * d;
					}
				}
			else
				clampS = true;

			if(clampS)
				{
				// 0x00034140: s clamped to 1, t solved along segment1. The
				// `fld [esp+0xc]` there reads the *narrowed* e, where the s
				// above multiplies the wide one still in st(0).
				const double edge = (double) e + b;
				s = 1.0f;
				if(edge >= 0.0)
					{
					t = 0.0f;
					result = (f + ((double) d + d)) + a;
					}
				else if(-edge >= c)
					{
					t = 1.0f;
					result = ((f + ((edge + d) + (edge + d))) + c) + a;
					}
				else
					{
					t = (NxReal) -(edge / c);
					result = (f + (edge * t + ((double) d + d))) + a;
					}
				}
			}
		else
			{
			// 0x0003428b: s negative.
			bool clampS = false;
			if(t >= 0.0f && t <= determinant)
				clampS = true;
			else if(t >= 0.0f)
				{
				// 0x000342d8. test ah,5 after fcom continues only on a strictly
				// negative edge; zero and unordered both fall into the s = 0
				// leaf below.
				const double edge = (double) d + b;
				if(edge < 0.0)
					{
					t = 1.0f;
					if(-edge >= a)
						{
						s = 1.0f;
						result = ((f + ((edge + e) + (edge + e))) + c) + a;
						}
					else
						{
						s = (NxReal) -(edge / a);
						result = (f + (edge * s + ((double) e + e))) + c;
						}
					}
				else
					clampS = true;
				}
			else if((double) d < 0.0)
				{
				// 0x00034348 into the shared tail at 0x00034365.
				t = 0.0f;
				if(-(double) d >= a)
					{
					s = 1.0f;
					result = (f + ((double) d + d)) + a;
					}
				else
					{
					const double quotient = -((double) d / a);
					s = (NxReal) quotient;
					result = f + quotient * d;
					}
				}
			else
				clampS = true;

			if(clampS)
				{
				// 0x000342af: s clamped to 0.
				s = 0.0f;
				if(e >= 0.0f)
					{
					t = 0.0f;
					result = f;
					}
				else if(-(double) e >= c)
					{
					t = 1.0f;
					result = (f + ((double) e + e)) + c;
					}
				else
					{
					const double quotient = -((double) e / c);
					t = (NxReal) quotient;
					result = f + quotient * e;
					}
				}
			}
		}
	else
		{
		// 0x000343cf, near-parallel. A second tree, and the two leaves that need
		// e recompute it -- one wide at 0x0003442d and one narrowed through the
		// same slot at 0x00034517.
		if(b > 0.0f)
			{
			if(d >= 0.0f)
				{
				s = 0.0f;
				t = 0.0f;
				result = f;
				}
			else if(-(double) d <= a)
				{
				// 0x00034410 stores the quotient and reads the narrowed value
				// back for the product, unlike its three siblings.
				s = (NxReal) -((double) d / a);
				t = 0.0f;
				result = f + (double) s * d;
				}
			else
				{
				// 0x0003442d: e recomputed and used wide.
				const double wideE = (-(double) offsetZ * otherZ + -(double) offsetY * otherY)
					+ -(double) offsetX * otherX;
				const double edge = (double) d + a;
				s = 1.0f;
				if(-edge >= b)
					{
					const double sum = (wideE + d) + b;
					t = 1.0f;
					result = ((f + (sum + sum)) + c) + a;
					}
				else
					{
					t = (NxReal) -(edge / b);
					const double doubled = (wideE + b) + (wideE + b);
					result = (f + ((doubled + (double) t * c) * t + ((double) d + d))) + a;
					}
				}
			}
		else if(-(double) d >= a)
			{
			s = 1.0f;
			t = 0.0f;
			result = (f + ((double) d + d)) + a;
			}
		else if((double) d <= 0.0)
			{
			s = (NxReal) -((double) d / a);
			t = 0.0f;
			result = f + (double) s * d;
			}
		else
			{
			// 0x00034517: e recomputed and narrowed through its own slot.
			const NxReal e = (NxReal) ((-(double) offsetZ * otherZ + -(double) offsetY * otherY)
				+ -(double) offsetX * otherX);
			s = 0.0f;
			if(-(double) b <= d)
				{
				t = 1.0f;
				result = (f + ((double) e + e)) + c;
				}
			else
				{
				const double quotient = -((double) d / b);
				t = (NxReal) quotient;
				result = f + ((quotient * c) + ((double) e + e)) * t;
				}
			}
		}

	// Both output pointers are optional -- test eax,eax at 0x0003458f and
	// 0x0003459c -- and both are written with integer moves, so what the caller
	// sees is exactly the 32-bit slot and not a value that has been through the
	// FPU again.
	if(parameter0)
		*parameter0 = s;
	if(parameter1)
		*parameter1 = t;
	// 0x000345a6, and it is a sign clear rather than a test, which is what
	// makes a negative NaN come back positive.
	return fabs(result);
	}
