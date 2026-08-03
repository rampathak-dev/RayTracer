#pragma once

#include "Vector3.hpp"
#include <array>
#include <cstddef>

class Perlin {
public:
  constexpr static std::size_t kPointCount = 256;
  using NoiseValueArray = std::array<Vector3, kPointCount>;
  using PermutationArray = std::array<std::size_t, kPointCount>;

  Perlin() noexcept;
  ~Perlin() = default;
  Perlin(const Perlin &) = default;
  Perlin &operator=(const Perlin &) = default;
  Perlin(Perlin &&) noexcept = default;
  Perlin &operator=(Perlin &&) noexcept = default;

  [[nodiscard]] double Turbulence(const Point3 &,
                                  std::size_t depth) const noexcept;
  [[nodiscard]] double Noise(const Point3 &) const noexcept;

  const NoiseValueArray &NoiseValues() const noexcept;
  const PermutationArray &XPermutation() const noexcept;
  const PermutationArray &YPermutation() const noexcept;
  const PermutationArray &ZPermutation() const noexcept;

private:
  NoiseValueArray mNoiseValues = {};
  PermutationArray mXPermutation = {};
  PermutationArray mYPermutation = {};
  PermutationArray mZPermutation = {};
};
