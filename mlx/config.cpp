// Copyright © 2026 Apple Inc.

#include "mlx/config.h"

#include <cstdlib>
#include <mutex>
#include <optional>
#include <shared_mutex>
#include <string>
#include <string_view>
#include <tuple>
#include <unordered_map>

namespace mlx::core::config {

namespace {

// One entry per setting name. `updated` is a value set with update();
// `env` is the value read from the environment on first use (first-read
// semantics; an unset variable is remembered as nullopt). The caller's default
// is never stored: it can depend on the call site (e.g. the gated-delta chunk
// size depends on the sequence length), so caching it would pin the first
// caller's value for the whole process.
struct Entry {
  std::optional<int> updated;
  std::optional<int> env;
  bool env_read = false;
};

// Keyed by std::string, looked up by std::string_view (transparent hash and
// equality): a lookup allocates nothing. config::get is called on hot paths
// (once per matmul/attention evaluation), and this keeps it a single lookup.
struct StringHash {
  using is_transparent = void;
  size_t operator()(std::string_view s) const {
    return std::hash<std::string_view>{}(s);
  }
};

using Entries =
    std::unordered_map<std::string, Entry, StringHash, std::equal_to<>>;

auto& global_configs() {
  static std::tuple<Entries, std::shared_mutex> configs_and_mtx;
  return configs_and_mtx;
}

auto& scoped_configs() {
  thread_local std::list<ConfigDict> configs;
  return configs;
}

auto& config_handler() {
  static ConfigHandler handler = nullptr;
  return handler;
}

} // namespace

void update(const char* name, int val) {
  auto& [configs, mtx] = global_configs();
  std::unique_lock lock(mtx);
  configs[name].updated = val;
}

int get(const char* name, int default_val) {
  // First pass to the hook.
  if (auto& handler = config_handler()) {
    return handler(name, default_val);
  }
  // Then try scoped configs.
  auto& list = scoped_configs();
  for (auto& configs : list) {
    if (auto it = configs.find(name); it != configs.end()) {
      return it->second;
    }
  }
  // Then the global value: an update(), else the environment.
  auto& [configs, mtx] = global_configs();
  {
    std::shared_lock lock(mtx);
    if (auto it = configs.find(std::string_view(name)); it != configs.end()) {
      const Entry& e = it->second;
      if (e.updated) {
        return *e.updated;
      }
      if (e.env_read) {
        return e.env.value_or(default_val);
      }
    }
  }
  // First use of this name: read the environment once.
  std::optional<int> env;
  if (const char* str = std::getenv(name)) {
    env = std::atoi(str);
  }
  std::unique_lock lock(mtx);
  Entry& e = configs[name];
  if (!e.env_read) {
    e.env = env;
    e.env_read = true;
  }
  return e.updated ? *e.updated : e.env.value_or(default_val);
}

ScopedUpdate::ScopedUpdate(ConfigDict configs) {
  auto& list = scoped_configs();
  list.push_front(std::move(configs));
  it_ = list.begin();
}

ScopedUpdate::ScopedUpdate(const ScopedUpdate& other) {
  auto& list = scoped_configs();
  list.push_front(*other.it_);
  it_ = list.begin();
}

ScopedUpdate::~ScopedUpdate() {
  auto& list = scoped_configs();
  list.erase(it_);
}

void register_config_handler(ConfigHandler handler) {
  config_handler() = handler;
}

} // namespace mlx::core::config
