// HullShape.cpp: the convex mesh's hull subclass (vtable 0x1010767c), the rows that sit on top of ConvexHull:
// phys_fn_001411 (after a build, the vertex valencies at +0x64 and the vertices' bounding box at +0x4c),
// 001415 (the destructor body), 001417 and 001418 (the overrides of the builder and the initialiser that run the
// base's, then 001411) and 001419 (the scalar deleting destructor). Naked listing transcriptions; the compare
// script is docs/reconstruction/novodex-physics/evidence/phase3-hull-shape-listing-compare.py.
//
// The object is HullBuild.cpp's ConvexHull (0x4c bytes) with, past it, the bounding box (+0x4c minimum,
// +0x58 maximum) and the Valencies at +0x64. Slot 3 (001413, the summed byte count) is reconstructed
// elsewhere (ObjectModel.cpp) and is left empty in this vtable.

#include "NxPhysicsSDK.h"

extern "C" void nxHullCallAllocator();
extern "C" void nxHullCallValenciesCtor();
extern "C" void nxHullCallValenciesDtor();
extern "C" void nxHullCallValenciesCompute();
#pragma comment(linker, "/alternatename:_nxHullCallAllocator=?nxGetSdkAllocator@@YAPAVSdkAllocator@@XZ")
#pragma comment(linker, "/alternatename:_nxHullCallValenciesCtor=??0Valencies@@QAE@XZ")
#pragma comment(linker, "/alternatename:_nxHullCallValenciesDtor=??1Valencies@@QAE@XZ")
#pragma comment(linker, "/alternatename:_nxHullCallValenciesCompute=?Compute@Valencies@@QAE_NABUVALENCESCREATE@@@Z")

void nxHullBuild();
void nxHullInit();
void nxHullRelease();
void nxHullShapeExtras();
void nxHullShapeDestruct();
void nxHullShapeBuild();
void nxHullShapeInit();
void nxHullShapeScalarDelete();

void* nxHullShapeVtable[4] = { (void*) &nxHullShapeScalarDelete, (void*) &nxHullShapeBuild, (void*) &nxHullShapeInit, 0 };


// phys_fn_001411 (0x00029780, 412 B), 135 instructions
__declspec(naked) void nxHullShapeExtras()
	{
	__asm
		{
		sub esp, 0x2c		// 0x00029780
		push esi		// 0x00029783
		mov esi, ecx		// 0x00029784
		call nxHullCallAllocator		// 0x00029786
		mov edx, dword ptr [eax]		// 0x0002978b
		push 0		// 0x0002978d
		push 0x14		// 0x0002978f
		mov ecx, eax		// 0x00029791
		call dword ptr [edx]		// 0x00029793
		test eax, eax		// 0x00029795
		je L297a2		// 0x00029797
		mov ecx, eax		// 0x00029799
		call nxHullCallValenciesCtor		// 0x0002979b
		jmp L297a4		// 0x000297a0
L297a2:
		xor eax, eax		// 0x000297a2
L297a4:
		test eax, eax		// 0x000297a4
		mov dword ptr [esi + 0x64], eax		// 0x000297a6
		jne L297b2		// 0x000297a9
		xor al, al		// 0x000297ab
		pop esi		// 0x000297ad
		add esp, 0x2c		// 0x000297ae
		ret		// 0x000297b1
L297b2:
		mov edx, dword ptr [esi + 0xc]		// 0x000297b2
		xor ecx, ecx		// 0x000297b5
		mov dword ptr [esp + 0x1c], ecx		// 0x000297b7
		mov dword ptr [esp + 0x20], ecx		// 0x000297bb
		mov dword ptr [esp + 0x24], ecx		// 0x000297bf
		mov dword ptr [esp + 0x28], ecx		// 0x000297c3
		mov dword ptr [esp + 0x2c], ecx		// 0x000297c7
		mov ecx, dword ptr [esi + 4]		// 0x000297cb
		mov dword ptr [esp + 0x20], ecx		// 0x000297ce
		lea ecx, [esp + 0x1c]		// 0x000297d2
		mov dword ptr [esp + 0x1c], edx		// 0x000297d6
		mov edx, dword ptr [esi + 8]		// 0x000297da
		push ecx		// 0x000297dd
		mov ecx, eax		// 0x000297de
		mov dword ptr [esp + 0x2c], edx		// 0x000297e0
		mov byte ptr [esp + 0x30], 1		// 0x000297e4
		call nxHullCallValenciesCompute		// 0x000297e9
		test al, al		// 0x000297ee
		jne L2981d		// 0x000297f0
		push edi		// 0x000297f2
		mov edi, dword ptr [esi + 0x64]		// 0x000297f3
		test edi, edi		// 0x000297f6
		je L29815		// 0x000297f8
		mov ecx, edi		// 0x000297fa
		call nxHullCallValenciesDtor		// 0x000297fc
		call nxHullCallAllocator		// 0x00029801
		mov edx, dword ptr [eax]		// 0x00029806
		push edi		// 0x00029808
		mov ecx, eax		// 0x00029809
		call dword ptr [edx + 0xc]		// 0x0002980b
		mov dword ptr [esi + 0x64], 0		// 0x0002980e
L29815:
		pop edi		// 0x00029815
		xor al, al		// 0x00029816
		pop esi		// 0x00029818
		add esp, 0x2c		// 0x00029819
		ret		// 0x0002981c
L2981d:
		mov ecx, dword ptr [esi + 0x10]		// 0x0002981d
		mov eax, dword ptr [esi + 0xc]		// 0x00029820
		add esi, 0x4c		// 0x00029823
		test ecx, ecx		// 0x00029826
		je L29915		// 0x00029828
		test eax, eax		// 0x0002982e
		mov dword ptr [esp + 0x10], 0xff7fffff		// 0x00029830
		mov dword ptr [esp + 0x14], 0xff7fffff		// 0x00029838
		mov dword ptr [esp + 0x18], 0xff7fffff		// 0x00029840
		mov dword ptr [esp + 4], 0x7f7fffff		// 0x00029848
		mov dword ptr [esp + 8], 0x7f7fffff		// 0x00029850
		mov dword ptr [esp + 0xc], 0x7f7fffff		// 0x00029858
		je L298e8		// 0x00029860
		mov edx, eax		// 0x00029866
L29868:
		fld dword ptr [ecx]		// 0x00029868
		fcomp dword ptr [esp + 0x10]		// 0x0002986a
		fnstsw ax		// 0x0002986e
		test ah, 0x41		// 0x00029870
		jne L2987b		// 0x00029873
		mov eax, dword ptr [ecx]		// 0x00029875
		mov dword ptr [esp + 0x10], eax		// 0x00029877
L2987b:
		fld dword ptr [ecx]		// 0x0002987b
		fcomp dword ptr [esp + 4]		// 0x0002987d
		fnstsw ax		// 0x00029881
		test ah, 5		// 0x00029883
		jp L2988e		// 0x00029886
		mov eax, dword ptr [ecx]		// 0x00029888
		mov dword ptr [esp + 4], eax		// 0x0002988a
L2988e:
		fld dword ptr [ecx + 4]		// 0x0002988e
		fcomp dword ptr [esp + 0x14]		// 0x00029891
		fnstsw ax		// 0x00029895
		test ah, 0x41		// 0x00029897
		jne L298a3		// 0x0002989a
		mov eax, dword ptr [ecx + 4]		// 0x0002989c
		mov dword ptr [esp + 0x14], eax		// 0x0002989f
L298a3:
		fld dword ptr [ecx + 4]		// 0x000298a3
		fcomp dword ptr [esp + 8]		// 0x000298a6
		fnstsw ax		// 0x000298aa
		test ah, 5		// 0x000298ac
		jp L298b8		// 0x000298af
		mov eax, dword ptr [ecx + 4]		// 0x000298b1
		mov dword ptr [esp + 8], eax		// 0x000298b4
L298b8:
		fld dword ptr [ecx + 8]		// 0x000298b8
		fcomp dword ptr [esp + 0x18]		// 0x000298bb
		fnstsw ax		// 0x000298bf
		test ah, 0x41		// 0x000298c1
		jne L298cd		// 0x000298c4
		mov eax, dword ptr [ecx + 8]		// 0x000298c6
		mov dword ptr [esp + 0x18], eax		// 0x000298c9
L298cd:
		fld dword ptr [ecx + 8]		// 0x000298cd
		fcomp dword ptr [esp + 0xc]		// 0x000298d0
		fnstsw ax		// 0x000298d4
		test ah, 5		// 0x000298d6
		jp L298e2		// 0x000298d9
		mov eax, dword ptr [ecx + 8]		// 0x000298db
		mov dword ptr [esp + 0xc], eax		// 0x000298de
L298e2:
		add ecx, 0xc		// 0x000298e2
		dec edx		// 0x000298e5
		jne L29868		// 0x000298e6
L298e8:
		mov edx, dword ptr [esp + 4]		// 0x000298e8
		mov eax, dword ptr [esp + 8]		// 0x000298ec
		mov ecx, esi		// 0x000298f0
		mov dword ptr [ecx], edx		// 0x000298f2
		mov edx, dword ptr [esp + 0xc]		// 0x000298f4
		mov dword ptr [ecx + 4], eax		// 0x000298f8
		mov eax, dword ptr [esp + 0x10]		// 0x000298fb
		add esi, 0xc		// 0x000298ff
		mov dword ptr [ecx + 8], edx		// 0x00029902
		mov ecx, dword ptr [esp + 0x14]		// 0x00029905
		mov edx, dword ptr [esp + 0x18]		// 0x00029909
		mov dword ptr [esi], eax		// 0x0002990d
		mov dword ptr [esi + 4], ecx		// 0x0002990f
		mov dword ptr [esi + 8], edx		// 0x00029912
L29915:
		mov al, 1		// 0x00029915
		pop esi		// 0x00029917
		add esp, 0x2c		// 0x00029918
		ret		// 0x0002991b
		}
	}

// phys_fn_001415 (0x00029940, 53 B), 19 instructions
__declspec(naked) void nxHullShapeDestruct()
	{
	__asm
		{
		push esi		// 0x00029940
		mov esi, ecx		// 0x00029941
		push edi		// 0x00029943
		mov edi, dword ptr [esi + 0x64]		// 0x00029944
		test edi, edi		// 0x00029947
		mov dword ptr [esi], offset nxHullShapeVtable		// 0x00029949
		je L2996c		// 0x0002994f
		mov ecx, edi		// 0x00029951
		call nxHullCallValenciesDtor		// 0x00029953
		call nxHullCallAllocator		// 0x00029958
		mov edx, dword ptr [eax]		// 0x0002995d
		push edi		// 0x0002995f
		mov ecx, eax		// 0x00029960
		call dword ptr [edx + 0xc]		// 0x00029962
		mov dword ptr [esi + 0x64], 0		// 0x00029965
L2996c:
		pop edi		// 0x0002996c
		mov ecx, esi		// 0x0002996d
		pop esi		// 0x0002996f
		jmp nxHullRelease		// 0x00029970
		}
	}

// phys_fn_001417 (0x00029980, 32 B), 13 instructions
__declspec(naked) void nxHullShapeBuild()
	{
	__asm
		{
		mov eax, dword ptr [esp + 4]		// 0x00029980
		push esi		// 0x00029984
		push eax		// 0x00029985
		mov esi, ecx		// 0x00029986
		call nxHullBuild		// 0x00029988
		test al, al		// 0x0002998d
		jne L29995		// 0x0002998f
		pop esi		// 0x00029991
		ret 4		// 0x00029992
L29995:
		mov ecx, esi		// 0x00029995
		call nxHullShapeExtras		// 0x00029997
		pop esi		// 0x0002999c
		ret 4		// 0x0002999d
		}
	}

// phys_fn_001418 (0x000299a0, 32 B), 13 instructions
__declspec(naked) void nxHullShapeInit()
	{
	__asm
		{
		mov eax, dword ptr [esp + 4]		// 0x000299a0
		push esi		// 0x000299a4
		push eax		// 0x000299a5
		mov esi, ecx		// 0x000299a6
		call nxHullInit		// 0x000299a8
		test al, al		// 0x000299ad
		jne L299b5		// 0x000299af
		pop esi		// 0x000299b1
		ret 4		// 0x000299b2
L299b5:
		mov ecx, esi		// 0x000299b5
		call nxHullShapeExtras		// 0x000299b7
		pop esi		// 0x000299bc
		ret 4		// 0x000299bd
		}
	}

// phys_fn_001419 (0x000299c0, 78 B), 28 instructions
__declspec(naked) void nxHullShapeScalarDelete()
	{
	__asm
		{
		push esi		// 0x000299c0
		mov esi, ecx		// 0x000299c1
		push edi		// 0x000299c3
		mov edi, dword ptr [esi + 0x64]		// 0x000299c4
		test edi, edi		// 0x000299c7
		mov dword ptr [esi], offset nxHullShapeVtable		// 0x000299c9
		je L299ec		// 0x000299cf
		mov ecx, edi		// 0x000299d1
		call nxHullCallValenciesDtor		// 0x000299d3
		call nxHullCallAllocator		// 0x000299d8
		mov edx, dword ptr [eax]		// 0x000299dd
		push edi		// 0x000299df
		mov ecx, eax		// 0x000299e0
		call dword ptr [edx + 0xc]		// 0x000299e2
		mov dword ptr [esi + 0x64], 0		// 0x000299e5
L299ec:
		mov ecx, esi		// 0x000299ec
		call nxHullRelease		// 0x000299ee
		test byte ptr [esp + 0xc], 1		// 0x000299f3
		je L29a07		// 0x000299f8
		call nxHullCallAllocator		// 0x000299fa
		mov edx, dword ptr [eax]		// 0x000299ff
		push esi		// 0x00029a01
		mov ecx, eax		// 0x00029a02
		call dword ptr [edx + 0xc]		// 0x00029a04
L29a07:
		pop edi		// 0x00029a07
		mov eax, esi		// 0x00029a08
		pop esi		// 0x00029a0a
		ret 4		// 0x00029a0b
		}
	}
