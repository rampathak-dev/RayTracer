#pragma once

#include "Material.hpp"

class MaterialVisitor {
public:
  MaterialVisitor() = default;
  virtual ~MaterialVisitor() = default;
  MaterialVisitor(const MaterialVisitor &) = default;
  MaterialVisitor &operator=(const MaterialVisitor &) = default;
  MaterialVisitor(MaterialVisitor &&) noexcept = default;
  MaterialVisitor &operator=(MaterialVisitor &&) noexcept = default;

  virtual void Visit(const Lambertian &) noexcept = 0;
  virtual void Visit(const Metal &) noexcept = 0;
  virtual void Visit(const Dielectric &) noexcept = 0;
  virtual void Visit(const DiffuseLight &) noexcept = 0;
  virtual void Visit(const Isotropic &) noexcept = 0;
};
