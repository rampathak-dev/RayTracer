#pragma once

#include "Colour.hpp"
#include "Config.hpp"
#include "Hittable.hpp"
#include <expected>
#include <span>
#include <string>
#include <string_view>

namespace Scene {
struct Environment {
  Colour skyColour = {};
  Colour groundColour = {};
};

struct Data {
  Config::Scene sceneConfig = {};
  Environment environment = {};
  HittableList world = {};
};
} // namespace Scene

namespace SceneCatalog {
struct SceneEntry {
  std::string_view name = "Hello, World!";
  std::expected<Scene::Data, std::string> (*create)();
};

[[nodiscard]] std::span<const SceneEntry> Get() noexcept;
} // namespace SceneCatalog
