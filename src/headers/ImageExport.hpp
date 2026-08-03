#pragma once

#include "Colour.hpp"
#include <cstddef>
#include <expected>
#include <span>
#include <string>

namespace ImageExport {
[[nodiscard]] std::expected<void, std::string>
PPMAscii(const std::string &fileName, std::size_t imageWidth,
         std::size_t imageHeight, std::span<const SRGBColour> pixels) noexcept;
}
