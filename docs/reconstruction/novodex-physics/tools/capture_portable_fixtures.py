import pathlib,struct,hashlib,json,math
from decimal import Decimal
root=pathlib.Path.cwd(); fixtures=root/'tests/portable/fixtures'
ops=['x87Fsqrt','x87FsqrtSum2','x87FsqrtSum3','x87FsqrtSum4','x87FsqrtDiffSum','x87FsqrtDiag','x87FsqrtMulSub','x87FsqrtDot2','x87FsqrtDot3','x87FsqrtDot4','x87FsqrtQuotDot3','x87FsinHalfOverNorm3','x87FcosHalfNorm3','x87RateOverRoot','x87CIacos','x87AcosRateOverRoot','jointCIacos','jointAcos']
raw=(fixtures/'shared-math-x87.nxpf').read_bytes()
repeat=(root/'build/task1-evidence/shared-math-repeat.nxpf').read_bytes()
assert raw==repeat
def sha(p):return hashlib.sha256(p.read_bytes()).hexdigest()
def cls(v):
    if math.isnan(v):return 'nan'
    if math.isinf(v):return 'infinity'
    if v==0:return 'signed-zero'
    return 'finite'
sensitivity=[]
for op,name in enumerate(ops):
    finite=[]; disagreements=[]; bitdiff=0
    for c in range(10):
        a=struct.unpack_from('<d',raw,16+(op*10+c)*80+72)[0]
        b=struct.unpack_from('<d',raw,16+(180+op*10+c)*80+72)[0]
        if math.isfinite(a) and math.isfinite(b):finite.append((abs(Decimal(a)-Decimal(b)),c,a,b))
        if cls(a)!=cls(b):disagreements.append({'case':c,'nearest_class':cls(a),'simulation_class':cls(b)})
        if raw[16+(op*10+c)*80+72:16+(op*10+c+1)*80]!=raw[16+(180+op*10+c)*80+72:16+(180+op*10+c+1)*80]:bitdiff+=1
    worst=max(finite,default=(Decimal(0),None,0,0))
    units='sqrt(input units)' if op<7 else 'sqrt(product units)' if op<10 else 'sqrt(numerator/dot units)' if op==10 else 'inverse length' if op==11 else 'dimensionless' if op==12 else 'radians/time' if op in (13,15) else 'radians'
    sensitivity.append({'operation_id':op,'operation':name,'output_units':units,'finite_pairs':len(finite),'bitwise_cw_differences':bitdiff,'max_nearest_vs_simulation_absolute_difference':str(worst[0]),'worst_case_index':worst[1],'class_disagreements':disagreements,'portable_acceptance_budget':None,'budget_gate':'Task 3 must measure scalar nearest against nearest x87 per operation; CW sensitivity is not an acceptance tolerance'})
# Preserve the unchanged existing geometry harness's semantic records separately.
import re
log=(root/'build/task1-evidence/differential-x87.log').read_text(encoding='utf-8-sig')
first=log.index('child target=NxPhysicsGeometryTests pair=oracle')
last=log.index('child_stdout_end',first)
lines=[l for l in log[first:last].splitlines() if l.startswith('case=')]
again=[l for l in (root/'build/task1-evidence/geometry-oracle-repeat.log').read_text(encoding='utf-8-sig').splitlines() if l.startswith('case=')]
assert lines==again and len(lines)==299
semantic=[]
for l in lines:
    m=re.fullmatch(r'case=(\S+) in=(\S+) ret=(\S+) out=(\S+)',l);assert m,l
    semantic.append({'case':m[1],'input_u32_hex':m[2].split(','),'return_u32_hex':m[3],'output_u32_hex':[] if m[4]=='-' else m[4].split(',')})
(fixtures/'geometry-shipped-oracle.json').write_text(json.dumps({'schema_version':1,'provenance_kind':'shipped-oracle','source_harness':'tests/PhysicsGeometryTests.cpp','word_encoding':'fixed-width 8-digit hexadecimal u32, IEEE binary32 for numeric words; no pointers/padding','records':semantic},indent=2)+'\n')
manifest={'schema_version':1,'reference_revision':'a4838ddf282ee859471e1526145b616de5ddedb6','compiler':'MSVC 19.51.36260.0 (VS18 2026)','configuration':'Release Win32','shared_helper_flags':['/arch:IA32','/fp:precise','/O2'],'oracle':{'physics_sha256':'4b7db3e126735c576f79fe5666e6fa661de9724b2a78808bb0924325ac79602c','foundation_sha256':'7e0596e45af2f1ab937a948100528e51c80dfd02b295ba678898081883ae0990','source_root':'D:/FlamingEnt__/Unreal_3/Binaries'},'reconstructed_dll':{'physics_sha256':sha(root/'build/x87-baseline/Release/NxPhysics.dll'),'foundation_sha256':sha(root/'build/x87-baseline/Release/NxFoundation.dll')},'fixtures':[{'path':'shared-math-x87.nxpf','sha256':sha(fixtures/'shared-math-x87.nxpf'),'provenance_kind':'reconstructed-reference','source':['Physics/src/include/X87Sqrt.h','Physics/src/include/core/JointAcos.h'],'exporter':'tests/portable/ExportSharedMathFixtures.cpp','record_count':360,'record_width':80,'encoding':'NXPF v1 LE: u32 op, u32 CW, eight IEEE binary64 input u64 words, binary64 output u64 word; output explicitly stored to double','control_words':{'0x027f':'53-bit nearest API reference','0x0f7f':'64-bit chop simulation reference; not portable nearest acceptance baseline'},'repeat_sha256':sha(root/'build/task1-evidence/shared-math-repeat.nxpf'),'repeat_exact_budget':{'absolute':0,'relative':0},'input_cases':['finite fractions','ones','positive zero','negative zero','least binary64 subnormal','adjacent-to-one and tiny addends','max finite','negative/domain/acos clamp','literal infinity','two literal quiet NaN payloads'],'boundary_policy':'special result classes handled separately; no NaN payload acceptance requirement; signed-zero/domain/clamp branches require explicit Task 3 classification','sensitivity':sensitivity},{'path':'geometry-shipped-oracle.json','sha256':sha(fixtures/'geometry-shipped-oracle.json'),'provenance_kind':'shipped-oracle','source_harness':'tests/PhysicsGeometryTests.cpp','record_count':299,'repeat_export':'same 299 semantic records on second oracle run; target transcript equality with worktree-built candidate also verified','portable_acceptance_budget':None,'budget_gate':'Task 4 measures geometry per output unit, boundary class and discrete outcome before acceptance'}],'family_obligations':[],'acceptance_policy':'No null/unmeasured budget can pass acceptance. Repeat identity budgets are not portable numeric budgets. No portable family translated by Task 1.'}
for family,task,units in [('conversion',3,['integer/index exact','text integer encoding']),('geometry',4,['length','length squared','unitless parameter','angle']),('support-topology-convex',5,['projection length','angle','topology/index exact']),('contacts',6,['penetration length','normal angle','hit/contact presence exact away from ambiguity']),('dynamics-environment',7,['pose length','velocity length/time','angle','constraint residual','sleep/wake exact']),('multistep-integration',8,['constraint residual','penetration','drift','energy accounting','timing']),('foundation-vendor-native',9,['layout/ABI exact per target','distance','BVH decisions'])]:
    manifest['family_obligations'].append({'family':family,'owner_task':task,'units':units,'portable_acceptance_budget':None,'status':'capture and measure before owning conversion acceptance','required_evidence':['fixed input bytes with seed/reference provenance','Debug/optimized baselines','worst numeric errors per unit/scale','discrete disagreements and boundary reproducer dispositions','real production path integration']})
(fixtures/'manifest.json').write_text(json.dumps(manifest,indent=2,allow_nan=False)+'\n')
print('shared_sha256',sha(fixtures/'shared-math-x87.nxpf'),'geometry_records',len(semantic),'sensitivity_ops',len(sensitivity))
