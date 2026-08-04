#pragma once

#include "Colour.hpp"
#include "Perlin.hpp"
#include "RTWImage.hpp"
#include "Vector3.hpp"
#include <cstddef>
#include <expected>
#include <memory>
#include <string>

class TextureVisitor;

class Texture {
public:
  Texture() = default;
  virtual ~Texture() = default;
  Texture(const Texture &) = default;
  Texture &operator=(const Texture &) = default;
  Texture(Texture &&) = default;
  Texture &operator=(Texture &&) = default;

  [[nodiscard]] virtual Colour Value(double u, double v,
                                     const Point3 &) const noexcept = 0;
  virtual void Accept(TextureVisitor &) const noexcept = 0;
};

class SolidColour : public Texture {
private:
  Colour mAlbedo = {};

public:
  SolidColour() = default;
  ~SolidColour() override = default;
  SolidColour(const SolidColour &) = default;
  SolidColour &operator=(const SolidColour &) = default;
  SolidColour(SolidColour &&) = default;
  SolidColour &operator=(SolidColour &&) = default;

  SolidColour(const Colour &) noexcept;
  SolidColour(double red, double green, double blue) noexcept;

  Colour Value(double u, double v, const Point3 &) const noexcept override;
  void Accept(TextureVisitor &) const noexcept override;

  [[nodiscard]] const Colour &Albedo() const noexcept;
};

class CheckerTexture : public Texture {
private:
  std::shared_ptr<Texture> mEven = nullptr;
  std::shared_ptr<Texture> mOdd = nullptr;
  double mInvScale = 1;

public:
  CheckerTexture() = delete;
  ~CheckerTexture() override = default;
  CheckerTexture(const CheckerTexture &) = default;
  CheckerTexture &operator=(const CheckerTexture &) = default;
  CheckerTexture(CheckerTexture &&) = default;
  CheckerTexture &operator=(CheckerTexture &&) = default;

  CheckerTexture(double scale, std::shared_ptr<Texture> even,
                 std::shared_ptr<Texture> odd) noexcept;
  CheckerTexture(double scale, const Colour &even, const Colour &odd) noexcept;

  Colour Value(double u, double v, const Point3 &) const noexcept override;
  void Accept(TextureVisitor &) const noexcept override;

  [[nodiscard]] std::shared_ptr<Texture> EvenTexture() const noexcept;
  [[nodiscard]] std::shared_ptr<Texture> OddTexture() const noexcept;
  [[nodiscard]] double InvScale() const noexcept;
};

class ImageTexture : public Texture {
private:
  RTWImage mImage;

  ImageTexture(RTWImage &&) noexcept;

public:
  ImageTexture() = delete;
  ~ImageTexture() override = default;
  ImageTexture(const ImageTexture &) = delete;
  ImageTexture &operator=(const ImageTexture &) = delete;
  ImageTexture(ImageTexture &&) noexcept = default;
  ImageTexture &operator=(ImageTexture &&) noexcept = default;

  Colour Value(double u, double v, const Point3 &p) const noexcept override;
  void Accept(TextureVisitor &) const noexcept override;

  [[nodiscard]] const RTWImage &Image() const noexcept;

  [[nodiscard]] static std::expected<ImageTexture, std::string>
  Create(const std::string &imageFilePath) noexcept;
};

class NoiseTexture : public Texture {
private:
  Perlin mPerlin = {};
  double mScale = 1;
  double mDistortion = 1;
  std::size_t mTurbulenceDepth = 1;

public:
  NoiseTexture(double scale, double distortion,
               std::size_t turbulenceDepth) noexcept;
  ~NoiseTexture() override = default;
  NoiseTexture(const NoiseTexture &) = delete;
  NoiseTexture &operator=(const NoiseTexture &) = delete;
  NoiseTexture(NoiseTexture &&) noexcept = default;
  NoiseTexture &operator=(NoiseTexture &&) noexcept = default;

  Colour Value(double u, double v, const Point3 &p) const noexcept override;
  void Accept(TextureVisitor &) const noexcept override;

  [[nodiscard]] const Perlin &GetPerlin() const noexcept;
  [[nodiscard]] double Scale() const noexcept;
  [[nodiscard]] double Distortion() const noexcept;
  [[nodiscard]] std::size_t TurbulenceDepth() const noexcept;
};
