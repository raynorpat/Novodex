/*----------------------------------------------------------------------------*\
|
|								NovodeX Technology
|
|							     www.novodex.com
|
\*----------------------------------------------------------------------------*/
#include "Nxf.h"
#include "NxFPU.h"
#include "NxVec3.h"

#ifdef WIN32
void NxSetFPUPrecision24()	{ _controlfp(_PC_24,	_MCW_PC); }
void NxSetFPUPrecision53()	{ _controlfp(_PC_53,	_MCW_PC); }
void NxSetFPUPrecision64()	{ _controlfp(_PC_64,	_MCW_PC); }

void NxSetFPURoundingChop()	{ _controlfp(_RC_CHOP,	_MCW_RC); }
void NxSetFPURoundingUp()	{ _controlfp(_RC_UP,	_MCW_RC); }
void NxSetFPURoundingDown()	{ _controlfp(_RC_DOWN,	_MCW_RC); }
void NxSetFPURoundingNear()	{ _controlfp(_RC_NEAR,	_MCW_RC); }
void NxSetFPUExceptions(bool exOn) 
	{ 
	if (exOn)
		{
		_controlfp(
			_EM_INVALID|
			_EM_DENORMAL|
			_EM_ZERODIVIDE|
			_EM_OVERFLOW|
			_EM_UNDERFLOW|
			_EM_INEXACT
			,_MCW_EM);
		}
	else
		{
		_controlfp(0,_MCW_EM);
		}
	}

#elif LINUX
#include <fpu_control.h>

//see http://www.stereopsis.com/FPU.html
#define __FPU_CW_EXCEPTION_MASK__   (0x003f)
#define __FPU_CW_PREC_MASK__        (0x0300)
#define __FPU_CW_ROUND_MASK__       (0x0c00)
#define __FPU_CW_MASK_ALL__         (0x1f3f)

	unsigned cw;
	void NxSetFPUPrecision24()	{ _FPU_GETCW(cw); cw = (cw & ~__FPU_CW_PREC_MASK__) | _FPU_SINGLE; _FPU_SETCW(cw); }
	void NxSetFPUPrecision53()	{ _FPU_GETCW(cw); cw = (cw & ~__FPU_CW_PREC_MASK__) | _FPU_DOUBLE; _FPU_SETCW(cw); }
	void NxSetFPUPrecision64()	{ _FPU_GETCW(cw); cw = (cw & ~__FPU_CW_PREC_MASK__) | _FPU_EXTENDED; _FPU_SETCW(cw); }

	void NxSetFPURoundingChop()	{ _FPU_GETCW(cw); cw = (cw & ~__FPU_CW_ROUND_MASK__) | _FPU_RC_ZERO; _FPU_SETCW(cw); }
	void NxSetFPURoundingUp()	{ _FPU_GETCW(cw); cw = (cw & ~__FPU_CW_ROUND_MASK__) | _FPU_RC_UP; _FPU_SETCW(cw); }
	void NxSetFPURoundingDown()	{ _FPU_GETCW(cw); cw = (cw & ~__FPU_CW_ROUND_MASK__) | _FPU_RC_DOWN; _FPU_SETCW(cw); }
	void NxSetFPURoundingNear()	{ _FPU_GETCW(cw); cw = (cw & ~__FPU_CW_ROUND_MASK__) | _FPU_RC_NEAREST; _FPU_SETCW(cw); }
	void NxSetFPUExceptions(bool exOn) 
		{ 
		
		_FPU_GETCW(cw); 
		if (exOn)
			{
			cw = (cw & ~__FPU_CW_EXCEPTION_MASK__) | 
			                                _FPU_MASK_IM |
							_FPU_MASK_DM |
							_FPU_MASK_ZM |
							_FPU_MASK_OM |
							_FPU_MASK_UM |
							_FPU_MASK_PM;
			_FPU_SETCW(cw);
			}
		else
			{
			cw = cw & ~__FPU_CW_EXCEPTION_MASK__;
			_FPU_SETCW(cw); 
			}
		}	


#endif

#define INT32	NxI32
#define UINT32	NxU32

int NxIntChop(const NxF32& f)
{ 
	INT32 a			= *reinterpret_cast<const INT32*>(&f);			// take bit pattern of float into a register
	INT32 sign		= (a>>31);										// sign = 0xFFFFFFFF if original value is negative, 0 if positive
	INT32 mantissa	= (a&((1<<23)-1))|(1<<23);						// extract mantissa and add the hidden bit
	INT32 exponent	= ((a&0x7fffffff)>>23)-127;						// extract the exponent
	INT32 r			= ((UINT32)(mantissa)<<8)>>(31-exponent);		// ((1<<exponent)*mantissa)>>24 -- (we know that mantissa > (1<<24))
	return ((r ^ (sign)) - sign ) &~ (exponent>>31);				// add original sign. If exponent was negative, make return value 0.
}

int NxIntFloor(const NxF32& f)
{ 
	INT32 a			= *reinterpret_cast<const INT32*>(&f);									// take bit pattern of float into a register
	INT32 sign		= (a>>31);																// sign = 0xFFFFFFFF if original value is negative, 0 if positive
	a&=0x7fffffff;																			// we don't need the sign any more
	INT32 exponent	= (a>>23)-127;															// extract the exponent
	INT32 expsign   = ~(exponent>>31);														// 0xFFFFFFFF if exponent is positive, 0 otherwise
	INT32 imask		= ( (1<<(31-(exponent))))-1;											// mask for true integer values
	INT32 mantissa	= (a&((1<<23)-1));														// extract mantissa (without the hidden bit)
	INT32 r			= ((UINT32)(mantissa|(1<<23))<<8)>>(31-exponent);						// ((1<<exponent)*(mantissa|hidden bit))>>24 -- (we know that mantissa > (1<<24))
	r = ((r & expsign) ^ (sign)) + ((!((mantissa<<8)&imask)&(expsign^((a-1)>>31)))&sign);	// if (fabs(value)<1.0) value = 0; copy sign; if (value < 0 && value==(int)(value)) value++; 
	return r;
}

int NxIntCeil(const NxF32& f)
{ 
	INT32 a			= *reinterpret_cast<const INT32*>(&f) ^ 0x80000000;						// take bit pattern of float into a register
	INT32 sign		= (a>>31);																// sign = 0xFFFFFFFF if original value is negative, 0 if positive
	a&=0x7fffffff;																			// we don't need the sign any more
	INT32 exponent	= (a>>23)-127;															// extract the exponent
	INT32 expsign   = ~(exponent>>31);														// 0xFFFFFFFF if exponent is positive, 0 otherwise
	INT32 imask		= ( (1<<(31-(exponent))))-1;											// mask for true integer values
	INT32 mantissa	= (a&((1<<23)-1));														// extract mantissa (without the hidden bit)
	INT32 r			= ((UINT32)(mantissa|(1<<23))<<8)>>(31-exponent);						// ((1<<exponent)*(mantissa|hidden bit))>>24 -- (we know that mantissa > (1<<24))
	r = ((r & expsign) ^ (sign)) + ((!((mantissa<<8)&imask)&(expsign^((a-1)>>31)))&sign);	// if (fabs(value)<1.0) value = 0; copy sign; if (value < 0 && value==(int)(value)) value++; 
	return -r;
}

