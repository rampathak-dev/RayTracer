#pragma once

#include "stb_image.h"
#include <cstddef>
#include <expected>
#include <memory>
#include <string>

class RTWImage {
private:
  std::size_t mImageWidth = 0;
  std::size_t mImageHeight = 0;
  std::size_t mBytesPerScanline = 0;
  std::unique_ptr<const std::uint8_t[]> mByteData = nullptr;

  RTWImage(std::size_t imageWidth, std::size_t imageHeight,
           std::size_t bytePerScanline, const std::uint8_t *byteData) noexcept;

public:
  RTWImage() = delete;
  RTWImage(const RTWImage &) = delete;
  RTWImage &operator=(const RTWImage &) = delete;
  RTWImage(RTWImage &&) noexcept = default;
  RTWImage &operator=(RTWImage &&) noexcept = default;

  const std::uint8_t *PixelData(std::size_t x, std::size_t y) const noexcept;
  [[nodiscard]] std::size_t Width() const noexcept;
  [[nodiscard]] std::size_t Height() const noexcept;

  [[nodiscard]] static std::expected<RTWImage, std::string>
  Create(const std::string &filePath) noexcept;
};
