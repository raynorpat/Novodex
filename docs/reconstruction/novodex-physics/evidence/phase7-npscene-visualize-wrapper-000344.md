# NpScene visualize wrapper mutation proof

Phase 7 row phys_fn_000344 (NpScene::visualize) is falsified through the registered NxPhysicsSceneVisualizeTests staged-pair renderer differential. Omitting the Scene::visualize forward changes the transcript by 1,008 bytes; both oracle and candidate processes exit zero and stderr is exact. The restored build matches exactly (stdout_delta=0, both exits zero, exact stderr). The full Phase 7 gate passes at 1,707/1,707 assertions. The gate transcript is archived with the raw mutation and control logs.
