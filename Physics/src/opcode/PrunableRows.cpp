/*----------------------------------------------------------------------------*\
|
|								NovodeX Technology
|
|							     www.novodex.com
|
\*----------------------------------------------------------------------------*/

// PrunableRows.cpp: phys_fn_001945 with 001947 (0x0004bbd0, one function the inventory splits at the loop target
// 0x0004bc20): the scene's removal of an owner's primary and compound-child prunables through the pruners' dispatcher,
// then the SweepAndPrune the manager owns destroyed and freed. A naked listing transcription; NxScenePrunerOwnerDestroy
// (IcePrunable.cpp) keeps its C++ body, which the ObjectModel-only test targets link without the joint rows. Compare script:
// docs/reconstruction/novodex-physics/evidence/phase3-prunable-owner-destroy-listing-compare.py.
// phys_fn_001935 (0x0004b9c0): the callback the pruners' visualisation walk (001967) passes: a node with a box at
// +0x18 is drawn through the SDK's debug renderable (slot +0x2c, colour 0xffff0000). The candidate's walk
// (SceneVisualize.cpp) uses its own callback, so nothing calls this one yet.

extern "C" void nxPodCall1SweepAndPrune();
extern "C" void nxPodCallDispatch();
extern "C" void nxPodCallGetDebugRenderable();
extern "C" void nxPodCallGetNbShapes();
extern "C" void nxPodCallGetShapes();
extern "C" void nxPodCallNxGetSdkAllocator();
#pragma comment(linker, "/alternatename:_nxPodCall1SweepAndPrune=??1SweepAndPrune@Opcode@@QAE@XZ")
#pragma comment(linker, "/alternatename:_nxPodCallDispatch=?Dispatch@PrunableOwnerDispatcher@@QAEIPAX@Z")
#pragma comment(linker, "/alternatename:_nxPodCallGetDebugRenderable=?getDebugRenderable@PhysicsSDK@@QAEPAVNxDebugRenderable@@XZ")
#pragma comment(linker, "/alternatename:_nxPodCallGetNbShapes=?getNbShapes@JointActorBody@@QBEIXZ")
#pragma comment(linker, "/alternatename:_nxPodCallGetShapes=?getShapes@JointActorBody@@QAEPAPAXXZ")
#pragma comment(linker, "/alternatename:_nxPodCallNxGetSdkAllocator=?nxGetSdkAllocator@@YAPAVSdkAllocator@@XZ")
extern "C" void* nxPodSdkInstance;
#pragma comment(linker, "/alternatename:_nxPodSdkInstance=?instance@PhysicsSDK@@2PAV1@A")

bool __cdecl nxPrunerVisualizeNodeCallback(void* unused, void* node);
void __fastcall nxPrunerOwnerDestroyRow(void* manager, void* edxUnused, void* owner);
// @@PROTOS-END@@


// phys_fn_001935 (0x0004b9c0, 46 B), 18 instructions
__declspec(naked) bool __cdecl nxPrunerVisualizeNodeCallback(void* unused, void* node)
	{
	__asm
		{
		push esi		// 0x0004b9c0
		mov esi, dword ptr [esp + 0xc]		// 0x0004b9c1
		test esi, esi		// 0x0004b9c5
		je L4b9ea		// 0x0004b9c7
		mov eax, dword ptr [esi + 0x18]		// 0x0004b9c9
		test eax, eax		// 0x0004b9cc
		je L4b9ea		// 0x0004b9ce
		mov ecx, dword ptr [nxPodSdkInstance]		// 0x0004b9d0
		call nxPodCallGetDebugRenderable		// 0x0004b9d6
		mov edx, dword ptr [eax]		// 0x0004b9db
		push 0		// 0x0004b9dd
		push 0xffff0000		// 0x0004b9df
		push esi		// 0x0004b9e4
		mov ecx, eax		// 0x0004b9e5
		call dword ptr [edx + 0x2c]		// 0x0004b9e7
L4b9ea:
		mov al, 1		// 0x0004b9ea
		pop esi		// 0x0004b9ec
		ret		// 0x0004b9ed
		}
	}

// phys_fn_001945 and phys_fn_001947 (one function) (0x0004bbd0, 169 B), 62 instructions
__declspec(naked) void __fastcall nxPrunerOwnerDestroyRow(void* manager, void* edxUnused, void* owner)
	{
	__asm
		{
		push ebx		// 0x0004bbd0
		push ebp		// 0x0004bbd1
		push esi		// 0x0004bbd2
		mov esi, dword ptr [esp + 0x10]		// 0x0004bbd3
		mov ebx, dword ptr [esi + 0x10]		// 0x0004bbd7
		push edi		// 0x0004bbda
		lea eax, [ebx + 0xa4]		// 0x0004bbdb
		push eax		// 0x0004bbe1
		mov edi, ecx		// 0x0004bbe2
		mov dword ptr [esp + 0x18], ebx		// 0x0004bbe4
		call nxPodCallDispatch		// 0x0004bbe8
		mov ebp, dword ptr [ebx + 0xd0]		// 0x0004bbed
		xor ecx, ecx		// 0x0004bbf3
		cmp ebp, 5		// 0x0004bbf5
		setne cl		// 0x0004bbf8
		dec ecx		// 0x0004bbfb
		test ebx, ecx		// 0x0004bbfc
		je L4bc3b		// 0x0004bbfe
		mov ecx, esi		// 0x0004bc00
		call nxPodCallGetNbShapes		// 0x0004bc02
		mov ecx, esi		// 0x0004bc07
		mov ebx, eax		// 0x0004bc09
		call nxPodCallGetShapes		// 0x0004bc0b
		xor esi, esi		// 0x0004bc10
		test ebx, ebx		// 0x0004bc12
		mov ebp, eax		// 0x0004bc14
		jbe L4bc37		// 0x0004bc16
		jmp L4bc20		// 0x0004bc18
		lea ebx, [ebx]		// 0x0004bc1a
L4bc20:
		mov edx, dword ptr [ebp + esi*4]		// 0x0004bc20
		add edx, 0xa4		// 0x0004bc24
		push edx		// 0x0004bc2a
		mov ecx, edi		// 0x0004bc2b
		call nxPodCallDispatch		// 0x0004bc2d
		inc esi		// 0x0004bc32
		cmp esi, ebx		// 0x0004bc33
		jb L4bc20		// 0x0004bc35
L4bc37:
		mov ebx, dword ptr [esp + 0x14]		// 0x0004bc37
L4bc3b:
		mov esi, dword ptr [edi + 0x2c]		// 0x0004bc3b
		test esi, esi		// 0x0004bc3e
		je L4bc68		// 0x0004bc40
		mov ecx, esi		// 0x0004bc42
		call nxPodCall1SweepAndPrune		// 0x0004bc44
		call nxPodCallNxGetSdkAllocator		// 0x0004bc49
		mov edx, dword ptr [eax]		// 0x0004bc4e
		push esi		// 0x0004bc50
		mov ecx, eax		// 0x0004bc51
		call dword ptr [edx + 0xc]		// 0x0004bc53
		xor eax, eax		// 0x0004bc56
		mov dword ptr [edi + 0x2c], eax		// 0x0004bc58
		pop edi		// 0x0004bc5b
		pop esi		// 0x0004bc5c
		pop ebp		// 0x0004bc5d
		mov dword ptr [ebx + 0xa0], eax		// 0x0004bc5e
		pop ebx		// 0x0004bc64
		ret 4		// 0x0004bc65
L4bc68:
		pop edi		// 0x0004bc68
		pop esi		// 0x0004bc69
		pop ebp		// 0x0004bc6a
		mov dword ptr [ebx + 0xa0], 0		// 0x0004bc6b
		pop ebx		// 0x0004bc75
		ret 4		// 0x0004bc76
		}
	}
// @@ROWS-END@@
