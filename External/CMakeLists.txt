# The two vendored third-party libraries. See README.md for the layout rule.
#
# HOW THE TWO HALVES ARE JOINED. upstream/ is never edited and novodex/ holds
# only the files that differ, so neither directory is a compilable tree on its
# own. This merges them into the build directory -- novodex/ wins where both
# have a file -- and compiles that.
#
# configure_file(... COPYONLY) rather than file(COPY) on purpose: it registers
# the input as a configure dependency, so editing a novodex/ file re-runs CMake
# and the change reaches the compiler. file(COPY) would not, and a mutation that
# silently fails to take effect is how a differential goes green for the wrong
# reason. The phase gate reconfigures with --fresh on every run anyway, so the
# dependency is a second belt rather than the only one.

function(nx_merge_vendor_tree out_var upstream_root novodex_root)
	file(GLOB_RECURSE upstream_files RELATIVE "${upstream_root}" "${upstream_root}/*")
	set(merged "")
	foreach(rel ${upstream_files})
		if(EXISTS "${novodex_root}/${rel}")
			configure_file("${novodex_root}/${rel}" "${CMAKE_CURRENT_BINARY_DIR}/${out_var}/${rel}" COPYONLY)
		else()
			configure_file("${upstream_root}/${rel}" "${CMAKE_CURRENT_BINARY_DIR}/${out_var}/${rel}" COPYONLY)
		endif()
		list(APPEND merged "${rel}")
	endforeach()
	# Files novodex/ adds that have no upstream counterpart -- the two host seam
	# headers. They are listed in MODIFICATIONS.md as additions.
	file(GLOB_RECURSE novodex_files RELATIVE "${novodex_root}" "${novodex_root}/*")
	foreach(rel ${novodex_files})
		list(FIND merged "${rel}" found)
		if(found EQUAL -1)
			configure_file("${novodex_root}/${rel}" "${CMAKE_CURRENT_BINARY_DIR}/${out_var}/${rel}" COPYONLY)
		endif()
	endforeach()
endfunction()

nx_merge_vendor_tree(qhull-tree  "${CMAKE_CURRENT_SOURCE_DIR}/qhull/upstream/src"     "${CMAKE_CURRENT_SOURCE_DIR}/qhull/novodex")
nx_merge_vendor_tree(opcode-tree "${CMAKE_CURRENT_SOURCE_DIR}/opcode/upstream/Opcode" "${CMAKE_CURRENT_SOURCE_DIR}/opcode/novodex")

set(QHULL_TREE  "${CMAKE_CURRENT_BINARY_DIR}/qhull-tree")
set(OPCODE_TREE "${CMAKE_CURRENT_BINARY_DIR}/opcode-tree")

#### QHULL ####

# The twelve library translation units, and only those. The eight command-line
# programs in the archive are not compiled: none of their
# "qhull internal warning (main): ..." literals occurs anywhere in the oracle,
# and the driver the oracle actually uses is NovodeX's own row at 0x0007d420,
# which calls qh_init_A + qh_initflags the way unix.c's main() does rather than
# going through qh_new_qhull -- a function whose literals are absent outright.
add_library(NxQhull STATIC
	${QHULL_TREE}/geom.c
	${QHULL_TREE}/geom2.c
	${QHULL_TREE}/global.c
	${QHULL_TREE}/io.c
	${QHULL_TREE}/mem.c
	${QHULL_TREE}/merge.c
	${QHULL_TREE}/poly.c
	${QHULL_TREE}/poly2.c
	${QHULL_TREE}/qhull.c
	${QHULL_TREE}/qset.c
	${QHULL_TREE}/stat.c
	${QHULL_TREE}/user.c)
target_include_directories(NxQhull PUBLIC ${QHULL_TREE})

#### OPCODE ####

# Everything in the archive except the four translation units the oracle
# positively does not contain: OPC_HybridModel.cpp (no HybridModel vtable and no
# Hybrid*Collider anywhere in the image), OPC_BoxPruning.cpp (no
# BipartiteBoxPruning/CompleteBoxPruning and no PRUNING_SORTER static-init
# guards), OPC_Picking.cpp (none of its five entry points) and Opcode.cpp (no
# InitOpcode/CloseOpcode/ModuleAttach).
add_library(NxOpcode STATIC
	${OPCODE_TREE}/OPC_AABBCollider.cpp
	${OPCODE_TREE}/OPC_AABBTree.cpp
	${OPCODE_TREE}/OPC_BaseModel.cpp
	${OPCODE_TREE}/OPC_Collider.cpp
	${OPCODE_TREE}/OPC_Common.cpp
	${OPCODE_TREE}/OPC_LSSCollider.cpp
	${OPCODE_TREE}/OPC_MeshInterface.cpp
	${OPCODE_TREE}/OPC_Model.cpp
	${OPCODE_TREE}/OPC_OBBCollider.cpp
	${OPCODE_TREE}/OPC_OptimizedTree.cpp
	${OPCODE_TREE}/OPC_PlanesCollider.cpp
	${OPCODE_TREE}/OPC_RayCollider.cpp
	${OPCODE_TREE}/OPC_SphereCollider.cpp
	${OPCODE_TREE}/OPC_SweepAndPrune.cpp
	${OPCODE_TREE}/OPC_TreeBuilders.cpp
	${OPCODE_TREE}/OPC_TreeCollider.cpp
	${OPCODE_TREE}/OPC_VolumeCollider.cpp
	${OPCODE_TREE}/StdAfx.cpp
	${OPCODE_TREE}/Ice/IceAABB.cpp
	${OPCODE_TREE}/Ice/IceContainer.cpp
	${OPCODE_TREE}/Ice/IceHPoint.cpp
	${OPCODE_TREE}/Ice/IceIndexedTriangle.cpp
	${OPCODE_TREE}/Ice/IceMatrix3x3.cpp
	${OPCODE_TREE}/Ice/IceMatrix4x4.cpp
	${OPCODE_TREE}/Ice/IceOBB.cpp
	${OPCODE_TREE}/Ice/IcePlane.cpp
	${OPCODE_TREE}/Ice/IcePoint.cpp
	${OPCODE_TREE}/Ice/IceRandom.cpp
	${OPCODE_TREE}/Ice/IceRay.cpp
	${OPCODE_TREE}/Ice/IceRevisitedRadix.cpp
	${OPCODE_TREE}/Ice/IceSegment.cpp
	${OPCODE_TREE}/Ice/IceTriangle.cpp
	${OPCODE_TREE}/Ice/IceUtils.cpp)
target_include_directories(NxOpcode PUBLIC ${OPCODE_TREE})
# ICE_NO_DLL makes OPCODE_API and ICECORE_API empty -- this is a static library
# inside NxPhysics.dll and exports nothing of its own, which is also what the
# oracle's export table says. OPCODE_EXPORTS additionally suppresses Opcode.h's
# `#pragma comment(lib, "Opcode.lib")` autolink.
target_compile_definitions(NxOpcode PUBLIC ICE_NO_DLL OPCODE_EXPORTS)

#### BUILD FLAGS FOR BOTH ####

if(MSVC)
	foreach(vendored NxQhull NxOpcode)
		# Two facts about the shipped build, not preferences:
		#
		#   no RTTI      -- zero `.?AV` type-descriptor strings in the image;
		#   no exception handling -- a /EHsc build wraps Model::Build in an SEH
		#                    frame (push -1, __ehhandler$, mov fs:0,esp) that
		#                    0x000e9100 does not have.
		#
		# /arch:IA32 for the same reason the five Physics/src kernels have it,
		# and with more of a claim to it: OPCODE runs under BOTH x87 control
		# words. Scene::simulate at 0x00013c40 installs _PC_64 | _RC_CHOP and at
		# least 21 rows in the OPCODE span are inside its closure once tail-call
		# jmp edges are counted, while the exported API sees 0x027f. An SSE2
		# build ignores the control word outright and can never follow either.
		# qhull is at zero rows under both methods, but it is one library built
		# one way and splitting the flag between them would only invite the
		# question again.
		#
		# target_compile_options, not set_source_files_properties: the rule in
		# the root CMakeLists is per source file and per directory, and a second
		# set_source_files_properties naming the same files silently overrode it
		# once. A target property cannot collide with it.
		target_compile_options(${vendored} PRIVATE /arch:IA32 /GR-)
		# CMake puts /EHsc in CMAKE_CXX_FLAGS. /EHs-c- after it is what turns it
		# off; D9025 says so out loud, which is better than a silent default.
		target_compile_options(${vendored} PRIVATE $<$<COMPILE_LANGUAGE:CXX>:/EHs-c->)
		# qhull 2003.1 and OPCODE 1.3 predate every one of these. Warnings that
		# say a 2003 library was written in 2003 are noise, and drowning the
		# build log is how a real diagnostic gets missed.
		target_compile_definitions(${vendored} PRIVATE _CRT_SECURE_NO_WARNINGS _CRT_NONSTDC_NO_WARNINGS)
		target_compile_options(${vendored} PRIVATE /wd4996 /wd4244 /wd4267 /wd4018 /wd4101 /wd4102 /wd4189 /wd4703 /wd4353)
	endforeach()
endif()
