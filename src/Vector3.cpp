#include "headers/Vector3.hpp"
#include "headers/Math.hpp"
#include <cassert>
#include <cmath>

Vector3::Vector3(const double x, const double y, const double z) noexcept
    : mE{x, y, z} {}

double Vector3::x() const noexcept { return mE[0]; }

double Vector3::y() const noexcept { return mE[1]; }

double Vector3::z() const noexcept { return mE[2]; }

double Vector3::LengthSquared() const noexcept {
  return mE[0] * mE[0] + mE[1] * mE[1] + mE[2] * mE[2];
}

double Vector3::Length() const noexcept { return std::sqrt(LengthSquared()); }

bool Vector3::NearZero() const noexcept {
  using namespace Math;
  return (std::fabs(mE[0]) <= kEpsilon) && (std::fabs(mE[1]) <= kEpsilon) &&
         (std::fabs(mE[2]) <= kEpsilon);
}

Vector3 Vector3::Normalize(const Vector3 &u) noexcept { return u / u.Length(); }

double Vector3::Dot(const Vector3 &u, const Vector3 &v) noexcept {
  return u.mE[0] * v.mE[0] + u.mE[1] * v.mE[1] + u.mE[2] * v.mE[2];
}

Vector3 Vector3::Cross(const Vector3 &u, const Vector3 &v) noexcept {
  return {u.mE[1] * v.mE[2] - u.mE[2] * v.mE[1],
          u.mE[2] * v.mE[0] - u.mE[0] * v.mE[2],
          u.mE[0] * v.mE[1] - u.mE[1] * v.mE[0]};
}

Vector3 Vector3::Reflect(const Vector3 &u, const Vector3 &normal) noexcept {
  return u - 2 * Dot(u, normal) * normal;
}

// Derived from Snell's law
std::optional<std::tuple<Vector3, double>>
Vector3::Refract(const Vector3 &incident, const Vector3 &normal,
                 const double refractiveRatio) noexcept {
  const auto unitIncident = Normalize(incident);

  double cosTheta = Dot(-unitIncident, normal);
  Vector3 rPerpendicular = refractiveRatio * (unitIncident + cosTheta * normal);

  double rParallelMagnitudeSqr = 1 - rPerpendicular.LengthSquared();
  if (rParallelMagnitudeSqr < 0)
    return std::nullopt;

  Vector3 rParallel = -std::sqrt(rParallelMagnitudeSqr) * normal;
  return std::tuple<Vector3, double>{rPerpendicular + rParallel, cosTheta};
}

Vector3 Vector3::operator-() const noexcept { return {-mE[0], -mE[1], -mE[2]}; }

Vector3 &Vector3::operator+=(const Vector3 &v) noexcept {
  mE[0] += v.mE[0];
  mE[1] += v.mE[1];
  mE[2] += v.mE[2];

  return *this;
}

Vector3 &Vector3::operator-=(const Vector3 &v) noexcept {
  mE[0] -= v.mE[0];
  mE[1] -= v.mE[1];
  mE[2] -= v.mE[2];

  return *this;
}

Vector3 &Vector3::operator*=(const Vector3 &u) noexcept {
  mE[0] *= u.x();
  mE[1] *= u.y();
  mE[2] *= u.z();
  return *this;
}
Vector3 &Vector3::operator/=(const Vector3 &u) noexcept {
  mE[0] /= u.x();
  mE[1] /= u.y();
  mE[2] /= u.z();
  return *this;
}

Vector3 &Vector3::operator*=(const double s) noexcept {
  mE[0] *= s;
  mE[1] *= s;
  mE[2] *= s;

  return *this;
}

Vector3 &Vector3::operator/=(const double t) noexcept {
  const double tInverse = 1 / t;
  mE[0] *= tInverse;
  mE[1] *= tInverse;
  mE[2] *= tInverse;

  return *this;
}

double Vector3::operator[](const std::size_t axis) const noexcept {
  assert(axis <= 2);
  return mE[axis];
}

double &Vector3::operator[](const std::size_t axis) noexcept {
  assert(axis <= 2);
  return mE[axis];
}

Vector3 operator+(const Vector3 &u, const Vector3 &v) noexcept {
  return {u.mE[0] + v.mE[0], u.mE[1] + v.mE[1], u.mE[2] + v.mE[2]};
}

Vector3 operator-(const Vector3 &u, const Vector3 &v) noexcept {
  return {u.mE[0] - v.mE[0], u.mE[1] - v.mE[1], u.mE[2] - v.mE[2]};
}

Vector3 operator*(const Vector3 &u, const Vector3 &v) noexcept {
  return {u.mE[0] * v.mE[0], u.mE[1] * v.mE[1], u.mE[2] * v.mE[2]};
}

Vector3 operator*(const double t, const Vector3 &u) noexcept {
  return {u.mE[0] * t, u.mE[1] * t, u.mE[2] * t};
}

Vector3 operator*(const Vector3 &u, const double t) noexcept { return t * u; }

Vector3 operator/(const Vector3 &u, const Vector3 &v) noexcept {
  return {u.mE[0] / v.mE[0], u.mE[1] / v.mE[1], u.mE[2] / v.mE[2]};
}

Vector3 operator/(const Vector3 &u, const double t) noexcept {
  const double tInverse = 1 / t;
  return {u.mE[0] * tInverse, u.mE[1] * tInverse, u.mE[2] * tInverse};
}
