#pragma once

#include "Vector3.hpp"
#include <cstddef>

namespace Config {
struct Camera {
  Point3 lookFrom = {};
  Point3 lookAt = {};
  double viewAngle = 0;
  double defocusAngle = 0;
  double focusDistance = 0;
};

struct Image {
  std::size_t width = 0;
  double aspectRatio = 0;
};

struct Scene {
  Config::Camera camera = {};
  Config::Image image = {};
  double motionStart = 0;
  double motionEnd = 0;
};

struct Rendering {
  static constexpr std::size_t kMaxRecursionDepth = 100;
  static constexpr std::size_t kMaxSamplesPerPixel = 10000;

  std::size_t recursionDepth = 100;
  std::size_t samplesPerPixel = 100;
};
} // namespace Config
