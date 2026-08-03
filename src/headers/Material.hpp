#pragma once

#include "Colour.hpp"
#include "Ray.hpp"
#include "Texture.hpp"
#include <memory>
#include <optional>

struct HitRecord;

class MaterialVisitor;

struct ScatterRecord {
  Ray ray = {{}, {}, 0};
  Colour albedo = {};
};

class Material {
public:
  Material() = default;
  virtual ~Material() noexcept = default;
  Material(const Material &) = default;
  Material &operator=(const Material &) = default;
  Material(Material &&) noexcept = default;
  Material &operator=(Material &&) noexcept = default;

  [[nodiscard]] virtual std::optional<ScatterRecord>
  Scatter(const Ray &, const HitRecord &) const noexcept;
  [[nodiscard]] virtual Colour Emit(double u, double v,
                                    const Point3 &p) const noexcept;
  virtual void Accept(MaterialVisitor &) const noexcept = 0;
};

class Lambertian : public Material {
private:
  std::shared_ptr<Texture> mTexture = nullptr;

public:
  Lambertian() = delete;
  ~Lambertian() override = default;
  Lambertian(const Lambertian &) = default;
  Lambertian &operator=(const Lambertian &) = default;
  Lambertian(Lambertian &&) noexcept = default;
  Lambertian &operator=(Lambertian &&) noexcept = default;

  Lambertian(const Colour &) noexcept;
  Lambertian(std::shared_ptr<Texture>) noexcept;

  std::optional<ScatterRecord>
  Scatter(const Ray &, const HitRecord &) const noexcept override;

  void Accept(MaterialVisitor &) const noexcept override;

  [[nodiscard]] const Texture &GetTexture() const noexcept;
};

class Metal : public Material {
private:
  Colour mAlbedo{};
  double mFuzz = 0;

public:
  Metal() = default;
  ~Metal() override = default;
  Metal(const Metal &) = default;
  Metal &operator=(const Metal &) = default;
  Metal(Metal &&) noexcept = default;
  Metal &operator=(Metal &&) noexcept = default;

  Metal(const Colour &, double) noexcept;

  std::optional<ScatterRecord>
  Scatter(const Ray &, const HitRecord &) const noexcept override;

  void Accept(MaterialVisitor &) const noexcept override;

  [[nodiscard]] const Colour &Albedo() const noexcept;
  [[nodiscard]] double Fuzz() const noexcept;
};

class Dielectric : public Material {
private:
  double mEtaRatio = 1;

public:
  Dielectric() = default;
  ~Dielectric() override = default;
  Dielectric(const Dielectric &) = default;
  Dielectric &operator=(const Dielectric &) = default;
  Dielectric(Dielectric &&) noexcept = default;
  Dielectric &operator=(Dielectric &&) noexcept = default;

  Dielectric(double refractiveIndexInside,
             double refractiveIndexOutside) noexcept;

  std::optional<ScatterRecord>
  Scatter(const Ray &, const HitRecord &) const noexcept override;

  void Accept(MaterialVisitor &) const noexcept override;

  [[nodiscard]] double EtaRatio() const noexcept;
};

class DiffuseLight : public Material {
private:
  std::shared_ptr<Texture> mTexture = nullptr;

public:
  DiffuseLight() = delete;
  ~DiffuseLight() override = default;
  DiffuseLight(const DiffuseLight &) = default;
  DiffuseLight &operator=(const DiffuseLight &) = default;
  DiffuseLight(DiffuseLight &&) noexcept = default;
  DiffuseLight &operator=(DiffuseLight &&) noexcept = default;

  DiffuseLight(std::shared_ptr<Texture>) noexcept;
  DiffuseLight(const Colour &) noexcept;

  Colour Emit(double u, double v, const Point3 &p) const noexcept override;
  void Accept(MaterialVisitor &) const noexcept override;

  [[nodiscard]] const Texture &GetTexture() const noexcept;
};

class Isotropic : public Material {
private:
  std::shared_ptr<Texture> mTexture = nullptr;

public:
  Isotropic() = delete;
  ~Isotropic() override = default;
  Isotropic(const Isotropic &) = default;
  Isotropic &operator=(const Isotropic &) = default;
  Isotropic(Isotropic &&) noexcept = default;
  Isotropic &operator=(Isotropic &&) noexcept = default;

  Isotropic(std::shared_ptr<Texture>) noexcept;
  Isotropic(const Colour &) noexcept;

  std::optional<ScatterRecord>
  Scatter(const Ray &, const HitRecord &) const noexcept override;

  void Accept(MaterialVisitor &) const noexcept override;

  [[nodiscard]] const Texture &GetTexture() const noexcept;
};
