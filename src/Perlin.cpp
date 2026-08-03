#include "headers/Perlin.hpp"
#include "headers/Math.hpp"
#include "headers/Random.hpp"
#include "headers/Vector3.hpp"
#include <array>
#include <cmath>
#include <cstddef>
#include <numeric>
#include <utility>

namespace {
Perlin::NoiseValueArray GenerateNoiseValueArray() noexcept {
  auto array = Perlin::NoiseValueArray{};

  for (auto &element : array)
    element = Random::UnitVector();

  return array;
}

Perlin::PermutationArray GeneratePermutationArray() noexcept {
  auto permutation = Perlin::PermutationArray{};

  // fill with 0, 1, 2 .... n -1
  std::iota(permutation.begin(), permutation.end(), 0uz);

  // knuth shuffle
  for (std::size_t i = permutation.size() - 1; i > 0; --i) {
    const auto randomIndex = static_cast<std::size_t>(
        Random::Int(Math::Interval{0, static_cast<int>(i)}));
    std::swap(permutation[i], permutation[randomIndex]);
  }

  return permutation;
}

double PerlinInterpolation(const Vector3 c[2][2][2], const double u,
                           const double v, const double w) noexcept {
  // hermitian smoothing : f(x) =  3x^2 - 2x^3
  const auto uu = u * u * (3 - 2 * u);
  const auto vv = v * v * (3 - 2 * v);
  const auto ww = w * w * (3 - 2 * w);

  double blend = 0;

  for (int i = 0; i <= 1; ++i)
    for (int j = 0; j <= 1; ++j)
      for (int k = 0; k <= 1; ++k) {
        const auto weightV = Vector3{u - i, v - j, w - k};
        blend += (i * uu + (1 - i) * (1 - uu)) * (j * vv + (1 - j) * (1 - vv)) *
                 (k * ww + (1 - k) * (1 - ww)) *
                 Vector3::Dot(weightV, c[i][j][k]);
      }

  return blend;
}
} // namespace

Perlin::Perlin() noexcept
    : mNoiseValues{GenerateNoiseValueArray()},
      mXPermutation{GeneratePermutationArray()},
      mYPermutation{GeneratePermutationArray()},
      mZPermutation{GeneratePermutationArray()} {}

double Perlin::Turbulence(const Point3 &p,
                          const std::size_t depth) const noexcept {
  double blend = 0;
  auto currentP = p;
  double weight = 1;

  for (std::size_t i = 0; i < depth; ++i) {
    blend += weight * this->Noise(currentP);
    currentP *= 2;
    weight *= 0.5;
  }

  return std::fabs(blend);
}

double Perlin::Noise(const Point3 &p) const noexcept {
  const auto i = static_cast<int>(std::floor(p.x()));
  const auto j = static_cast<int>(std::floor(p.y()));
  const auto k = static_cast<int>(std::floor(p.z()));
  Vector3 c[2][2][2];

  const auto wrap = [](const int value) noexcept -> std::size_t {
    const auto wrapped = value % static_cast<int>(kPointCount);
    return wrapped < 0 ? static_cast<std::size_t>(wrapped + kPointCount)
                       : static_cast<std::size_t>(wrapped);
  };

  for (int di = 0; di <= 1; ++di)
    for (int dj = 0; dj <= 1; ++dj)
      for (int dk = 0; dk <= 1; ++dk)
        c[di][dj][dk] = mNoiseValues[mXPermutation[wrap(i + di)] ^
                                     mYPermutation[wrap(j + dj)] ^
                                     mZPermutation[wrap(k + dk)]];

  const auto u = p.x() - i;
  const auto v = p.y() - j;
  const auto w = p.z() - k;

  return PerlinInterpolation(c, u, v, w);
}

const Perlin::NoiseValueArray &Perlin::NoiseValues() const noexcept {
  return mNoiseValues;
}

const Perlin::PermutationArray &Perlin::XPermutation() const noexcept {
  return mXPermutation;
}

const Perlin::PermutationArray &Perlin::YPermutation() const noexcept {
  return mYPermutation;
}

const Perlin::PermutationArray &Perlin::ZPermutation() const noexcept {
  return mZPermutation;
}
