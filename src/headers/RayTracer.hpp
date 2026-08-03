#pragma once

#include "Colour.hpp"
#include "RenderJob.hpp"
#include <cstddef>
#include <expected>
#include <functional>
#include <memory>
#include <string>

enum class RenderMode : std::size_t { CPU = 1, GPU };

struct Image {
  RenderData::ImageDimensions dimensions = {};
  std::vector<SRGBColour> pixels = {};
};

class RayTracer {
public:
  RayTracer() = default;
  virtual ~RayTracer() = default;
  RayTracer(const RayTracer &) = delete;
  RayTracer &operator=(const RayTracer &) = delete;
  RayTracer(RayTracer &&) noexcept = default;
  RayTracer &operator=(RayTracer &&) noexcept = default;

  [[nodiscard]] virtual std::expected<Image, std::string>
  Render() const noexcept = 0;

  using FactoryFunction =
      std::function<std::expected<std::unique_ptr<RayTracer>, std::string>(
          RenderJob)>;
  static void Register(RenderMode, FactoryFunction) noexcept;

  [[nodiscard]] static std::expected<std::unique_ptr<RayTracer>, std::string>
      Create(RenderMode, RenderJob) noexcept;
};
