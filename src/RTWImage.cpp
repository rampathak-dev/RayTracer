#include "headers/RTWImage.hpp"
#define STB_IMAGE_IMPLEMENTATION
#define STBI_FAILURE_USERMSG
#include "stb_image.h"
#include <algorithm>
#include <cstddef>
#include <cstdlib>
#include <expected>
#include <format>
#include <string>

namespace {
constexpr int kChannelsRequired = 3; // r. g, b
constexpr int kBytesPerPixel = 3;
} // namespace

RTWImage::RTWImage(const std::size_t imageWidth, const std::size_t imageHeight,
                   const std::size_t bytesPerScanline,
                   const std::uint8_t *const byteData) noexcept
    : mImageWidth{imageWidth}, mImageHeight{imageHeight},
      mBytesPerScanline{bytesPerScanline}, mByteData{byteData} {}

const std::uint8_t *RTWImage::PixelData(const std::size_t x,
                                        const std::size_t y) const noexcept {
  const auto clampedX = std::clamp(x, 0uz, mImageWidth - 1);
  const auto clampedY = std::clamp(y, 0uz, mImageHeight - 1);

  return mByteData.get() + clampedY * mBytesPerScanline +
         clampedX * kBytesPerPixel;
}

std::size_t RTWImage::Width() const noexcept { return mImageWidth; }

std::size_t RTWImage::Height() const noexcept { return mImageHeight; }

std::expected<RTWImage, std::string>
RTWImage::Create(const std::string &filePath) noexcept {
  auto imageWidth = 0;
  auto imageHeight = 0;
  auto _ = 0; // channels_in_file not required

  stbi_set_flip_vertically_on_load(true);
  const auto floatData = stbi_loadf(filePath.c_str(), &imageWidth, &imageHeight,
                                    &_, kChannelsRequired);

  if (!floatData)
    return std::unexpected(
        std::format("Failed to load file at file path: \"{}\"", filePath));

  const auto bytesPerScanline = imageWidth * kBytesPerPixel;
  const auto byteData =
      [imageWidth, imageHeight,
       floatData]() constexpr noexcept -> const std::uint8_t * {
    const auto totalBytes = imageWidth * imageHeight * kBytesPerPixel;
    const auto bData = new std::uint8_t[totalBytes];

    for (auto i = 0; i < totalBytes; ++i)
      bData[i] = static_cast<std::uint8_t>(
          std::clamp(floatData[i] * 256.0f, 0.0f, 255.0f));

    return bData;
  }();

  stbi_image_free(floatData);

  return RTWImage{
      static_cast<std::size_t>(imageWidth),
      static_cast<std::size_t>(imageHeight),
      static_cast<std::size_t>(bytesPerScanline),
      byteData,
  };
}
