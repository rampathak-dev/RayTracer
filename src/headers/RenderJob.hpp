#pragma once

#include "Config.hpp"
#include "Hittable.hpp"
#include "Scene.hpp"
#include "Vector3.hpp"
#include <cstddef>
#include <memory>

namespace RenderData {
struct ImageDimensions {
  std::size_t width = 0;
  std::size_t height = 0;
};

struct Camera {
  Point3 center = {};
  Point3 pixel00 = {};
  Vector3 pixelDeltaU = {};
  Vector3 pixelDeltaV = {};
  Vector3 defocusDiskU = {};
  Vector3 defocusDiskV = {};

  Camera(const Config::Camera &, const RenderData::ImageDimensions &) noexcept;

  Camera() = default;
  ~Camera() = default;
  Camera(const Camera &) = default;
  Camera &operator=(const Camera &) = default;
  Camera(Camera &&) noexcept = default;
  Camera &operator=(Camera &&) noexcept = default;
};

struct RenderSettings {
  double shutterOpenTime = 0;
  double shutterCloseTime = 0;
  std::size_t recursionDepth = 0;
  std::size_t samplesPerPixel = 0;
};
}; // namespace RenderData

struct RenderJob {
  RenderData::Camera camera = {};
  Scene::Environment environment = {};
  RenderData::RenderSettings renderSettings = {};
  RenderData::ImageDimensions imageDimensions = {};
  std::unique_ptr<Hittable> world = nullptr;

  RenderJob(Scene::Data, const Config::Rendering &) noexcept;

  RenderJob() = delete;
  ~RenderJob() = default;
  RenderJob(const RenderJob &) = delete;
  RenderJob &operator=(const RenderJob &) = delete;
  RenderJob(RenderJob &&) noexcept = default;
  RenderJob &operator=(RenderJob &&) noexcept = default;
};
