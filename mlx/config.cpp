// Copyright © 2026 Apple Inc.

#include "mlx/config.h"

#include <cstdlib>
#include <mutex>
#include <optional>
#include <shared_mutex>
#include <tuple>

namespace mlx::core::config {

namespace {

// Values read from the environment, cached on first read. A variable that is
// not set is cached as std::nullopt: the caller's default is never stored,
// because defaults can depend on the call site (e.g. the gated-delta chunk
// size depends on the sequence length).
auto& env_cache() {
  static std::tuple<
      std::unordered_map<std::string, std::optional<int>>,
      std::shared_mutex>
      cache;
  return cache;
}

std::optional<int> get_env(const char* name) {
  auto& [cache, mtx] = env_cache();
  {
    std::shared_lock lock(mtx);
    if (auto it = cache.find(name); it != cache.end()) {
      return it->second;
    }
  }
  std::optional<int> val;
  if (const char* str = std::getenv(name)) {
    val = std::atoi(str);
  }
  std::unique_lock lock(mtx);
  return cache.emplace(name, val).first->second;
}

auto& global_configs() {
  static std::tuple<ConfigDict, std::shared_mutex> configs_and_mtx;
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
  configs[name] = val;
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
  // Then try global configs.
  auto& [configs, mtx] = global_configs();
  {
    std::shared_lock lock(mtx);
    if (auto it = configs.find(name); it != configs.end()) {
      return it->second;
    }
  }
  // Then the environment. Only a value that is set is kept; a default is
  // returned as is, never cached.
  return get_env(name).value_or(default_val);
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
