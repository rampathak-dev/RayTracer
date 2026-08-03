#include "headers/Ray.hpp"
#include "headers/Vector3.hpp"

Ray::Ray(const Point3 &origin, const Vector3 &direction, double time) noexcept
    : mOrigin{origin}, mDirection{direction}, mTime{time} {}

const Point3 &Ray::Origin() const noexcept { return mOrigin; }

const Vector3 &Ray::Direction() const noexcept { return mDirection; }

double Ray::Time() const noexcept { return mTime; }

Point3 Ray::At(const double t) const noexcept {
  return mOrigin + mDirection * t;
}
