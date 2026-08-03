#pragma once

#include "GPUContext.hpp"
#include "Hittable.hpp"
#include "HittableVisitor.hpp"
#include "Material.hpp"
#include "MaterialVisitor.hpp"
#include "ShaderInterop.hpp"
#include "Texture.hpp"
#include "TextureVisitor.hpp"
#include "Vector3.hpp"
#include <cstdint>
#include <unordered_map>

class ShaderPayloadBuilder : public TextureVisitor,
                             public MaterialVisitor,
                             public HittableVisitor {
private:
  struct BVHEntry {
    std::uint32_t objectIndex = 0;
    AABB box = AABB::Empty();
  };

  struct RotateYRecord {
    double theta = 0;
    double sinTheta = 0;
    double cosTheta = 1;
  };

  GPUContext::ShaderPayload mPayload = {};
  std::unordered_map<const Texture *, std::uint32_t> mTextureIndices = {};
  std::unordered_map<const Material *, std::uint32_t> mMaterialIndices = {};
  std::vector<BVHEntry> mBVHEntries = {};
  Vector3 mTranslateOffset = {};
  RotateYRecord mRotateYRecord = {};
  double mTime0 = 0;
  double mTime1 = 0;
  std::uint32_t mIsVolumeBoundary = ~ShaderInterop::kIsVolumeBoundary;

  [[nodiscard]] std::size_t BuildBVHNodes(std::span<BVHEntry> entries) noexcept;

  [[nodiscard]] std::uint32_t MapTexture(const Texture &) noexcept;
  [[nodiscard]] std::uint32_t MapMaterial(const Material &) noexcept;

public:
  ShaderPayloadBuilder() = delete;
  ~ShaderPayloadBuilder() override = default;
  ShaderPayloadBuilder(const ShaderPayloadBuilder &) = delete;
  ShaderPayloadBuilder &operator=(const ShaderPayloadBuilder &) = delete;
  ShaderPayloadBuilder(ShaderPayloadBuilder &&) noexcept = default;
  ShaderPayloadBuilder &operator=(ShaderPayloadBuilder &&) noexcept = default;

  ShaderPayloadBuilder(const Hittable &, double time0, double time1) noexcept;

  void Visit(const SolidColour &) noexcept override;
  void Visit(const CheckerTexture &) noexcept override;
  void Visit(const ImageTexture &) noexcept override;
  void Visit(const NoiseTexture &) noexcept override;
  void Visit(const Lambertian &) noexcept override;
  void Visit(const Metal &) noexcept override;
  void Visit(const Dielectric &) noexcept override;
  void Visit(const DiffuseLight &) noexcept override;
  void Visit(const Isotropic &) noexcept override;
  void Visit(const NullHittable &) noexcept override;
  void Visit(const Translate &) noexcept override;
  void Visit(const RotateY &) noexcept override;
  void Visit(const ConstantMedium &) noexcept override;
  void Visit(const Sphere &) noexcept override;
  void Visit(const MovingSphere &) noexcept override;
  void Visit(const Parallelogram &) noexcept override;
  void Visit(const HittableList &) noexcept override;
  void Visit(const BVHNode &) noexcept override;

  [[nodiscard]] GPUContext::ShaderPayload TakePayload() noexcept;
};
