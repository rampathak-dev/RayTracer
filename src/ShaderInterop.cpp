#include "headers/ShaderInterop.hpp"
#include "headers/Colour.hpp"
#include "headers/Hittable.hpp"
#include "headers/RenderJob.hpp"
#include "headers/Vector3.hpp"
#include <cassert>
#include <cstddef>
#include <cstdint>

ShaderInterop::Vec4f::Vec4f(const Vector3 &u) noexcept
    : x{static_cast<float>(u.x())}, y{static_cast<float>(u.y())},
      z{static_cast<float>(u.z())} {}

ShaderInterop::Vec4f::Vec4f(const double x_, const double y_,
                            const double z_) noexcept
    : x{static_cast<float>(x_)}, y{static_cast<float>(y_)},
      z{static_cast<float>(z_)} {}

SRGBColour ShaderInterop::ToSRGBColour(const ShaderInterop::Vec4f &u) noexcept {
  return SRGBColour{u.x, u.y, u.z};
}

ShaderInterop::ImageDimensions::ImageDimensions(
    const RenderData::ImageDimensions imageDimensions) noexcept
    : width{static_cast<std::uint32_t>(imageDimensions.width)},
      height{static_cast<std::uint32_t>(imageDimensions.height)} {}

ShaderInterop::Camera::Camera(const RenderData::Camera &camera) noexcept
    : center{camera.center}, pixel00{camera.pixel00},
      pixelDeltaU{camera.pixelDeltaU}, pixelDeltaV{camera.pixelDeltaV},
      defocusDiskU{camera.defocusDiskU}, defocusDiskV{camera.defocusDiskV} {}

ShaderInterop::RenderSettings::RenderSettings(
    const RenderData::RenderSettings &renderSettings) noexcept
    : recursionDepth{static_cast<uint32_t>(renderSettings.recursionDepth)},
      samplesPerPixel{static_cast<uint32_t>(renderSettings.samplesPerPixel)},
      shutterOpenTime{static_cast<float>(renderSettings.shutterOpenTime)},
      shutterCloseTime{static_cast<float>(renderSettings.shutterCloseTime)} {}

ShaderInterop::Environment::Environment(
    const Scene::Environment &environment) noexcept
    : skyColour{environment.skyColour}, groundColour{environment.groundColour} {
}

ShaderInterop::AABB::AABB(const ::AABB &box) noexcept {
  min = {static_cast<float>(box.AxisInterval(0).Min()),
         static_cast<float>(box.AxisInterval(1).Min()),
         static_cast<float>(box.AxisInterval(2).Min())};

  max = {static_cast<float>(box.AxisInterval(0).Max()),
         static_cast<float>(box.AxisInterval(1).Max()),
         static_cast<float>(box.AxisInterval(2).Max())};
}

ShaderInterop::AABB::AABB(const ShaderInterop::AABB &a, const ShaderInterop::AABB &b) noexcept {
  min = {std::min(a.min.x, b.min.x), std::min(a.min.y, b.min.y),
         std::min(a.min.z, b.min.z)};

  max = {std::max(a.max.x, b.max.x), std::max(a.max.y, b.max.y),
         std::max(a.max.z, b.max.z)};
}
