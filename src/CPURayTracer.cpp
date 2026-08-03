#include "headers/Colour.hpp"
#include "headers/Hittable.hpp"
#include "headers/Material.hpp"
#include "headers/Math.hpp"
#include "headers/Ray.hpp"
#include "headers/RayTracer.hpp"
#include "headers/RenderJob.hpp"
#include "headers/Scene.hpp"
#include "headers/Vector3.hpp"
#include <cstddef>
#include <expected>
#include <memory>
#ifndef NDEBUG
#include <print>
#endif
#include "headers/Random.hpp"
#include <string>
#include <vector>

namespace {
class CPURayTracer : public RayTracer {
private:
  RenderJob mRenderJob;

  CPURayTracer(RenderJob) noexcept;

public:
  CPURayTracer() = delete;
  ~CPURayTracer() noexcept override = default;
  CPURayTracer(const CPURayTracer &) = delete;
  CPURayTracer &operator=(const CPURayTracer &) = delete;
  CPURayTracer(CPURayTracer &&) = default;
  CPURayTracer &operator=(CPURayTracer &&) = default;

  std::expected<Image, std::string> Render() const noexcept override;
  [[nodiscard]] Colour RayColour(const Ray &ray,
                                 const Hittable &hittable) const noexcept;
  [[nodiscard]] Ray RandomRay(std::size_t x, std::size_t y) const noexcept;
  [[nodiscard]] Vector3 SampleDefocusDisk() const noexcept;

  [[nodiscard]] static std::expected<std::unique_ptr<RayTracer>, std::string>
      Create(RenderJob) noexcept;
};

CPURayTracer::CPURayTracer(RenderJob job) noexcept
    : mRenderJob{std::move(job)} {}

std::expected<Image, std::string> CPURayTracer::Render() const noexcept {
  auto pixels = std::vector<SRGBColour>{};
  pixels.reserve(mRenderJob.imageDimensions.width *
                 mRenderJob.imageDimensions.height);

  const auto invSamplesPerPixel =
      1.0 / mRenderJob.renderSettings.samplesPerPixel;

  for (std::size_t y = 0; y < mRenderJob.imageDimensions.height; ++y) {
#ifndef NDEBUG
    std::println(stdout, "Scanlines remaining: {}",
                 mRenderJob.imageDimensions.height - y);
#endif

    for (std::size_t x = 0; x < mRenderJob.imageDimensions.width; ++x) {
      auto colour = Colour{};

      for (std::size_t _ = 0; _ < mRenderJob.renderSettings.samplesPerPixel;
           ++_) {
        const auto ray = this->RandomRay(x, y);
        colour += this->RayColour(ray, *mRenderJob.world);
      }

      colour *= invSamplesPerPixel;
      pixels.emplace_back(colour);
    }
  }

  return Image{.dimensions = mRenderJob.imageDimensions, .pixels = pixels};
}

Colour CPURayTracer::RayColour(const Ray &original,
                               const Hittable &hittable) const noexcept {
  auto ray = original;
  auto radiance = Colour{0, 0, 0};
  auto attenuation = Colour{1, 1, 1};

  std::size_t depth = 0;
  while (depth < mRenderJob.renderSettings.recursionDepth) {
    constexpr double tMin = 0.001;

    const auto hitRecord = hittable.Hit(ray, {tMin, Math::kInfinity});

    if (!hitRecord) {
      const auto unitRayDirection = Vector3::Normalize(ray.Direction());
      const auto interpolation = (unitRayDirection.y() + 1) * 0.5;

      radiance += attenuation *
                  ((1 - interpolation) * mRenderJob.environment.groundColour +
                   interpolation * mRenderJob.environment.skyColour);

      return radiance;
    }

    const auto emitted = hitRecord->material->Emit(hitRecord->u, hitRecord->v,
                                                   hitRecord->intersection);

    radiance += attenuation * emitted;

    const auto scatterRecord =
        hitRecord->material->Scatter(ray, hitRecord.value());

    if (!scatterRecord)
      return radiance;

    attenuation *= scatterRecord->albedo;
    ray = scatterRecord->ray;

    ++depth;
  }

  return {0, 0, 0};
}

Ray CPURayTracer::RandomRay(const std::size_t x,
                            const std::size_t y) const noexcept {
  const auto offset = Random::SampleUnitSquare();

  const auto sample = mRenderJob.camera.pixel00 +
                      ((x + offset.x()) * mRenderJob.camera.pixelDeltaU) +
                      ((y + offset.y()) * mRenderJob.camera.pixelDeltaV);

  const auto origin = this->SampleDefocusDisk();

  const auto time =
      mRenderJob.renderSettings.shutterOpenTime +
      Random::Double() * (mRenderJob.renderSettings.shutterCloseTime -
                          mRenderJob.renderSettings.shutterOpenTime);

  return {origin, sample - origin, time};
}

Vector3 CPURayTracer::SampleDefocusDisk() const noexcept {
  const auto u = Random::SampleUnitDisk();
  return mRenderJob.camera.center + u.x() * mRenderJob.camera.defocusDiskU +
         u.y() * mRenderJob.camera.defocusDiskV;
}

std::expected<std::unique_ptr<RayTracer>, std::string>
CPURayTracer::Create(RenderJob renderJob) noexcept {
  return std::unique_ptr<CPURayTracer>(new CPURayTracer{std::move(renderJob)});
}

struct Registrar {
  Registrar() noexcept {
    RayTracer::Register(RenderMode::CPU, &CPURayTracer::Create);
  }
} registrar;
} // namespace
