#pragma once

#include "Colour.hpp"
#include "Math.hpp"
#include "Vector3.hpp"

namespace Random {
[[nodiscard]] double Double(const Math::Interval &interval = Math::Interval{
                                0, 1}) noexcept;
[[nodiscard]] int Int(const Math::Interval &interval = Math::Interval{
                          0, 1}) noexcept;
[[nodiscard]] Vector3 UnitVector() noexcept;
[[nodiscard]] Vector3 SampleUnitSquare() noexcept;
[[nodiscard]] Vector3 SampleUnitDisk() noexcept;
[[nodiscard]] Colour Colour() noexcept;
}; // namespace Random
