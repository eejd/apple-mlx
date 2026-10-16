# Work carried on `local` that is not in ml-explore/mlx

`local` is the upstream release tag the newest `v<base>-local<NN>` tag names, plus the commits
below. `main` mirrors `ml-explore/mlx` `main` (fast-forward only).

| What | Source | Status |
|---|---|---|
| `mx.config` / `mlx::core::config` runtime configuration module | `ml-explore/mlx#4647` by Cheng (zcbenz), backported by eejd | open upstream; drop when a release contains it |
| tests: matmul-verified and linalg checks run with `MLX_ENABLE_TF32` 0 and 1, tolerances matched to each format; Metal test that the setting selects the reduced-precision route | eejd | depends on the config module; not submitted |
| docs: runtime precision control via `mx.config` | eejd | depends on the config module; not submitted |
| config review fixes (`ScopedUpdate` copy assignment deleted; `MLX_CUDA_USE_CUDNN_SDPA=0` honored) | eejd | also present in the upstream PR |

Retired from `local` at v0.32.4: the macOS 27 SDK NAX cooperative-tensor build fix
(`ml-explore/mlx#4594`, merged upstream and contained in v0.32.4).

Pre-rebase tip (base v0.32.3): tag `archive/local-pre-rebase-78e357d`.
