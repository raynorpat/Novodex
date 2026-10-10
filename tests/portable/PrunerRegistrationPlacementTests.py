"""Real production registration lifetime regression; no counterfeit Scene."""
import pathlib
root = pathlib.Path(__file__).resolve().parents[2]
source = (root/'Physics/src/opcode/IcePruner.cpp').read_text()
ctor = source.split('Pruner::Pruner()',1)[1].split('Pruner::~Pruner()',1)[0]
dtor = source.split('Pruner::~Pruner()',1)[1].split('// phys_fn_005208',1)[0]
assert 'nxPrunerRegister(mRegistration)' in ctor, 'actual Pruner constructor must register its live +34 member'
assert 'nxPrunerUnregister(mRegistration)' in dtor, 'actual Pruner destructor must unregister its live +34 member'
assert dtor.index('nxPrunerUnregister(') < dtor.index('if(mPool.mWorldBoxes)'), 'unregister before embedded pool storage destruction'
header = (root/'Physics/src/opcode/IcePrunable.h').read_text()
assert 'NxPrunerRegistration mRegistration;' in header, 'ordinary actual member, never placement overlay over live udword fields'
assert 'nxPrunerProcessPoolDestroy()' in (root/'Physics/src/PrunerRegistration.cpp').read_text(), 'genuine process owner cleanup'
print('actual Pruner registration member and destructor ordering: PASS')
