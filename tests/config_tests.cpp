// Copyright © 2026 Apple Inc.

#include "doctest/doctest.h"

#include <cstdlib>
#include <string>

#include "mlx/config.h"

using namespace mlx::core;

namespace {

std::string handler_name;
int handler_default;

int test_config_handler(const char* name, int default_val) {
  handler_name = name;
  handler_default = default_val;
  return 42;
}

class ConfigHandlerReset {
 public:
  ~ConfigHandlerReset() {
    config::register_config_handler(nullptr);
  }
};

} // namespace

TEST_CASE("test scoped config update") {
  constexpr auto name = "MLX_TEST_SCOPED_CONFIG_UPDATE";
  config::update(name, 1);
  CHECK_EQ(config::get(name, 0), 1);

  {
    auto outer = config::scoped_update({{name, 2}});
    CHECK_EQ(config::get(name, 0), 2);

    {
      auto inner = config::scoped_update({{name, 3}});
      CHECK_EQ(config::get(name, 0), 3);
    }

    CHECK_EQ(config::get(name, 0), 2);
  }

  CHECK_EQ(config::get(name, 0), 1);
}

TEST_CASE("test config default is not cached") {
  // The default can depend on the call site (e.g. the gated-delta chunk size
  // depends on the sequence length); the first caller's default must not
  // become the value every later caller sees.
  constexpr auto name = "MLX_TEST_CONFIG_DEFAULT_NOT_CACHED";
  CHECK_EQ(config::get(name, 1), 1);
  CHECK_EQ(config::get(name, 8), 8);
  CHECK_EQ(config::get(name, 16), 16);

  // An explicit update still wins over every default.
  config::update(name, 5);
  CHECK_EQ(config::get(name, 1), 5);
  CHECK_EQ(config::get(name, 8), 5);
}

TEST_CASE("test config environment is read once and update wins") {
  constexpr auto name = "MLX_TEST_CONFIG_ENV_ONCE";
  setenv(name, "13", 1);
  CHECK_EQ(config::get(name, 0), 13);
  // First-read semantics: a later change of the environment is not seen.
  setenv(name, "14", 1);
  CHECK_EQ(config::get(name, 0), 13);
  // An update overrides the environment value, and the default never applies.
  config::update(name, 2);
  CHECK_EQ(config::get(name, 99), 2);
  unsetenv(name);
}

TEST_CASE("test config handler") {
  ConfigHandlerReset reset;
  config::update("MLX_TEST_CONFIG_HANDLER", 1);
  config::register_config_handler(test_config_handler);

  CHECK_EQ(config::get("MLX_TEST_CONFIG_HANDLER", 7), 42);
  CHECK_EQ(handler_name, "MLX_TEST_CONFIG_HANDLER");
  CHECK_EQ(handler_default, 7);
}
