// PMapCompute.cpp: the rows under PenetrationMap's compute arm (phys_fn_002047), transcribed from the
// pinned x86 listing in the style of the other naked rows (the compare script is
// docs/reconstruction/novodex-physics/evidence/phase4-pmap-compute-listing-compare.py):
//
//   phys_fn_002019 (0x0004e540)  a ray from a point, the number of surface hits as a parity
//   phys_fn_002021 (0x0004e5d0)  whether a point is inside the mesh: one axis ray, one random ray or
//                                the vote of three, by mode (the compute arm always passes 3)
//   phys_fn_002025 (0x0004e870)  every grid corner of a cell touching a surface seed gets its nearest face
//   phys_fn_005324..005353       the nearest-face collider they query: a point against an OPCODE tree of
//                                any of the four kinds, closest triangle and squared distance, with its
//                                constructor, destructor and function-local statics
//
// The register conventions are the oracle's: 2019 takes the direction in eax and the origin in edi, 2021
// the mode in eax and the point in ecx, 2025 and the collider's methods are thiscall.

#include "NxPhysicsSDK.h"
#include "PMap.h"
#include "PMapCompute.h"

#include <stdlib.h>

extern "C" void nxPMapCallInvertPR();
extern "C" void nxPMapCallAtexit();
extern "C" void nxPMapCallDelete();
extern "C" void nxPMapCallRayCtor();
extern "C" void nxPMapCallRayDtor();
extern "C" void nxPMapCallRayCollide();
extern "C" void nxPMapCallRand();
#pragma comment(linker, "/alternatename:_nxPMapCallInvertPR=?InvertPRMatrix@IceMaths@@YAXAAVMatrix4x4@1@ABV21@@Z")
#pragma comment(linker, "/alternatename:_nxPMapCallAtexit=_atexit")
#pragma comment(linker, "/alternatename:_nxPMapCallDelete=??3@YAXPAX@Z")
#pragma comment(linker, "/alternatename:_nxPMapCallRayCtor=??0RayCollider@Opcode@@QAE@XZ")
#pragma comment(linker, "/alternatename:_nxPMapCallRayDtor=??1RayCollider@Opcode@@UAE@XZ")
#pragma comment(linker, "/alternatename:_nxPMapCallRayCollide=?Collide@RayCollider@Opcode@@QAE_NABVRay@IceMaths@@ABVModel@2@PBVMatrix4x4@4@PAI@Z")
#pragma comment(linker, "/alternatename:_nxPMapCallRand=_rand")

// The constants and function-local statics the listing reads, by their bit patterns.
static const unsigned nxPMapOne = 0x3f800000u;
static const unsigned nxPMapZero = 0u;
static const unsigned nxPMapFltMax = 0x7f7fffffu;
static const unsigned nxPMapTwo32 = 0x4f800000u;
static const unsigned nxPMapInv32768 = 0x38000100u;
static const unsigned nxPMapHalf = 0x3f000000u;
static unsigned nxPMapNearestIdx0 = 0, nxPMapNearestIdx1 = 0, nxPMapNearestIdx2 = 0, nxPMapNearestGuard = 0;

static __declspec(naked) void nxPMapNullsub() { __asm ret }


void nxPMapNearestTriangle();
void nxPMapNearestNodeBox();
void nxPMapNearestInit();
void nxPMapNearestFini();
void nxPMapNearestDelete();
void nxPMapNearestLeaf();
void nxPMapNearestWalkA();
void nxPMapNearestWalkB();
void nxPMapNearestWalkC();
void nxPMapNearestWalkD();
void nxPMapNearestEntryA();
void nxPMapNearestEntryB();
void nxPMapNearestEntryC();
void nxPMapNearestEntryD();
void nxPMapNearest();
void nxPMapRayParity_row();
void nxPMapClassify();
void nxPMapMarkCorners_row();


// The collider's vtable (declared before the rows that store its address): its one slot is the scalar deleting destructor.
void* nxPMapNearestVtable[1] = { (void*) &nxPMapNearestDelete };

// phys_fn_005324 (0x000e7c50, 1314 B), 372 instructions
__declspec(naked) void nxPMapNearestTriangle()
	{
	__asm
		{
		sub esp, 0x2c		// 0x000e7c50
		mov eax, dword ptr [esp + 0x34]		// 0x000e7c53
		fld dword ptr [edx]		// 0x000e7c57
		fsub dword ptr [eax]		// 0x000e7c59
		push esi		// 0x000e7c5b
		fld dword ptr [edx + 4]		// 0x000e7c5c
		mov esi, dword ptr [esp + 0x34]		// 0x000e7c5f
		fsub dword ptr [eax + 4]		// 0x000e7c63
		fld dword ptr [edx + 8]		// 0x000e7c66
		fsub dword ptr [eax + 8]		// 0x000e7c69
		fld dword ptr [ecx]		// 0x000e7c6c
		fsub dword ptr [eax]		// 0x000e7c6e
		fstp dword ptr [esp + 0x18]		// 0x000e7c70
		fld dword ptr [ecx + 4]		// 0x000e7c74
		fsub dword ptr [eax + 4]		// 0x000e7c77
		fstp dword ptr [esp + 0x1c]		// 0x000e7c7a
		fld dword ptr [ecx + 8]		// 0x000e7c7e
		fsub dword ptr [eax + 8]		// 0x000e7c81
		fstp dword ptr [esp + 0x20]		// 0x000e7c84
		fld dword ptr [eax]		// 0x000e7c88
		fsub dword ptr [esi]		// 0x000e7c8a
		fstp dword ptr [esp + 0x24]		// 0x000e7c8c
		fld dword ptr [eax + 4]		// 0x000e7c90
		fsub dword ptr [esi + 4]		// 0x000e7c93
		fstp dword ptr [esp + 0x28]		// 0x000e7c96
		fld dword ptr [eax + 8]		// 0x000e7c9a
		fsub dword ptr [esi + 8]		// 0x000e7c9d
		fstp dword ptr [esp + 0x2c]		// 0x000e7ca0
		fld st(2)		// 0x000e7ca4
		fmul st(0), st(3)		// 0x000e7ca6
		fld st(1)		// 0x000e7ca8
		fmul st(0), st(2)		// 0x000e7caa
		faddp st(1), st(0)		// 0x000e7cac
		fld st(2)		// 0x000e7cae
		fmul st(0), st(3)		// 0x000e7cb0
		faddp st(1), st(0)		// 0x000e7cb2
		fstp dword ptr [esp + 0x10]		// 0x000e7cb4
		fld dword ptr [esp + 0x18]		// 0x000e7cb8
		fmul st(0), st(3)		// 0x000e7cbc
		fld dword ptr [esp + 0x20]		// 0x000e7cbe
		fmul st(0), st(2)		// 0x000e7cc2
		faddp st(1), st(0)		// 0x000e7cc4
		fld dword ptr [esp + 0x1c]		// 0x000e7cc6
		fmul st(0), st(3)		// 0x000e7cca
		faddp st(1), st(0)		// 0x000e7ccc
		fstp dword ptr [esp + 0x14]		// 0x000e7cce
		fld dword ptr [esp + 0x20]		// 0x000e7cd2
		fmul dword ptr [esp + 0x20]		// 0x000e7cd6
		fld dword ptr [esp + 0x1c]		// 0x000e7cda
		fmul dword ptr [esp + 0x1c]		// 0x000e7cde
		faddp st(1), st(0)		// 0x000e7ce2
		fld dword ptr [esp + 0x18]		// 0x000e7ce4
		fmul dword ptr [esp + 0x18]		// 0x000e7ce8
		faddp st(1), st(0)		// 0x000e7cec
		fstp dword ptr [esp + 0xc]		// 0x000e7cee
		fld dword ptr [esp + 0x24]		// 0x000e7cf2
		fmul st(0), st(3)		// 0x000e7cf6
		fld dword ptr [esp + 0x2c]		// 0x000e7cf8
		fmul st(0), st(2)		// 0x000e7cfc
		faddp st(1), st(0)		// 0x000e7cfe
		fld dword ptr [esp + 0x28]		// 0x000e7d00
		fmul st(0), st(3)		// 0x000e7d04
		faddp st(1), st(0)		// 0x000e7d06
		fstp dword ptr [esp + 8]		// 0x000e7d08
		fstp st(0)		// 0x000e7d0c
		fstp st(0)		// 0x000e7d0e
		fstp st(0)		// 0x000e7d10
		fld dword ptr [esp + 0x2c]		// 0x000e7d12
		fmul dword ptr [esp + 0x20]		// 0x000e7d16
		fld dword ptr [esp + 0x28]		// 0x000e7d1a
		fmul dword ptr [esp + 0x1c]		// 0x000e7d1e
		faddp st(1), st(0)		// 0x000e7d22
		fld dword ptr [esp + 0x24]		// 0x000e7d24
		fmul dword ptr [esp + 0x18]		// 0x000e7d28
		faddp st(1), st(0)		// 0x000e7d2c
		fstp dword ptr [esp + 4]		// 0x000e7d2e
		fld dword ptr [esp + 0x2c]		// 0x000e7d32
		fmul dword ptr [esp + 0x2c]		// 0x000e7d36
		fld dword ptr [esp + 0x28]		// 0x000e7d3a
		fmul dword ptr [esp + 0x28]		// 0x000e7d3e
		faddp st(1), st(0)		// 0x000e7d42
		fld dword ptr [esp + 0x24]		// 0x000e7d44
		fmul dword ptr [esp + 0x24]		// 0x000e7d48
		pop esi		// 0x000e7d4c
		faddp st(1), st(0)		// 0x000e7d4d
		fld dword ptr [esp + 8]		// 0x000e7d4f
		fmul dword ptr [esp + 0xc]		// 0x000e7d53
		fld dword ptr [esp + 0x10]		// 0x000e7d57
		fmul dword ptr [esp + 0x10]		// 0x000e7d5b
		fsubp st(1), st(0)		// 0x000e7d5f
		fabs		// 0x000e7d61
		fstp dword ptr [esp + 0x30]		// 0x000e7d63
		fld dword ptr [esp]		// 0x000e7d67
		fmul dword ptr [esp + 0x10]		// 0x000e7d6a
		fld dword ptr [esp + 4]		// 0x000e7d6e
		fmul dword ptr [esp + 8]		// 0x000e7d72
		fsubp st(1), st(0)		// 0x000e7d76
		fstp dword ptr [esp + 0x34]		// 0x000e7d78
		fld dword ptr [esp + 4]		// 0x000e7d7c
		fmul dword ptr [esp + 0x10]		// 0x000e7d80
		fld dword ptr [esp]		// 0x000e7d84
		fmul dword ptr [esp + 0xc]		// 0x000e7d87
		fsubp st(1), st(0)		// 0x000e7d8b
		fld dword ptr [esp + 0x34]		// 0x000e7d8d
		fadd st(0), st(1)		// 0x000e7d91
		fcomp dword ptr [esp + 0x30]		// 0x000e7d93
		fld dword ptr [esp + 0x34]		// 0x000e7d97
		fnstsw ax		// 0x000e7d9b
		fcomp dword ptr [nxPMapZero]		// 0x000e7d9d
		test ah, 0x41		// 0x000e7da3
		fnstsw ax		// 0x000e7da6
		jp Le7eec		// 0x000e7da8
		test ah, 5		// 0x000e7dae
		jp Le7e5c		// 0x000e7db1
		fcomp dword ptr [nxPMapZero]		// 0x000e7db7
		fnstsw ax		// 0x000e7dbd
		test ah, 5		// 0x000e7dbf
		jp Le7e0b		// 0x000e7dc2
		fld dword ptr [esp + 4]		// 0x000e7dc4
		fcomp dword ptr [nxPMapZero]		// 0x000e7dc8
		fnstsw ax		// 0x000e7dce
		test ah, 5		// 0x000e7dd0
		jp Le7e0b		// 0x000e7dd3
		mov dword ptr [esp + 0x30], 0		// 0x000e7dd5
Le7ddd:
		fld dword ptr [esp + 4]		// 0x000e7ddd
		fchs		// 0x000e7de1
		fcomp dword ptr [esp + 0xc]		// 0x000e7de3
		fld dword ptr [esp + 4]		// 0x000e7de7
		fnstsw ax		// 0x000e7deb
		test ah, 1		// 0x000e7ded
		jne Le8097		// 0x000e7df0
		fadd st(0), st(0)		// 0x000e7df6
		mov dword ptr [esp + 0x34], 0x3f800000		// 0x000e7df8
		faddp st(1), st(0)		// 0x000e7e00
		fadd dword ptr [esp + 0xc]		// 0x000e7e02
		jmp Le8158		// 0x000e7e06
Le7e0b:
		fld dword ptr [esp]		// 0x000e7e0b
		mov dword ptr [esp + 0x34], 0		// 0x000e7e0e
		fcomp dword ptr [nxPMapZero]		// 0x000e7e16
		fnstsw ax		// 0x000e7e1c
		test ah, 1		// 0x000e7e1e
		jne Le7e30		// 0x000e7e21
		mov dword ptr [esp + 0x30], 0		// 0x000e7e23
		jmp Le8158		// 0x000e7e2b
Le7e30:
		fld dword ptr [esp]		// 0x000e7e30
		fchs		// 0x000e7e33
		fcomp dword ptr [esp + 8]		// 0x000e7e35
		fld dword ptr [esp]		// 0x000e7e39
		fnstsw ax		// 0x000e7e3c
		test ah, 1		// 0x000e7e3e
		jne Le7fa8		// 0x000e7e41
		fadd st(0), st(0)		// 0x000e7e47
		mov dword ptr [esp + 0x30], 0x3f800000		// 0x000e7e49
		faddp st(1), st(0)		// 0x000e7e51
		fadd dword ptr [esp + 8]		// 0x000e7e53
		jmp Le8158		// 0x000e7e57
Le7e5c:
		fcom dword ptr [nxPMapZero]		// 0x000e7e5c
		fnstsw ax		// 0x000e7e62
		test ah, 5		// 0x000e7e64
		jp Le7e95		// 0x000e7e67
		fstp st(0)		// 0x000e7e69
		mov dword ptr [esp + 0x30], 0		// 0x000e7e6b
		fld dword ptr [esp + 4]		// 0x000e7e73
		fcomp dword ptr [nxPMapZero]		// 0x000e7e77
		fnstsw ax		// 0x000e7e7d
		test ah, 1		// 0x000e7e7f
		jne Le7ddd		// 0x000e7e82
		mov dword ptr [esp + 0x34], 0		// 0x000e7e88
		jmp Le8158		// 0x000e7e90
Le7e95:
		fld dword ptr [nxPMapZero]		// 0x000e7e95
		fld dword ptr [esp + 0x30]		// 0x000e7e9b
		fucompp		// 0x000e7e9f
		fnstsw ax		// 0x000e7ea1
		test ah, 0x44		// 0x000e7ea3
		jp Le7ec7		// 0x000e7ea6
		fstp st(0)		// 0x000e7ea8
		mov dword ptr [esp + 0x34], 0		// 0x000e7eaa
		fstp st(0)		// 0x000e7eb2
		mov dword ptr [esp + 0x30], 0		// 0x000e7eb4
		fld dword ptr [nxPMapFltMax]		// 0x000e7ebc
		jmp Le8158		// 0x000e7ec2
Le7ec7:
		fld dword ptr [nxPMapOne]		// 0x000e7ec7
		fdiv dword ptr [esp + 0x30]		// 0x000e7ecd
		fld dword ptr [esp + 0x34]		// 0x000e7ed1
		fmul st(0), st(1)		// 0x000e7ed5
		fstp dword ptr [esp + 0x34]		// 0x000e7ed7
		fmul st(0), st(1)		// 0x000e7edb
		fstp dword ptr [esp + 0x30]		// 0x000e7edd
		fstp st(0)		// 0x000e7ee1
		fld dword ptr [esp + 0x30]		// 0x000e7ee3
		jmp Le811d		// 0x000e7ee7
Le7eec:
		test ah, 5		// 0x000e7eec
		jp Le7fba		// 0x000e7eef
		fstp st(0)		// 0x000e7ef5
		fld dword ptr [esp + 4]		// 0x000e7ef7
		fadd dword ptr [esp + 0x10]		// 0x000e7efb
		fstp dword ptr [esp + 0x30]		// 0x000e7eff
		fld dword ptr [esp]		// 0x000e7f03
		fadd dword ptr [esp + 8]		// 0x000e7f06
		fcom dword ptr [esp + 0x30]		// 0x000e7f0a
		fnstsw ax		// 0x000e7f0e
		test ah, 0x41		// 0x000e7f10
		jne Le7f5e		// 0x000e7f13
		fsub dword ptr [esp + 0x30]		// 0x000e7f15
		fld dword ptr [esp + 0x10]		// 0x000e7f19
		fadd st(0), st(0)		// 0x000e7f1d
		fsubr dword ptr [esp + 0xc]		// 0x000e7f1f
		fadd dword ptr [esp + 8]		// 0x000e7f23
		fld st(1)		// 0x000e7f27
		fcomp st(1)		// 0x000e7f29
		fnstsw ax		// 0x000e7f2b
		test ah, 1		// 0x000e7f2d
		jne Le7f57		// 0x000e7f30
		fstp st(0)		// 0x000e7f32
Le7f34:
		fstp st(0)		// 0x000e7f34
		mov dword ptr [esp + 0x34], 0x3f800000		// 0x000e7f36
		fld dword ptr [esp + 4]		// 0x000e7f3e
		mov dword ptr [esp + 0x30], 0		// 0x000e7f42
		fadd st(0), st(0)		// 0x000e7f4a
		faddp st(1), st(0)		// 0x000e7f4c
		fadd dword ptr [esp + 0xc]		// 0x000e7f4e
		jmp Le8158		// 0x000e7f52
Le7f57:
		fxch st(1)		// 0x000e7f57
		jmp Le8107		// 0x000e7f59
Le7f5e:
		fcomp dword ptr [nxPMapZero]		// 0x000e7f5e
		mov dword ptr [esp + 0x34], 0		// 0x000e7f64
		fld dword ptr [esp]		// 0x000e7f6c
		fnstsw ax		// 0x000e7f6f
		test ah, 0x41		// 0x000e7f71
		jp Le7f8b		// 0x000e7f74
		fadd st(0), st(0)		// 0x000e7f76
		mov dword ptr [esp + 0x30], 0x3f800000		// 0x000e7f78
		faddp st(1), st(0)		// 0x000e7f80
		fadd dword ptr [esp + 8]		// 0x000e7f82
		jmp Le8158		// 0x000e7f86
Le7f8b:
		fcomp dword ptr [nxPMapZero]		// 0x000e7f8b
		fnstsw ax		// 0x000e7f91
		test ah, 1		// 0x000e7f93
		jne Le7fa5		// 0x000e7f96
		mov dword ptr [esp + 0x30], 0		// 0x000e7f98
		jmp Le8158		// 0x000e7fa0
Le7fa5:
		fld dword ptr [esp]		// 0x000e7fa5
Le7fa8:
		fdiv dword ptr [esp + 8]		// 0x000e7fa8
		fchs		// 0x000e7fac
		fst dword ptr [esp + 0x30]		// 0x000e7fae
		fmul dword ptr [esp]		// 0x000e7fb2
		jmp Le8156		// 0x000e7fb5
Le7fba:
		fcomp dword ptr [nxPMapZero]		// 0x000e7fba
		fld dword ptr [esp]		// 0x000e7fc0
		fnstsw ax		// 0x000e7fc3
		test ah, 5		// 0x000e7fc5
		jp Le80aa		// 0x000e7fc8
		fadd dword ptr [esp + 0x10]		// 0x000e7fce
		fstp dword ptr [esp + 0x30]		// 0x000e7fd2
		fld dword ptr [esp + 4]		// 0x000e7fd6
		fadd dword ptr [esp + 0xc]		// 0x000e7fda
		fcom dword ptr [esp + 0x30]		// 0x000e7fde
		fnstsw ax		// 0x000e7fe2
		test ah, 0x41		// 0x000e7fe4
		jne Le804b		// 0x000e7fe7
		fsub dword ptr [esp + 0x30]		// 0x000e7fe9
		fld dword ptr [esp + 0x10]		// 0x000e7fed
		fadd st(0), st(0)		// 0x000e7ff1
		fsubr dword ptr [esp + 0xc]		// 0x000e7ff3
		fadd dword ptr [esp + 8]		// 0x000e7ff7
		fld st(1)		// 0x000e7ffb
		fcomp st(1)		// 0x000e7ffd
		fnstsw ax		// 0x000e7fff
		test ah, 1		// 0x000e8001
		jne Le802a		// 0x000e8004
		fstp st(0)		// 0x000e8006
		mov dword ptr [esp + 0x30], 0x3f800000		// 0x000e8008
		fstp st(0)		// 0x000e8010
		mov dword ptr [esp + 0x34], 0		// 0x000e8012
		fld dword ptr [esp]		// 0x000e801a
		fadd st(0), st(0)		// 0x000e801d
		faddp st(1), st(0)		// 0x000e801f
		fadd dword ptr [esp + 8]		// 0x000e8021
		jmp Le8158		// 0x000e8025
Le802a:
		fxch st(1)		// 0x000e802a
		fdiv st(0), st(1)		// 0x000e802c
		fstp dword ptr [esp + 0x30]		// 0x000e802e
		fstp st(0)		// 0x000e8032
		fld dword ptr [nxPMapOne]		// 0x000e8034
		fsub dword ptr [esp + 0x30]		// 0x000e803a
		fstp dword ptr [esp + 0x34]		// 0x000e803e
		fld dword ptr [esp + 0x30]		// 0x000e8042
		jmp Le811d		// 0x000e8046
Le804b:
		fcomp dword ptr [nxPMapZero]		// 0x000e804b
		mov dword ptr [esp + 0x30], 0		// 0x000e8051
		fld dword ptr [esp + 4]		// 0x000e8059
		fnstsw ax		// 0x000e805d
		test ah, 0x41		// 0x000e805f
		jp Le8079		// 0x000e8062
		fadd st(0), st(0)		// 0x000e8064
		mov dword ptr [esp + 0x34], 0x3f800000		// 0x000e8066
		faddp st(1), st(0)		// 0x000e806e
		fadd dword ptr [esp + 0xc]		// 0x000e8070
		jmp Le8158		// 0x000e8074
Le8079:
		fcomp dword ptr [nxPMapZero]		// 0x000e8079
		fnstsw ax		// 0x000e807f
		test ah, 1		// 0x000e8081
		jne Le8093		// 0x000e8084
		mov dword ptr [esp + 0x34], 0		// 0x000e8086
		jmp Le8158		// 0x000e808e
Le8093:
		fld dword ptr [esp + 4]		// 0x000e8093
Le8097:
		fdiv dword ptr [esp + 0xc]		// 0x000e8097
		fchs		// 0x000e809b
		fst dword ptr [esp + 0x34]		// 0x000e809d
		fmul dword ptr [esp + 4]		// 0x000e80a1
		jmp Le8156		// 0x000e80a5
Le80aa:
		fadd dword ptr [esp + 8]		// 0x000e80aa
		fsub dword ptr [esp + 0x10]		// 0x000e80ae
		fsub dword ptr [esp + 4]		// 0x000e80b2
		fst dword ptr [esp + 0x30]		// 0x000e80b6
		fcomp dword ptr [nxPMapZero]		// 0x000e80ba
		fnstsw ax		// 0x000e80c0
		test ah, 0x41		// 0x000e80c2
		jp Le80e4		// 0x000e80c5
		fld dword ptr [esp]		// 0x000e80c7
		mov dword ptr [esp + 0x34], 0		// 0x000e80ca
		fadd st(0), st(0)		// 0x000e80d2
		mov dword ptr [esp + 0x30], 0x3f800000		// 0x000e80d4
		faddp st(1), st(0)		// 0x000e80dc
		fadd dword ptr [esp + 8]		// 0x000e80de
		jmp Le8158		// 0x000e80e2
Le80e4:
		fld dword ptr [esp + 0x10]		// 0x000e80e4
		fadd st(0), st(0)		// 0x000e80e8
		fsubr dword ptr [esp + 0xc]		// 0x000e80ea
		fadd dword ptr [esp + 8]		// 0x000e80ee
		fld dword ptr [esp + 0x30]		// 0x000e80f2
		fcomp st(1)		// 0x000e80f6
		fnstsw ax		// 0x000e80f8
		test ah, 1		// 0x000e80fa
		je Le7f34		// 0x000e80fd
		fld dword ptr [esp + 0x30]		// 0x000e8103
Le8107:
		fdiv st(0), st(1)		// 0x000e8107
		fstp dword ptr [esp + 0x34]		// 0x000e8109
		fstp st(0)		// 0x000e810d
		fld dword ptr [nxPMapOne]		// 0x000e810f
		fsub dword ptr [esp + 0x34]		// 0x000e8115
		fst dword ptr [esp + 0x30]		// 0x000e8119
Le811d:
		fmul dword ptr [esp + 8]		// 0x000e811d
		fld dword ptr [esp + 0x34]		// 0x000e8121
		fmul dword ptr [esp + 0x10]		// 0x000e8125
		faddp st(1), st(0)		// 0x000e8129
		fld dword ptr [esp]		// 0x000e812b
		fadd st(0), st(0)		// 0x000e812e
		faddp st(1), st(0)		// 0x000e8130
		fmul dword ptr [esp + 0x30]		// 0x000e8132
		fld dword ptr [esp + 0x30]		// 0x000e8136
		fmul dword ptr [esp + 0x10]		// 0x000e813a
		fld dword ptr [esp + 0x34]		// 0x000e813e
		fmul dword ptr [esp + 0xc]		// 0x000e8142
		faddp st(1), st(0)		// 0x000e8146
		fld dword ptr [esp + 4]		// 0x000e8148
		fadd st(0), st(0)		// 0x000e814c
		faddp st(1), st(0)		// 0x000e814e
		fmul dword ptr [esp + 0x34]		// 0x000e8150
		faddp st(1), st(0)		// 0x000e8154
Le8156:
		faddp st(1), st(0)		// 0x000e8156
Le8158:
		test edi, edi		// 0x000e8158
		je Le8162		// 0x000e815a
		mov eax, dword ptr [esp + 0x34]		// 0x000e815c
		mov dword ptr [edi], eax		// 0x000e8160
Le8162:
		test ebx, ebx		// 0x000e8162
		je Le816c		// 0x000e8164
		mov ecx, dword ptr [esp + 0x30]		// 0x000e8166
		mov dword ptr [ebx], ecx		// 0x000e816a
Le816c:
		fabs		// 0x000e816c
		add esp, 0x2c		// 0x000e816e
		ret		// 0x000e8171
		}
	}

// phys_fn_005326 (0x000e8180, 282 B), 103 instructions
__declspec(naked) void nxPMapNearestNodeBox()
	{
	__asm
		{
		sub esp, 0x10		// 0x000e8180
		fld dword ptr [edx]		// 0x000e8183
		fsub dword ptr [eax]		// 0x000e8185
		fstp dword ptr [esp + 4]		// 0x000e8187
		fld dword ptr [edx + 4]		// 0x000e818b
		fsub dword ptr [eax + 4]		// 0x000e818e
		fstp dword ptr [esp + 8]		// 0x000e8191
		fld dword ptr [edx + 8]		// 0x000e8195
		fsub dword ptr [eax + 8]		// 0x000e8198
		fstp dword ptr [esp + 0xc]		// 0x000e819b
		fld dword ptr [nxPMapZero]		// 0x000e819f
		fld dword ptr [ecx]		// 0x000e81a5
		fchs		// 0x000e81a7
		fstp dword ptr [esp]		// 0x000e81a9
		fld dword ptr [esp + 4]		// 0x000e81ac
		fcomp dword ptr [esp]		// 0x000e81b0
		fnstsw ax		// 0x000e81b3
		test ah, 5		// 0x000e81b5
		jp Le81cb		// 0x000e81b8
		mov eax, dword ptr [esp]		// 0x000e81ba
		fstp st(0)		// 0x000e81bd
		fld dword ptr [esp + 4]		// 0x000e81bf
		mov dword ptr [esp + 4], eax		// 0x000e81c3
		fadd dword ptr [ecx]		// 0x000e81c7
		jmp Le81e6		// 0x000e81c9
Le81cb:
		fld dword ptr [esp + 4]		// 0x000e81cb
		fcomp dword ptr [ecx]		// 0x000e81cf
		fnstsw ax		// 0x000e81d1
		test ah, 0x41		// 0x000e81d3
		jne Le81ea		// 0x000e81d6
		mov edx, dword ptr [ecx]		// 0x000e81d8
		fstp st(0)		// 0x000e81da
		fld dword ptr [esp + 4]		// 0x000e81dc
		mov dword ptr [esp + 4], edx		// 0x000e81e0
		fsub dword ptr [ecx]		// 0x000e81e4
Le81e6:
		fld st(0)		// 0x000e81e6
		fmulp st(1), st(0)		// 0x000e81e8
Le81ea:
		fld dword ptr [ecx + 4]		// 0x000e81ea
		fchs		// 0x000e81ed
		fcomp dword ptr [esp + 8]		// 0x000e81ef
		fld dword ptr [esp + 8]		// 0x000e81f3
		fnstsw ax		// 0x000e81f7
		test ah, 0x41		// 0x000e81f9
		jne Le8214		// 0x000e81fc
		fadd dword ptr [ecx + 4]		// 0x000e81fe
		fld st(0)		// 0x000e8201
		fmul st(0), st(1)		// 0x000e8203
		faddp st(2), st(0)		// 0x000e8205
		fstp st(0)		// 0x000e8207
		fld dword ptr [ecx + 4]		// 0x000e8209
		fchs		// 0x000e820c
		fstp dword ptr [esp + 8]		// 0x000e820e
		jmp Le8234		// 0x000e8212
Le8214:
		fcomp dword ptr [ecx + 4]		// 0x000e8214
		fnstsw ax		// 0x000e8217
		test ah, 0x41		// 0x000e8219
		jne Le8234		// 0x000e821c
		fld dword ptr [esp + 8]		// 0x000e821e
		mov eax, dword ptr [ecx + 4]		// 0x000e8222
		fsub dword ptr [ecx + 4]		// 0x000e8225
		mov dword ptr [esp + 8], eax		// 0x000e8228
		fld st(0)		// 0x000e822c
		fmul st(0), st(1)		// 0x000e822e
		faddp st(2), st(0)		// 0x000e8230
		fstp st(0)		// 0x000e8232
Le8234:
		fld dword ptr [ecx + 8]		// 0x000e8234
		fchs		// 0x000e8237
		fcomp dword ptr [esp + 0xc]		// 0x000e8239
		fld dword ptr [esp + 0xc]		// 0x000e823d
		fnstsw ax		// 0x000e8241
		test ah, 0x41		// 0x000e8243
		jne Le825e		// 0x000e8246
		fadd dword ptr [ecx + 8]		// 0x000e8248
		fld st(0)		// 0x000e824b
		fmul st(0), st(1)		// 0x000e824d
		faddp st(2), st(0)		// 0x000e824f
		fstp st(0)		// 0x000e8251
		fld dword ptr [ecx + 8]		// 0x000e8253
		fchs		// 0x000e8256
		fstp dword ptr [esp + 0xc]		// 0x000e8258
		jmp Le827e		// 0x000e825c
Le825e:
		fcomp dword ptr [ecx + 8]		// 0x000e825e
		fnstsw ax		// 0x000e8261
		test ah, 0x41		// 0x000e8263
		jne Le827e		// 0x000e8266
		fld dword ptr [esp + 0xc]		// 0x000e8268
		fsub dword ptr [ecx + 8]		// 0x000e826c
		mov ecx, dword ptr [ecx + 8]		// 0x000e826f
		mov dword ptr [esp + 0xc], ecx		// 0x000e8272
		fld st(0)		// 0x000e8276
		fmul st(0), st(1)		// 0x000e8278
		faddp st(2), st(0)		// 0x000e827a
		fstp st(0)		// 0x000e827c
Le827e:
		test esi, esi		// 0x000e827e
		je Le8296		// 0x000e8280
		mov edx, dword ptr [esp + 4]		// 0x000e8282
		mov eax, dword ptr [esp + 8]		// 0x000e8286
		mov ecx, dword ptr [esp + 0xc]		// 0x000e828a
		mov dword ptr [esi], edx		// 0x000e828e
		mov dword ptr [esi + 4], eax		// 0x000e8290
		mov dword ptr [esi + 8], ecx		// 0x000e8293
Le8296:
		add esp, 0x10		// 0x000e8296
		ret		// 0x000e8299
		}
	}

// phys_fn_005328 (0x000e82a0, 16 B), 4 instructions
__declspec(naked) void nxPMapNearestInit()
	{
	__asm
		{
		mov eax, ecx		// 0x000e82a0
		mov dword ptr [eax], offset nxPMapNearestVtable		// 0x000e82a2
		mov dword ptr [eax + 0x38], 0xffffffff		// 0x000e82a8
		ret		// 0x000e82af
		}
	}

// phys_fn_005329 (0x000e82b0, 7 B), 2 instructions
__declspec(naked) void nxPMapNearestFini()
	{
	__asm
		{
		mov dword ptr [ecx], offset nxPMapNearestVtable		// 0x000e82b0
		ret		// 0x000e82b6
		}
	}

// phys_fn_005331 (0x000e82c0, 31 B), 11 instructions
__declspec(naked) void nxPMapNearestDelete()
	{
	__asm
		{
		test byte ptr [esp + 4], 1		// 0x000e82c0
		push esi		// 0x000e82c5
		mov esi, ecx		// 0x000e82c6
		mov dword ptr [esi], offset nxPMapNearestVtable		// 0x000e82c8
		je Le82d9		// 0x000e82ce
		push esi		// 0x000e82d0
		call nxPMapCallDelete		// 0x000e82d1
		add esp, 4		// 0x000e82d6
Le82d9:
		mov eax, esi		// 0x000e82d9
		pop esi		// 0x000e82db
		ret 4		// 0x000e82dc
		}
	}

// phys_fn_005333 (0x000e82e0, 472 B), 146 instructions
__declspec(naked) void nxPMapNearestLeaf()
	{
	__asm
		{
		mov eax, dword ptr [esp + 4]		// 0x000e82e0
		mov edx, dword ptr [eax]		// 0x000e82e4
		sub esp, 0x4c		// 0x000e82e6
		push ebp		// 0x000e82e9
		push esi		// 0x000e82ea
		mov esi, ecx		// 0x000e82eb
		lea ebp, [esi + 0x20]		// 0x000e82ed
		mov ecx, ebp		// 0x000e82f0
		mov dword ptr [ecx], edx		// 0x000e82f2
		mov edx, dword ptr [eax + 4]		// 0x000e82f4
		mov dword ptr [ecx + 4], edx		// 0x000e82f7
		mov eax, dword ptr [eax + 8]		// 0x000e82fa
		mov dword ptr [ecx + 8], eax		// 0x000e82fd
		mov eax, dword ptr [esp + 0x5c]		// 0x000e8300
		test eax, eax		// 0x000e8304
		je Le8378		// 0x000e8306
		push eax		// 0x000e8308
		lea ecx, [esp + 0x18]		// 0x000e8309
		push ecx		// 0x000e830d
		call nxPMapCallInvertPR		// 0x000e830e
		fld dword ptr [esp + 0x40]		// 0x000e8313
		fmul dword ptr [ebp + 8]		// 0x000e8317
		add esp, 8		// 0x000e831a
		fld dword ptr [esp + 0x28]		// 0x000e831d
		fmul dword ptr [ebp + 4]		// 0x000e8321
		faddp st(1), st(0)		// 0x000e8324
		fld dword ptr [esp + 0x18]		// 0x000e8326
		fmul dword ptr [ebp]		// 0x000e832a
		faddp st(1), st(0)		// 0x000e832d
		fadd dword ptr [esp + 0x48]		// 0x000e832f
		fld dword ptr [esp + 0x3c]		// 0x000e8333
		fmul dword ptr [ebp + 8]		// 0x000e8337
		fld dword ptr [esp + 0x2c]		// 0x000e833a
		fmul dword ptr [ebp + 4]		// 0x000e833e
		faddp st(1), st(0)		// 0x000e8341
		fld dword ptr [esp + 0x1c]		// 0x000e8343
		fmul dword ptr [ebp]		// 0x000e8347
		faddp st(1), st(0)		// 0x000e834a
		fadd dword ptr [esp + 0x4c]		// 0x000e834c
		fld dword ptr [esp + 0x34]		// 0x000e8350
		fmul dword ptr [ebp + 8]		// 0x000e8354
		fld dword ptr [esp + 0x24]		// 0x000e8357
		fmul dword ptr [ebp + 4]		// 0x000e835b
		faddp st(1), st(0)		// 0x000e835e
		fld dword ptr [esp + 0x14]		// 0x000e8360
		fmul dword ptr [ebp]		// 0x000e8364
		faddp st(1), st(0)		// 0x000e8367
		fadd dword ptr [esp + 0x44]		// 0x000e8369
		fstp dword ptr [ebp]		// 0x000e836d
		fxch st(1)		// 0x000e8370
		fstp dword ptr [ebp + 4]		// 0x000e8372
		fstp dword ptr [ebp + 8]		// 0x000e8375
Le8378:
		mov eax, dword ptr [esp + 0x60]		// 0x000e8378
		test eax, eax		// 0x000e837c
		mov dword ptr [esi + 0x2c], 0x7f7fffff		// 0x000e837e
		je Le84b0		// 0x000e8385
		mov eax, dword ptr [eax]		// 0x000e838b
		cmp eax, -1		// 0x000e838d
		mov dword ptr [esp + 0x58], eax		// 0x000e8390
		je Le84b0		// 0x000e8394
		mov ecx, dword ptr [esi + 4]		// 0x000e839a
		lea edx, [eax + eax*2]		// 0x000e839d
		mov eax, dword ptr [ecx + 8]		// 0x000e83a0
		mov ecx, dword ptr [ecx + 0xc]		// 0x000e83a3
		lea eax, [eax + edx*4]		// 0x000e83a6
		mov edx, dword ptr [eax]		// 0x000e83a9
		push ebx		// 0x000e83ab
		lea edx, [edx + edx*2]		// 0x000e83ac
		push edi		// 0x000e83af
		lea edi, [ecx + edx*4]		// 0x000e83b0
		mov edx, dword ptr [eax + 4]		// 0x000e83b3
		mov eax, dword ptr [eax + 8]		// 0x000e83b6
		lea edx, [edx + edx*2]		// 0x000e83b9
		lea eax, [eax + eax*2]		// 0x000e83bc
		lea ebx, [ecx + edx*4]		// 0x000e83bf
		lea ecx, [ecx + eax*4]		// 0x000e83c2
		mov dword ptr [esp + 0x18], ecx		// 0x000e83c5
		mov cl, byte ptr [nxPMapNearestGuard]		// 0x000e83c9
		mov eax, 1		// 0x000e83cf
		test al, cl		// 0x000e83d4
		jne Le840c		// 0x000e83d6
		mov edx, dword ptr [nxPMapNearestGuard]		// 0x000e83d8
		or edx, eax		// 0x000e83de
		push offset nxPMapNullsub		// 0x000e83e0
		mov dword ptr [nxPMapNearestGuard], edx		// 0x000e83e5
		mov dword ptr [nxPMapNearestIdx0], 0		// 0x000e83eb
		mov dword ptr [nxPMapNearestIdx1], eax		// 0x000e83f5
		mov dword ptr [nxPMapNearestIdx2], 2		// 0x000e83fa
		call nxPMapCallAtexit		// 0x000e8404
		add esp, 4		// 0x000e8409
Le840c:
		mov edx, dword ptr [edi]		// 0x000e840c
		mov eax, dword ptr [edi + 4]		// 0x000e840e
		mov ecx, dword ptr [edi + 8]		// 0x000e8411
		mov dword ptr [esp + 0x1c], edx		// 0x000e8414
		mov edx, dword ptr [ebx]		// 0x000e8418
		mov dword ptr [esp + 0x20], eax		// 0x000e841a
		mov eax, dword ptr [ebx + 4]		// 0x000e841e
		mov dword ptr [esp + 0x28], edx		// 0x000e8421
		mov edx, dword ptr [esp + 0x18]		// 0x000e8425
		mov dword ptr [esp + 0x2c], eax		// 0x000e8429
		mov eax, dword ptr [edx]		// 0x000e842d
		mov dword ptr [esp + 0x24], ecx		// 0x000e842f
		mov ecx, dword ptr [ebx + 8]		// 0x000e8433
		mov dword ptr [esp + 0x34], eax		// 0x000e8436
		mov eax, dword ptr [nxPMapNearestIdx2]		// 0x000e843a
		mov dword ptr [esp + 0x30], ecx		// 0x000e843f
		mov ecx, dword ptr [edx + 4]		// 0x000e8443
		mov edx, dword ptr [edx + 8]		// 0x000e8446
		mov dword ptr [esp + 0x38], ecx		// 0x000e8449
		lea ecx, [eax + eax*2]		// 0x000e844d
		mov eax, dword ptr [nxPMapNearestIdx1]		// 0x000e8450
		mov dword ptr [esp + 0x3c], edx		// 0x000e8455
		lea edx, [eax + eax*2]		// 0x000e8459
		mov eax, dword ptr [nxPMapNearestIdx0]		// 0x000e845c
		lea eax, [eax + eax*2]		// 0x000e8461
		lea eax, [esp + eax*4 + 0x1c]		// 0x000e8464
		push eax		// 0x000e8468
		lea ecx, [esp + ecx*4 + 0x20]		// 0x000e8469
		lea edx, [esp + edx*4 + 0x20]		// 0x000e846d
		push ebp		// 0x000e8471
		lea ebx, [esp + 0x70]		// 0x000e8472
		lea edi, [esp + 0x6c]		// 0x000e8476
		call nxPMapNearestTriangle		// 0x000e847a
		fcom dword ptr [esi + 0x2c]		// 0x000e847f
		add esp, 8		// 0x000e8482
		pop edi		// 0x000e8485
		pop ebx		// 0x000e8486
		fnstsw ax		// 0x000e8487
		test ah, 5		// 0x000e8489
		jp Le84ae		// 0x000e848c
		mov ecx, dword ptr [esp + 0x58]		// 0x000e848e
		fstp dword ptr [esi + 0x2c]		// 0x000e8492
		mov edx, dword ptr [esp + 0x5c]		// 0x000e8495
		mov eax, dword ptr [esp + 0x60]		// 0x000e8499
		mov dword ptr [esi + 0x38], ecx		// 0x000e849d
		mov dword ptr [esi + 0x30], edx		// 0x000e84a0
		mov dword ptr [esi + 0x34], eax		// 0x000e84a3
		pop esi		// 0x000e84a6
		pop ebp		// 0x000e84a7
		add esp, 0x4c		// 0x000e84a8
		ret 0xc		// 0x000e84ab
Le84ae:
		fstp st(0)		// 0x000e84ae
Le84b0:
		pop esi		// 0x000e84b0
		pop ebp		// 0x000e84b1
		add esp, 0x4c		// 0x000e84b2
		ret 0xc		// 0x000e84b5
		}
	}

// phys_fn_005335 (0x000e84c0, 392 B), 128 instructions
__declspec(naked) void nxPMapNearestWalkA()
	{
	__asm
		{
		sub esp, 0x34		// 0x000e84c0
		push ebx		// 0x000e84c3
		push ebp		// 0x000e84c4
		push esi		// 0x000e84c5
		mov ebp, ecx		// 0x000e84c6
		push edi		// 0x000e84c8
		mov edi, dword ptr [esp + 0x48]		// 0x000e84c9
		lea ebx, [ebp + 0x20]		// 0x000e84cd
		lea ecx, [edi + 0xc]		// 0x000e84d0
		xor esi, esi		// 0x000e84d3
		mov eax, edi		// 0x000e84d5
		mov edx, ebx		// 0x000e84d7
		call nxPMapNearestNodeBox		// 0x000e84d9
		fcomp dword ptr [ebp + 0x2c]		// 0x000e84de
		fnstsw ax		// 0x000e84e1
		test ah, 0x41		// 0x000e84e3
		je Le863e		// 0x000e84e6
		lea esp, [esp]		// 0x000e84ec
Le84f0:
		mov eax, dword ptr [edi + 0x18]		// 0x000e84f0
		test al, 1		// 0x000e84f3
		jne Le8527		// 0x000e84f5
		push eax		// 0x000e84f7
		mov ecx, ebp		// 0x000e84f8
		call nxPMapNearestWalkA		// 0x000e84fa
		mov edi, dword ptr [edi + 0x18]		// 0x000e84ff
		add edi, 0x1c		// 0x000e8502
		lea ecx, [edi + 0xc]		// 0x000e8505
		xor esi, esi		// 0x000e8508
		mov eax, edi		// 0x000e850a
		mov edx, ebx		// 0x000e850c
		call nxPMapNearestNodeBox		// 0x000e850e
		fcomp dword ptr [ebp + 0x2c]		// 0x000e8513
		fnstsw ax		// 0x000e8516
		test ah, 0x41		// 0x000e8518
		jne Le84f0		// 0x000e851b
		pop edi		// 0x000e851d
		pop esi		// 0x000e851e
		pop ebp		// 0x000e851f
		pop ebx		// 0x000e8520
		add esp, 0x34		// 0x000e8521
		ret 4		// 0x000e8524
Le8527:
		mov esi, dword ptr [edi + 0x18]		// 0x000e8527
		mov ecx, dword ptr [ebp + 4]		// 0x000e852a
		mov edx, dword ptr [ecx + 8]		// 0x000e852d
		mov ecx, dword ptr [ecx + 0xc]		// 0x000e8530
		shr esi, 1		// 0x000e8533
		lea eax, [esi + esi*2]		// 0x000e8535
		lea eax, [edx + eax*4]		// 0x000e8538
		mov edx, dword ptr [eax]		// 0x000e853b
		lea edx, [edx + edx*2]		// 0x000e853d
		lea edi, [ecx + edx*4]		// 0x000e8540
		mov edx, dword ptr [eax + 4]		// 0x000e8543
		mov eax, dword ptr [eax + 8]		// 0x000e8546
		lea edx, [edx + edx*2]		// 0x000e8549
		lea eax, [eax + eax*2]		// 0x000e854c
		lea ebx, [ecx + edx*4]		// 0x000e854f
		lea ecx, [ecx + eax*4]		// 0x000e8552
		test byte ptr [nxPMapNearestGuard], 1		// 0x000e8555
		mov dword ptr [esp + 0x1c], ecx		// 0x000e855c
		jne Le859b		// 0x000e8560
		mov edx, dword ptr [nxPMapNearestGuard]		// 0x000e8562
		mov eax, 1		// 0x000e8568
		or edx, eax		// 0x000e856d
		push offset nxPMapNullsub		// 0x000e856f
		mov dword ptr [nxPMapNearestGuard], edx		// 0x000e8574
		mov dword ptr [nxPMapNearestIdx0], 0		// 0x000e857a
		mov dword ptr [nxPMapNearestIdx1], eax		// 0x000e8584
		mov dword ptr [nxPMapNearestIdx2], 2		// 0x000e8589
		call nxPMapCallAtexit		// 0x000e8593
		add esp, 4		// 0x000e8598
Le859b:
		mov edx, dword ptr [edi]		// 0x000e859b
		mov eax, dword ptr [edi + 4]		// 0x000e859d
		mov ecx, dword ptr [edi + 8]		// 0x000e85a0
		mov dword ptr [esp + 0x20], edx		// 0x000e85a3
		mov edx, dword ptr [ebx]		// 0x000e85a7
		mov dword ptr [esp + 0x24], eax		// 0x000e85a9
		mov eax, dword ptr [ebx + 4]		// 0x000e85ad
		mov dword ptr [esp + 0x2c], edx		// 0x000e85b0
		mov edx, dword ptr [esp + 0x1c]		// 0x000e85b4
		mov dword ptr [esp + 0x30], eax		// 0x000e85b8
		mov eax, dword ptr [edx]		// 0x000e85bc
		mov dword ptr [esp + 0x28], ecx		// 0x000e85be
		mov ecx, dword ptr [ebx + 8]		// 0x000e85c2
		mov dword ptr [esp + 0x38], eax		// 0x000e85c5
		mov eax, dword ptr [nxPMapNearestIdx2]		// 0x000e85c9
		mov dword ptr [esp + 0x34], ecx		// 0x000e85ce
		mov ecx, dword ptr [edx + 4]		// 0x000e85d2
		mov edx, dword ptr [edx + 8]		// 0x000e85d5
		mov dword ptr [esp + 0x3c], ecx		// 0x000e85d8
		lea ecx, [eax + eax*2]		// 0x000e85dc
		mov eax, dword ptr [nxPMapNearestIdx1]		// 0x000e85df
		mov dword ptr [esp + 0x40], edx		// 0x000e85e4
		lea edx, [eax + eax*2]		// 0x000e85e8
		mov eax, dword ptr [nxPMapNearestIdx0]		// 0x000e85eb
		lea eax, [eax + eax*2]		// 0x000e85f0
		lea eax, [esp + eax*4 + 0x20]		// 0x000e85f3
		push eax		// 0x000e85f7
		lea eax, [ebp + 0x20]		// 0x000e85f8
		lea ecx, [esp + ecx*4 + 0x24]		// 0x000e85fb
		lea edx, [esp + edx*4 + 0x24]		// 0x000e85ff
		push eax		// 0x000e8603
		lea ebx, [esp + 0x18]		// 0x000e8604
		lea edi, [esp + 0x50]		// 0x000e8608
		call nxPMapNearestTriangle		// 0x000e860c
		fcom dword ptr [ebp + 0x2c]		// 0x000e8611
		add esp, 8		// 0x000e8614
		fnstsw ax		// 0x000e8617
		test ah, 5		// 0x000e8619
		jp Le863c		// 0x000e861c
		mov ecx, dword ptr [esp + 0x48]		// 0x000e861e
		fstp dword ptr [ebp + 0x2c]		// 0x000e8622
		mov edx, dword ptr [esp + 0x10]		// 0x000e8625
		pop edi		// 0x000e8629
		mov dword ptr [ebp + 0x38], esi		// 0x000e862a
		pop esi		// 0x000e862d
		mov dword ptr [ebp + 0x30], ecx		// 0x000e862e
		mov dword ptr [ebp + 0x34], edx		// 0x000e8631
		pop ebp		// 0x000e8634
		pop ebx		// 0x000e8635
		add esp, 0x34		// 0x000e8636
		ret 4		// 0x000e8639
Le863c:
		fstp st(0)		// 0x000e863c
Le863e:
		pop edi		// 0x000e863e
		pop esi		// 0x000e863f
		pop ebp		// 0x000e8640
		pop ebx		// 0x000e8641
		add esp, 0x34		// 0x000e8642
		ret 4		// 0x000e8645
		}
	}

// phys_fn_005337, phys_fn_005339 (0x000e8650, 684 B), 207 instructions
__declspec(naked) void nxPMapNearestWalkB()
	{
	__asm
		{
		mov eax, dword ptr [esp + 4]		// 0x000e8650
		sub esp, 0x38		// 0x000e8654
		push ebp		// 0x000e8657
		mov ebp, ecx		// 0x000e8658
		push esi		// 0x000e865a
		lea edx, [ebp + 0x20]		// 0x000e865b
		lea ecx, [eax + 0xc]		// 0x000e865e
		xor esi, esi		// 0x000e8661
		call nxPMapNearestNodeBox		// 0x000e8663
		fcomp dword ptr [ebp + 0x2c]		// 0x000e8668
		fnstsw ax		// 0x000e866b
		test ah, 0x41		// 0x000e866d
		je Le88f4		// 0x000e8670
		push ebx		// 0x000e8676
		push edi		// 0x000e8677
		jmp Le8680		// 0x000e8678
		lea ebx, [ebx]		// 0x000e867a
Le8680:
		mov eax, dword ptr [esp + 0x4c]		// 0x000e8680
		mov eax, dword ptr [eax + 0x18]		// 0x000e8684
		test al, 1		// 0x000e8687
		je Le879b		// 0x000e8689
		mov ecx, dword ptr [ebp + 4]		// 0x000e868f
		shr eax, 1		// 0x000e8692
		mov esi, eax		// 0x000e8694
		mov eax, dword ptr [ecx + 8]		// 0x000e8696
		mov ecx, dword ptr [ecx + 0xc]		// 0x000e8699
		lea edx, [esi + esi*2]		// 0x000e869c
		lea eax, [eax + edx*4]		// 0x000e869f
		mov edx, dword ptr [eax]		// 0x000e86a2
		lea edx, [edx + edx*2]		// 0x000e86a4
		lea edi, [ecx + edx*4]		// 0x000e86a7
		mov edx, dword ptr [eax + 4]		// 0x000e86aa
		mov eax, dword ptr [eax + 8]		// 0x000e86ad
		lea edx, [edx + edx*2]		// 0x000e86b0
		lea eax, [eax + eax*2]		// 0x000e86b3
		lea ebx, [ecx + edx*4]		// 0x000e86b6
		lea ecx, [ecx + eax*4]		// 0x000e86b9
		test byte ptr [nxPMapNearestGuard], 1		// 0x000e86bc
		mov dword ptr [esp + 0x20], ecx		// 0x000e86c3
		jne Le8702		// 0x000e86c7
		mov edx, dword ptr [nxPMapNearestGuard]		// 0x000e86c9
		mov eax, 1		// 0x000e86cf
		or edx, eax		// 0x000e86d4
		push offset nxPMapNullsub		// 0x000e86d6
		mov dword ptr [nxPMapNearestGuard], edx		// 0x000e86db
		mov dword ptr [nxPMapNearestIdx0], 0		// 0x000e86e1
		mov dword ptr [nxPMapNearestIdx1], eax		// 0x000e86eb
		mov dword ptr [nxPMapNearestIdx2], 2		// 0x000e86f0
		call nxPMapCallAtexit		// 0x000e86fa
		add esp, 4		// 0x000e86ff
Le8702:
		mov edx, dword ptr [edi]		// 0x000e8702
		mov eax, dword ptr [edi + 4]		// 0x000e8704
		mov ecx, dword ptr [edi + 8]		// 0x000e8707
		mov dword ptr [esp + 0x24], edx		// 0x000e870a
		mov edx, dword ptr [ebx]		// 0x000e870e
		mov dword ptr [esp + 0x28], eax		// 0x000e8710
		mov eax, dword ptr [ebx + 4]		// 0x000e8714
		mov dword ptr [esp + 0x30], edx		// 0x000e8717
		mov edx, dword ptr [esp + 0x20]		// 0x000e871b
		mov dword ptr [esp + 0x34], eax		// 0x000e871f
		mov eax, dword ptr [edx]		// 0x000e8723
		mov dword ptr [esp + 0x2c], ecx		// 0x000e8725
		mov ecx, dword ptr [ebx + 8]		// 0x000e8729
		mov dword ptr [esp + 0x3c], eax		// 0x000e872c
		mov eax, dword ptr [nxPMapNearestIdx2]		// 0x000e8730
		mov dword ptr [esp + 0x38], ecx		// 0x000e8735
		mov ecx, dword ptr [edx + 4]		// 0x000e8739
		mov edx, dword ptr [edx + 8]		// 0x000e873c
		mov dword ptr [esp + 0x40], ecx		// 0x000e873f
		lea ecx, [eax + eax*2]		// 0x000e8743
		mov eax, dword ptr [nxPMapNearestIdx1]		// 0x000e8746
		mov dword ptr [esp + 0x44], edx		// 0x000e874b
		lea edx, [eax + eax*2]		// 0x000e874f
		mov eax, dword ptr [nxPMapNearestIdx0]		// 0x000e8752
		lea eax, [eax + eax*2]		// 0x000e8757
		lea eax, [esp + eax*4 + 0x24]		// 0x000e875a
		push eax		// 0x000e875e
		lea eax, [ebp + 0x20]		// 0x000e875f
		lea ecx, [esp + ecx*4 + 0x28]		// 0x000e8762
		lea edx, [esp + edx*4 + 0x28]		// 0x000e8766
		push eax		// 0x000e876a
		lea ebx, [esp + 0x1c]		// 0x000e876b
		lea edi, [esp + 0x18]		// 0x000e876f
		call nxPMapNearestTriangle		// 0x000e8773
		fcom dword ptr [ebp + 0x2c]		// 0x000e8778
		add esp, 8		// 0x000e877b
		fnstsw ax		// 0x000e877e
		test ah, 5		// 0x000e8780
		jp Le87a5		// 0x000e8783
		mov ecx, dword ptr [esp + 0x10]		// 0x000e8785
		fstp dword ptr [ebp + 0x2c]		// 0x000e8789
		mov edx, dword ptr [esp + 0x14]		// 0x000e878c
		mov dword ptr [ebp + 0x38], esi		// 0x000e8790
		mov dword ptr [ebp + 0x30], ecx		// 0x000e8793
		mov dword ptr [ebp + 0x34], edx		// 0x000e8796
		jmp Le87a7		// 0x000e8799
Le879b:
		push eax		// 0x000e879b
		mov ecx, ebp		// 0x000e879c
		call nxPMapNearestWalkB		// 0x000e879e
		jmp Le87a7		// 0x000e87a3
Le87a5:
		fstp st(0)		// 0x000e87a5
Le87a7:
		mov ecx, dword ptr [esp + 0x4c]		// 0x000e87a7
		mov eax, dword ptr [ecx + 0x1c]		// 0x000e87ab
		test al, 1		// 0x000e87ae
		jne Le87db		// 0x000e87b0
		lea ecx, [eax + 0xc]		// 0x000e87b2
		xor esi, esi		// 0x000e87b5
		lea edx, [ebp + 0x20]		// 0x000e87b7
		mov dword ptr [esp + 0x4c], eax		// 0x000e87ba
		call nxPMapNearestNodeBox		// 0x000e87be
		fcomp dword ptr [ebp + 0x2c]		// 0x000e87c3
		fnstsw ax		// 0x000e87c6
		test ah, 0x41		// 0x000e87c8
		jne Le8680		// 0x000e87cb
		pop edi		// 0x000e87d1
		pop ebx		// 0x000e87d2
		pop esi		// 0x000e87d3
		pop ebp		// 0x000e87d4
		add esp, 0x38		// 0x000e87d5
		ret 4		// 0x000e87d8
Le87db:
		mov esi, dword ptr [ecx + 0x1c]		// 0x000e87db
		mov ecx, dword ptr [ebp + 4]		// 0x000e87de
		mov edx, dword ptr [ecx + 8]		// 0x000e87e1
		mov ecx, dword ptr [ecx + 0xc]		// 0x000e87e4
		shr esi, 1		// 0x000e87e7
		lea eax, [esi + esi*2]		// 0x000e87e9
		lea eax, [edx + eax*4]		// 0x000e87ec
		mov edx, dword ptr [eax]		// 0x000e87ef
		lea edx, [edx + edx*2]		// 0x000e87f1
		lea edi, [ecx + edx*4]		// 0x000e87f4
		mov edx, dword ptr [eax + 4]		// 0x000e87f7
		mov eax, dword ptr [eax + 8]		// 0x000e87fa
		lea edx, [edx + edx*2]		// 0x000e87fd
		lea eax, [eax + eax*2]		// 0x000e8800
		lea ebx, [ecx + edx*4]		// 0x000e8803
		lea ecx, [ecx + eax*4]		// 0x000e8806
		test byte ptr [nxPMapNearestGuard], 1		// 0x000e8809
		mov dword ptr [esp + 0x20], ecx		// 0x000e8810
		jne Le884f		// 0x000e8814
		mov edx, dword ptr [nxPMapNearestGuard]		// 0x000e8816
		mov eax, 1		// 0x000e881c
		or edx, eax		// 0x000e8821
		push offset nxPMapNullsub		// 0x000e8823
		mov dword ptr [nxPMapNearestGuard], edx		// 0x000e8828
		mov dword ptr [nxPMapNearestIdx0], 0		// 0x000e882e
		mov dword ptr [nxPMapNearestIdx1], eax		// 0x000e8838
		mov dword ptr [nxPMapNearestIdx2], 2		// 0x000e883d
		call nxPMapCallAtexit		// 0x000e8847
		add esp, 4		// 0x000e884c
Le884f:
		mov edx, dword ptr [edi]		// 0x000e884f
		mov eax, dword ptr [edi + 4]		// 0x000e8851
		mov ecx, dword ptr [edi + 8]		// 0x000e8854
		mov dword ptr [esp + 0x24], edx		// 0x000e8857
		mov edx, dword ptr [ebx]		// 0x000e885b
		mov dword ptr [esp + 0x28], eax		// 0x000e885d
		mov eax, dword ptr [ebx + 4]		// 0x000e8861
		mov dword ptr [esp + 0x30], edx		// 0x000e8864
		mov edx, dword ptr [esp + 0x20]		// 0x000e8868
		mov dword ptr [esp + 0x34], eax		// 0x000e886c
		mov eax, dword ptr [edx]		// 0x000e8870
		mov dword ptr [esp + 0x2c], ecx		// 0x000e8872
		mov ecx, dword ptr [ebx + 8]		// 0x000e8876
		mov dword ptr [esp + 0x3c], eax		// 0x000e8879
		mov eax, dword ptr [nxPMapNearestIdx2]		// 0x000e887d
		mov dword ptr [esp + 0x38], ecx		// 0x000e8882
		mov ecx, dword ptr [edx + 4]		// 0x000e8886
		mov edx, dword ptr [edx + 8]		// 0x000e8889
		mov dword ptr [esp + 0x40], ecx		// 0x000e888c
		lea ecx, [eax + eax*2]		// 0x000e8890
		mov eax, dword ptr [nxPMapNearestIdx1]		// 0x000e8893
		mov dword ptr [esp + 0x44], edx		// 0x000e8898
		lea edx, [eax + eax*2]		// 0x000e889c
		mov eax, dword ptr [nxPMapNearestIdx0]		// 0x000e889f
		lea eax, [eax + eax*2]		// 0x000e88a4
		lea eax, [esp + eax*4 + 0x24]		// 0x000e88a7
		push eax		// 0x000e88ab
		lea eax, [ebp + 0x20]		// 0x000e88ac
		lea ecx, [esp + ecx*4 + 0x28]		// 0x000e88af
		lea edx, [esp + edx*4 + 0x28]		// 0x000e88b3
		push eax		// 0x000e88b7
		lea ebx, [esp + 0x1c]		// 0x000e88b8
		lea edi, [esp + 0x54]		// 0x000e88bc
		call nxPMapNearestTriangle		// 0x000e88c0
		fcom dword ptr [ebp + 0x2c]		// 0x000e88c5
		add esp, 8		// 0x000e88c8
		fnstsw ax		// 0x000e88cb
		test ah, 5		// 0x000e88cd
		jp Le88f0		// 0x000e88d0
		mov ecx, dword ptr [esp + 0x4c]		// 0x000e88d2
		fstp dword ptr [ebp + 0x2c]		// 0x000e88d6
		mov edx, dword ptr [esp + 0x14]		// 0x000e88d9
		pop edi		// 0x000e88dd
		pop ebx		// 0x000e88de
		mov dword ptr [ebp + 0x38], esi		// 0x000e88df
		pop esi		// 0x000e88e2
		mov dword ptr [ebp + 0x30], ecx		// 0x000e88e3
		mov dword ptr [ebp + 0x34], edx		// 0x000e88e6
		pop ebp		// 0x000e88e9
		add esp, 0x38		// 0x000e88ea
		ret 4		// 0x000e88ed
Le88f0:
		pop edi		// 0x000e88f0
		fstp st(0)		// 0x000e88f1
		pop ebx		// 0x000e88f3
Le88f4:
		pop esi		// 0x000e88f4
		pop ebp		// 0x000e88f5
		add esp, 0x38		// 0x000e88f6
		ret 4		// 0x000e88f9
		}
	}

// phys_fn_005341 (0x000e8900, 475 B), 143 instructions
__declspec(naked) void nxPMapNearestWalkC()
	{
	__asm
		{
		sub esp, 0x40		// 0x000e8900
		push ebx		// 0x000e8903
		push ebp		// 0x000e8904
		push esi		// 0x000e8905
		mov ebp, ecx		// 0x000e8906
		push edi		// 0x000e8908
		mov edi, dword ptr [esp + 0x54]		// 0x000e8909
		lea ebx, [ebp + 0x20]		// 0x000e890d
Le8910:
		movsx eax, word ptr [edi]		// 0x000e8910
		movsx ecx, word ptr [edi + 2]		// 0x000e8913
		movsx edx, word ptr [edi + 4]		// 0x000e8917
		mov dword ptr [esp + 0x54], eax		// 0x000e891b
		movzx eax, word ptr [edi + 6]		// 0x000e891f
		fild dword ptr [esp + 0x54]		// 0x000e8923
		mov dword ptr [esp + 0x54], ecx		// 0x000e8927
		fmul dword ptr [ebp + 8]		// 0x000e892b
		movzx ecx, word ptr [edi + 8]		// 0x000e892e
		fstp dword ptr [esp + 0x20]		// 0x000e8932
		xor esi, esi		// 0x000e8936
		fild dword ptr [esp + 0x54]		// 0x000e8938
		mov dword ptr [esp + 0x54], edx		// 0x000e893c
		movzx edx, word ptr [edi + 0xa]		// 0x000e8940
		fmul dword ptr [ebp + 0xc]		// 0x000e8944
		fstp dword ptr [esp + 0x24]		// 0x000e8947
		fild dword ptr [esp + 0x54]		// 0x000e894b
		mov dword ptr [esp + 0x54], eax		// 0x000e894f
		lea eax, [esp + 0x20]		// 0x000e8953
		fmul dword ptr [ebp + 0x10]		// 0x000e8957
		fstp dword ptr [esp + 0x28]		// 0x000e895a
		fild dword ptr [esp + 0x54]		// 0x000e895e
		mov dword ptr [esp + 0x54], ecx		// 0x000e8962
		lea ecx, [esp + 0x14]		// 0x000e8966
		fmul dword ptr [ebp + 0x14]		// 0x000e896a
		fstp dword ptr [esp + 0x14]		// 0x000e896d
		fild dword ptr [esp + 0x54]		// 0x000e8971
		mov dword ptr [esp + 0x54], edx		// 0x000e8975
		mov edx, ebx		// 0x000e8979
		fmul dword ptr [ebp + 0x18]		// 0x000e897b
		fstp dword ptr [esp + 0x18]		// 0x000e897e
		fild dword ptr [esp + 0x54]		// 0x000e8982
		fmul dword ptr [ebp + 0x1c]		// 0x000e8986
		fstp dword ptr [esp + 0x1c]		// 0x000e8989
		call nxPMapNearestNodeBox		// 0x000e898d
		fcomp dword ptr [ebp + 0x2c]		// 0x000e8992
		fnstsw ax		// 0x000e8995
		test ah, 0x41		// 0x000e8997
		je Le8ad1		// 0x000e899a
		mov eax, dword ptr [edi + 0xc]		// 0x000e89a0
		test al, 1		// 0x000e89a3
		jne Le89ba		// 0x000e89a5
		push eax		// 0x000e89a7
		mov ecx, ebp		// 0x000e89a8
		call nxPMapNearestWalkC		// 0x000e89aa
		mov edi, dword ptr [edi + 0xc]		// 0x000e89af
		add edi, 0x10		// 0x000e89b2
		jmp Le8910		// 0x000e89b5
Le89ba:
		mov esi, dword ptr [edi + 0xc]		// 0x000e89ba
		mov ecx, dword ptr [ebp + 4]		// 0x000e89bd
		mov edx, dword ptr [ecx + 8]		// 0x000e89c0
		mov ecx, dword ptr [ecx + 0xc]		// 0x000e89c3
		shr esi, 1		// 0x000e89c6
		lea eax, [esi + esi*2]		// 0x000e89c8
		lea eax, [edx + eax*4]		// 0x000e89cb
		mov edx, dword ptr [eax]		// 0x000e89ce
		lea edx, [edx + edx*2]		// 0x000e89d0
		lea edi, [ecx + edx*4]		// 0x000e89d3
		mov edx, dword ptr [eax + 4]		// 0x000e89d6
		mov eax, dword ptr [eax + 8]		// 0x000e89d9
		lea edx, [edx + edx*2]		// 0x000e89dc
		lea eax, [eax + eax*2]		// 0x000e89df
		lea ebx, [ecx + edx*4]		// 0x000e89e2
		lea ecx, [ecx + eax*4]		// 0x000e89e5
		test byte ptr [nxPMapNearestGuard], 1		// 0x000e89e8
		mov dword ptr [esp + 0x28], ecx		// 0x000e89ef
		jne Le8a2e		// 0x000e89f3
		mov edx, dword ptr [nxPMapNearestGuard]		// 0x000e89f5
		mov eax, 1		// 0x000e89fb
		or edx, eax		// 0x000e8a00
		push offset nxPMapNullsub		// 0x000e8a02
		mov dword ptr [nxPMapNearestGuard], edx		// 0x000e8a07
		mov dword ptr [nxPMapNearestIdx0], 0		// 0x000e8a0d
		mov dword ptr [nxPMapNearestIdx1], eax		// 0x000e8a17
		mov dword ptr [nxPMapNearestIdx2], 2		// 0x000e8a1c
		call nxPMapCallAtexit		// 0x000e8a26
		add esp, 4		// 0x000e8a2b
Le8a2e:
		mov edx, dword ptr [edi]		// 0x000e8a2e
		mov eax, dword ptr [edi + 4]		// 0x000e8a30
		mov ecx, dword ptr [edi + 8]		// 0x000e8a33
		mov dword ptr [esp + 0x2c], edx		// 0x000e8a36
		mov edx, dword ptr [ebx]		// 0x000e8a3a
		mov dword ptr [esp + 0x30], eax		// 0x000e8a3c
		mov eax, dword ptr [ebx + 4]		// 0x000e8a40
		mov dword ptr [esp + 0x38], edx		// 0x000e8a43
		mov edx, dword ptr [esp + 0x28]		// 0x000e8a47
		mov dword ptr [esp + 0x3c], eax		// 0x000e8a4b
		mov eax, dword ptr [edx]		// 0x000e8a4f
		mov dword ptr [esp + 0x34], ecx		// 0x000e8a51
		mov ecx, dword ptr [ebx + 8]		// 0x000e8a55
		mov dword ptr [esp + 0x44], eax		// 0x000e8a58
		mov eax, dword ptr [nxPMapNearestIdx2]		// 0x000e8a5c
		mov dword ptr [esp + 0x40], ecx		// 0x000e8a61
		mov ecx, dword ptr [edx + 4]		// 0x000e8a65
		mov edx, dword ptr [edx + 8]		// 0x000e8a68
		mov dword ptr [esp + 0x48], ecx		// 0x000e8a6b
		lea ecx, [eax + eax*2]		// 0x000e8a6f
		mov eax, dword ptr [nxPMapNearestIdx1]		// 0x000e8a72
		mov dword ptr [esp + 0x4c], edx		// 0x000e8a77
		lea edx, [eax + eax*2]		// 0x000e8a7b
		mov eax, dword ptr [nxPMapNearestIdx0]		// 0x000e8a7e
		lea eax, [eax + eax*2]		// 0x000e8a83
		lea eax, [esp + eax*4 + 0x2c]		// 0x000e8a86
		push eax		// 0x000e8a8a
		lea eax, [ebp + 0x20]		// 0x000e8a8b
		lea ecx, [esp + ecx*4 + 0x30]		// 0x000e8a8e
		lea edx, [esp + edx*4 + 0x30]		// 0x000e8a92
		push eax		// 0x000e8a96
		lea ebx, [esp + 0x18]		// 0x000e8a97
		lea edi, [esp + 0x5c]		// 0x000e8a9b
		call nxPMapNearestTriangle		// 0x000e8a9f
		fcom dword ptr [ebp + 0x2c]		// 0x000e8aa4
		add esp, 8		// 0x000e8aa7
		fnstsw ax		// 0x000e8aaa
		test ah, 5		// 0x000e8aac
		jp Le8acf		// 0x000e8aaf
		mov ecx, dword ptr [esp + 0x54]		// 0x000e8ab1
		fstp dword ptr [ebp + 0x2c]		// 0x000e8ab5
		mov edx, dword ptr [esp + 0x10]		// 0x000e8ab8
		pop edi		// 0x000e8abc
		mov dword ptr [ebp + 0x38], esi		// 0x000e8abd
		pop esi		// 0x000e8ac0
		mov dword ptr [ebp + 0x30], ecx		// 0x000e8ac1
		mov dword ptr [ebp + 0x34], edx		// 0x000e8ac4
		pop ebp		// 0x000e8ac7
		pop ebx		// 0x000e8ac8
		add esp, 0x40		// 0x000e8ac9
		ret 4		// 0x000e8acc
Le8acf:
		fstp st(0)		// 0x000e8acf
Le8ad1:
		pop edi		// 0x000e8ad1
		pop esi		// 0x000e8ad2
		pop ebp		// 0x000e8ad3
		pop ebx		// 0x000e8ad4
		add esp, 0x40		// 0x000e8ad5
		ret 4		// 0x000e8ad8
		}
	}

// phys_fn_005343 (0x000e8ae0, 765 B), 223 instructions
__declspec(naked) void nxPMapNearestWalkD()
	{
	__asm
		{
		sub esp, 0x54		// 0x000e8ae0
		push ebx		// 0x000e8ae3
		push ebp		// 0x000e8ae4
		push esi		// 0x000e8ae5
		push edi		// 0x000e8ae6
		mov ebp, ecx		// 0x000e8ae7
		lea esp, [esp]		// 0x000e8ae9
Le8af0:
		mov edi, dword ptr [esp + 0x68]		// 0x000e8af0
		movsx eax, word ptr [edi]		// 0x000e8af4
		movsx ecx, word ptr [edi + 2]		// 0x000e8af7
		movsx edx, word ptr [edi + 4]		// 0x000e8afb
		mov dword ptr [esp + 0x10], eax		// 0x000e8aff
		movzx eax, word ptr [edi + 6]		// 0x000e8b03
		fild dword ptr [esp + 0x10]		// 0x000e8b07
		mov dword ptr [esp + 0x10], ecx		// 0x000e8b0b
		movzx ecx, word ptr [edi + 8]		// 0x000e8b0f
		fmul dword ptr [ebp + 8]		// 0x000e8b13
		xor esi, esi		// 0x000e8b16
		fstp dword ptr [esp + 0x28]		// 0x000e8b18
		fild dword ptr [esp + 0x10]		// 0x000e8b1c
		mov dword ptr [esp + 0x10], edx		// 0x000e8b20
		movzx edx, word ptr [edi + 0xa]		// 0x000e8b24
		fmul dword ptr [ebp + 0xc]		// 0x000e8b28
		fstp dword ptr [esp + 0x2c]		// 0x000e8b2b
		fild dword ptr [esp + 0x10]		// 0x000e8b2f
		mov dword ptr [esp + 0x10], eax		// 0x000e8b33
		lea eax, [esp + 0x28]		// 0x000e8b37
		fmul dword ptr [ebp + 0x10]		// 0x000e8b3b
		fstp dword ptr [esp + 0x30]		// 0x000e8b3e
		fild dword ptr [esp + 0x10]		// 0x000e8b42
		mov dword ptr [esp + 0x10], ecx		// 0x000e8b46
		lea ecx, [esp + 0x1c]		// 0x000e8b4a
		fmul dword ptr [ebp + 0x14]		// 0x000e8b4e
		fstp dword ptr [esp + 0x1c]		// 0x000e8b51
		fild dword ptr [esp + 0x10]		// 0x000e8b55
		mov dword ptr [esp + 0x10], edx		// 0x000e8b59
		lea edx, [ebp + 0x20]		// 0x000e8b5d
		fmul dword ptr [ebp + 0x18]		// 0x000e8b60
		fstp dword ptr [esp + 0x20]		// 0x000e8b63
		fild dword ptr [esp + 0x10]		// 0x000e8b67
		fmul dword ptr [ebp + 0x1c]		// 0x000e8b6b
		fstp dword ptr [esp + 0x24]		// 0x000e8b6e
		call nxPMapNearestNodeBox		// 0x000e8b72
		fcomp dword ptr [ebp + 0x2c]		// 0x000e8b77
		fnstsw ax		// 0x000e8b7a
		test ah, 0x41		// 0x000e8b7c
		je Le8dd3		// 0x000e8b7f
		mov eax, dword ptr [edi + 0xc]		// 0x000e8b85
		test al, 1		// 0x000e8b88
		je Le8ca0		// 0x000e8b8a
		mov ecx, dword ptr [ebp + 4]		// 0x000e8b90
		mov edx, dword ptr [ecx + 8]		// 0x000e8b93
		mov ecx, dword ptr [ecx + 0xc]		// 0x000e8b96
		shr eax, 1		// 0x000e8b99
		mov esi, eax		// 0x000e8b9b
		lea eax, [esi + esi*2]		// 0x000e8b9d
		lea eax, [edx + eax*4]		// 0x000e8ba0
		mov edx, dword ptr [eax]		// 0x000e8ba3
		lea edx, [edx + edx*2]		// 0x000e8ba5
		lea edi, [ecx + edx*4]		// 0x000e8ba8
		mov edx, dword ptr [eax + 4]		// 0x000e8bab
		mov eax, dword ptr [eax + 8]		// 0x000e8bae
		lea edx, [edx + edx*2]		// 0x000e8bb1
		lea eax, [eax + eax*2]		// 0x000e8bb4
		lea ebx, [ecx + edx*4]		// 0x000e8bb7
		lea ecx, [ecx + eax*4]		// 0x000e8bba
		test byte ptr [nxPMapNearestGuard], 1		// 0x000e8bbd
		mov dword ptr [esp + 0x3c], ecx		// 0x000e8bc4
		jne Le8c03		// 0x000e8bc8
		mov edx, dword ptr [nxPMapNearestGuard]		// 0x000e8bca
		mov eax, 1		// 0x000e8bd0
		or edx, eax		// 0x000e8bd5
		push offset nxPMapNullsub		// 0x000e8bd7
		mov dword ptr [nxPMapNearestGuard], edx		// 0x000e8bdc
		mov dword ptr [nxPMapNearestIdx0], 0		// 0x000e8be2
		mov dword ptr [nxPMapNearestIdx1], eax		// 0x000e8bec
		mov dword ptr [nxPMapNearestIdx2], 2		// 0x000e8bf1
		call nxPMapCallAtexit		// 0x000e8bfb
		add esp, 4		// 0x000e8c00
Le8c03:
		mov edx, dword ptr [edi]		// 0x000e8c03
		mov eax, dword ptr [edi + 4]		// 0x000e8c05
		mov ecx, dword ptr [edi + 8]		// 0x000e8c08
		mov dword ptr [esp + 0x40], edx		// 0x000e8c0b
		mov edx, dword ptr [ebx]		// 0x000e8c0f
		mov dword ptr [esp + 0x44], eax		// 0x000e8c11
		mov eax, dword ptr [ebx + 4]		// 0x000e8c15
		mov dword ptr [esp + 0x4c], edx		// 0x000e8c18
		mov edx, dword ptr [esp + 0x3c]		// 0x000e8c1c
		mov dword ptr [esp + 0x50], eax		// 0x000e8c20
		mov eax, dword ptr [edx]		// 0x000e8c24
		mov dword ptr [esp + 0x48], ecx		// 0x000e8c26
		mov ecx, dword ptr [ebx + 8]		// 0x000e8c2a
		mov dword ptr [esp + 0x58], eax		// 0x000e8c2d
		mov eax, dword ptr [nxPMapNearestIdx2]		// 0x000e8c31
		mov dword ptr [esp + 0x54], ecx		// 0x000e8c36
		mov ecx, dword ptr [edx + 4]		// 0x000e8c3a
		mov edx, dword ptr [edx + 8]		// 0x000e8c3d
		mov dword ptr [esp + 0x5c], ecx		// 0x000e8c40
		lea ecx, [eax + eax*2]		// 0x000e8c44
		mov eax, dword ptr [nxPMapNearestIdx1]		// 0x000e8c47
		mov dword ptr [esp + 0x60], edx		// 0x000e8c4c
		lea edx, [eax + eax*2]		// 0x000e8c50
		mov eax, dword ptr [nxPMapNearestIdx0]		// 0x000e8c53
		lea eax, [eax + eax*2]		// 0x000e8c58
		lea eax, [esp + eax*4 + 0x40]		// 0x000e8c5b
		push eax		// 0x000e8c5f
		lea eax, [ebp + 0x20]		// 0x000e8c60
		lea ecx, [esp + ecx*4 + 0x44]		// 0x000e8c63
		lea edx, [esp + edx*4 + 0x44]		// 0x000e8c67
		push eax		// 0x000e8c6b
		lea ebx, [esp + 0x20]		// 0x000e8c6c
		lea edi, [esp + 0x1c]		// 0x000e8c70
		call nxPMapNearestTriangle		// 0x000e8c74
		fcom dword ptr [ebp + 0x2c]		// 0x000e8c79
		mov edi, dword ptr [esp + 0x70]		// 0x000e8c7c
		add esp, 8		// 0x000e8c80
		fnstsw ax		// 0x000e8c83
		test ah, 5		// 0x000e8c85
		jp Le8caa		// 0x000e8c88
		mov ecx, dword ptr [esp + 0x14]		// 0x000e8c8a
		fstp dword ptr [ebp + 0x2c]		// 0x000e8c8e
		mov edx, dword ptr [esp + 0x18]		// 0x000e8c91
		mov dword ptr [ebp + 0x38], esi		// 0x000e8c95
		mov dword ptr [ebp + 0x30], ecx		// 0x000e8c98
		mov dword ptr [ebp + 0x34], edx		// 0x000e8c9b
		jmp Le8cac		// 0x000e8c9e
Le8ca0:
		push eax		// 0x000e8ca0
		mov ecx, ebp		// 0x000e8ca1
		call nxPMapNearestWalkD		// 0x000e8ca3
		jmp Le8cac		// 0x000e8ca8
Le8caa:
		fstp st(0)		// 0x000e8caa
Le8cac:
		mov eax, dword ptr [edi + 0x10]		// 0x000e8cac
		test al, 1		// 0x000e8caf
		jne Le8cbc		// 0x000e8cb1
		mov dword ptr [esp + 0x68], eax		// 0x000e8cb3
		jmp Le8af0		// 0x000e8cb7
Le8cbc:
		mov esi, dword ptr [edi + 0x10]		// 0x000e8cbc
		mov ecx, dword ptr [ebp + 4]		// 0x000e8cbf
		mov edx, dword ptr [ecx + 8]		// 0x000e8cc2
		mov ecx, dword ptr [ecx + 0xc]		// 0x000e8cc5
		shr esi, 1		// 0x000e8cc8
		lea eax, [esi + esi*2]		// 0x000e8cca
		lea eax, [edx + eax*4]		// 0x000e8ccd
		mov edx, dword ptr [eax]		// 0x000e8cd0
		lea edx, [edx + edx*2]		// 0x000e8cd2
		lea edi, [ecx + edx*4]		// 0x000e8cd5
		mov edx, dword ptr [eax + 4]		// 0x000e8cd8
		mov eax, dword ptr [eax + 8]		// 0x000e8cdb
		lea edx, [edx + edx*2]		// 0x000e8cde
		lea eax, [eax + eax*2]		// 0x000e8ce1
		lea ebx, [ecx + edx*4]		// 0x000e8ce4
		lea ecx, [ecx + eax*4]		// 0x000e8ce7
		test byte ptr [nxPMapNearestGuard], 1		// 0x000e8cea
		mov dword ptr [esp + 0x3c], ecx		// 0x000e8cf1
		jne Le8d30		// 0x000e8cf5
		mov edx, dword ptr [nxPMapNearestGuard]		// 0x000e8cf7
		mov eax, 1		// 0x000e8cfd
		or edx, eax		// 0x000e8d02
		push offset nxPMapNullsub		// 0x000e8d04
		mov dword ptr [nxPMapNearestGuard], edx		// 0x000e8d09
		mov dword ptr [nxPMapNearestIdx0], 0		// 0x000e8d0f
		mov dword ptr [nxPMapNearestIdx1], eax		// 0x000e8d19
		mov dword ptr [nxPMapNearestIdx2], 2		// 0x000e8d1e
		call nxPMapCallAtexit		// 0x000e8d28
		add esp, 4		// 0x000e8d2d
Le8d30:
		mov edx, dword ptr [edi]		// 0x000e8d30
		mov eax, dword ptr [edi + 4]		// 0x000e8d32
		mov ecx, dword ptr [edi + 8]		// 0x000e8d35
		mov dword ptr [esp + 0x40], edx		// 0x000e8d38
		mov edx, dword ptr [ebx]		// 0x000e8d3c
		mov dword ptr [esp + 0x44], eax		// 0x000e8d3e
		mov eax, dword ptr [ebx + 4]		// 0x000e8d42
		mov dword ptr [esp + 0x4c], edx		// 0x000e8d45
		mov edx, dword ptr [esp + 0x3c]		// 0x000e8d49
		mov dword ptr [esp + 0x50], eax		// 0x000e8d4d
		mov eax, dword ptr [edx]		// 0x000e8d51
		mov dword ptr [esp + 0x48], ecx		// 0x000e8d53
		mov ecx, dword ptr [ebx + 8]		// 0x000e8d57
		mov dword ptr [esp + 0x58], eax		// 0x000e8d5a
		mov eax, dword ptr [nxPMapNearestIdx2]		// 0x000e8d5e
		mov dword ptr [esp + 0x54], ecx		// 0x000e8d63
		mov ecx, dword ptr [edx + 4]		// 0x000e8d67
		mov edx, dword ptr [edx + 8]		// 0x000e8d6a
		mov dword ptr [esp + 0x5c], ecx		// 0x000e8d6d
		lea ecx, [eax + eax*2]		// 0x000e8d71
		mov eax, dword ptr [nxPMapNearestIdx1]		// 0x000e8d74
		mov dword ptr [esp + 0x60], edx		// 0x000e8d79
		lea edx, [eax + eax*2]		// 0x000e8d7d
		mov eax, dword ptr [nxPMapNearestIdx0]		// 0x000e8d80
		lea eax, [eax + eax*2]		// 0x000e8d85
		lea eax, [esp + eax*4 + 0x40]		// 0x000e8d88
		push eax		// 0x000e8d8c
		lea eax, [ebp + 0x20]		// 0x000e8d8d
		lea ecx, [esp + ecx*4 + 0x44]		// 0x000e8d90
		lea edx, [esp + edx*4 + 0x44]		// 0x000e8d94
		push eax		// 0x000e8d98
		lea ebx, [esp + 0x20]		// 0x000e8d99
		lea edi, [esp + 0x70]		// 0x000e8d9d
		call nxPMapNearestTriangle		// 0x000e8da1
		fcom dword ptr [ebp + 0x2c]		// 0x000e8da6
		add esp, 8		// 0x000e8da9
		fnstsw ax		// 0x000e8dac
		test ah, 5		// 0x000e8dae
		jp Le8dd1		// 0x000e8db1
		mov ecx, dword ptr [esp + 0x68]		// 0x000e8db3
		fstp dword ptr [ebp + 0x2c]		// 0x000e8db7
		mov edx, dword ptr [esp + 0x18]		// 0x000e8dba
		pop edi		// 0x000e8dbe
		mov dword ptr [ebp + 0x38], esi		// 0x000e8dbf
		pop esi		// 0x000e8dc2
		mov dword ptr [ebp + 0x30], ecx		// 0x000e8dc3
		mov dword ptr [ebp + 0x34], edx		// 0x000e8dc6
		pop ebp		// 0x000e8dc9
		pop ebx		// 0x000e8dca
		add esp, 0x54		// 0x000e8dcb
		ret 4		// 0x000e8dce
Le8dd1:
		fstp st(0)		// 0x000e8dd1
Le8dd3:
		pop edi		// 0x000e8dd3
		pop esi		// 0x000e8dd4
		pop ebp		// 0x000e8dd5
		pop ebx		// 0x000e8dd6
		add esp, 0x54		// 0x000e8dd7
		ret 4		// 0x000e8dda
		}
	}

// phys_fn_005345 (0x000e8de0, 54 B), 21 instructions
__declspec(naked) void nxPMapNearestEntryA()
	{
	__asm
		{
		mov eax, dword ptr [esp + 0x10]		// 0x000e8de0
		mov edx, dword ptr [esp + 4]		// 0x000e8de4
		push esi		// 0x000e8de8
		mov esi, ecx		// 0x000e8de9
		mov ecx, dword ptr [esp + 0x10]		// 0x000e8deb
		push eax		// 0x000e8def
		push ecx		// 0x000e8df0
		push edx		// 0x000e8df1
		mov ecx, esi		// 0x000e8df2
		call nxPMapNearestLeaf		// 0x000e8df4
		mov eax, dword ptr [esp + 0xc]		// 0x000e8df9
		mov ecx, dword ptr [eax + 8]		// 0x000e8dfd
		push ecx		// 0x000e8e00
		mov ecx, esi		// 0x000e8e01
		call nxPMapNearestWalkA		// 0x000e8e03
		fld dword ptr [esi + 0x2c]		// 0x000e8e08
		fsqrt		// 0x000e8e0b
		mov al, 1		// 0x000e8e0d
		fstp dword ptr [esi + 0x2c]		// 0x000e8e0f
		pop esi		// 0x000e8e12
		ret 0x10		// 0x000e8e13
		}
	}

// phys_fn_005347 (0x000e8e20, 54 B), 21 instructions
__declspec(naked) void nxPMapNearestEntryB()
	{
	__asm
		{
		mov eax, dword ptr [esp + 0x10]		// 0x000e8e20
		mov edx, dword ptr [esp + 4]		// 0x000e8e24
		push esi		// 0x000e8e28
		mov esi, ecx		// 0x000e8e29
		mov ecx, dword ptr [esp + 0x10]		// 0x000e8e2b
		push eax		// 0x000e8e2f
		push ecx		// 0x000e8e30
		push edx		// 0x000e8e31
		mov ecx, esi		// 0x000e8e32
		call nxPMapNearestLeaf		// 0x000e8e34
		mov eax, dword ptr [esp + 0xc]		// 0x000e8e39
		mov ecx, dword ptr [eax + 8]		// 0x000e8e3d
		push ecx		// 0x000e8e40
		mov ecx, esi		// 0x000e8e41
		call nxPMapNearestWalkB		// 0x000e8e43
		fld dword ptr [esi + 0x2c]		// 0x000e8e48
		fsqrt		// 0x000e8e4b
		mov al, 1		// 0x000e8e4d
		fstp dword ptr [esi + 0x2c]		// 0x000e8e4f
		pop esi		// 0x000e8e52
		ret 0x10		// 0x000e8e53
		}
	}

// phys_fn_005349 (0x000e8e60, 100 B), 39 instructions
__declspec(naked) void nxPMapNearestEntryC()
	{
	__asm
		{
		mov eax, dword ptr [esp + 0x10]		// 0x000e8e60
		mov edx, dword ptr [esp + 4]		// 0x000e8e64
		push esi		// 0x000e8e68
		push edi		// 0x000e8e69
		mov esi, ecx		// 0x000e8e6a
		mov ecx, dword ptr [esp + 0x14]		// 0x000e8e6c
		push eax		// 0x000e8e70
		push ecx		// 0x000e8e71
		push edx		// 0x000e8e72
		mov ecx, esi		// 0x000e8e73
		call nxPMapNearestLeaf		// 0x000e8e75
		mov eax, dword ptr [esp + 0x10]		// 0x000e8e7a
		lea ecx, [eax + 0xc]		// 0x000e8e7e
		mov edi, dword ptr [ecx]		// 0x000e8e81
		lea edx, [esi + 8]		// 0x000e8e83
		mov dword ptr [edx], edi		// 0x000e8e86
		mov edi, dword ptr [ecx + 4]		// 0x000e8e88
		mov dword ptr [edx + 4], edi		// 0x000e8e8b
		mov ecx, dword ptr [ecx + 8]		// 0x000e8e8e
		mov dword ptr [edx + 8], ecx		// 0x000e8e91
		lea edx, [eax + 0x18]		// 0x000e8e94
		mov edi, dword ptr [edx]		// 0x000e8e97
		lea ecx, [esi + 0x14]		// 0x000e8e99
		mov dword ptr [ecx], edi		// 0x000e8e9c
		mov edi, dword ptr [edx + 4]		// 0x000e8e9e
		mov dword ptr [ecx + 4], edi		// 0x000e8ea1
		mov edx, dword ptr [edx + 8]		// 0x000e8ea4
		mov dword ptr [ecx + 8], edx		// 0x000e8ea7
		mov eax, dword ptr [eax + 8]		// 0x000e8eaa
		push eax		// 0x000e8ead
		mov ecx, esi		// 0x000e8eae
		call nxPMapNearestWalkC		// 0x000e8eb0
		fld dword ptr [esi + 0x2c]		// 0x000e8eb5
		fsqrt		// 0x000e8eb8
		pop edi		// 0x000e8eba
		mov al, 1		// 0x000e8ebb
		fstp dword ptr [esi + 0x2c]		// 0x000e8ebd
		pop esi		// 0x000e8ec0
		ret 0x10		// 0x000e8ec1
		}
	}

// phys_fn_005351 (0x000e8ed0, 100 B), 39 instructions
__declspec(naked) void nxPMapNearestEntryD()
	{
	__asm
		{
		mov eax, dword ptr [esp + 0x10]		// 0x000e8ed0
		mov edx, dword ptr [esp + 4]		// 0x000e8ed4
		push esi		// 0x000e8ed8
		push edi		// 0x000e8ed9
		mov esi, ecx		// 0x000e8eda
		mov ecx, dword ptr [esp + 0x14]		// 0x000e8edc
		push eax		// 0x000e8ee0
		push ecx		// 0x000e8ee1
		push edx		// 0x000e8ee2
		mov ecx, esi		// 0x000e8ee3
		call nxPMapNearestLeaf		// 0x000e8ee5
		mov eax, dword ptr [esp + 0x10]		// 0x000e8eea
		lea ecx, [eax + 0xc]		// 0x000e8eee
		mov edi, dword ptr [ecx]		// 0x000e8ef1
		lea edx, [esi + 8]		// 0x000e8ef3
		mov dword ptr [edx], edi		// 0x000e8ef6
		mov edi, dword ptr [ecx + 4]		// 0x000e8ef8
		mov dword ptr [edx + 4], edi		// 0x000e8efb
		mov ecx, dword ptr [ecx + 8]		// 0x000e8efe
		mov dword ptr [edx + 8], ecx		// 0x000e8f01
		lea edx, [eax + 0x18]		// 0x000e8f04
		mov edi, dword ptr [edx]		// 0x000e8f07
		lea ecx, [esi + 0x14]		// 0x000e8f09
		mov dword ptr [ecx], edi		// 0x000e8f0c
		mov edi, dword ptr [edx + 4]		// 0x000e8f0e
		mov dword ptr [ecx + 4], edi		// 0x000e8f11
		mov edx, dword ptr [edx + 8]		// 0x000e8f14
		mov dword ptr [ecx + 8], edx		// 0x000e8f17
		mov eax, dword ptr [eax + 8]		// 0x000e8f1a
		push eax		// 0x000e8f1d
		mov ecx, esi		// 0x000e8f1e
		call nxPMapNearestWalkD		// 0x000e8f20
		fld dword ptr [esi + 0x2c]		// 0x000e8f25
		fsqrt		// 0x000e8f28
		pop edi		// 0x000e8f2a
		mov al, 1		// 0x000e8f2b
		fstp dword ptr [esi + 0x2c]		// 0x000e8f2d
		pop esi		// 0x000e8f30
		ret 0x10		// 0x000e8f31
		}
	}

// phys_fn_005353 (0x000e8f40, 88 B), 30 instructions
__declspec(naked) void nxPMapNearest()
	{
	__asm
		{
		mov eax, dword ptr [esp + 8]		// 0x000e8f40
		test eax, eax		// 0x000e8f44
		jne Le8f4d		// 0x000e8f46
Le8f48:
		xor al, al		// 0x000e8f48
		ret 0x10		// 0x000e8f4a
Le8f4d:
		mov edx, dword ptr [eax + 4]		// 0x000e8f4d
		test edx, edx		// 0x000e8f50
		mov dword ptr [ecx + 4], edx		// 0x000e8f52
		je Le8f48		// 0x000e8f55
		mov edx, dword ptr [eax + 8]		// 0x000e8f57
		mov eax, dword ptr [eax + 0x10]		// 0x000e8f5a
		push ebx		// 0x000e8f5d
		mov ebx, edx		// 0x000e8f5e
		shr ebx, 1		// 0x000e8f60
		not ebx		// 0x000e8f62
		test bl, 1		// 0x000e8f64
		pop ebx		// 0x000e8f67
		jne Le8f81		// 0x000e8f68
		test dl, 1		// 0x000e8f6a
		je Le8f78		// 0x000e8f6d
		mov dword ptr [esp + 8], eax		// 0x000e8f6f
		jmp nxPMapNearestEntryD		// 0x000e8f73
Le8f78:
		mov dword ptr [esp + 8], eax		// 0x000e8f78
		jmp nxPMapNearestEntryB		// 0x000e8f7c
Le8f81:
		test dl, 1		// 0x000e8f81
		je Le8f8f		// 0x000e8f84
		mov dword ptr [esp + 8], eax		// 0x000e8f86
		jmp nxPMapNearestEntryC		// 0x000e8f8a
Le8f8f:
		mov dword ptr [esp + 8], eax		// 0x000e8f8f
		jmp nxPMapNearestEntryA		// 0x000e8f93
		}
	}

// phys_fn_002019 (0x0004e540, 143 B), 38 instructions
__declspec(naked) void nxPMapRayParity_row()
	{
	__asm
		{
		sub esp, 0xa8		// 0x0004e540
		push esi		// 0x0004e546
		lea ecx, [esp + 0x1c]		// 0x0004e547
		mov esi, eax		// 0x0004e54b
		call nxPMapCallRayCtor		// 0x0004e54d
		mov eax, dword ptr [esp + 0x20]		// 0x0004e552
		mov ecx, dword ptr [edi + 4]		// 0x0004e556
		mov edx, dword ptr [edi + 8]		// 0x0004e559
		and eax, 0xfffffffc		// 0x0004e55c
		mov dword ptr [esp + 0x20], eax		// 0x0004e55f
		mov eax, dword ptr [edi]		// 0x0004e563
		mov dword ptr [esp + 4], eax		// 0x0004e565
		mov eax, dword ptr [esi]		// 0x0004e569
		mov dword ptr [esp + 8], ecx		// 0x0004e56b
		mov ecx, dword ptr [esi + 4]		// 0x0004e56f
		push 0		// 0x0004e572
		mov dword ptr [esp + 0x10], edx		// 0x0004e574
		mov edx, dword ptr [esi + 8]		// 0x0004e578
		mov dword ptr [esp + 0x14], eax		// 0x0004e57b
		mov eax, dword ptr [esp + 0xb4]		// 0x0004e57f
		mov dword ptr [esp + 0x18], ecx		// 0x0004e586
		mov ecx, dword ptr [eax + 0x28]		// 0x0004e58a
		push 0		// 0x0004e58d
		mov dword ptr [esp + 0x20], edx		// 0x0004e58f
		push ecx		// 0x0004e593
		lea edx, [esp + 0x10]		// 0x0004e594
		push edx		// 0x0004e598
		lea ecx, [esp + 0x2c]		// 0x0004e599
		mov byte ptr [esp + 0xb9], 0		// 0x0004e59d
		mov dword ptr [esp + 0xb0], 0x7f7fffff		// 0x0004e5a5
		call nxPMapCallRayCollide		// 0x0004e5b0
		mov esi, dword ptr [esp + 0x84]		// 0x0004e5b5
		lea ecx, [esp + 0x1c]		// 0x0004e5bc
		call nxPMapCallRayDtor		// 0x0004e5c0
		mov eax, esi		// 0x0004e5c5
		pop esi		// 0x0004e5c7
		add esp, 0xa8		// 0x0004e5c8
		ret		// 0x0004e5ce
		}
	}

// phys_fn_002021, phys_fn_002023 (0x0004e5d0, 657 B), 188 instructions
__declspec(naked) void nxPMapClassify()
	{
	__asm
		{
		sub esp, 0x34		// 0x0004e5d0
		test eax, eax		// 0x0004e5d3
		push ebx		// 0x0004e5d5
		push esi		// 0x0004e5d6
		push edi		// 0x0004e5d7
		mov edi, ecx		// 0x0004e5d8
		mov ebx, 1		// 0x0004e5da
		jne L4e612		// 0x0004e5df
		mov dword ptr [esp + 0x10], 0x3f800000		// 0x0004e5e1
		mov dword ptr [esp + 0x14], eax		// 0x0004e5e9
L4e5ed:
		mov dword ptr [esp + 0x18], 0		// 0x0004e5ed
L4e5f5:
		mov eax, dword ptr [esp + 0x10]		// 0x0004e5f5
		mov ecx, dword ptr [esp + 0x14]		// 0x0004e5f9
		mov edx, dword ptr [esp + 0x18]		// 0x0004e5fd
		mov dword ptr [esp + 0x1c], eax		// 0x0004e601
		mov dword ptr [esp + 0x20], ecx		// 0x0004e605
		mov dword ptr [esp + 0x24], edx		// 0x0004e609
		jmp L4e818		// 0x0004e60d
L4e612:
		cmp eax, 1		// 0x0004e612
		jne L4e629		// 0x0004e615
		mov dword ptr [esp + 0x10], 0		// 0x0004e617
		mov dword ptr [esp + 0x14], 0x3f800000		// 0x0004e61f
		jmp L4e5ed		// 0x0004e627
L4e629:
		cmp eax, 2		// 0x0004e629
		jne L4e648		// 0x0004e62c
		mov dword ptr [esp + 0x10], 0		// 0x0004e62e
		mov dword ptr [esp + 0x14], 0		// 0x0004e636
		mov dword ptr [esp + 0x18], 0x3f800000		// 0x0004e63e
		jmp L4e5f5		// 0x0004e646
L4e648:
		cmp eax, 3		// 0x0004e648
		jne L4e72c		// 0x0004e64b
		call nxPMapCallRand		// 0x0004e651
		test eax, eax		// 0x0004e656
		mov dword ptr [esp + 0xc], eax		// 0x0004e658
		fild dword ptr [esp + 0xc]		// 0x0004e65c
		jge L4e668		// 0x0004e660
		fadd dword ptr [nxPMapTwo32]		// 0x0004e662
L4e668:
		fmul dword ptr [nxPMapInv32768]		// 0x0004e668
		fstp dword ptr [esp + 0x1c]		// 0x0004e66e
		call nxPMapCallRand		// 0x0004e672
		test eax, eax		// 0x0004e677
		mov dword ptr [esp + 0xc], eax		// 0x0004e679
		fild dword ptr [esp + 0xc]		// 0x0004e67d
		jge L4e689		// 0x0004e681
		fadd dword ptr [nxPMapTwo32]		// 0x0004e683
L4e689:
		fmul dword ptr [nxPMapInv32768]		// 0x0004e689
		fstp dword ptr [esp + 0x20]		// 0x0004e68f
		call nxPMapCallRand		// 0x0004e693
		test eax, eax		// 0x0004e698
		mov dword ptr [esp + 0xc], eax		// 0x0004e69a
		fild dword ptr [esp + 0xc]		// 0x0004e69e
		jge L4e6aa		// 0x0004e6a2
		fadd dword ptr [nxPMapTwo32]		// 0x0004e6a4
L4e6aa:
		fmul dword ptr [nxPMapInv32768]		// 0x0004e6aa
		fld dword ptr [esp + 0x1c]		// 0x0004e6b0
		fsub dword ptr [nxPMapHalf]		// 0x0004e6b4
		fstp dword ptr [esp + 0x1c]		// 0x0004e6ba
		fld dword ptr [esp + 0x20]		// 0x0004e6be
		fsub dword ptr [nxPMapHalf]		// 0x0004e6c2
		fstp dword ptr [esp + 0x20]		// 0x0004e6c8
		fsub dword ptr [nxPMapHalf]		// 0x0004e6cc
		fst dword ptr [esp + 0x24]		// 0x0004e6d2
		fmul dword ptr [esp + 0x24]		// 0x0004e6d6
		fld dword ptr [esp + 0x20]		// 0x0004e6da
		fmul dword ptr [esp + 0x20]		// 0x0004e6de
		faddp st(1), st(0)		// 0x0004e6e2
		fld dword ptr [esp + 0x1c]		// 0x0004e6e4
		fmul dword ptr [esp + 0x1c]		// 0x0004e6e8
		faddp st(1), st(0)		// 0x0004e6ec
		fld dword ptr [nxPMapZero]		// 0x0004e6ee
		fld st(1)		// 0x0004e6f4
		fucompp		// 0x0004e6f6
		fnstsw ax		// 0x0004e6f8
		test ah, 0x44		// 0x0004e6fa
		jnp L4e816		// 0x0004e6fd
		fsqrt		// 0x0004e703
		fdivr dword ptr [nxPMapOne]		// 0x0004e705
		fld dword ptr [esp + 0x1c]		// 0x0004e70b
		fmul st(0), st(1)		// 0x0004e70f
		fstp dword ptr [esp + 0x1c]		// 0x0004e711
		fld dword ptr [esp + 0x20]		// 0x0004e715
		fmul st(0), st(1)		// 0x0004e719
		fstp dword ptr [esp + 0x20]		// 0x0004e71b
		fmul dword ptr [esp + 0x24]		// 0x0004e71f
		fstp dword ptr [esp + 0x24]		// 0x0004e723
		jmp L4e818		// 0x0004e727
L4e72c:
		lea esi, [esp + 0x24]		// 0x0004e72c
		mov ebx, 3		// 0x0004e730
L4e735:
		call nxPMapCallRand		// 0x0004e735
		test eax, eax		// 0x0004e73a
		mov dword ptr [esp + 0xc], eax		// 0x0004e73c
		fild dword ptr [esp + 0xc]		// 0x0004e740
		jge L4e74c		// 0x0004e744
		fadd dword ptr [nxPMapTwo32]		// 0x0004e746
L4e74c:
		fmul dword ptr [nxPMapInv32768]		// 0x0004e74c
		fstp dword ptr [esi - 8]		// 0x0004e752
		call nxPMapCallRand		// 0x0004e755
		test eax, eax		// 0x0004e75a
		mov dword ptr [esp + 0xc], eax		// 0x0004e75c
		fild dword ptr [esp + 0xc]		// 0x0004e760
		jge L4e76c		// 0x0004e764
		fadd dword ptr [nxPMapTwo32]		// 0x0004e766
L4e76c:
		fmul dword ptr [nxPMapInv32768]		// 0x0004e76c
		fstp dword ptr [esi - 4]		// 0x0004e772
		call nxPMapCallRand		// 0x0004e775
		test eax, eax		// 0x0004e77a
		mov dword ptr [esp + 0xc], eax		// 0x0004e77c
		fild dword ptr [esp + 0xc]		// 0x0004e780
		jge L4e78c		// 0x0004e784
		fadd dword ptr [nxPMapTwo32]		// 0x0004e786
L4e78c:
		fmul dword ptr [nxPMapInv32768]		// 0x0004e78c
		fstp dword ptr [esi]		// 0x0004e792
		fld dword ptr [esi - 8]		// 0x0004e794
		fsub dword ptr [nxPMapHalf]		// 0x0004e797
		fstp dword ptr [esi - 8]		// 0x0004e79d
		fld dword ptr [esi - 4]		// 0x0004e7a0
		fsub dword ptr [nxPMapHalf]		// 0x0004e7a3
		fstp dword ptr [esi - 4]		// 0x0004e7a9
		fld dword ptr [esi]		// 0x0004e7ac
		fsub dword ptr [nxPMapHalf]		// 0x0004e7ae
		fstp dword ptr [esi]		// 0x0004e7b4
		fld dword ptr [esi]		// 0x0004e7b6
		fld dword ptr [esi - 4]		// 0x0004e7b8
		fld dword ptr [esi - 8]		// 0x0004e7bb
		fld st(0)		// 0x0004e7be
		fmul st(0), st(1)		// 0x0004e7c0
		fld st(2)		// 0x0004e7c2
		fmul st(0), st(3)		// 0x0004e7c4
		faddp st(1), st(0)		// 0x0004e7c6
		fld st(3)		// 0x0004e7c8
		fmul st(0), st(4)		// 0x0004e7ca
		faddp st(1), st(0)		// 0x0004e7cc
		fstp st(3)		// 0x0004e7ce
		fstp st(0)		// 0x0004e7d0
		fstp st(0)		// 0x0004e7d2
		fld dword ptr [nxPMapZero]		// 0x0004e7d4
		fld st(1)		// 0x0004e7da
		fucompp		// 0x0004e7dc
		fnstsw ax		// 0x0004e7de
		test ah, 0x44		// 0x0004e7e0
		jnp L4e803		// 0x0004e7e3
		fsqrt		// 0x0004e7e5
		fdivr dword ptr [nxPMapOne]		// 0x0004e7e7
		fld st(0)		// 0x0004e7ed
		fmul dword ptr [esi - 8]		// 0x0004e7ef
		fstp dword ptr [esi - 8]		// 0x0004e7f2
		fld st(0)		// 0x0004e7f5
		fmul dword ptr [esi - 4]		// 0x0004e7f7
		fstp dword ptr [esi - 4]		// 0x0004e7fa
		fmul dword ptr [esi]		// 0x0004e7fd
		fstp dword ptr [esi]		// 0x0004e7ff
		jmp L4e805		// 0x0004e801
L4e803:
		fstp st(0)		// 0x0004e803
L4e805:
		add esi, 0xc		// 0x0004e805
		dec ebx		// 0x0004e808
		jne L4e735		// 0x0004e809
		mov ebx, 3		// 0x0004e80f
		jmp L4e818		// 0x0004e814
L4e816:
		fstp st(0)		// 0x0004e816
L4e818:
		test ebx, ebx		// 0x0004e818
		mov dword ptr [esp + 0xc], 0		// 0x0004e81a
		jbe L4e84e		// 0x0004e822
		push ebp		// 0x0004e824
		lea esi, [esp + 0x20]		// 0x0004e825
		mov ebp, ebx		// 0x0004e829
		jmp L4e830		// 0x0004e82b
		lea ecx, [ecx]		// 0x0004e82d
L4e830:
		mov eax, dword ptr [esp + 0x48]		// 0x0004e830
		push eax		// 0x0004e834
		mov eax, esi		// 0x0004e835
		call nxPMapRayParity_row		// 0x0004e837
		add esp, 4		// 0x0004e83c
		test al, 1		// 0x0004e83f
		je L4e847		// 0x0004e841
		inc dword ptr [esp + 0x10]		// 0x0004e843
L4e847:
		add esi, 0xc		// 0x0004e847
		dec ebp		// 0x0004e84a
		jne L4e830		// 0x0004e84b
		pop ebp		// 0x0004e84d
L4e84e:
		mov ecx, dword ptr [esp + 0xc]		// 0x0004e84e
		pop edi		// 0x0004e852
		lea edx, [ecx + ecx]		// 0x0004e853
		cmp edx, ebx		// 0x0004e856
		pop esi		// 0x0004e858
		seta al		// 0x0004e859
		pop ebx		// 0x0004e85c
		add esp, 0x34		// 0x0004e85d
		ret		// 0x0004e860
		}
	}

// phys_fn_002025, phys_fn_002027 (0x0004e870, 2045 B), 593 instructions
__declspec(naked) void nxPMapMarkCorners_row()
	{
	__asm
		{
		sub esp, 0xc8		// 0x0004e870
		push esi		// 0x0004e876
		mov esi, ecx		// 0x0004e877
		lea ecx, [esp + 0x90]		// 0x0004e879
		mov dword ptr [esp + 0x10], 0xffffffff		// 0x0004e880
		call nxPMapNearestInit		// 0x0004e888
		mov eax, dword ptr [esi + 0x5c]		// 0x0004e88d
		xor edx, edx		// 0x0004e890
		cmp eax, edx		// 0x0004e892
		mov dword ptr [esp + 0x2c], edx		// 0x0004e894
		mov dword ptr [esp + 0xc], edx		// 0x0004e898
		jbe L4f053		// 0x0004e89c
		push ebx		// 0x0004e8a2
		push ebp		// 0x0004e8a3
		push edi		// 0x0004e8a4
		jmp L4e8b0		// 0x0004e8a5
		lea esp, [esp]		// 0x0004e8a7
		mov edi, edi		// 0x0004e8ae
L4e8b0:
		mov eax, dword ptr [esi + 0x5c]		// 0x0004e8b0
		test eax, eax		// 0x0004e8b3
		mov dword ptr [esp + 0x14], 0		// 0x0004e8b5
		jbe L4f040		// 0x0004e8bd
L4e8c3:
		mov eax, dword ptr [esi + 0x5c]		// 0x0004e8c3
		xor ebx, ebx		// 0x0004e8c6
		test eax, eax		// 0x0004e8c8
		jbe L4f02c		// 0x0004e8ca
L4e8d0:
		mov ecx, dword ptr [esi + 0x60]		// 0x0004e8d0
		mov eax, dword ptr [esi + 0x5c]		// 0x0004e8d3
		mov ebp, ecx		// 0x0004e8d6
		imul ebp, edx		// 0x0004e8d8
		mov edi, eax		// 0x0004e8db
		imul edi, dword ptr [esp + 0x14]		// 0x0004e8dd
		add ebp, ebx		// 0x0004e8e2
		add edi, ebp		// 0x0004e8e4
		lea ebp, [edi + 1]		// 0x0004e8e6
		mov dword ptr [esp + 0x34], ebp		// 0x0004e8e9
		lea ebp, [eax + edi]		// 0x0004e8ed
		mov dword ptr [esp + 0x30], ebp		// 0x0004e8f0
		inc ebp		// 0x0004e8f4
		mov dword ptr [esp + 0x2c], ebp		// 0x0004e8f5
		lea ebp, [ecx + edi]		// 0x0004e8f9
		lea ecx, [ebp + 1]		// 0x0004e8fc
		mov dword ptr [esp + 0x28], ecx		// 0x0004e8ff
		lea ecx, [eax + ebp]		// 0x0004e903
		mov dword ptr [esp + 0x24], ecx		// 0x0004e906
		inc ecx		// 0x0004e90a
		dec eax		// 0x0004e90b
		cmp ebx, eax		// 0x0004e90c
		mov dword ptr [esp + 0x20], ecx		// 0x0004e90e
		jne L4e927		// 0x0004e912
		or eax, 0xffffffff		// 0x0004e914
		mov dword ptr [esp + 0x20], eax		// 0x0004e917
		mov dword ptr [esp + 0x28], eax		// 0x0004e91b
		mov dword ptr [esp + 0x2c], eax		// 0x0004e91f
		mov dword ptr [esp + 0x34], eax		// 0x0004e923
L4e927:
		mov eax, dword ptr [esi + 0x5c]		// 0x0004e927
		mov ecx, dword ptr [esp + 0x14]		// 0x0004e92a
		dec eax		// 0x0004e92e
		cmp ecx, eax		// 0x0004e92f
		jne L4e948		// 0x0004e931
		or ecx, 0xffffffff		// 0x0004e933
		mov dword ptr [esp + 0x20], ecx		// 0x0004e936
		mov dword ptr [esp + 0x24], ecx		// 0x0004e93a
		mov dword ptr [esp + 0x2c], ecx		// 0x0004e93e
		mov dword ptr [esp + 0x30], ecx		// 0x0004e942
		jmp L4e94b		// 0x0004e946
L4e948:
		or ecx, 0xffffffff		// 0x0004e948
L4e94b:
		cmp edx, eax		// 0x0004e94b
		jne L4e95d		// 0x0004e94d
		mov dword ptr [esp + 0x20], ecx		// 0x0004e94f
		mov dword ptr [esp + 0x24], ecx		// 0x0004e953
		mov dword ptr [esp + 0x28], ecx		// 0x0004e957
		mov ebp, ecx		// 0x0004e95b
L4e95d:
		cmp edi, ecx		// 0x0004e95d
		je L4e975		// 0x0004e95f
		mov eax, dword ptr [esi + 0x70]		// 0x0004e961
		mov ecx, dword ptr [eax + edi*4]		// 0x0004e964
		shr ecx, 0x1f		// 0x0004e967
		not ecx		// 0x0004e96a
		test cl, 1		// 0x0004e96c
		jne L4ea36		// 0x0004e96f
L4e975:
		mov ecx, dword ptr [esp + 0x34]		// 0x0004e975
		cmp ecx, -1		// 0x0004e979
		je L4e992		// 0x0004e97c
		mov eax, dword ptr [esi + 0x70]		// 0x0004e97e
		mov ecx, dword ptr [eax + ecx*4]		// 0x0004e981
		shr ecx, 0x1f		// 0x0004e984
		not ecx		// 0x0004e987
		test cl, 1		// 0x0004e989
		jne L4ea36		// 0x0004e98c
L4e992:
		mov ecx, dword ptr [esp + 0x30]		// 0x0004e992
		cmp ecx, -1		// 0x0004e996
		je L4e9af		// 0x0004e999
		mov eax, dword ptr [esi + 0x70]		// 0x0004e99b
		mov ecx, dword ptr [eax + ecx*4]		// 0x0004e99e
		shr ecx, 0x1f		// 0x0004e9a1
		not ecx		// 0x0004e9a4
		test cl, 1		// 0x0004e9a6
		jne L4ea36		// 0x0004e9a9
L4e9af:
		cmp ebp, -1		// 0x0004e9af
		je L4e9c4		// 0x0004e9b2
		mov eax, dword ptr [esi + 0x70]		// 0x0004e9b4
		mov ecx, dword ptr [eax + ebp*4]		// 0x0004e9b7
		shr ecx, 0x1f		// 0x0004e9ba
		not ecx		// 0x0004e9bd
		test cl, 1		// 0x0004e9bf
		jne L4ea36		// 0x0004e9c2
L4e9c4:
		cmp dword ptr [esp + 0x28], -1		// 0x0004e9c4
		je L4e9df		// 0x0004e9c9
		mov eax, dword ptr [esi + 0x70]		// 0x0004e9cb
		mov ecx, dword ptr [esp + 0x28]		// 0x0004e9ce
		mov ecx, dword ptr [eax + ecx*4]		// 0x0004e9d2
		shr ecx, 0x1f		// 0x0004e9d5
		not ecx		// 0x0004e9d8
		test cl, 1		// 0x0004e9da
		jne L4ea36		// 0x0004e9dd
L4e9df:
		cmp dword ptr [esp + 0x24], -1		// 0x0004e9df
		je L4e9fa		// 0x0004e9e4
		mov eax, dword ptr [esi + 0x70]		// 0x0004e9e6
		mov ecx, dword ptr [esp + 0x24]		// 0x0004e9e9
		mov ecx, dword ptr [eax + ecx*4]		// 0x0004e9ed
		shr ecx, 0x1f		// 0x0004e9f0
		not ecx		// 0x0004e9f3
		test cl, 1		// 0x0004e9f5
		jne L4ea36		// 0x0004e9f8
L4e9fa:
		mov ecx, dword ptr [esp + 0x2c]		// 0x0004e9fa
		cmp ecx, -1		// 0x0004e9fe
		je L4ea13		// 0x0004ea01
		mov eax, dword ptr [esi + 0x70]		// 0x0004ea03
		mov ecx, dword ptr [eax + ecx*4]		// 0x0004ea06
		shr ecx, 0x1f		// 0x0004ea09
		not ecx		// 0x0004ea0c
		test cl, 1		// 0x0004ea0e
		jne L4ea36		// 0x0004ea11
L4ea13:
		cmp dword ptr [esp + 0x20], -1		// 0x0004ea13
		je L4f020		// 0x0004ea18
		mov eax, dword ptr [esi + 0x70]		// 0x0004ea1e
		mov ecx, dword ptr [esp + 0x20]		// 0x0004ea21
		mov ecx, dword ptr [eax + ecx*4]		// 0x0004ea25
		shr ecx, 0x1f		// 0x0004ea28
		not ecx		// 0x0004ea2b
		test cl, 1		// 0x0004ea2d
		je L4f020		// 0x0004ea30
L4ea36:
		mov ecx, dword ptr [esp + 0x38]		// 0x0004ea36
		inc ecx		// 0x0004ea3a
		cmp edi, -1		// 0x0004ea3b
		mov dword ptr [esp + 0x38], ecx		// 0x0004ea3e
		je L4eaee		// 0x0004ea42
		mov eax, dword ptr [eax + edi*4]		// 0x0004ea48
		shr eax, 0x1f		// 0x0004ea4b
		not eax		// 0x0004ea4e
		test al, 1		// 0x0004ea50
		jne L4eaee		// 0x0004ea52
		test ebx, ebx		// 0x0004ea58
		mov dword ptr [esp + 0x10], ebx		// 0x0004ea5a
		fild dword ptr [esp + 0x10]		// 0x0004ea5e
		jge L4ea6a		// 0x0004ea62
		fadd dword ptr [nxPMapTwo32]		// 0x0004ea64
L4ea6a:
		fmul dword ptr [esi + 0x50]		// 0x0004ea6a
		mov ecx, dword ptr [esp + 0x14]		// 0x0004ea6d
		test ecx, ecx		// 0x0004ea71
		fsub dword ptr [esi + 0x2c]		// 0x0004ea73
		fadd dword ptr [esi + 0x20]		// 0x0004ea76
		fild dword ptr [esp + 0x14]		// 0x0004ea79
		jge L4ea85		// 0x0004ea7d
		fadd dword ptr [nxPMapTwo32]		// 0x0004ea7f
L4ea85:
		test edx, edx		// 0x0004ea85
		fmul dword ptr [esi + 0x54]		// 0x0004ea87
		mov dword ptr [esp + 0x10], edx		// 0x0004ea8a
		fsub dword ptr [esi + 0x30]		// 0x0004ea8e
		fadd dword ptr [esi + 0x24]		// 0x0004ea91
		fxch st(1)		// 0x0004ea94
		fstp dword ptr [esp + 0x48]		// 0x0004ea96
		fstp dword ptr [esp + 0x4c]		// 0x0004ea9a
		fild dword ptr [esp + 0x10]		// 0x0004ea9e
		jge L4eaaa		// 0x0004eaa2
		fadd dword ptr [nxPMapTwo32]		// 0x0004eaa4
L4eaaa:
		fmul dword ptr [esi + 0x58]		// 0x0004eaaa
		mov eax, dword ptr [esp + 0xdc]		// 0x0004eaad
		lea edx, [esp + 0x1c]		// 0x0004eab4
		push edx		// 0x0004eab8
		fsub dword ptr [esi + 0x34]		// 0x0004eab9
		push 0		// 0x0004eabc
		push eax		// 0x0004eabe
		fadd dword ptr [esi + 0x28]		// 0x0004eabf
		lea ecx, [esp + 0x54]		// 0x0004eac2
		push ecx		// 0x0004eac6
		lea ecx, [esp + 0xac]		// 0x0004eac7
		fstp dword ptr [esp + 0x60]		// 0x0004eace
		call nxPMapNearest		// 0x0004ead2
		mov edx, dword ptr [esp + 0xd4]		// 0x0004ead7
		mov eax, dword ptr [esi + 0x70]		// 0x0004eade
		or edx, 0x80000000		// 0x0004eae1
		mov dword ptr [eax + edi*4], edx		// 0x0004eae7
		mov edx, dword ptr [esp + 0x18]		// 0x0004eaea
L4eaee:
		cmp ebp, -1		// 0x0004eaee
		je L4eba9		// 0x0004eaf1
		mov ecx, dword ptr [esi + 0x70]		// 0x0004eaf7
		mov eax, dword ptr [ecx + ebp*4]		// 0x0004eafa
		shr eax, 0x1f		// 0x0004eafd
		not eax		// 0x0004eb00
		test al, 1		// 0x0004eb02
		jne L4eba9		// 0x0004eb04
		test ebx, ebx		// 0x0004eb0a
		mov dword ptr [esp + 0x10], ebx		// 0x0004eb0c
		fild dword ptr [esp + 0x10]		// 0x0004eb10
		jge L4eb1c		// 0x0004eb14
		fadd dword ptr [nxPMapTwo32]		// 0x0004eb16
L4eb1c:
		fmul dword ptr [esi + 0x50]		// 0x0004eb1c
		mov ecx, dword ptr [esp + 0x14]		// 0x0004eb1f
		test ecx, ecx		// 0x0004eb23
		fsub dword ptr [esi + 0x2c]		// 0x0004eb25
		fadd dword ptr [esi + 0x20]		// 0x0004eb28
		fild dword ptr [esp + 0x14]		// 0x0004eb2b
		jge L4eb37		// 0x0004eb2f
		fadd dword ptr [nxPMapTwo32]		// 0x0004eb31
L4eb37:
		fmul dword ptr [esi + 0x54]		// 0x0004eb37
		inc edx		// 0x0004eb3a
		test edx, edx		// 0x0004eb3b
		mov dword ptr [esp + 0x10], edx		// 0x0004eb3d
		fsub dword ptr [esi + 0x30]		// 0x0004eb41
		fadd dword ptr [esi + 0x24]		// 0x0004eb44
		fxch st(1)		// 0x0004eb47
		fstp dword ptr [esp + 0x78]		// 0x0004eb49
		fstp dword ptr [esp + 0x7c]		// 0x0004eb4d
		fild dword ptr [esp + 0x10]		// 0x0004eb51
		jge L4eb5d		// 0x0004eb55
		fadd dword ptr [nxPMapTwo32]		// 0x0004eb57
L4eb5d:
		fmul dword ptr [esi + 0x58]		// 0x0004eb5d
		mov edi, dword ptr [esp + 0xdc]		// 0x0004eb60
		lea edx, [esp + 0x1c]		// 0x0004eb67
		push edx		// 0x0004eb6b
		fsub dword ptr [esi + 0x34]		// 0x0004eb6c
		push 0		// 0x0004eb6f
		push edi		// 0x0004eb71
		fadd dword ptr [esi + 0x28]		// 0x0004eb72
		lea eax, [esp + 0x84]		// 0x0004eb75
		push eax		// 0x0004eb7c
		lea ecx, [esp + 0xac]		// 0x0004eb7d
		fstp dword ptr [esp + 0x90]		// 0x0004eb84
		call nxPMapNearest		// 0x0004eb8b
		mov ecx, dword ptr [esp + 0xd4]		// 0x0004eb90
		mov edx, dword ptr [esi + 0x70]		// 0x0004eb97
		or ecx, 0x80000000		// 0x0004eb9a
		mov dword ptr [edx + ebp*4], ecx		// 0x0004eba0
		mov edx, dword ptr [esp + 0x18]		// 0x0004eba3
		jmp L4ebb0		// 0x0004eba7
L4eba9:
		mov edi, dword ptr [esp + 0xdc]		// 0x0004eba9
L4ebb0:
		mov ebp, dword ptr [esp + 0x30]		// 0x0004ebb0
		cmp ebp, -1		// 0x0004ebb4
		je L4ec68		// 0x0004ebb7
		mov eax, dword ptr [esi + 0x70]		// 0x0004ebbd
		mov ecx, dword ptr [eax + ebp*4]		// 0x0004ebc0
		shr ecx, 0x1f		// 0x0004ebc3
		not ecx		// 0x0004ebc6
		test cl, 1		// 0x0004ebc8
		jne L4ec68		// 0x0004ebcb
		test ebx, ebx		// 0x0004ebd1
		mov dword ptr [esp + 0x10], ebx		// 0x0004ebd3
		fild dword ptr [esp + 0x10]		// 0x0004ebd7
		jge L4ebe3		// 0x0004ebdb
		fadd dword ptr [nxPMapTwo32]		// 0x0004ebdd
L4ebe3:
		fmul dword ptr [esi + 0x50]		// 0x0004ebe3
		mov eax, dword ptr [esp + 0x14]		// 0x0004ebe6
		inc eax		// 0x0004ebea
		test eax, eax		// 0x0004ebeb
		fsub dword ptr [esi + 0x2c]		// 0x0004ebed
		mov dword ptr [esp + 0x10], eax		// 0x0004ebf0
		fadd dword ptr [esi + 0x20]		// 0x0004ebf4
		fild dword ptr [esp + 0x10]		// 0x0004ebf7
		jge L4ec03		// 0x0004ebfb
		fadd dword ptr [nxPMapTwo32]		// 0x0004ebfd
L4ec03:
		test edx, edx		// 0x0004ec03
		fmul dword ptr [esi + 0x54]		// 0x0004ec05
		mov dword ptr [esp + 0x10], edx		// 0x0004ec08
		fsub dword ptr [esi + 0x30]		// 0x0004ec0c
		fadd dword ptr [esi + 0x24]		// 0x0004ec0f
		fxch st(1)		// 0x0004ec12
		fstp dword ptr [esp + 0x60]		// 0x0004ec14
		fstp dword ptr [esp + 0x64]		// 0x0004ec18
		fild dword ptr [esp + 0x10]		// 0x0004ec1c
		jge L4ec28		// 0x0004ec20
		fadd dword ptr [nxPMapTwo32]		// 0x0004ec22
L4ec28:
		fmul dword ptr [esi + 0x58]		// 0x0004ec28
		lea ecx, [esp + 0x1c]		// 0x0004ec2b
		push ecx		// 0x0004ec2f
		push 0		// 0x0004ec30
		fsub dword ptr [esi + 0x34]		// 0x0004ec32
		push edi		// 0x0004ec35
		lea edx, [esp + 0x6c]		// 0x0004ec36
		fadd dword ptr [esi + 0x28]		// 0x0004ec3a
		push edx		// 0x0004ec3d
		lea ecx, [esp + 0xac]		// 0x0004ec3e
		fstp dword ptr [esp + 0x78]		// 0x0004ec45
		call nxPMapNearest		// 0x0004ec49
		mov eax, dword ptr [esp + 0xd4]		// 0x0004ec4e
		mov edx, dword ptr [esp + 0x30]		// 0x0004ec55
		mov ecx, dword ptr [esi + 0x70]		// 0x0004ec59
		or eax, 0x80000000		// 0x0004ec5c
		mov dword ptr [ecx + edx*4], eax		// 0x0004ec61
		mov edx, dword ptr [esp + 0x18]		// 0x0004ec64
L4ec68:
		mov ebp, dword ptr [esp + 0x24]		// 0x0004ec68
		cmp ebp, -1		// 0x0004ec6c
		je L4ed2d		// 0x0004ec6f
		mov eax, dword ptr [esi + 0x70]		// 0x0004ec75
		mov ecx, dword ptr [eax + ebp*4]		// 0x0004ec78
		shr ecx, 0x1f		// 0x0004ec7b
		not ecx		// 0x0004ec7e
		test cl, 1		// 0x0004ec80
		jne L4ed2d		// 0x0004ec83
		test ebx, ebx		// 0x0004ec89
		mov dword ptr [esp + 0x10], ebx		// 0x0004ec8b
		fild dword ptr [esp + 0x10]		// 0x0004ec8f
		jge L4ec9b		// 0x0004ec93
		fadd dword ptr [nxPMapTwo32]		// 0x0004ec95
L4ec9b:
		fmul dword ptr [esi + 0x50]		// 0x0004ec9b
		mov eax, dword ptr [esp + 0x14]		// 0x0004ec9e
		inc eax		// 0x0004eca2
		test eax, eax		// 0x0004eca3
		fsub dword ptr [esi + 0x2c]		// 0x0004eca5
		mov dword ptr [esp + 0x10], eax		// 0x0004eca8
		fadd dword ptr [esi + 0x20]		// 0x0004ecac
		fild dword ptr [esp + 0x10]		// 0x0004ecaf
		jge L4ecbb		// 0x0004ecb3
		fadd dword ptr [nxPMapTwo32]		// 0x0004ecb5
L4ecbb:
		fmul dword ptr [esi + 0x54]		// 0x0004ecbb
		inc edx		// 0x0004ecbe
		test edx, edx		// 0x0004ecbf
		mov dword ptr [esp + 0x10], edx		// 0x0004ecc1
		fsub dword ptr [esi + 0x30]		// 0x0004ecc5
		fadd dword ptr [esi + 0x24]		// 0x0004ecc8
		fxch st(1)		// 0x0004eccb
		fstp dword ptr [esp + 0x90]		// 0x0004eccd
		fstp dword ptr [esp + 0x94]		// 0x0004ecd4
		fild dword ptr [esp + 0x10]		// 0x0004ecdb
		jge L4ece7		// 0x0004ecdf
		fadd dword ptr [nxPMapTwo32]		// 0x0004ece1
L4ece7:
		fmul dword ptr [esi + 0x58]		// 0x0004ece7
		lea ecx, [esp + 0x1c]		// 0x0004ecea
		push ecx		// 0x0004ecee
		push 0		// 0x0004ecef
		fsub dword ptr [esi + 0x34]		// 0x0004ecf1
		push edi		// 0x0004ecf4
		lea edx, [esp + 0x9c]		// 0x0004ecf5
		fadd dword ptr [esi + 0x28]		// 0x0004ecfc
		push edx		// 0x0004ecff
		lea ecx, [esp + 0xac]		// 0x0004ed00
		fstp dword ptr [esp + 0xa8]		// 0x0004ed07
		call nxPMapNearest		// 0x0004ed0e
		mov eax, dword ptr [esp + 0xd4]		// 0x0004ed13
		mov edx, dword ptr [esp + 0x24]		// 0x0004ed1a
		mov ecx, dword ptr [esi + 0x70]		// 0x0004ed1e
		or eax, 0x80000000		// 0x0004ed21
		mov dword ptr [ecx + edx*4], eax		// 0x0004ed26
		mov edx, dword ptr [esp + 0x18]		// 0x0004ed29
L4ed2d:
		mov ebp, dword ptr [esp + 0x34]		// 0x0004ed2d
		cmp ebp, -1		// 0x0004ed31
		je L4ede4		// 0x0004ed34
		mov eax, dword ptr [esi + 0x70]		// 0x0004ed3a
		mov ecx, dword ptr [eax + ebp*4]		// 0x0004ed3d
		shr ecx, 0x1f		// 0x0004ed40
		not ecx		// 0x0004ed43
		test cl, 1		// 0x0004ed45
		jne L4ede4		// 0x0004ed48
		lea eax, [ebx + 1]		// 0x0004ed4e
		test eax, eax		// 0x0004ed51
		mov dword ptr [esp + 0x10], eax		// 0x0004ed53
		fild dword ptr [esp + 0x10]		// 0x0004ed57
		jge L4ed63		// 0x0004ed5b
		fadd dword ptr [nxPMapTwo32]		// 0x0004ed5d
L4ed63:
		fmul dword ptr [esi + 0x50]		// 0x0004ed63
		mov ecx, dword ptr [esp + 0x14]		// 0x0004ed66
		test ecx, ecx		// 0x0004ed6a
		fsub dword ptr [esi + 0x2c]		// 0x0004ed6c
		fadd dword ptr [esi + 0x20]		// 0x0004ed6f
		fild dword ptr [esp + 0x14]		// 0x0004ed72
		jge L4ed7e		// 0x0004ed76
		fadd dword ptr [nxPMapTwo32]		// 0x0004ed78
L4ed7e:
		test edx, edx		// 0x0004ed7e
		fmul dword ptr [esi + 0x54]		// 0x0004ed80
		mov dword ptr [esp + 0x10], edx		// 0x0004ed83
		fsub dword ptr [esi + 0x30]		// 0x0004ed87
		fadd dword ptr [esi + 0x24]		// 0x0004ed8a
		fxch st(1)		// 0x0004ed8d
		fstp dword ptr [esp + 0x3c]		// 0x0004ed8f
		fstp dword ptr [esp + 0x40]		// 0x0004ed93
		fild dword ptr [esp + 0x10]		// 0x0004ed97
		jge L4eda3		// 0x0004ed9b
		fadd dword ptr [nxPMapTwo32]		// 0x0004ed9d
L4eda3:
		fmul dword ptr [esi + 0x58]		// 0x0004eda3
		lea edx, [esp + 0x1c]		// 0x0004eda6
		push edx		// 0x0004edaa
		push 0		// 0x0004edab
		fsub dword ptr [esi + 0x34]		// 0x0004edad
		push edi		// 0x0004edb0
		lea eax, [esp + 0x48]		// 0x0004edb1
		fadd dword ptr [esi + 0x28]		// 0x0004edb5
		push eax		// 0x0004edb8
		lea ecx, [esp + 0xac]		// 0x0004edb9
		fstp dword ptr [esp + 0x54]		// 0x0004edc0
		call nxPMapNearest		// 0x0004edc4
		mov ecx, dword ptr [esp + 0xd4]		// 0x0004edc9
		mov edx, dword ptr [esi + 0x70]		// 0x0004edd0
		mov eax, dword ptr [esp + 0x34]		// 0x0004edd3
		or ecx, 0x80000000		// 0x0004edd7
		mov dword ptr [edx + eax*4], ecx		// 0x0004eddd
		mov edx, dword ptr [esp + 0x18]		// 0x0004ede0
L4ede4:
		mov ebp, dword ptr [esp + 0x28]		// 0x0004ede4
		cmp ebp, -1		// 0x0004ede8
		je L4ee9a		// 0x0004edeb
		mov ecx, dword ptr [esi + 0x70]		// 0x0004edf1
		mov eax, dword ptr [ecx + ebp*4]		// 0x0004edf4
		shr eax, 0x1f		// 0x0004edf7
		not eax		// 0x0004edfa
		test al, 1		// 0x0004edfc
		jne L4ee9a		// 0x0004edfe
		lea ecx, [ebx + 1]		// 0x0004ee04
		test ecx, ecx		// 0x0004ee07
		mov dword ptr [esp + 0x10], ecx		// 0x0004ee09
		fild dword ptr [esp + 0x10]		// 0x0004ee0d
		jge L4ee19		// 0x0004ee11
		fadd dword ptr [nxPMapTwo32]		// 0x0004ee13
L4ee19:
		fmul dword ptr [esi + 0x50]		// 0x0004ee19
		mov eax, dword ptr [esp + 0x14]		// 0x0004ee1c
		test eax, eax		// 0x0004ee20
		fsub dword ptr [esi + 0x2c]		// 0x0004ee22
		fadd dword ptr [esi + 0x20]		// 0x0004ee25
		fild dword ptr [esp + 0x14]		// 0x0004ee28
		jge L4ee34		// 0x0004ee2c
		fadd dword ptr [nxPMapTwo32]		// 0x0004ee2e
L4ee34:
		fmul dword ptr [esi + 0x54]		// 0x0004ee34
		inc edx		// 0x0004ee37
		test edx, edx		// 0x0004ee38
		mov dword ptr [esp + 0x10], edx		// 0x0004ee3a
		fsub dword ptr [esi + 0x30]		// 0x0004ee3e
		fadd dword ptr [esi + 0x24]		// 0x0004ee41
		fxch st(1)		// 0x0004ee44
		fstp dword ptr [esp + 0x54]		// 0x0004ee46
		fstp dword ptr [esp + 0x58]		// 0x0004ee4a
		fild dword ptr [esp + 0x10]		// 0x0004ee4e
		jge L4ee5a		// 0x0004ee52
		fadd dword ptr [nxPMapTwo32]		// 0x0004ee54
L4ee5a:
		fmul dword ptr [esi + 0x58]		// 0x0004ee5a
		lea ecx, [esp + 0x1c]		// 0x0004ee5d
		push ecx		// 0x0004ee61
		push 0		// 0x0004ee62
		fsub dword ptr [esi + 0x34]		// 0x0004ee64
		push edi		// 0x0004ee67
		lea edx, [esp + 0x60]		// 0x0004ee68
		fadd dword ptr [esi + 0x28]		// 0x0004ee6c
		push edx		// 0x0004ee6f
		lea ecx, [esp + 0xac]		// 0x0004ee70
		fstp dword ptr [esp + 0x6c]		// 0x0004ee77
		call nxPMapNearest		// 0x0004ee7b
		mov eax, dword ptr [esp + 0xd4]		// 0x0004ee80
		mov edx, dword ptr [esp + 0x28]		// 0x0004ee87
		mov ecx, dword ptr [esi + 0x70]		// 0x0004ee8b
		or eax, 0x80000000		// 0x0004ee8e
		mov dword ptr [ecx + edx*4], eax		// 0x0004ee93
		mov edx, dword ptr [esp + 0x18]		// 0x0004ee96
L4ee9a:
		mov ebp, dword ptr [esp + 0x2c]		// 0x0004ee9a
		cmp ebp, -1		// 0x0004ee9e
		je L4ef59		// 0x0004eea1
		mov eax, dword ptr [esi + 0x70]		// 0x0004eea7
		mov ecx, dword ptr [eax + ebp*4]		// 0x0004eeaa
		shr ecx, 0x1f		// 0x0004eead
		not ecx		// 0x0004eeb0
		test cl, 1		// 0x0004eeb2
		jne L4ef59		// 0x0004eeb5
		lea eax, [ebx + 1]		// 0x0004eebb
		test eax, eax		// 0x0004eebe
		mov dword ptr [esp + 0x10], eax		// 0x0004eec0
		fild dword ptr [esp + 0x10]		// 0x0004eec4
		jge L4eed0		// 0x0004eec8
		fadd dword ptr [nxPMapTwo32]		// 0x0004eeca
L4eed0:
		fmul dword ptr [esi + 0x50]		// 0x0004eed0
		mov ecx, dword ptr [esp + 0x14]		// 0x0004eed3
		inc ecx		// 0x0004eed7
		test ecx, ecx		// 0x0004eed8
		fsub dword ptr [esi + 0x2c]		// 0x0004eeda
		mov dword ptr [esp + 0x10], ecx		// 0x0004eedd
		fadd dword ptr [esi + 0x20]		// 0x0004eee1
		fild dword ptr [esp + 0x10]		// 0x0004eee4
		jge L4eef0		// 0x0004eee8
		fadd dword ptr [nxPMapTwo32]		// 0x0004eeea
L4eef0:
		test edx, edx		// 0x0004eef0
		fmul dword ptr [esi + 0x54]		// 0x0004eef2
		mov dword ptr [esp + 0x10], edx		// 0x0004eef5
		fsub dword ptr [esi + 0x30]		// 0x0004eef9
		fadd dword ptr [esi + 0x24]		// 0x0004eefc
		fxch st(1)		// 0x0004eeff
		fstp dword ptr [esp + 0x6c]		// 0x0004ef01
		fstp dword ptr [esp + 0x70]		// 0x0004ef05
		fild dword ptr [esp + 0x10]		// 0x0004ef09
		jge L4ef15		// 0x0004ef0d
		fadd dword ptr [nxPMapTwo32]		// 0x0004ef0f
L4ef15:
		fmul dword ptr [esi + 0x58]		// 0x0004ef15
		lea edx, [esp + 0x1c]		// 0x0004ef18
		push edx		// 0x0004ef1c
		push 0		// 0x0004ef1d
		fsub dword ptr [esi + 0x34]		// 0x0004ef1f
		push edi		// 0x0004ef22
		lea eax, [esp + 0x78]		// 0x0004ef23
		fadd dword ptr [esi + 0x28]		// 0x0004ef27
		push eax		// 0x0004ef2a
		lea ecx, [esp + 0xac]		// 0x0004ef2b
		fstp dword ptr [esp + 0x84]		// 0x0004ef32
		call nxPMapNearest		// 0x0004ef39
		mov ecx, dword ptr [esp + 0xd4]		// 0x0004ef3e
		mov edx, dword ptr [esi + 0x70]		// 0x0004ef45
		mov eax, dword ptr [esp + 0x2c]		// 0x0004ef48
		or ecx, 0x80000000		// 0x0004ef4c
		mov dword ptr [edx + eax*4], ecx		// 0x0004ef52
		mov edx, dword ptr [esp + 0x18]		// 0x0004ef55
L4ef59:
		mov ebp, dword ptr [esp + 0x20]		// 0x0004ef59
		cmp ebp, -1		// 0x0004ef5d
		je L4f020		// 0x0004ef60
		mov ecx, dword ptr [esi + 0x70]		// 0x0004ef66
		mov eax, dword ptr [ecx + ebp*4]		// 0x0004ef69
		shr eax, 0x1f		// 0x0004ef6c
		not eax		// 0x0004ef6f
		test al, 1		// 0x0004ef71
		jne L4f020		// 0x0004ef73
		lea ecx, [ebx + 1]		// 0x0004ef79
		test ecx, ecx		// 0x0004ef7c
		mov dword ptr [esp + 0x10], ecx		// 0x0004ef7e
		fild dword ptr [esp + 0x10]		// 0x0004ef82
		jge L4ef8e		// 0x0004ef86
		fadd dword ptr [nxPMapTwo32]		// 0x0004ef88
L4ef8e:
		fmul dword ptr [esi + 0x50]		// 0x0004ef8e
		mov eax, dword ptr [esp + 0x14]		// 0x0004ef91
		inc eax		// 0x0004ef95
		test eax, eax		// 0x0004ef96
		fsub dword ptr [esi + 0x2c]		// 0x0004ef98
		mov dword ptr [esp + 0x10], eax		// 0x0004ef9b
		fadd dword ptr [esi + 0x20]		// 0x0004ef9f
		fild dword ptr [esp + 0x10]		// 0x0004efa2
		jge L4efae		// 0x0004efa6
		fadd dword ptr [nxPMapTwo32]		// 0x0004efa8
L4efae:
		fmul dword ptr [esi + 0x54]		// 0x0004efae
		inc edx		// 0x0004efb1
		test edx, edx		// 0x0004efb2
		mov dword ptr [esp + 0x10], edx		// 0x0004efb4
		fsub dword ptr [esi + 0x30]		// 0x0004efb8
		fadd dword ptr [esi + 0x24]		// 0x0004efbb
		fxch st(1)		// 0x0004efbe
		fstp dword ptr [esp + 0x84]		// 0x0004efc0
		fstp dword ptr [esp + 0x88]		// 0x0004efc7
		fild dword ptr [esp + 0x10]		// 0x0004efce
		jge L4efda		// 0x0004efd2
		fadd dword ptr [nxPMapTwo32]		// 0x0004efd4
L4efda:
		fmul dword ptr [esi + 0x58]		// 0x0004efda
		lea ecx, [esp + 0x1c]		// 0x0004efdd
		push ecx		// 0x0004efe1
		push 0		// 0x0004efe2
		fsub dword ptr [esi + 0x34]		// 0x0004efe4
		push edi		// 0x0004efe7
		lea edx, [esp + 0x90]		// 0x0004efe8
		fadd dword ptr [esi + 0x28]		// 0x0004efef
		push edx		// 0x0004eff2
		lea ecx, [esp + 0xac]		// 0x0004eff3
		fstp dword ptr [esp + 0x9c]		// 0x0004effa
		call nxPMapNearest		// 0x0004f001
		mov eax, dword ptr [esp + 0xd4]		// 0x0004f006
		mov edx, dword ptr [esp + 0x20]		// 0x0004f00d
		mov ecx, dword ptr [esi + 0x70]		// 0x0004f011
		or eax, 0x80000000		// 0x0004f014
		mov dword ptr [ecx + edx*4], eax		// 0x0004f019
		mov edx, dword ptr [esp + 0x18]		// 0x0004f01c
L4f020:
		mov eax, dword ptr [esi + 0x5c]		// 0x0004f020
		inc ebx		// 0x0004f023
		cmp ebx, eax		// 0x0004f024
		jb L4e8d0		// 0x0004f026
L4f02c:
		mov eax, dword ptr [esp + 0x14]		// 0x0004f02c
		mov ecx, dword ptr [esi + 0x5c]		// 0x0004f030
		inc eax		// 0x0004f033
		cmp eax, ecx		// 0x0004f034
		mov dword ptr [esp + 0x14], eax		// 0x0004f036
		jb L4e8c3		// 0x0004f03a
L4f040:
		mov eax, dword ptr [esi + 0x5c]		// 0x0004f040
		inc edx		// 0x0004f043
		cmp edx, eax		// 0x0004f044
		mov dword ptr [esp + 0x18], edx		// 0x0004f046
		jb L4e8b0		// 0x0004f04a
		pop edi		// 0x0004f050
		pop ebp		// 0x0004f051
		pop ebx		// 0x0004f052
L4f053:
		lea ecx, [esp + 0x90]		// 0x0004f053
		call nxPMapNearestFini		// 0x0004f05a
		mov eax, dword ptr [esp + 0x2c]		// 0x0004f05f
		pop esi		// 0x0004f063
		add esp, 0xc8		// 0x0004f064
		ret 4		// 0x0004f06a
		}
	}

// ---- callers' views of the register conventions ---------------------------------------------

unsigned nxPMapRayParity(const NxF32* direction, const NxF32* origin, const void* mesh)
	{
	unsigned result;
	__asm
		{
		mov eax, direction
		mov edi, origin
		push mesh
		call nxPMapRayParity_row
		add esp, 4
		mov result, eax
		}
	return result;
	}

bool nxPMapClassifyPoint(unsigned mode, const NxF32* point, const void* mesh)
	{
	unsigned char result;
	__asm
		{
		mov eax, mode
		mov ecx, point
		push mesh
		call nxPMapClassify
		add esp, 4
		mov result, al
		}
	return result != 0;
	}

void NxPMapNearest::init()
	{
	NxPMapNearest* self = this;
	__asm
		{
		mov ecx, self
		call nxPMapNearestInit
		}
	}

void NxPMapNearest::fini()
	{
	NxPMapNearest* self = this;
	__asm
		{
		mov ecx, self
		call nxPMapNearestFini
		}
	}

void* NxPMapNearest::destroy(unsigned flags)
	{
	NxPMapNearest* self = this;
	void* result;
	__asm
		{
		push flags
		mov ecx, self
		call nxPMapNearestDelete
		mov result, eax
		}
	return result;
	}

bool NxPMapNearest::query(const NxF32* point, const void* model, const void* world, unsigned* hint)
	{
	NxPMapNearest* self = this;
	unsigned char result;
	__asm
		{
		push hint
		push world
		push model
		push point
		mov ecx, self
		call nxPMapNearest
		mov result, al
		}
	return result != 0;
	}

unsigned nxPMapMarkCorners(void* map, const void* model)
	{
	unsigned result;
	__asm
		{
		mov ecx, map
		push model
		call nxPMapMarkCorners_row
		mov result, eax
		}
	return result;
	}
