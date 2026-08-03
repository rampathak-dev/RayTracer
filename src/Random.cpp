#include "headers/Random.hpp"
#include "headers/Colour.hpp"
#include "headers/Math.hpp"
#include "headers/Vector3.hpp"
#include <cmath>
#include <random>

namespace {
auto kGenerator = []() noexcept -> std::mt19937 {
  std::random_device device = {};
  std::seed_seq sequence{device(), device(), device(), device(),
                         device(), device(), device(), device()};
  return std::mt19937{sequence};
}();
} // namespace

double Random::Double(const Math::Interval &interval) noexcept {
  std::uniform_real_distribution<double> distribution{interval.Min(),
                                                      interval.Max()};
  return distribution(kGenerator);
}

int Random::Int(const Math::Interval &interval) noexcept {
  std::uniform_real_distribution<double> distribution{interval.Min(),
                                                      interval.Max()};
  return static_cast<int>(distribution(kGenerator));
}

Vector3 Random::UnitVector() noexcept {
  using namespace Random;

  const auto z = Double() * 2 - 1;
  const auto radius = std::sqrt(1 - z * z);
  const auto theta = Double() * 2 * Math::kPi;

  const auto x = radius * std::cos(theta);
  const auto y = radius * std::sin(theta);

  return {x, y, z};
}

Vector3 Random::SampleUnitSquare() noexcept {
  using namespace Random;
  return {Double() - 0.5, Double() - 0.5, 0};
}

Vector3 Random::SampleUnitDisk() noexcept {
  using namespace Random;

  const auto radius = std::sqrt(Double());
  const auto theta = Double() * 2.0 * Math::kPi;

  const auto x = radius * std::cos(theta);
  const auto y = radius * std::sin(theta);

  return {x, y, 0};
}

Colour Random::Colour() noexcept { return {Double(), Double(), Double()}; }
