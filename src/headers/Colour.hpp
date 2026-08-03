#pragma once

#include "Vector3.hpp"
#include <cstdint>
#include <format>

class Colour : public Vector3 {
public:
  static constexpr std::size_t kMaxColourValue = 255;

  using Vector3::Vector3;

  Colour() = default;
  ~Colour() = default;
  Colour(const Colour &) = default;
  Colour &operator=(const Colour &) = default;
  Colour(Colour &&) noexcept = default;
  Colour &operator=(Colour &&) noexcept = default;

  Colour(const Vector3 &) noexcept;
};

struct SRGBColour {
  std::uint8_t r = 0;
  std::uint8_t g = 0;
  std::uint8_t b = 0;

  SRGBColour() = default;
  ~SRGBColour() = default;
  SRGBColour(const SRGBColour &) = default;
  SRGBColour &operator=(const SRGBColour &) = default;
  SRGBColour(SRGBColour &&) noexcept = default;
  SRGBColour &operator=(SRGBColour &&) noexcept = default;

  SRGBColour(const Colour &) noexcept;
  SRGBColour(double r, double g, double b) noexcept;
};

template <> struct std::formatter<SRGBColour> {
  constexpr auto parse(std::format_parse_context &ctx) noexcept {
    return ctx.begin();
  }

  auto format(const SRGBColour &colour,
              std::format_context &ctx) const noexcept {
    return std::format_to(ctx.out(), "{} {} {}\n", colour.r, colour.g,
                          colour.b);
  };
};
