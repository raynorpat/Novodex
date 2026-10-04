# Old cl* source compatibility

The supplied pre-refactor headers contained unconditional #error and declarations
absent from the shipped GraphicsLib archive. Their comments/interfaces define
this adapter. This is source compatibility, rather than the old binary ABI.

clScene delegates ownership, resources, camera/lights, projection and rendering
to the same SceneGraph singleton. Resource creation records nonowning entries in
the old protected FNode lists; Scene owns registered resources until purge.
Deletion/detachment maintains one tree/root owner. Precamera objects render in
camera space; pretranslation objects use camera rotation without translation.
Modern Scene owns both special objects. The fullscreen hint skips color clears
on following old Clear calls until Purge.

clTexture, clMesh, clMaterial, clModel and clSceneObj derive modern classes.
Case-named methods delegate. Position/orientation/matrix/color/specular fields
are references to modern state. Typed pointer proxies bind old object/model/tree
names to modern slots without reinterpret-casts. Legacy creation uses derived
transparent overflow storage nodes. Object copies are disabled for ownership.

Mesh3 preserves its owned linked SubMesh interface while maintaining modern
indexed geometry copies. changed validates edited buffers, refreshes geometry
and bounds, and invalidates lists. Dynamic mode uses the shared geometry renderer
without display-list caching. DynamicMesh retains exposed GLM model access and
uses the same GLM renderer. Model3 loads through modern ODScript Model, adapting
mesh/material references; copies share registered mesh/material resources.

Primitives delegate to modern primitives. Containers own primitives and aggregate
line/plane/billboard bounds. Clients can implement getBounds for custom primitives;
an opaque display-list/custom primitive without bounds throws a diagnostic.
Empty containers have zero dimensions. MirrorFloor uses stencil masking and
reflected SceneGraph geometry with scoped GL state restoration. Its plane is
the transformed local Y plane, including parent transforms.

Both old texture-rendering entry points forward to the modern Scene overload,
including modern Texture storage, target/camera traversal and mode flags.

GraphicsLegacyTests covers independent fixture loads/bounds/cache, alias fields,
overflow traversal, reparenting/removal, special object replacement, collapse,
linked edits, Model3 load/copy, primitive destruction/bounds, dynamic recreation
and purge. The red run compiled headers/test then failed linking 35 expected old
interface definitions before implementation.

Exception and virtual-dispatch repair: the coordinated RED run reproduced native
mirror/camera-space matrix stack leaks and four CPU assertions showing bypassed
legacy material overrides. Camera-space and mirror helpers now restore their own
modelview matrix, attribute frame and incoming matrix mode on every exit. The
mirror recursion flag is scoped so a failed reflection does not suppress future
reflection draws. The ordinary floor pass runs after the reflection frame closes.

Ordinary Model explicitly dispatches legacy Activate/Deactivate around one virtual
old mesh Render() call, including meshes without modern submeshes. Model3 dispatches
materials per indexed submesh, including client Material3 subclasses. Modern materials continue
through qualified modern calls. Drawing failures release an activated material;
cleanup preserves the original exception and falls back to the legacy base
cleanup if a client Deactivate override throws. Legacy material activation and
pass-2/override-texture activation release their own attribute frame and reset
the activated flag if modern texture activation throws. This retains the recovered
single-pass model behavior. CPU tests exercise old and modern model entry points,
Material3 overrides and drawing failure cleanup; root-owned native tests exercise
GL stack restoration, failed texture activation and repeated failed reflections.
Render-only mesh subclasses are also exercised through both old and modern model
entry points, with multiple or zero submeshes and throwing draws.
