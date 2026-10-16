# Work carried on `local` that is not in ml-explore/mlx

`local` is the upstream release tag the newest `v<base>-local<NN>` tag names, plus the commits
below. `main` mirrors `ml-explore/mlx` `main` (fast-forward only).

| What | Source | Status |
|---|---|---|
| `mx.config` / `mlx::core::config` runtime configuration module | `ml-explore/mlx#4647` by Cheng (zcbenz), backported by eejd | open upstream; drop when a release contains it |
| tests: matmul-verified and linalg checks run with `MLX_ENABLE_TF32` 0 and 1, tolerances matched to each format; Metal test that the setting selects the reduced-precision route | eejd | depends on the config module; not submitted |
| docs: runtime precision control via `mx.config` | eejd | depends on the config module; not submitted |
| config does not cache a caller's default (only values read from the environment are cached) | eejd | **diverges from #4647**, whose `config.cpp` caches the first default (e.g. `gated_delta_chunk_size(T)` is pinned by whichever of decode/prefill ran first); tracked in eejd/cpp-mlx#TRACK; not posted upstream |
| python tests `test_conv` / `test_fast_gated_delta` converted to `mx.config.scoped_update` | #4647 does the same; applied to the v0.32.4 lines that landed after the PR base | follows #4647 |
| config review fixes (`ScopedUpdate` copy assignment deleted; `MLX_CUDA_USE_CUDNN_SDPA=0` honored) | eejd | also present in the upstream PR |

Retired from `local` at v0.32.4: the macOS 27 SDK NAX cooperative-tensor build fix
(`ml-explore/mlx#4594`, merged upstream and contained in v0.32.4).

Pre-rebase tip (base v0.32.3): tag `archive/local-pre-rebase-78e357d` (branch `macports/v0.32.3` is stale, kept for history).
Tags: `v0.32.4-local01` (superseded: config cached the first caller's default) and `v0.32.4-local02`.
`archive/local-unmerged-branch-82afd34`: the `local` branch ref as first pushed, which did not match the `local01` tag.
