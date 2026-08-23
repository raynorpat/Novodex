# Phase 5 Task 1: the object model, locked from constructors and vtable stores

Every claim below carries the address that establishes it. Where Phase 3
borrowed a Shape offset under its relay rule, this task re-derives it from a
constructor rather than inheriting it — and the re-derivation changed the
picture in one important way (three poses, not two).

Inputs: `evidence/phase5-vtable-census.json` (slot maps rebuilt from
`oracle/pe.json`'s decoded pointer records and validated against every
recorded preview), the Capstone listing, and constructor listings read
instruction by instruction.

---

## 1. The actor family

**NxActor declares exactly 88 virtuals** (`NxActor.h`). The census found two
large tables:

| table | rva | slots | distinct targets | reading |
| --- | --- | ---: | ---: | --- |
| `phys_data_000678` | `.rdata 0x101043d0` | 87 | **4** | abstract base's vtable: slot 0 dtor + slots 63/64 real; the other 84 are one pure-call filler, `phys_fn_005667` |
| `phys_data_000679` | `.rdata 0x10104530` | 88 | **88**, all phase 5 | the concrete dynamic-actor implementation |

The two share exactly slots 63/64 (`phys_fn_000038`/`phys_fn_000040`) — the
only behaviour the base implements beyond destruction.

The concrete pair, from their listings:

- **ctor `phys_fn_000044` (0x2480, 53 bytes)** — stores the base vptr at +0
  (`0x101043d0`, 0x2489), zeroes +4, constructs an embedded polymorphic
  subobject at **+8** (member ctor `phys_fn_002404` at call `0x1005ba70`,
  which installs vptr `0x101088b8` and zeroes two dwords), then overwrites
  that subobject's vptr with `0x1010468c` (0x249f) — a one-slot table sitting
  at TBL_88's tail, which is why the census merged it into the 88-slot run —
  stores its argument at **+0x14** (0x24a5), and finishes with the derived
  vptr `0x10104530` (0x24a9).
- **dtor `phys_fn_000118` (0x3650, 55 bytes)** — mirrors it: derived vptr,
  member vptr, member dtor `phys_fn_002406`, base vptr, then conditional
  delete through the SDK allocator global `0x101041bc` slot +0x14.
- **base deleting dtor `phys_fn_000042` (0x2460, 31 bytes)**.

Open: what the +8 subobject interfaces with (one virtual), and what the +0x14
argument means.

## 2. The shape family

Base-shape vtable `0x10107494`, installed by the shared ctor. Per-type finals,
with both dispatch slots Phase 3 could not close:

| type | final vtable | ctor rows | slot 5 (raycast) | slot 7 (sweep dispatch) |
| --- | --- | --- | --- | --- |
| BOX | `0x10106ab8` | `000977`, `000979` | `000949` p3 | `000951` p5 |
| CAPSULE | `0x10106b20` | `000987` | `001010` p3 | `001012` p3 |
| PLANE | `0x10107430` | `001247` | `001261` p3 | `001035` p3 (= base's target; plane does not override) |
| SPHERE | `0x10107528` | `001349` | `001377` p3 | `001373` p3 |
| MESH | `0x10107630` | `001379` | `001405` p3 | `001407` p3 |

**Slot 7 resolves at byte level for every type.** Phase 3's "unresolved" was
behavioural (what the sweep does), not byte absence — the continuous-collision
stop is narrower than it looked.

## 3. The Shape layout, from the shared constructor

`phys_fn_001273` (0x25530, 424 bytes) writes:

- `+0x00` vptr `0x10107494` (0x2553d); `+0x04` = **owner** = ctor arg 1
  (0x25543) — the borrowed claim confirmed from the constructor side;
- `+0x08` zeroed; then **THREE identity-initialised poses**: m33 diagonals at
  `+0x0c/+0x1c/+0x2c` with t at `+0x30..+0x38`; `+0x3c/+0x4c/+0x5c` with t at
  `+0x60..+0x68`; `+0x6c/+0x7c/+0x8c` with t at `+0x90..+0x98`
  (ebx=`0x3f800000` throughout);
- tail: `lea ecx,[esi+0xa4]` — another subobject constructed at +0xa4.

Consequences:

1. **Phase 3's "+0x3c..+0x68 second pose, unestablished" is established**: it
   is a real pose, identity-initialised by the constructor.
2. **There is a third pose at +0x6c..+0x98** that no earlier evidence named.
   Its role (saved previous? sweep source?) is open.
3. The sphere ctor confirms `+0x9c` end to end: allocates **exactly 0x1c
   bytes** through the SDK allocator (`[0x101041bc]` slot +8 with (size,0)),
   constructs them with `phys_fn_001193` (0x247c0), stores at **+0x9c**
   (0x27805); also initialises `+0xd0 = 1`.
4. `+0xe0` is a secondary-base vptr where a desc mix-in exists (box: base
   table `A @0x106a58` replaced by `B @0x106a88`, both 12 slots; sphere:
   NULLed). With geometry floats following at `+0xe4..+0xec`, this is
   consistent with Phase 3's box-half-extents/capsule-radius reads and
   refines rather than contradicts them.

## 4. The census merge resolved

The census flagged its 41-slot row at `0x106a58` as overrunning BOX. It is
three real tables ending together at `0x106afc`: A(12) + B(12) + BOX(17).
Separately, `phys_fn_000973` (p2, 913 bytes) installs **twelve** small
vtables (`0x10106998`–`0x10106a48`) — almost certainly the descriptor
`setToDefault` family, which Task 2 owns confirming.

## 5. What this task did not do

- No behavioural reconstruction: every row here stays `discovered` until a
  differential drives it.
- Actor +8 subobject semantics, TBL_87 slots 63/64 identity, third-pose role,
  and the twelve-descriptor mapping are recorded as open questions in
  `object_model.json`.
