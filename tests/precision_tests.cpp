// Copyright © 2026 Apple Inc.

#include "doctest/doctest.h"

#include "mlx/backend/metal/device.h"
#include "mlx/config.h"
#include "mlx/mlx.h"

using namespace mlx::core;

namespace {

// float32 rounding in a K-term dot product: gamma_K = K u / (1 - K u) with
// u = 2^-24. CPU and GPU sum in different orders, so each is within gamma_K of
// the exact product and they are within 2 gamma_K of each other.
float float32_bound(int K) {
  float ku = K * (1.0f / (1 << 24));
  return 2.0f * ku / (1.0f - ku);
}

// TF32 keeps 10 explicit mantissa bits: each rounded input has relative error
// below 2^-10, a product of two below 2 * 2^-10. Accumulation stays float32.
float tf32_bound(int K) {
  return 2.0f / 1024 + float32_bound(K);
}

} // namespace

TEST_CASE("test float32 matmul precision setting") {
  // Shapes large enough to take the GEMM path rather than GEMV, including
  // ones that are not multiples of the tile sizes.
  const std::vector<std::tuple<int, int, int>> shapes = {
      {64, 64, 64}, {33, 257, 130}, {256, 128, 512}};

  auto key = random::key(0);
  for (auto [M, N, K] : shapes) {
    CAPTURE(M);
    CAPTURE(N);
    CAPTURE(K);
    auto keys = random::split(key);
    key = keys.first;
    auto ab = random::split(keys.second);
    auto a = random::normal({M, K}, float32, ab.first, Device::cpu);
    auto b = random::normal({K, N}, float32, ab.second, Device::cpu);
    auto expected = matmul(a, b, Device::cpu);
    auto magnitude =
        matmul(abs(a, Device::cpu), abs(b, Device::cpu), Device::cpu);
    eval(a, b, expected, magnitude);

    auto run = [&](int tf32) {
      auto scope = config::scoped_update({{"MLX_ENABLE_TF32", tf32}});
      auto out = matmul(a, b, Device::gpu);
      eval(out);
      return out;
    };
    auto within = [&](const array& out, float bound) {
      auto err = abs(subtract(out, expected, Device::cpu), Device::cpu);
      return all(less_equal(
                     err,
                     multiply(magnitude, array(bound), Device::cpu),
                     Device::cpu),
                 Device::cpu)
          .item<bool>();
    };

    // Off, on, off again: the setting is read at dispatch time, so each
    // change within the process must take effect.
    auto full = run(0);
    auto reduced = run(1);
    auto full_again = run(0);

    CHECK(within(full, float32_bound(K)));
    CHECK(within(reduced, tf32_bound(K)));
    CHECK(array_equal(full, full_again, Device::cpu).item<bool>());

    // Where the reduced-precision route exists it must actually be taken
    // when allowed; elsewhere both settings run the same kernel.
    bool same = array_equal(full, reduced, Device::cpu).item<bool>();
    if (metal::is_nax_available()) {
      CHECK_FALSE(same);
    } else {
      CHECK(same);
    }
  }
}
