# Offscreen rendering and transform baking

The original archive's render-to-texture entry point is empty. The source
implementation deliberately supplies useful behavior rather than reconstructing
that no-op: a core/EXT framebuffer, RGBA color attachment, depth/stencil storage,
scene or target-subtree rendering, readback, aspect-preserving resizing and texture
upload. Horizontal FOV matches the scene API. Tests use real OpenGL pixels.

The capture scope restores projection/modelview matrices, viewport, attributes,
client arrays and texture unit, pixel pack state/buffer, renderbuffer and distinct
core read/draw framebuffer bindings. Capture and upload reset color transfer
parameters temporarily. Failures unwind the scopes and release temporary storage.

Object transform baking rewrites copied mesh geometry, preserving shared source
meshes. An owned primitive wrapper applies the cumulative transform to arbitrary
primitive drawing, including display lists, and forwards destruction exactly once.
Models with virtual drawing but no mesh retain a per-object baked model transform,
preserving subtype callbacks and shared model identity. Repeated baking composes
these transforms; the model scope does not affect descendants or primitives.

Regression tests compare native framebuffer pixels before and after baking for
modern planes and legacy primitive containers. They also exercise throwing
primitives, materials, reflected passes and camera-space rendering, checking matrix
and attribute stack depths and subsequent successful rendering.
