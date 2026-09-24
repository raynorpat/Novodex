#ifndef NP_ACTOR_DYNAMIC_MATH_H
#define NP_ACTOR_DYNAMIC_MATH_H

// Dynamic record matrices are row-major. The shipped Win32 build evaluates
// quaternion products in x87 precision, then rounds each matrix element to
// float; its tensor helper rounds the scaled columns before accumulating.
static inline float nxNpActorRoundProduct(float a, float b)
	{
	volatile float result = a * b;
	return result;
	}

static inline void nxNpActorRotationFromQuaternion(
	const unsigned char* record, float* rotation)
	{
	const float* q = reinterpret_cast<const float*>(record + 0x5c);
	const double x = q[0], y = q[1], z = q[2], w = q[3];
	rotation[0] = static_cast<float>(1.0 - 2.0 * y * y - 2.0 * z * z);
	rotation[1] = static_cast<float>(2.0 * x * y - 2.0 * w * z);
	rotation[2] = static_cast<float>(2.0 * x * z + 2.0 * w * y);
	rotation[3] = static_cast<float>(2.0 * x * y + 2.0 * w * z);
	rotation[4] = static_cast<float>(1.0 - 2.0 * x * x - 2.0 * z * z);
	rotation[5] = static_cast<float>(2.0 * y * z - 2.0 * w * x);
	rotation[6] = static_cast<float>(2.0 * x * z - 2.0 * w * y);
	rotation[7] = static_cast<float>(2.0 * y * z + 2.0 * w * x);
	rotation[8] = static_cast<float>(1.0 - 2.0 * x * x - 2.0 * y * y);
	}

static inline void nxNpActorWorldTensor(const float* diagonal,
	const float* rotation, float* world)
	{
	for(unsigned row = 0; row < 3; ++row)
		{
		const float x = nxNpActorRoundProduct(diagonal[0], rotation[row * 3]);
		const float y = nxNpActorRoundProduct(diagonal[1], rotation[row * 3 + 1]);
		const float z = nxNpActorRoundProduct(diagonal[2], rotation[row * 3 + 2]);
		for(unsigned col = row; col < 3; ++col)
			{
			const double xx = static_cast<double>(x) * rotation[col * 3];
			const double zz = static_cast<double>(z) * rotation[col * 3 + 2];
			const double yy = static_cast<double>(y) * rotation[col * 3 + 1];
			world[row * 3 + col] = static_cast<float>(xx + zz + yy);
			world[col * 3 + row] = world[row * 3 + col];
			}
		}
	}

static inline void nxNpActorUpdateInertiaMatrices(unsigned char* record)
	{
	float* rotation = reinterpret_cast<float*>(record + 0x134);
	float* inverse = reinterpret_cast<float*>(record + 0x164);
	float bodyRotation[9];
	nxNpActorRotationFromQuaternion(record, bodyRotation);
	const float* frame = reinterpret_cast<const float*>(record + 0xdc);
	for(unsigned row = 0; row < 3; ++row)
		for(unsigned col = 0; col < 3; ++col)
			rotation[row * 3 + col] = static_cast<float>(
				static_cast<double>(bodyRotation[row * 3]) * frame[col] +
				static_cast<double>(bodyRotation[row * 3 + 1]) * frame[3 + col] +
				static_cast<double>(bodyRotation[row * 3 + 2]) * frame[6 + col]);
	nxNpActorWorldTensor(reinterpret_cast<const float*>(record + 0xc4),
		rotation, inverse);
	}

#endif
