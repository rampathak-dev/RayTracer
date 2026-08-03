#include "headers/Math.hpp"
#include <cassert>

double Math::ToRadians(double degrees) noexcept {
  return degrees * (kPi / 180.0);
}

Math::Interval::Interval(double min, double max) noexcept
    : mMin{min}, mMax{max} {
  assert(min <= max);
}

Math::Interval::Interval(int min, int max) noexcept
    : mMin{static_cast<double>(min)}, mMax{static_cast<double>(max)} {
  assert(min <= max);
}

Math::Interval::Interval(const Interval a, const Interval b) noexcept {
  mMin = a.Min() <= b.Min() ? a.Min() : b.Min();
  mMax = a.Max() >= b.Max() ? a.Max() : b.Max();
}

double Math::Interval::Min() const noexcept { return mMin; }

double Math::Interval::Max() const noexcept { return mMax; }

void Math::Interval::SetMin(const double min) noexcept {
  assert(min <= mMax);
  mMin = min;
}

void Math::Interval::SetMax(const double max) noexcept {
  assert(max >= mMin);
  mMax = max;
}

double Math::Interval::Size() const noexcept { return mMax - mMin; }

double Math::Interval::Midpoint() const noexcept { return (mMin + mMax) * 0.5; }

bool Math::Interval::Contains(const double x) const noexcept {
  return (x >= mMin) && (x <= mMax);
}

bool Math::Interval::Surrounds(const double x) const noexcept {
  return (x > mMin) && (x < mMax);
}

Math::Interval Math::Interval::Expand(const double delta) const noexcept {
  const auto halfDelta = delta * 0.5;
  return {mMin - halfDelta, mMax + halfDelta};
}

Math::Interval Math::Interval::Empty() noexcept { return {}; }

Math::Interval operator+(const Math::Interval interval,
                         const double d) noexcept {
  return {interval.Min() + d, interval.Max() + d};
}

Math::Interval operator+(const double d,
                         const Math::Interval interval) noexcept {
  return interval + d;
}
