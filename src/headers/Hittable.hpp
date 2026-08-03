#pragma once

#include "Material.hpp"
#include "Math.hpp"
#include "Ray.hpp"
#include "Vector3.hpp"
#include <concepts>
#include <memory>
#include <optional>
#include <span>
#include <vector>

struct HitRecord {
  Point3 intersection = {};
  Vector3 normal = {};
  std::shared_ptr<Material> material = nullptr;
  double t = -1;
  double u = 0;
  double v = 0;
  bool frontFace = true;
};

class AABB {
private:
  Math::Interval mE[3] = {Math::Interval::Empty(), Math::Interval::Empty(),
                          Math::Interval::Empty()};

  AABB() = default;

public:
  ~AABB() = default;
  AABB(const AABB &) = default;
  AABB &operator=(const AABB &) = default;
  AABB(AABB &&) noexcept = default;
  AABB &operator=(AABB &&) noexcept = default;

  AABB(Math::Interval x, Math::Interval y, Math::Interval z) noexcept;
  AABB(const Point3 &a, const Point3 &b) noexcept;
  AABB(const AABB &, const AABB &) noexcept;

  bool Hit(const Ray &, Math::Interval) const noexcept;
  std::size_t LongestAxis() const noexcept;
  Math::Interval AxisInterval(std::size_t axis) const noexcept;

  [[nodiscard]] static AABB Empty() noexcept;
};

[[nodiscard]] AABB operator+(const AABB &, const Vector3 &) noexcept;
[[nodiscard]] AABB operator+(const Vector3 &, const AABB &) noexcept;

class HittableVisitor;

class Hittable {
public:
  Hittable() = default;
  virtual ~Hittable() = default;
  Hittable(const Hittable &) = delete;
  Hittable &operator=(const Hittable &) = delete;
  Hittable(Hittable &&) noexcept = default;
  Hittable &operator=(Hittable &&) noexcept = default;

  [[nodiscard]] virtual AABB BoundingBox(double time0,
                                         double time1) const noexcept = 0;
  [[nodiscard]] virtual std::optional<HitRecord>
  Hit(const Ray &, Math::Interval) const noexcept = 0;
  virtual void Accept(HittableVisitor &) const noexcept = 0;
};

class NullHittable : public Hittable {
public:
  NullHittable() = default;
  virtual ~NullHittable() = default;
  NullHittable(const NullHittable &) = delete;
  NullHittable &operator=(const NullHittable &) = delete;
  NullHittable(NullHittable &&) noexcept = default;
  NullHittable &operator=(NullHittable &&) noexcept = default;

  AABB BoundingBox(double time0, double time1) const noexcept override;
  std::optional<HitRecord> Hit(const Ray &,
                               Math::Interval) const noexcept override;
  void Accept(HittableVisitor &) const noexcept override;
};

class Translate : public Hittable {
private:
  Vector3 mOffset = {};
  std::unique_ptr<Hittable> mObject = nullptr;

public:
  Translate() = delete;
  virtual ~Translate() = default;
  Translate(const Translate &) = delete;
  Translate &operator=(const Translate &) = delete;
  Translate(Translate &&) noexcept = default;
  Translate &operator=(Translate &&) noexcept = default;

  Translate(std::unique_ptr<Hittable>, const Vector3 &) noexcept;

  AABB BoundingBox(double time0, double time1) const noexcept override;
  std::optional<HitRecord> Hit(const Ray &,
                               Math::Interval) const noexcept override;
  void Accept(HittableVisitor &) const noexcept override;

  [[nodiscard]] const Vector3 &Offset() const noexcept;
  [[nodiscard]] const Hittable &Object() const noexcept;
};

class RotateY : public Hittable {
private:
  std::unique_ptr<Hittable> mObject = nullptr;
  double mSinTheta = 0;
  double mCosTheta = 1;

public:
  RotateY() = delete;
  virtual ~RotateY() = default;
  RotateY(const RotateY &) = delete;
  RotateY &operator=(const RotateY &) = delete;
  RotateY(RotateY &&) noexcept = default;
  RotateY &operator=(RotateY &&) noexcept = default;

  RotateY(std::unique_ptr<Hittable>, double theta) noexcept;

  AABB BoundingBox(double time0, double time1) const noexcept override;
  std::optional<HitRecord> Hit(const Ray &,
                               Math::Interval) const noexcept override;
  void Accept(HittableVisitor &) const noexcept override;

  [[nodiscard]] const Hittable &Object() const noexcept;
  [[nodiscard]] double SinTheta() const noexcept;
  [[nodiscard]] double CosTheta() const noexcept;
};

class Volume : public Hittable {
protected:
  std::unique_ptr<Hittable> mBoundary = nullptr;
  std::shared_ptr<Material> mPhaseFunction = nullptr;

public:
  Volume() = delete;
  virtual ~Volume() = default;
  Volume(const Volume &) = delete;
  Volume &operator=(const Volume &) = delete;
  Volume(Volume &&) noexcept = default;

  Volume(std::unique_ptr<Hittable>, std::shared_ptr<Texture>) noexcept;

  Volume(std::unique_ptr<Hittable>, const Colour &albedo) noexcept;

  AABB BoundingBox(double time0, double time1) const noexcept override;

  [[nodiscard]] const Hittable &Boundary() const noexcept;
  [[nodiscard]] const Material &PhaseFunction() const noexcept;
};

class ConstantMedium : public Volume {
private:
  double mNegInvDensity = 0;

public:
  ConstantMedium() = delete;
  virtual ~ConstantMedium() = default;
  ConstantMedium(const ConstantMedium &) = delete;
  ConstantMedium &operator=(const ConstantMedium &) = delete;
  ConstantMedium(ConstantMedium &&) noexcept = default;

  ConstantMedium(std::unique_ptr<Hittable>, std::shared_ptr<Texture>,
                 double density) noexcept;

  ConstantMedium(std::unique_ptr<Hittable>, const Colour &albedo,
                 double density) noexcept;

  std::optional<HitRecord> Hit(const Ray &,
                               Math::Interval) const noexcept override;
  void Accept(HittableVisitor &) const noexcept override;

  [[nodiscard]] double NegInvDensity() const noexcept;
};

class Primitive : public Hittable {
protected:
  std::shared_ptr<Material> mMaterial = nullptr;

public:
  Primitive() = delete;
  ~Primitive() override = default;
  Primitive(const Primitive &) = delete;
  Primitive &operator=(const Primitive &) = delete;
  Primitive(Primitive &&) noexcept = default;
  Primitive &operator=(Primitive &&) noexcept = default;

  Primitive(std::shared_ptr<Material>) noexcept;

  [[nodiscard]] std::shared_ptr<Material> GetMaterial() const noexcept;
};

class Sphere : public Primitive {
private:
  Point3 mCenter = {};
  double mRadius = 0;

public:
  Sphere() = delete;
  ~Sphere() override = default;
  Sphere(const Sphere &) = delete;
  Sphere &operator=(const Sphere &) = delete;
  Sphere(Sphere &&) noexcept = default;
  Sphere &operator=(Sphere &&) noexcept = default;

  Sphere(std::shared_ptr<Material>, const Point3 &, double) noexcept;

  AABB BoundingBox(double time0, double time1) const noexcept override;
  std::optional<HitRecord> Hit(const Ray &,
                               Math::Interval) const noexcept override;
  void Accept(HittableVisitor &) const noexcept override;

  [[nodiscard]] const Point3 &Center() const noexcept;
  [[nodiscard]] double Radius() const noexcept;
};

class MovingSphere : public Primitive {
private:
  Point3 mCenter0 = {}, mCenter1 = {};
  double mRadius = 0;
  double mTime0 = 0, mTime1 = 0;

  [[nodiscard]] Point3 ComputeCenter(double time) const noexcept;

public:
  MovingSphere() = delete;
  ~MovingSphere() override = default;
  MovingSphere(const MovingSphere &) = delete;
  MovingSphere &operator=(const MovingSphere &) = delete;
  MovingSphere(MovingSphere &&) noexcept = default;
  MovingSphere &operator=(MovingSphere &&) noexcept = default;

  MovingSphere(std::shared_ptr<Material>, const Point3 &center0,
               const Point3 &center1, double radius, double time0,
               double time1) noexcept;

  AABB BoundingBox(double time0, double time1) const noexcept override;
  std::optional<HitRecord> Hit(const Ray &,
                               Math::Interval) const noexcept override;
  void Accept(HittableVisitor &) const noexcept override;

  [[nodiscard]] const Point3 &Center0() const noexcept;
  [[nodiscard]] const Point3 &Center1() const noexcept;
  [[nodiscard]] double Radius() const noexcept;
  [[nodiscard]] double Time0() const noexcept;
  [[nodiscard]] double Time1() const noexcept;
};

class Parallelogram : public Primitive {
private:
  Point3 mQ = {};
  Vector3 mU = {};
  Vector3 mV = {};
  Vector3 mN = {};
  Vector3 mW = {};
  double mD = 0;

public:
  Parallelogram() = delete;
  ~Parallelogram() override = default;
  Parallelogram(const Parallelogram &) = delete;
  Parallelogram &operator=(const Parallelogram &) = delete;
  Parallelogram(Parallelogram &&) noexcept = default;
  Parallelogram &operator=(Parallelogram &&) noexcept = default;

  Parallelogram(std::shared_ptr<Material>, const Point3 &Q, const Vector3 &u,
                const Vector3 &v) noexcept;

  AABB BoundingBox(double time0, double time1) const noexcept override;
  std::optional<HitRecord> Hit(const Ray &,
                               Math::Interval) const noexcept override;
  void Accept(HittableVisitor &) const noexcept override;

  [[nodiscard]] const Point3 &Q() const noexcept;
  [[nodiscard]] const Vector3 &U() const noexcept;
  [[nodiscard]] const Vector3 &V() const noexcept;
  [[nodiscard]] const Vector3 &Normal() const noexcept;
  [[nodiscard]] const Vector3 &W() const noexcept;
  [[nodiscard]] double Displacement() const noexcept;
};

class HittableList : public Hittable {

private:
  std::vector<std::unique_ptr<Hittable>> mObjects = {};

public:
  HittableList() = default;
  ~HittableList() override = default;
  HittableList(const HittableList &) = delete;
  HittableList &operator=(const HittableList &) = delete;
  HittableList(HittableList &&) noexcept = default;
  HittableList &operator=(HittableList &&) noexcept = default;

  template <typename... Args>
    requires(std::derived_from<Args, Hittable> && ...) &&
            (!std::is_lvalue_reference_v<Args> && ...)
  HittableList(Args &&...args) noexcept {
    mObjects.reserve(sizeof...(args));
    (mObjects.emplace_back(std::make_unique<Args>(std::forward<Args>(args))),
     ...);
  }

  AABB BoundingBox(double time0, double time1) const noexcept override;
  std::optional<HitRecord> Hit(const Ray &,
                               Math::Interval) const noexcept override;
  void Accept(HittableVisitor &) const noexcept override;

  const std::vector<std::unique_ptr<Hittable>> &Objects() const noexcept;
  std::vector<std::unique_ptr<Hittable>> TakeObjects() noexcept;
  void Add(std::unique_ptr<Hittable>) noexcept;
  void Clear() noexcept;
};

class BVHNode : public Hittable {
private:
  std::unique_ptr<Hittable> mLeft = nullptr;
  std::unique_ptr<Hittable> mRight = nullptr;
  AABB mBox = AABB::Empty();

  BVHNode(std::unique_ptr<Hittable> left, std::unique_ptr<Hittable> right,
          const AABB &box) noexcept;

  [[nodiscard]] static std::unique_ptr<Hittable>
  BuildImpl(std::span<std::unique_ptr<Hittable>>, const double time0,
            const double time1) noexcept;

public:
  BVHNode() = delete;
  ~BVHNode() override = default;
  BVHNode(const BVHNode &) = delete;
  BVHNode &operator=(const BVHNode &) = delete;
  BVHNode(BVHNode &&) noexcept = default;
  BVHNode &operator=(BVHNode &&) noexcept = default;

  AABB BoundingBox(double time0, double time1) const noexcept override;
  std::optional<HitRecord> Hit(const Ray &,
                               Math::Interval) const noexcept override;
  void Accept(HittableVisitor &) const noexcept override;

  [[nodiscard]] const Hittable &Left() const noexcept;
  [[nodiscard]] const Hittable &Right() const noexcept;

  [[nodiscard]] static std::unique_ptr<Hittable>
  Build(HittableList, const double time0, const double time1) noexcept;
};
