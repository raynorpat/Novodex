# Phase 6 closure: shared joint type, actor, value, and name wrappers

The staged joint matrix records each family’s type, actor owners, value-return anchor and axis, and D6 name. These five shared NpJoint wrappers were mutated one at a time in production source and caught by the registered `NxPhysicsJointStagedPairTests` differential.

| Row | Mutation | `stdout_delta` | Candidate exit | Mutant candidate SHA-256 | Restored candidate SHA-256 |
|---|---|---:|---:|---|---|
| `phys_fn_004443` | reported `NX_JOINT_FIXED` for every joint family instead of reading the internal type | 3128 | `-1073741819` | `c731cd1890d7188554e67f0f9ea6a207715626e75c424ca157bf7ffea75d3009` | `9b7c705c122d5ce3adf3cd231668d07e5f83b3c426350681b624e60a0c6e7a2a` |
| `phys_fn_004539` | returned null for actor 0 instead of reading its joint-body owner | 244 | `0` | `54bc55756a16cb2e7f2d67d9918161c222362330b4d5a26e372e5ca02057311b` | `fd0ca20035953968f416d19abfdafb5f5c4d4fce64591e21756586004ca01e62` |
| `phys_fn_004497` | returned a zero vector instead of the global-anchor value | 224 | `0` | `21d32bd7be52b5312874237e6edaa76b01e76817007a1c3950b037d414eab2f5` | `033a651fb47ecc7b67047c54ccd7de3a5b576b7a90bad5ab7398e3367e076f70` |
| `phys_fn_004499` | returned a zero vector instead of the global-axis value | 244 | `0` | `59680f6bc7c8435fcfe4f360605da60cd7b3edf90b4a21980e9c1dbd9d8416f3` | `3134a54d601bf867b60c7e00adc8a7852f17441a2cdf82bcba3c356525891938` |
| `phys_fn_004743` | returned null instead of looking up the joint name | 2 | `0` | `12b069ecd3b7bab554f619d9e1d2315988d771ddb853c50e32faa90bdb438fc8` | `472bf64fa073f5f88c267d65bc8d5f42f9669d78744d1c6d896c5798f5378cd7` |

For `phys_fn_004443`, the wrong type result caused the candidate to access-violate after the transcript diverged; the runner reported candidate exit `-1073741819`, `stdout_delta=3128`, and exact stderr. For the other four rows both processes exited 0 and stderr matched exactly. Each restored build returned `stdout_delta=0` with exact stderr.

Build, mutation, and restored-control logs are retained in ignored `build/phase6-004443-*`, `phase6-004539-*`, `phase6-004497-*`, `phase6-004499-*`, and `phase6-004743-*` files.
