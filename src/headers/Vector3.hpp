#pragma once

#include <cstddef>
#include <format>
#include <optional>
#include <tuple>

class Vector3 {
private:
  double mE[3] = {0, 0, 0};

public:
  Vector3() = default;
  ~Vector3() = default;
  Vector3(const Vector3 &) = default;
  Vector3 &operator=(const Vector3 &) = default;
  Vector3(Vector3 &&) noexcept = default;
  Vector3 &operator=(Vector3 &&) noexcept = default;

  Vector3(double x, double y, double z) noexcept;

  double x() const noexcept;
  double y() const noexcept;
  double z() const noexcept;
  double LengthSquared() const noexcept;
  double Length() const noexcept;
  bool NearZero() const noexcept;
  static Vector3 Normalize(const Vector3 &) noexcept;
  static double Dot(const Vector3 &, const Vector3 &) noexcept;
  static Vector3 Cross(const Vector3 &, const Vector3 &) noexcept;
  static Vector3 Reflect(const Vector3 &u, const Vector3 &normal) noexcept;
  static std::optional<std::tuple<Vector3, double>>
  Refract(const Vector3 &unitIncident, const Vector3 &normal,
          double refractiveRatio) noexcept;

  [[nodiscard]] Vector3 operator-() const noexcept;
  Vector3 &operator+=(const Vector3 &) noexcept;
  Vector3 &operator-=(const Vector3 &) noexcept;
  Vector3 &operator*=(const Vector3 &) noexcept;
  Vector3 &operator/=(const Vector3 &) noexcept;
  Vector3 &operator*=(double) noexcept;
  Vector3 &operator/=(double) noexcept;
  [[nodiscard]] double operator[](std::size_t) const noexcept;
  [[nodiscard]] double &operator[](const std::size_t axis) noexcept;

  friend Vector3 operator+(const Vector3 &, const Vector3 &) noexcept;
  friend Vector3 operator-(const Vector3 &, const Vector3 &) noexcept;
  friend Vector3 operator*(const Vector3 &, const Vector3 &) noexcept;
  friend Vector3 operator*(double, const Vector3 &) noexcept;
  friend Vector3 operator*(const Vector3 &, double) noexcept;
  friend Vector3 operator/(const Vector3 &, const Vector3 &) noexcept;
  friend Vector3 operator/(const Vector3 &, double) noexcept;
};

template <> struct std::formatter<Vector3> {
  constexpr auto parse(std::format_parse_context &ctx) noexcept {
    return ctx.begin();
  }

  auto format(const Vector3 &u, std::format_context &ctx) const noexcept {
    return std::format_to(ctx.out(), "({}, {}, {})", u.x(), u.y(), u.z());
  };
};

using Point3 = Vector3;
