#pragma once

#include "Colour.hpp"
#include "Hittable.hpp"
#include "Math.hpp"
#include "RenderJob.hpp"
#include "Vector3.hpp"
#include <cstddef>
#include <cstdint>
#include <limits>
#include <utility>

namespace ShaderInterop {
enum class TextureKind : std::uint32_t {
  SolidColour = 0,
  CheckerTexture,
  ImageTexture,
  NoiseTexture
};

enum class MaterialKind : std::uint32_t {
  Lambertian = 0,
  Metal,
  Dielectric,
  DiffuseLight,
  Isotropic
};

enum class VolumeKind : std::uint32_t { ConstantMedium = 0 };

enum class ObjectKind : std::uint32_t {
  Sphere = 0,
  MovingSphere,
  Parallelogram
};

constexpr auto kUint32Sentinel = std::numeric_limits<std::uint32_t>::max();
constexpr auto kIsVolumeBoundary = kUint32Sentinel;
constexpr auto kNoObject = kUint32Sentinel;
constexpr std::uint32_t kComputePassWorkGroupSize = 16;

struct alignas(16) Vec4f {
  float x = 0;
  float y = 0;
  float z = 0;
  float w = 0;

  Vec4f() = default;
  ~Vec4f() = default;
  Vec4f(const Vec4f &) = default;
  Vec4f &operator=(const Vec4f &) = default;
  Vec4f(Vec4f &&) noexcept = default;
  Vec4f &operator=(Vec4f &&) noexcept = default;

  Vec4f(const Vector3 &) noexcept;
  Vec4f(double x, double y, double z) noexcept;

  [[nodiscard]] float operator[](const std::size_t axis) const noexcept;
};

[[nodiscard]] SRGBColour ToSRGBColour(const Vec4f &u) noexcept;

struct ImageDimensions {
  std::uint32_t width = 0;
  std::uint32_t height = 0;
  std::uint32_t : 32;
  std::uint32_t : 32;

  ImageDimensions() = default;
  ~ImageDimensions() = default;
  ImageDimensions(const ImageDimensions &) = default;
  ImageDimensions &operator=(const ImageDimensions &) = default;
  ImageDimensions(ImageDimensions &&) noexcept = default;
  ImageDimensions &operator=(ImageDimensions &&) noexcept = default;

  ImageDimensions(RenderData::ImageDimensions) noexcept;
};

struct Camera {
  Vec4f center = {};
  Vec4f pixel00 = {};
  Vec4f pixelDeltaU = {};
  Vec4f pixelDeltaV = {};
  Vec4f defocusDiskU = {};
  Vec4f defocusDiskV = {};

  Camera(const RenderData::Camera &) noexcept;

  Camera() = default;
  ~Camera() = default;
  Camera(const Camera &) = default;
  Camera &operator=(const Camera &) = default;
  Camera(Camera &&) noexcept = default;
  Camera &operator=(Camera &&) noexcept = default;
};

struct RenderSettings {
  std::uint32_t recursionDepth = 0;
  std::uint32_t samplesPerPixel = 0;
  float shutterOpenTime = 0;
  float shutterCloseTime = 0;

  RenderSettings() = default;
  ~RenderSettings() = default;
  RenderSettings(const RenderSettings &) = default;
  RenderSettings &operator=(const RenderSettings &) = default;
  RenderSettings(RenderSettings &&) noexcept = default;
  RenderSettings &operator=(RenderSettings &&) noexcept = default;

  RenderSettings(const RenderData::RenderSettings &) noexcept;
};

struct Environment {
  Vec4f skyColour = {};
  Vec4f groundColour = {};

  Environment() = default;
  ~Environment() = default;
  Environment(const Environment &) = default;
  Environment &operator=(const Environment &) = default;
  Environment(Environment &&) noexcept = default;
  Environment &operator=(Environment &&) noexcept = default;

  Environment(const Scene::Environment &) noexcept;
};

struct Texture {
  Vec4f albedo = {};
  std::uint32_t evenIndex = 0;
  std::uint32_t oddIndex = 0;
  float invScale = 1;
  std::uint32_t width = 0;
  std::uint32_t height = 0;
  std::uint32_t pixelOffset = 0;
  std::uint32_t perlinOffset = 0;
  float noiseScale = 1;
  float distortion = 1;
  std::uint32_t turbulenceDepth = 1;
  std::uint32_t kind = std::to_underlying(TextureKind::SolidColour);
};

struct TextureListHeader {
  std::uint32_t count = 0;
  std::uint32_t : 32;
  std::uint32_t : 32;
  std::uint32_t : 32;
};

struct WGPUPixelDataHeader {
  std::uint32_t maxOffset = 0;
  std::uint32_t : 32;
  std::uint32_t : 32;
  std::uint32_t : 32;
};

struct PerlinNoiseValuesHeader {
  std::uint32_t maxOffset = 0;
  std::uint32_t pointCount = 256;
  std::uint32_t : 32;
  std::uint32_t : 32;
};

struct PerlinPermutationsHeader {
  std::uint32_t maxOffset = 0;
  std::uint32_t : 32;
  std::uint32_t : 32;
  std::uint32_t : 32;
};

struct Material {
  Vec4f albedo = {};
  std::uint32_t textureIndex = 0;
  float fuzz = 0;
  float etaRatio = 1;
  std::uint32_t kind = 0;
};

struct MaterialListHeader {
  std::uint32_t count = 0;
  std::uint32_t : 32;
  std::uint32_t : 32;
  std::uint32_t : 32;
};

struct Volume {
  std::uint32_t materialIndex = 0;
  float negInvDensity = 0;
  std::uint32_t boundaryIndex = 0;
  std::uint32_t boundaryCount = 0;
  std::uint32_t kind = 0;
};

struct VolumeListHeader {
  std::uint32_t count = 0;
  std::uint32_t : 32;
  std::uint32_t : 32;
  std::uint32_t : 32;
};

struct Object {
  Vec4f center0 = {};
  Vec4f center1 = {};
  Vec4f Q = {};
  Vec4f u = {};
  Vec4f v = {};
  Vec4f n = {};
  Vec4f w = {};
  float radius = 0;
  float time0 = 0;
  float time1 = 0;
  float D = 0;
  std::uint32_t materialIndex = 0;
  std::uint32_t isVolumeBoundary = ~ShaderInterop::kIsVolumeBoundary;
  std::uint32_t kind = 0;
};

struct ObjectListHeader {
  std::uint32_t count = 0;
  std::uint32_t : 32;
  std::uint32_t : 32;
  std::uint32_t : 32;
};

struct AABB {
  Vec4f min = {Math::kInfinity, Math::kInfinity, Math::kInfinity};
  Vec4f max = {-Math::kInfinity, -Math::kInfinity, -Math::kInfinity};

  AABB() = default;
  ~AABB() = default;
  AABB(const AABB &) = default;
  AABB &operator=(const AABB &) = default;
  AABB(AABB &&) = default;
  AABB &operator=(AABB &&) = default;

  AABB(const ::AABB &) noexcept;
  AABB(const AABB &a, const AABB &b) noexcept;
};

struct BVHNodeListHeader {
  std::uint32_t count = 0;
  std::uint32_t rootIndex = 0;
  std::uint32_t : 32;
  std::uint32_t : 32;
};

struct BVHNode {
  AABB aabb = {};
  std::uint32_t leftIndex = 0;
  std::uint32_t rightIndex = 0;
  std::uint32_t objectIndex = ShaderInterop::kNoObject;
};
} // namespace ShaderInterop
