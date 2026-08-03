#pragma once

#include <limits>

namespace Math {
constexpr double kInfinity = std::numeric_limits<double>::infinity();
constexpr double kPi = 3.1415926535897932385;
constexpr auto kEpsilon = 1e-8;

double ToRadians(double degrees) noexcept;

class Interval {
private:
  double mMin = kInfinity, mMax = -kInfinity;

  Interval() = default;

public:
  ~Interval() = default;
  Interval(const Interval &) = default;
  Interval &operator=(const Interval &) = default;
  Interval(Interval &&) noexcept = default;
  Interval &operator=(Interval &&) noexcept = default;

  Interval(double min, double max) noexcept;
  Interval(int a, int b) noexcept;
  Interval(Interval a, Interval b) noexcept;

  [[nodiscard]] double Min() const noexcept;
  [[nodiscard]] double Max() const noexcept;
  void SetMin(double) noexcept;
  void SetMax(double) noexcept;
  [[nodiscard]] double Size() const noexcept;
  [[nodiscard]] double Midpoint() const noexcept;
  [[nodiscard]] bool Contains(double) const noexcept;
  [[nodiscard]] bool Surrounds(double) const noexcept;
  [[nodiscard]] Interval Expand(double) const noexcept;

  [[nodiscard]] static Interval Empty() noexcept;
};
} // namespace Math

[[nodiscard]] Math::Interval operator+(Math::Interval, double) noexcept;
[[nodiscard]] Math::Interval operator+(double, Math::Interval) noexcept;
