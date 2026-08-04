#include "headers/ShaderPayloadBuilder.hpp"
#include "headers/GPUContext.hpp"
#include "headers/Hittable.hpp"
#include "headers/Perlin.hpp"
#include "headers/ShaderInterop.hpp"
#include "headers/Texture.hpp"
#include "headers/Vector3.hpp"
#include <algorithm>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <unordered_map>
#include <utility>
#include <vector>

std::size_t
ShaderPayloadBuilder::BuildBVHNodes(std::span<BVHEntry> entries) noexcept {
  const auto n = entries.size();

  if (n == 0) {
    mPayload.nodes.push_back({
        .aabb = AABB::Empty(),
        .leftIndex = 0,
        .rightIndex = 0,
        .objectIndex = ShaderInterop::kNoObject,
    });
    return mPayload.nodes.size() - 1;
  }

  auto allBox = AABB::Empty();

  for (const auto entry : entries)
    allBox = AABB{allBox, entry.box};

  const auto axis = allBox.LongestAxis();

  const auto boxComparator = [axis](const BVHEntry &a,
                                    const BVHEntry &b) noexcept {
    return a.box.AxisInterval(axis).Midpoint() <
           b.box.AxisInterval(axis).Midpoint();
  };

  const auto mid = n / 2;
  std::ranges::nth_element(entries, entries.begin() + mid, boxComparator);

  if (n == 1) {
    mPayload.nodes.push_back(
        {.aabb = entries[0].box, .objectIndex = entries[0].objectIndex});
  } else {
    const auto leftIndex = BuildBVHNodes(entries.subspan(0, mid));
    const auto rightIndex = BuildBVHNodes(entries.subspan(mid));

    const auto &leftNode = mPayload.nodes[leftIndex];
    const auto &rightNode = mPayload.nodes[rightIndex];
    const auto aabb = ShaderInterop::AABB{leftNode.aabb, rightNode.aabb};

    mPayload.nodes.push_back({
        .aabb = aabb,
        .leftIndex = static_cast<std::uint32_t>(leftIndex),
        .rightIndex = static_cast<std::uint32_t>(rightIndex),
        .objectIndex = ShaderInterop::kNoObject,
    });
  }

  return mPayload.nodes.size() - 1;
}

std::uint32_t
ShaderPayloadBuilder::MapTexture(const Texture &texture) noexcept {
  if (const auto it = mTextureIndices.find(&texture);
      it != mTextureIndices.end())
    return it->second;

  texture.Accept(*this);

  const auto index = static_cast<std::uint32_t>(mPayload.textures.size() - 1);
  mTextureIndices[&texture] = index;

  return index;
}

std::uint32_t
ShaderPayloadBuilder::MapMaterial(const Material &material) noexcept {
  if (const auto it = mMaterialIndices.find(&material);
      it != mMaterialIndices.end())
    return it->second;

  material.Accept(*this);

  const auto index = static_cast<std::uint32_t>(mPayload.materials.size() - 1);
  mMaterialIndices[&material] = index;

  return index;
}

ShaderPayloadBuilder::ShaderPayloadBuilder(const Hittable &hittable,
                                           const double time0,
                                           const double time1) noexcept
    : mTime0{time0}, mTime1{time1} {
  hittable.Accept(*this);

  mPayload.nodes.reserve(mPayload.objects.size() * 2 - 1);

  mPayload.rootNodeIndex =
      static_cast<std::uint32_t>(this->BuildBVHNodes(mBVHEntries));
}

void ShaderPayloadBuilder::Visit(const SolidColour &solidColour) noexcept {
  mPayload.textures.push_back({.albedo = solidColour.Albedo(),
                               .kind = static_cast<std::uint32_t>(
                                   ShaderInterop::TextureKind::SolidColour)});
}

void ShaderPayloadBuilder::Visit(
    const CheckerTexture &checkerTexture) noexcept {
  mPayload.textures.push_back(
      {.evenIndex = this->MapTexture(*checkerTexture.EvenTexture()),
       .oddIndex = this->MapTexture(*checkerTexture.OddTexture()),
       .invScale = static_cast<float>(checkerTexture.InvScale()),
       .kind = static_cast<std::uint32_t>(
           ShaderInterop::TextureKind::CheckerTexture)});
}

void ShaderPayloadBuilder::Visit(const ImageTexture &imageTexture) noexcept {
  const auto &image = imageTexture.Image();

  const auto width = static_cast<std::uint32_t>(image.Width());
  const auto height = static_cast<std::uint32_t>(image.Height());
  const auto pixelOffset =
      static_cast<std::uint32_t>(mPayload.pixelData.size());

  for (std::uint32_t y = 0; y < height; ++y)
    for (std::uint32_t x = 0; x < width; ++x) {
      const auto pixel = image.PixelData(x, y);
      const auto packed = static_cast<std::uint32_t>(pixel[0]) |
                          (static_cast<std::uint32_t>(pixel[1]) << 8) |
                          (static_cast<std::uint32_t>(pixel[2]) << 16);

      mPayload.pixelData.push_back(packed);
    }

  mPayload.textures.push_back({.width = width,
                               .height = height,
                               .pixelOffset = pixelOffset,
                               .kind = static_cast<std::uint32_t>(
                                   ShaderInterop::TextureKind::ImageTexture)});
}

void ShaderPayloadBuilder::Visit(const NoiseTexture &noiseTexture) noexcept {
  const auto &perlin = noiseTexture.GetPerlin();

  const auto perlinOffset =
      static_cast<std::uint32_t>(mPayload.perlinNoiseValues.size());

  for (const auto value : perlin.NoiseValues())
    mPayload.perlinNoiseValues.emplace_back(value);

  for (const auto value : perlin.XPermutation())
    mPayload.perlinPermutations.push_back(static_cast<std::uint32_t>(value));
  for (const auto value : perlin.YPermutation())
    mPayload.perlinPermutations.push_back(static_cast<std::uint32_t>(value));
  for (const auto value : perlin.ZPermutation())
    mPayload.perlinPermutations.push_back(static_cast<std::uint32_t>(value));

  mPayload.textures.push_back(
      {.perlinOffset = perlinOffset,
       .noiseScale = static_cast<float>(noiseTexture.Scale()),
       .distortion = static_cast<float>(noiseTexture.Distortion()),
       .turbulenceDepth =
           static_cast<std::uint32_t>(noiseTexture.TurbulenceDepth()),
       .kind = static_cast<std::uint32_t>(
           ShaderInterop::TextureKind::NoiseTexture)});
}

void ShaderPayloadBuilder::Visit(const Lambertian &lambertian) noexcept {
  mPayload.materials.push_back(
      {.textureIndex = this->MapTexture(lambertian.GetTexture()),
       .kind = std::to_underlying(ShaderInterop::MaterialKind::Lambertian)});
}

void ShaderPayloadBuilder::Visit(const Metal &metal) noexcept {
  mPayload.materials.push_back(
      {.albedo = metal.Albedo(),
       .fuzz = static_cast<float>(metal.Fuzz()),
       .kind = std::to_underlying(ShaderInterop::MaterialKind::Metal)});
}

void ShaderPayloadBuilder::Visit(const Dielectric &dielectric) noexcept {
  mPayload.materials.push_back(
      {.etaRatio = static_cast<float>(dielectric.EtaRatio()),
       .kind = std::to_underlying(ShaderInterop::MaterialKind::Dielectric)});
}

void ShaderPayloadBuilder::Visit(const DiffuseLight &diffuseLight) noexcept {
  mPayload.materials.push_back(
      {.textureIndex = this->MapTexture(diffuseLight.GetTexture()),
       .kind = std::to_underlying(ShaderInterop::MaterialKind::DiffuseLight)});
}

void ShaderPayloadBuilder::Visit(const Isotropic &isotropic) noexcept {
  mPayload.materials.push_back(
      {.textureIndex = this->MapTexture(isotropic.GetTexture()),
       .kind = std::to_underlying(ShaderInterop::MaterialKind::Isotropic)});
}

void ShaderPayloadBuilder::Visit(const NullHittable &) noexcept {
  // do nothing
}

void ShaderPayloadBuilder::Visit(const HittableList &list) noexcept {
  const auto &objects = list.Objects();

  for (const auto &object : objects)
    object->Accept(*this);
}

void ShaderPayloadBuilder::Visit(const BVHNode &node) noexcept {
  node.Left().Accept(*this);
  node.Right().Accept(*this);
}

void ShaderPayloadBuilder::Visit(const Translate &translate) noexcept {
  const auto &offset = translate.Offset();
  mTranslateOffset += offset;

  translate.Object().Accept(*this);

  mTranslateOffset -= offset;
}

void ShaderPayloadBuilder::Visit(const RotateY &rotateY) noexcept {
  const auto sin = mRotateYRecord.sinTheta;
  const auto cos = mRotateYRecord.cosTheta;

  const auto theta = std::atan2(rotateY.SinTheta(), rotateY.CosTheta());

  mRotateYRecord.theta += theta;
  mRotateYRecord.sinTheta = std::sin(mRotateYRecord.theta);
  mRotateYRecord.cosTheta = std::cos(mRotateYRecord.theta);

  rotateY.Object().Accept(*this);

  mRotateYRecord.theta -= theta;
  mRotateYRecord.sinTheta = sin;
  mRotateYRecord.cosTheta = cos;
}

void ShaderPayloadBuilder::Visit(const ConstantMedium &medium) noexcept {
  const auto previous = mIsVolumeBoundary;
  mIsVolumeBoundary = ShaderInterop::kIsVolumeBoundary;

  const auto boundaryIndex = mPayload.objects.size();

  medium.Boundary().Accept(*this);

  mPayload.volumes.push_back(
      {.materialIndex = this->MapMaterial(medium.PhaseFunction()),
       .negInvDensity = static_cast<float>(medium.NegInvDensity()),
       .boundaryIndex = static_cast<uint32_t>(boundaryIndex),
       .boundaryCount =
           static_cast<std::uint32_t>(mPayload.objects.size() - boundaryIndex),
       .kind = std::to_underlying(ShaderInterop::VolumeKind::ConstantMedium)});

  mIsVolumeBoundary = previous;
}

void ShaderPayloadBuilder::Visit(const Sphere &sphere) noexcept {
  const auto center = sphere.Center();

  const auto newCenter =
      mTranslateOffset + Point3{center.x() * mRotateYRecord.cosTheta +
                                    center.z() * mRotateYRecord.sinTheta,
                                center.y(),
                                -mRotateYRecord.sinTheta * center.x() +
                                    mRotateYRecord.cosTheta * center.z()};

  mPayload.objects.push_back(
      {.center0 = newCenter,
       .radius = static_cast<float>(sphere.Radius()),
       .materialIndex = this->MapMaterial(*sphere.GetMaterial()),
       .isVolumeBoundary = mIsVolumeBoundary,
       .kind = std::to_underlying(ShaderInterop::ObjectKind::Sphere)});

  if (mIsVolumeBoundary == ShaderInterop::kIsVolumeBoundary)
    return;

  mBVHEntries.push_back(
      {.objectIndex = static_cast<std::uint32_t>(mPayload.objects.size() - 1),
       .box = {newCenter +
                   Vector3{sphere.Radius(), sphere.Radius(), sphere.Radius()},
               newCenter - Vector3{sphere.Radius(), sphere.Radius(),
                                   sphere.Radius()}}});
}

void ShaderPayloadBuilder::Visit(const MovingSphere &movingSphere) noexcept {
  const auto center0 = movingSphere.Center0();

  const auto newCenter0 =
      mTranslateOffset + Point3{center0.x() * mRotateYRecord.cosTheta +
                                    center0.z() * mRotateYRecord.sinTheta,
                                center0.y(),
                                -mRotateYRecord.sinTheta * center0.x() +
                                    mRotateYRecord.cosTheta * center0.z()};

  const auto center1 = movingSphere.Center1();

  const auto newCenter1 =
      mTranslateOffset + Point3{center1.x() * mRotateYRecord.cosTheta +
                                    center1.z() * mRotateYRecord.sinTheta,
                                center1.y(),
                                -mRotateYRecord.sinTheta * center1.x() +
                                    mRotateYRecord.cosTheta * center1.z()};

  mPayload.objects.push_back(
      {.center0 = newCenter0,
       .center1 = newCenter1,
       .radius = static_cast<float>(movingSphere.Radius()),
       .time0 = static_cast<float>(movingSphere.Time0()),
       .time1 = static_cast<float>(movingSphere.Time1()),
       .materialIndex = this->MapMaterial(*movingSphere.GetMaterial()),
       .isVolumeBoundary = mIsVolumeBoundary,
       .kind = std::to_underlying(ShaderInterop::ObjectKind::MovingSphere)});

  if (mIsVolumeBoundary == ShaderInterop::kIsVolumeBoundary)
    return;

  const auto box0 =
      AABB{newCenter0 + Vector3{movingSphere.Radius(), movingSphere.Radius(),
                                movingSphere.Radius()},
           newCenter0 - Vector3{movingSphere.Radius(), movingSphere.Radius(),
                                movingSphere.Radius()}};

  const auto box1 =
      AABB{newCenter1 + Vector3{movingSphere.Radius(), movingSphere.Radius(),
                                movingSphere.Radius()},
           newCenter1 - Vector3{movingSphere.Radius(), movingSphere.Radius(),
                                movingSphere.Radius()}};

  mBVHEntries.push_back(
      {.objectIndex = static_cast<std::uint32_t>(mPayload.objects.size() - 1),
       .box = AABB{box0, box1}});
};

void ShaderPayloadBuilder::Visit(const Parallelogram &parallelogram) noexcept {
  const auto Q = parallelogram.Q();
  const auto newQ =
      mTranslateOffset +
      Point3{Q.x() * mRotateYRecord.cosTheta + Q.z() * mRotateYRecord.sinTheta,
             Q.y(),
             -mRotateYRecord.sinTheta * Q.x() +
                 mRotateYRecord.cosTheta * Q.z()};

  const auto u = parallelogram.U();
  const auto newU = Point3{
      u.x() * mRotateYRecord.cosTheta + u.z() * mRotateYRecord.sinTheta, u.y(),
      -mRotateYRecord.sinTheta * u.x() + mRotateYRecord.cosTheta * u.z()};

  const auto v = parallelogram.V();
  const auto newV = Point3{
      v.x() * mRotateYRecord.cosTheta + v.z() * mRotateYRecord.sinTheta, v.y(),
      -mRotateYRecord.sinTheta * v.x() + mRotateYRecord.cosTheta * v.z()};

  const auto newParallelogram =
      Parallelogram{parallelogram.GetMaterial(), newQ, newU, newV};

  mPayload.objects.push_back(
      {.Q = newQ,
       .u = newU,
       .v = newV,
       .n = newParallelogram.Normal(),
       .w = newParallelogram.W(),
       .D = static_cast<float>(newParallelogram.Displacement()),
       .materialIndex = this->MapMaterial(*parallelogram.GetMaterial()),
       .isVolumeBoundary = mIsVolumeBoundary,
       .kind = std::to_underlying(ShaderInterop::ObjectKind::Parallelogram)});

  if (mIsVolumeBoundary == ShaderInterop::kIsVolumeBoundary)
    return;

  mBVHEntries.push_back(
      {.objectIndex = static_cast<std::uint32_t>(mPayload.objects.size() - 1),
       .box = newParallelogram.BoundingBox(mTime0, mTime1)});
};

GPUContext::ShaderPayload ShaderPayloadBuilder::TakePayload() noexcept {
  return std::move(mPayload);
}
