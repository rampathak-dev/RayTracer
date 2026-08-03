#pragma once

#include "Hittable.hpp"

class HittableVisitor {
public:
  HittableVisitor() = default;
  virtual ~HittableVisitor() = default;
  HittableVisitor(const HittableVisitor &) = default;
  HittableVisitor &operator=(const HittableVisitor &) = default;
  HittableVisitor(HittableVisitor &&) noexcept = default;
  HittableVisitor &operator=(HittableVisitor &&) noexcept = default;

  virtual void Visit(const NullHittable &) noexcept = 0;
  virtual void Visit(const Translate &) noexcept = 0;
  virtual void Visit(const RotateY &) noexcept = 0;
  virtual void Visit(const ConstantMedium &) noexcept = 0;
  virtual void Visit(const Sphere &) noexcept = 0;
  virtual void Visit(const MovingSphere &) noexcept = 0;
  virtual void Visit(const Parallelogram &) noexcept = 0;
  virtual void Visit(const HittableList &) noexcept = 0;
  virtual void Visit(const BVHNode &) noexcept = 0;
};
