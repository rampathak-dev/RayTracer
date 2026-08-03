#include "headers/Colour.hpp"
#include "headers/Vector3.hpp"
#include <algorithm>

namespace {
[[nodiscard]] double LinearToGammaCorrected(const double value) noexcept {
  const auto clamped = std::clamp(value, 0.0, 1.0);

  if (clamped <= 0.0031308)
    return 12.92 * clamped;

  return 1.055 * std::pow(clamped, 1.0 / 2.4) - 0.055;
}

[[nodiscard]] std::uint8_t ToSRGB(const double value) noexcept {
  return static_cast<uint8_t>(
      std::round(Colour::kMaxColourValue * LinearToGammaCorrected(value)));
}
} // namespace

Colour::Colour(const Vector3 &u) noexcept : Vector3{u} {}

SRGBColour::SRGBColour(const Colour &colour) noexcept
    : r{ToSRGB(colour.x())}, g{ToSRGB(colour.y())}, b{ToSRGB(colour.z())} {}

SRGBColour::SRGBColour(const double r_, const double g_,
                       const double b_) noexcept
    : r{ToSRGB(r_)}, g{ToSRGB(g_)}, b{ToSRGB(b_)} {}
