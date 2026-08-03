#include "headers/ImageExport.hpp"
#include "headers/Colour.hpp"
#include <cstddef>
#include <expected>
#include <filesystem>
#include <print>
#include <span>
#include <string>

std::expected<void, std::string>
ImageExport::PPMAscii(const std::string &fileName, const std::size_t imageWidth,
                      const std::size_t imageHeight,
                      const std::span<const SRGBColour> pixels) noexcept {
  auto finalName = fileName + ".ppm";

  if (std::filesystem::exists(finalName)) {
    std::size_t count = 0;
    do {
      finalName = fileName + std::to_string(count) + ".ppm";
      ++count;
    } while (std::filesystem::exists(finalName));
  }

  const auto file = std::fopen(finalName.c_str(), "w");

  if (!file) [[unlikely]]
    return std::unexpected("Failed to open / create file: " + finalName);

  std::println(file, "P3\n{} {}\n{}", imageWidth, imageHeight,
               Colour::kMaxColourValue);

  for (const auto &colour : pixels)
    std::println(file, "{}", colour);

  return std::expected<void, std::string>{};
}
