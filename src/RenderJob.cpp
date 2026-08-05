#include "headers/RenderJob.hpp"
#include "headers/Config.hpp"
#include "headers/Hittable.hpp"
#include "headers/Scene.hpp"
#include "headers/Vector3.hpp"
#include <cmath>
#include <cstddef>
#include <utility>

RenderData::Camera::Camera(const Config::Camera &config,
                           const ImageDimensions &dimensions) noexcept {
  center = config.lookFrom;

  auto vUp = Vector3{0, 1, 0};
  const auto w = Vector3::Normalize(config.lookFrom - config.lookAt);
  // if w = 0, then our entire basis becomes zero
  // this only happens if lookAt - lookFrom aligns with vUp, so we change vUp to
  // accomodate, this creates inconsistency regarding the orientation in this
  // particular case, compared to the rest of the cases but its the most
  // pragmatic solution for defining orientation using the this method
  if ((w - vUp).NearZero() || (w + vUp).NearZero())
    vUp = {0, 0, 1};

  const auto u = Vector3::Normalize(Vector3::Cross(vUp, w));
  const auto v = Vector3::Cross(w, u);

  const auto halfHeightByFocusDistance = std::tan(config.viewAngle / 2);
  const auto viewportHeight =
      2 * config.focusDistance * halfHeightByFocusDistance;
  const auto viewportWidth =
      viewportHeight *
      (static_cast<double>(dimensions.width) / dimensions.height);

  const auto viewportU = viewportWidth * u;
  const auto viewportV = -viewportHeight * v;

  pixelDeltaU = viewportU / static_cast<double>(dimensions.width);
  pixelDeltaV = viewportV / static_cast<double>(dimensions.height);

  const auto viewportUpperLeft = config.lookFrom - config.focusDistance * w -
                                 (viewportU + viewportV) * 0.5;
  pixel00 = viewportUpperLeft + (pixelDeltaU + pixelDeltaV) * 0.5;

  const auto halfAperture =
      std::tan(config.defocusAngle / 2) * config.focusDistance;

  defocusDiskU = u * halfAperture;
  defocusDiskV = v * halfAperture;
}

RenderJob::RenderJob(Scene::Data data,
                     const Config::Rendering &config) noexcept {
  environment = data.environment;

  renderSettings = {.shutterOpenTime = data.sceneConfig.motionStart,
                    .shutterCloseTime = data.sceneConfig.motionEnd,
                    .recursionDepth = config.recursionDepth,
                    .samplesPerPixel = config.samplesPerPixel};

  world = BVHNode::Build(std::move(data.world), renderSettings.shutterOpenTime,
                         renderSettings.shutterCloseTime);

  imageDimensions = {
      .width = data.sceneConfig.image.width,
      .height = static_cast<std::size_t>(data.sceneConfig.image.width /
                                         data.sceneConfig.image.aspectRatio)};

  imageDimensions.height =
      imageDimensions.height == 0 ? 1 : imageDimensions.height;

  camera = {data.sceneConfig.camera, imageDimensions};
}
