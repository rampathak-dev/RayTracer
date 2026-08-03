#pragma once

#include "Vector3.hpp"

class Ray {
private:
  Point3 mOrigin = {};
  Vector3 mDirection = {};
  double mTime = 0;

public:
  Ray() = delete;
  ~Ray() = default;
  Ray(const Ray &) = default;
  Ray &operator=(const Ray &) = default;
  Ray(Ray &&) noexcept = default;
  Ray &operator=(Ray &&) noexcept = default;

  Ray(const Point3 &, const Vector3 &, double) noexcept;

  [[nodiscard]] const Point3 &Origin() const noexcept;
  [[nodiscard]] const Vector3 &Direction() const noexcept;
  [[nodiscard]] double Time() const noexcept;
  [[nodiscard]] Point3 At(double) const noexcept;
};
