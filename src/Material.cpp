#include "headers/Material.hpp"
#include "headers/Colour.hpp"
#include "headers/Hittable.hpp"
#include "headers/MaterialVisitor.hpp"
#include "headers/Random.hpp"
#include "headers/Ray.hpp"
#include "headers/Texture.hpp"
#include "headers/Vector3.hpp"
#include <algorithm>
#include <memory>
#include <optional>

namespace {
double SchlickApproximation(const double etaRatio,
                            const double cosTheta) noexcept {
  const double r0 =
      (1 - etaRatio) / (1 + etaRatio) * (1 - etaRatio) / (1 + etaRatio);
  return r0 + (1 - r0) * std::pow(1 - cosTheta, 5);
}
} // namespace

std::optional<ScatterRecord>
Material::Scatter(const Ray &, const HitRecord &) const noexcept {
  return std::nullopt;
}

Colour Material::Emit(const double, const double,
                      const Point3 &) const noexcept {
  return {0, 0, 0};
}

Lambertian::Lambertian(const Colour &albedo) noexcept
    : mTexture{std::make_shared<SolidColour>(albedo)} {}

Lambertian::Lambertian(std::shared_ptr<Texture> texture) noexcept
    : mTexture{texture} {}

std::optional<ScatterRecord>
Lambertian::Scatter(const Ray &rayIn, const HitRecord &record) const noexcept {
  Vector3 scattered = record.normal + Random::UnitVector();

  if (scattered.NearZero())
    scattered = record.normal;

  return ScatterRecord{
      {record.intersection, scattered, rayIn.Time()},
      mTexture->Value(record.u, record.v, record.intersection)};
}

void Lambertian::Accept(MaterialVisitor &visitor) const noexcept {
  visitor.Visit(*this);
}

const Texture &Lambertian::GetTexture() const noexcept { return *mTexture; }

Metal::Metal(const Colour &albedo, double fuzz) noexcept
    : mAlbedo{albedo}, mFuzz{std::clamp(fuzz, 0.0, 1.0)} {}

std::optional<ScatterRecord>
Metal::Scatter(const Ray &rayIn, const HitRecord &record) const noexcept {
  auto reflected = Vector3::Reflect(rayIn.Direction(), record.normal);
  reflected = Vector3::Normalize(reflected) + Random::UnitVector() * mFuzz;

  if (Vector3::Dot(reflected, record.normal) < 0)
    return std::nullopt;

  return ScatterRecord{{record.intersection, reflected, rayIn.Time()}, mAlbedo};
}

void Metal::Accept(MaterialVisitor &visitor) const noexcept {
  visitor.Visit(*this);
}

const Colour &Metal::Albedo() const noexcept { return mAlbedo; }
double Metal::Fuzz() const noexcept { return mFuzz; }

Dielectric::Dielectric(const double refractiveIndexInside,
                       const double refractiveIndexOutside) noexcept
    : mEtaRatio{refractiveIndexOutside / refractiveIndexInside} {}

std::optional<ScatterRecord>
Dielectric::Scatter(const Ray &rayIn, const HitRecord &record) const noexcept {
  const auto etaRatio = record.frontFace ? mEtaRatio : 1 / mEtaRatio;
  auto newDirection = Vector3{};

  if (const auto refractionRecord =
          Vector3::Refract(rayIn.Direction(), record.normal, etaRatio)) {
    const auto [refracted, cosTheta] = refractionRecord.value();

    newDirection = SchlickApproximation(etaRatio, cosTheta) > Random::Double()
                       ? Vector3::Reflect(rayIn.Direction(), record.normal)
                       : refracted;
  } else {
    newDirection = Vector3::Reflect(rayIn.Direction(), record.normal);
  }

  return ScatterRecord{
      {record.intersection, newDirection, rayIn.Time()},
      {1, 1, 1},
  };
}

void Dielectric::Accept(MaterialVisitor &visitor) const noexcept {
  visitor.Visit(*this);
}

double Dielectric::EtaRatio() const noexcept { return mEtaRatio; }

DiffuseLight::DiffuseLight(const std::shared_ptr<Texture> texture) noexcept
    : mTexture{texture} {}

DiffuseLight::DiffuseLight(const Colour &colour) noexcept
    : mTexture{std::make_shared<SolidColour>(colour)} {}

Colour DiffuseLight::Emit(const double u, const double v,
                          const Point3 &p) const noexcept {
  return mTexture->Value(u, v, p);
}

void DiffuseLight::Accept(MaterialVisitor &visitor) const noexcept {
  return visitor.Visit(*this);
}

const Texture &DiffuseLight::GetTexture() const noexcept { return *mTexture; }

Isotropic::Isotropic(const std::shared_ptr<Texture> texture) noexcept
    : mTexture{texture} {}

Isotropic::Isotropic(const Colour &albedo) noexcept
    : mTexture{std::make_shared<SolidColour>(albedo)} {}

std::optional<ScatterRecord>
Isotropic::Scatter(const Ray &ray, const HitRecord &hitRecord) const noexcept {
  return ScatterRecord{
      .ray = {hitRecord.intersection, Random::UnitVector(), ray.Time()},
      .albedo =
          mTexture->Value(hitRecord.u, hitRecord.v, hitRecord.intersection)};
}

void Isotropic::Accept(MaterialVisitor &visitor) const noexcept {
  visitor.Visit(*this);
}

const Texture &Isotropic::GetTexture() const noexcept { return *mTexture; }
