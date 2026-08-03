#include "headers/Texture.hpp"
#include "headers/Colour.hpp"
#include "headers/Perlin.hpp"
#include "headers/RTWImage.hpp"
#include "headers/TextureVisitor.hpp"
#include "headers/Vector3.hpp"
#include <cmath>
#include <cstddef>
#include <expected>
#include <memory>
#include <string>
#include <utility>

SolidColour::SolidColour(const double red, const double green,
                         const double blue) noexcept
    : mAlbedo{red, green, blue} {}

SolidColour::SolidColour(const Colour &colour) noexcept : mAlbedo{colour} {}

Colour SolidColour::Value(const double, const double,
                          const Point3 &) const noexcept {
  return mAlbedo;
}

void SolidColour::Accept(TextureVisitor &visitor) const noexcept {
  visitor.Visit(*this);
}

const Colour &SolidColour::Albedo() const noexcept { return mAlbedo; }

CheckerTexture::CheckerTexture(const double scale,
                               const std::shared_ptr<Texture> even,
                               const std::shared_ptr<Texture> odd) noexcept
    : mEven{even}, mOdd{odd}, mInvScale{1 / scale} {}

CheckerTexture::CheckerTexture(const double scale, const Colour &even,
                               const Colour &odd) noexcept
    : mEven{std::make_shared<SolidColour>(even)},
      mOdd{std::make_shared<SolidColour>(odd)}, mInvScale{1 / scale} {}

Colour CheckerTexture::Value(const double u, const double v,
                             const Point3 &p) const noexcept {
  const auto xi = static_cast<int>(std::floor(mInvScale * p.x()));
  const auto yi = static_cast<int>(std::floor(mInvScale * p.y()));
  const auto zi = static_cast<int>(std::floor(mInvScale * p.z()));

  return (xi + yi + zi) % 2 == 0 ? mEven->Value(u, v, p) : mOdd->Value(u, v, p);
}

void CheckerTexture::Accept(TextureVisitor &visitor) const noexcept {
  visitor.Visit(*this);
}

std::shared_ptr<Texture> CheckerTexture::EvenTexture() const noexcept {
  return mEven;
}

std::shared_ptr<Texture> CheckerTexture::OddTexture() const noexcept {
  return mOdd;
}

double CheckerTexture::InvScale() const noexcept { return mInvScale; }

ImageTexture::ImageTexture(RTWImage &&image) noexcept
    : mImage{std::move(image)} {}

Colour ImageTexture::Value(const double u, const double v,
                           const Point3 &) const noexcept {
  const auto clampedU = std::clamp(u, 0.0, 1.0);
  const auto clampedV = std::clamp(v, 0.0, 1.0);

  const auto x = static_cast<std::size_t>(clampedU * mImage.Width());
  const auto y = static_cast<std::size_t>(clampedV * mImage.Height());
  const auto pixel = mImage.PixelData(x, y);

  constexpr auto normalise = 1.0 / Colour::kMaxColourValue;
  return {pixel[0] * normalise, pixel[1] * normalise, pixel[2] * normalise};
}

void ImageTexture::Accept(TextureVisitor &visitor) const noexcept {
  visitor.Visit(*this);
}

const RTWImage &ImageTexture::Image() const noexcept { return mImage; }

std::expected<ImageTexture, std::string>
ImageTexture::Create(const std::string &imageFilePath) noexcept {
  auto rtwImage = RTWImage::Create(imageFilePath);

  if (!rtwImage)
    return std::unexpected(std::move(rtwImage.error()));

  return ImageTexture{std::move(rtwImage.value())};
}

NoiseTexture::NoiseTexture(const double scale, const double distortion,
                           const std::size_t turbulenceDepth) noexcept
    : mScale{scale}, mDistortion{distortion},
      mTurbulenceDepth{turbulenceDepth} {}

Colour NoiseTexture::Value(const double, const double,
                           const Point3 &p) const noexcept {
  return Colour{0.5, 0.5, 0.5} *
         (1 + std::sin(p.z() * mScale +
                       mDistortion * mPerlin.Turbulence(p, mTurbulenceDepth)));
}

void NoiseTexture::Accept(TextureVisitor &visitor) const noexcept {
  visitor.Visit(*this);
}

const Perlin &NoiseTexture::Perlin() const noexcept { return mPerlin; }

double NoiseTexture::Scale() const noexcept { return mScale; }

double NoiseTexture::Distortion() const noexcept { return mDistortion; }

std::size_t NoiseTexture::TurbulenceDepth() const noexcept {
  return mTurbulenceDepth;
}
