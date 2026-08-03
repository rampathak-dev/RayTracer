#pragma once

#include "Texture.hpp"

class TextureVisitor {
public:
  TextureVisitor() = default;
  virtual ~TextureVisitor() = default;
  TextureVisitor(const TextureVisitor &) = default;
  TextureVisitor &operator=(const TextureVisitor &) = default;
  TextureVisitor(TextureVisitor &&) noexcept = default;
  TextureVisitor &operator=(TextureVisitor &&) noexcept = default;

  virtual void Visit(const SolidColour &) noexcept = 0;
  virtual void Visit(const CheckerTexture &) noexcept = 0;
  virtual void Visit(const ImageTexture &) noexcept = 0;
  virtual void Visit(const NoiseTexture &) noexcept = 0;
};
