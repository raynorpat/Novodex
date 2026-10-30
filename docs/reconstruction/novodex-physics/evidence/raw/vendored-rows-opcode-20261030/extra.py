# Each row is defined under its own symbol (the candidate symbol of opcode_match.csv).
R = {
 'phys_fn_004844': ('??0Container@IceCore@@QAE@ABV01@@Z', 'IceCore::Container::Container(const Container& object)'),
 'phys_fn_005069': ('?SphereTriOverlap@SphereCollider@Opcode@@IAEHABVPoint@IceMaths@@00@Z', 'BOOL Opcode::SphereCollider::SphereTriOverlap(const Point& vert0, const Point& vert1, const Point& vert2)'),
 'phys_fn_005111': ('?ValidateSettings@OBBCollider@Opcode@@UAEPBDXZ', 'const char* Opcode::OBBCollider::ValidateSettings()'),
 'phys_fn_005155': ('?Set@Plane@IceMaths@@QAEAAV12@ABVPoint@2@00@Z', 'IceMaths::Plane& IceMaths::Plane::Set(const Point& p0, const Point& p1, const Point& p2)'),
 'phys_fn_005179': ('?Area@Triangle@IceMaths@@QBEMXZ', 'float IceMaths::Triangle::Area() const'),
 'phys_fn_005181': ('?Normal@Triangle@IceMaths@@QBEXAAVPoint@2@@Z', 'void IceMaths::Triangle::Normal(Point& normal) const'),
 'phys_fn_005183': ('?Center@Triangle@IceMaths@@QBEXAAVPoint@2@@Z', 'void IceMaths::Triangle::Center(Point& center) const'),
 'phys_fn_005185': ('?Inflate@Triangle@IceMaths@@QAEXM_N@Z', 'void IceMaths::Triangle::Inflate(float fat_coeff, bool constant_border)'),
 'phys_fn_005193': ('?CoFactor@Matrix4x4@IceMaths@@QBEMII@Z', 'float IceMaths::Matrix4x4::CoFactor(udword row, udword col) const'),
 'phys_fn_005195': ('?Determinant@Matrix4x4@IceMaths@@QBEMXZ', 'float IceMaths::Matrix4x4::Determinant() const'),
 'phys_fn_005197': ('?Invert@Matrix4x4@IceMaths@@QAEAAV12@XZ', 'IceMaths::Matrix4x4& IceMaths::Matrix4x4::Invert()'),
 'phys_fn_005204': ('?ComputeWorldEdgeNormal@OBB@IceMaths@@QBEXIAAVPoint@2@@Z', 'void IceMaths::OBB::ComputeWorldEdgeNormal(udword edge_index, Point& world_normal) const'),
 'phys_fn_005281': ('?Init@SAP_PairData@Opcode@@QAE_NI@Z', 'bool Opcode::SAP_PairData::Init(udword nb_objects)'),
 'phys_fn_005366': ('??1Model@Opcode@@UAE@XZ', 'Opcode::Model::~Model()'),
 'phys_fn_005368': ('?Build@Model@Opcode@@UAE_NABUOPCODECREATE@2@@Z', 'bool Opcode::Model::Build(const OPCODECREATE& create)'),
}
NAMES = {k: v[0] for k, v in R.items()}
DECLS = {k: '__declspec(naked) ' + v[1] for k, v in R.items()}
INCLUDES = ['Stdafx.h']
HEAD_COMMENT = 'see head.txt'
SYMBOLS = {
    0x000b4000: '?nxGetSdkAllocator@@YAPAVSdkAllocator@@XZ',      # the allocator getter (OpcodeNovodeXHost.h [1])
    0x000539b0: '?opcNovodeXSetIceError@@YA_NPBD0H@Z',             # the SetIceError reporter (OpcodeNovodeXHost.h [2])
    0x000213e0: '?ValidateSettings@VolumeCollider@Opcode@@UAEPBDXZ',  # ICF-folded `xor eax,eax; ret`; the candidate's returns null
}
PUSHES = {0x1011b6ec: 'nxOpStrTemporalCoherence', 0x1011bac8: 'nxOpVtModel'}
DATADECL = ['static const char nxOpStrTemporalCoherence[] = "Temporal coherence only works with First contact mode!";',
            'extern "C" void* nxOpVtModel;',
            '#pragma comment(linker, "/alternatename:_nxOpVtModel=??_7Model@Opcode@@6B@")']
# OBB::GetLocalEdgeNormals' local static table, which the oracle inlines into ComputeWorldEdgeNormal (its only user)
# at .data 0x00122520. The candidate's copy is function-local (internal linkage), so the table is built here,
# word for word from the oracle's bytes.
import struct as _struct
import sys as _sys
_sys.path.insert(0, 'D:/github/Novodex/docs/reconstruction/novodex-physics/tools')
import transcribe_rows as _tr
_words = _struct.unpack('<36I', _tr.Image.from_pe(_tr.ORACLE).read(0x122520, 144))
DATA = {0x10122520: 'nxOpEdgeNormals', 0x10122524: 'nxOpEdgeNormals + 4'}
DATADECL += ['// OBB::GetLocalEdgeNormals\' table (.data 0x00122520, 12 Points), word for word from the oracle.',
             'static unsigned nxOpEdgeNormals[36] = {' + ', '.join('0x%08x' % w for w in _words) + '};']
# Model::~Model: MSVC stores the vptr itself ahead of a naked destructor's body, so the oracle's destructor is
# transcribed as a plain naked function (ecx = this, no stack arguments) and the destructor's symbol is aliased to it.
NAMES['phys_fn_005366'] = 'nxOpModelDtor'
DECLS['phys_fn_005366'] = 'extern "C" __declspec(naked) void __cdecl nxOpModelDtor()'
DATADECL += ['#pragma comment(linker, "/alternatename:??1Model@Opcode@@UAE@XZ=_nxOpModelDtor")']
