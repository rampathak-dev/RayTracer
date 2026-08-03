#include "headers/Hittable.hpp"
#include "headers/HittableVisitor.hpp"
#include "headers/Math.hpp"
#include "headers/Random.hpp"
#include "headers/Ray.hpp"
#include "headers/Vector3.hpp"
#include <algorithm>
#include <cassert>
#include <cmath>
#include <cstddef>
#include <memory>
#include <optional>
#include <utility>
#include <vector>

namespace {
std::tuple<double, double> ComputeSphereUV(const Point3 &p) noexcept {
  const auto phi = atan2(-p.z(), p.x()) + Math::kPi;
  const auto theta = acos(-p.y());

  return {phi / (2 * Math::kPi), theta / Math::kPi};
}

void PadToMinimums(Math::Interval &x, Math::Interval &y, Math::Interval &z,
                   const double delta) noexcept {
  if (x.Size() < delta)
    x = x.Expand(delta);

  if (y.Size() < delta)
    y = y.Expand(delta);

  if (z.Size() < delta)
    z = z.Expand(delta);
}
} // namespace

AABB::AABB(Math::Interval x, Math::Interval y, Math::Interval z) noexcept
    : mE{x, y, z} {
  constexpr double delta = 0.0001;
  PadToMinimums(mE[0], mE[1], mE[2], delta);
}

AABB::AABB(const Point3 &a, const Point3 &b) noexcept {
  mE[0] =
      (a[0] <= b[0]) ? Math::Interval(a[0], b[0]) : Math::Interval(b[0], a[0]);
  mE[1] =
      (a[1] <= b[1]) ? Math::Interval(a[1], b[1]) : Math::Interval(b[1], a[1]);
  mE[2] =
      (a[2] <= b[2]) ? Math::Interval(a[2], b[2]) : Math::Interval(b[2], a[2]);

  constexpr double delta = 0.0001;
  PadToMinimums(mE[0], mE[1], mE[2], delta);
}

AABB::AABB(const AABB &a, const AABB &b) noexcept {
  mE[0] = {a.AxisInterval(0), b.AxisInterval(0)};
  mE[1] = {a.AxisInterval(1), b.AxisInterval(1)};
  mE[2] = {a.AxisInterval(2), b.AxisInterval(2)};

  constexpr double delta = 0.0001;
  PadToMinimums(mE[0], mE[1], mE[2], delta);
}

AABB NullHittable::BoundingBox(double, double) const noexcept {
  return AABB::Empty();
}

bool AABB::Hit(const Ray &ray, Math::Interval interval) const noexcept {
  for (std::size_t axis = 0; axis <= 2; ++axis) {
    const auto originA = ray.Origin()[axis];
    const auto invDirectionA = 1 / ray.Direction()[axis];

    auto t0 = (mE[axis].Min() - originA) * invDirectionA;
    auto t1 = (mE[axis].Max() - originA) * invDirectionA;

    if (invDirectionA < 0)
      std::swap(t0, t1);

    auto tMin = interval.Min();
    auto tMax = interval.Max();

    tMin = t0 > tMin ? t0 : tMin;
    tMax = t1 < tMax ? t1 : tMax;

    if (tMax <= tMin)
      return false;

    interval.SetMin(tMin);
    interval.SetMax(tMax);
  }

  return true;
}

std::size_t AABB::LongestAxis() const noexcept {
  const auto xLen = mE[0].Size();
  const auto yLen = mE[1].Size();
  const auto zLen = mE[2].Size();

  if (xLen > yLen)
    return xLen > zLen ? 0 : 2;
  else
    return yLen > zLen ? 1 : 2;
}

Math::Interval AABB::AxisInterval(const std::size_t axis) const noexcept {
  assert(axis <= 2);
  return mE[axis];
}

AABB AABB::Empty() noexcept { return {}; }

AABB operator+(const AABB &aabb, const Vector3 &u) noexcept {
  return {aabb.AxisInterval(0) + u.x(), aabb.AxisInterval(1) + u.y(),
          aabb.AxisInterval(2) + u.z()};
}

AABB operator+(const Vector3 &u, const AABB &aabb) noexcept { return aabb + u; }

std::optional<HitRecord> NullHittable::Hit(const Ray &,
                                           Math::Interval) const noexcept {
  return std::nullopt;
}

void NullHittable::Accept(HittableVisitor &visitor) const noexcept {
  visitor.Visit(*this);
}

Translate::Translate(std::unique_ptr<Hittable> object,
                     const Vector3 &offset) noexcept
    : mOffset{offset}, mObject{std::move(object)} {}

AABB Translate::BoundingBox(const double time0,
                            const double time1) const noexcept {
  return mObject->BoundingBox(time0, time1) + mOffset;
}

std::optional<HitRecord>
Translate::Hit(const Ray &ray, Math::Interval interval) const noexcept {
  const auto offsetRay =
      Ray{ray.Origin() - mOffset, ray.Direction(), ray.Time()};

  auto record = mObject->Hit(offsetRay, interval);
  if (record.has_value())
    record->intersection += mOffset;

  return record;
}

void Translate::Accept(HittableVisitor &visitor) const noexcept {
  visitor.Visit(*this);
}

const Vector3 &Translate::Offset() const noexcept { return mOffset; }

const Hittable &Translate::Object() const noexcept { return *mObject; }

RotateY::RotateY(std::unique_ptr<Hittable> object, const double theta) noexcept
    : mObject{std::move(object)}, mSinTheta{std::sin(theta)},
      mCosTheta{std::cos(theta)} {}

AABB RotateY::BoundingBox(const double time0,
                          const double time1) const noexcept {
  const auto box = mObject->BoundingBox(time0, time1);

  auto min = Point3{Math::kInfinity, Math::kInfinity, Math::kInfinity};
  auto max = Point3{-Math::kInfinity, -Math::kInfinity, -Math::kInfinity};

  for (std::size_t i = 0; i <= 1; ++i)
    for (std::size_t j = 0; j <= 1; ++j)
      for (std::size_t k = 0; k <= 1; ++k) {
        const auto x =
            i * box.AxisInterval(0).Min() + (1 - i) * box.AxisInterval(0).Max();
        const auto y =
            j * box.AxisInterval(1).Min() + (1 - j) * box.AxisInterval(1).Max();
        const auto z =
            k * box.AxisInterval(2).Min() + (1 - k) * box.AxisInterval(2).Max();

        const auto newX = mCosTheta * x + mSinTheta * z;
        const auto newZ = -mSinTheta * x + mCosTheta * z;

        const auto tester = Point3{newX, y, newZ};

        for (std::size_t l = 0; l <= 2; ++l) {
          min[l] = std::min(min[l], tester[l]);
          max[l] = std::max(max[l], tester[l]);
        }
      }

  return {min, max};
}

std::optional<HitRecord> RotateY::Hit(const Ray &ray,
                                      Math::Interval tInterval) const noexcept {
  // rotate ray by -θ
  const auto origin =
      Point3{mCosTheta * ray.Origin().x() - mSinTheta * ray.Origin().z(),
             ray.Origin().y(),
             mSinTheta * ray.Origin().x() + mCosTheta * ray.Origin().z()};

  const auto direction = Vector3{
      mCosTheta * ray.Direction().x() - mSinTheta * ray.Direction().z(),
      ray.Direction().y(),
      mSinTheta * ray.Direction().x() + mCosTheta * ray.Direction().z()};

  const auto rotated = Ray{origin, direction, ray.Time()};

  auto record = mObject->Hit(rotated, tInterval);

  if (!record.has_value())
    return std::nullopt;

  // rotate the intersection point and normal by θ
  record->intersection = Point3{mCosTheta * record->intersection.x() +
                                    mSinTheta * record->intersection.z(),
                                record->intersection.y(),
                                -mSinTheta * record->intersection.x() +
                                    mCosTheta * record->intersection.z()};

  record->normal =
      Point3{mCosTheta * record->normal.x() + mSinTheta * record->normal.z(),
             record->normal.y(),
             -mSinTheta * record->normal.x() + mCosTheta * record->normal.z()};

  return record;
}

void RotateY::Accept(HittableVisitor &visitor) const noexcept {
  visitor.Visit(*this);
}

const Hittable &RotateY::Object() const noexcept { return *mObject; }

double RotateY::SinTheta() const noexcept { return mSinTheta; }

double RotateY::CosTheta() const noexcept { return mCosTheta; }

Volume::Volume(std::unique_ptr<Hittable> boundary,
               const std::shared_ptr<Texture> texture) noexcept
    : mBoundary{std::move(boundary)},
      mPhaseFunction{std::make_shared<Isotropic>(texture)} {}

Volume::Volume(std::unique_ptr<Hittable> boundary,
               const Colour &albedo) noexcept
    : mBoundary{std::move(boundary)},
      mPhaseFunction{std::make_shared<Isotropic>(albedo)} {}

AABB Volume::BoundingBox(const double time0,
                         const double time1) const noexcept {
  return mBoundary->BoundingBox(time0, time1);
}

const Hittable &Volume::Boundary() const noexcept { return *mBoundary; }

const Material &Volume::PhaseFunction() const noexcept {
  return *mPhaseFunction;
}

ConstantMedium::ConstantMedium(std::unique_ptr<Hittable> boundary,
                               const std::shared_ptr<Texture> texture,
                               const double density) noexcept
    : Volume{std::move(boundary), texture}, mNegInvDensity{-1 / density} {}

ConstantMedium::ConstantMedium(std::unique_ptr<Hittable> boundary,
                               const Colour &albedo,
                               const double density) noexcept
    : Volume{std::move(boundary), albedo}, mNegInvDensity{-1 / density} {}

std::optional<HitRecord>
ConstantMedium::Hit(const Ray &ray, Math::Interval tInterval) const noexcept {
  auto record1 =
      mBoundary->Hit(ray, Math::Interval{-Math::kInfinity, Math::kInfinity});

  if (!record1.has_value())
    return std::nullopt;

  auto record2 =
      mBoundary->Hit(ray, Math::Interval{record1->t + 0.0001, Math::kInfinity});

  if (!record2.has_value())
    return std::nullopt;

  record1->t = record1->t < tInterval.Min() ? tInterval.Min() : record1->t;
  record2->t = record2->t > tInterval.Max() ? tInterval.Max() : record2->t;

  if (record1->t >= record2->t)
    return std::nullopt;

  record1->t = std::max(record1->t, 0.0);

  const auto rayLength = ray.Direction().Length();
  const auto distanceInsideBoundary = rayLength * (record2->t - record1->t);
  const auto hitDistance = mNegInvDensity * std::log(Random::Double());

  if (hitDistance > distanceInsideBoundary)
    return std::nullopt;

  const auto t = record1->t + hitDistance / rayLength;
  return HitRecord{
      .intersection = ray.At(t),
      .normal = Vector3{1, 0, 0}, // arbitrary
      .material = mPhaseFunction,
      .t = t,
      .frontFace = true, // arbitrary
  };
}

void ConstantMedium::Accept(HittableVisitor &visitor) const noexcept {
  visitor.Visit(*this);
}

double ConstantMedium::NegInvDensity() const noexcept { return mNegInvDensity; }

Primitive::Primitive(const std::shared_ptr<Material> material) noexcept
    : mMaterial{material} {}

std::shared_ptr<Material> Primitive::GetMaterial() const noexcept {
  return mMaterial;
}

Sphere::Sphere(std::shared_ptr<Material> material, const Point3 &center,
               double radius) noexcept
    : Primitive{material}, mCenter{center}, mRadius{std::fmax(radius, 0)} {}

AABB Sphere::BoundingBox(const double, // time0 unused
                         const double  // time1 unused
) const noexcept {
  return AABB{
      mCenter - Vector3{mRadius, mRadius, mRadius},
      mCenter + Vector3{mRadius, mRadius, mRadius},
  };
}

std::optional<HitRecord> Sphere::Hit(const Ray &ray,
                                     Math::Interval tInterval) const noexcept {
  const auto oc = mCenter - ray.Origin(); // vector from ray origin to center
  const auto a = ray.Direction().LengthSquared();
  // b = -2h, where h = ray.Direction() . oc,
  // substituting b = -2h in the quadratic formula, simplifies the equation
  const auto h = Vector3::Dot(ray.Direction(), oc);
  const auto c = oc.LengthSquared() - mRadius * mRadius;

  const auto discriminant = h * h - a * c;
  if (discriminant < 0)
    return std::nullopt;

  const auto sqrtD = std::sqrt(discriminant);
  auto t = (h - sqrtD) / a;

  if (!tInterval.Surrounds(t)) {
    t = (h + sqrtD) / a;
    if (!tInterval.Surrounds(t))
      return std::nullopt;
  }
  const auto intersection = ray.At(t); // Intersection point
  const auto outwardNormal =
      (intersection - mCenter) /
      mRadius; // Points from center to intersection point (outward)

  const auto [u, v] = ComputeSphereUV(outwardNormal);
  const bool frontFace = Vector3::Dot(ray.Direction(), outwardNormal) <
                         0; // True if ray originates from outside the sphere

  // We flip normal = -normal, if ray originates from inside,
  // to keep the sign of the dot product consistent
  return HitRecord{.intersection = intersection,
                   .normal = frontFace ? outwardNormal : -outwardNormal,
                   .material = mMaterial,
                   .t = t,
                   .u = u,
                   .v = v,
                   .frontFace = frontFace};
}

void Sphere::Accept(HittableVisitor &visitor) const noexcept {
  visitor.Visit(*this);
}

const Point3 &Sphere::Center() const noexcept { return mCenter; }

double Sphere::Radius() const noexcept { return mRadius; }

Point3 MovingSphere::ComputeCenter(double time) const noexcept {
  return mCenter0 +
         ((time - mTime0) / (mTime1 - mTime0)) * (mCenter1 - mCenter0);
}

MovingSphere::MovingSphere(std::shared_ptr<Material> material,
                           const Point3 &center0, const Point3 &center1,
                           double radius, double time0, double time1) noexcept
    : Primitive{material}, mCenter0{center0}, mCenter1{center1},
      mRadius{radius}, mTime0{time0}, mTime1{time1} {}

AABB MovingSphere::BoundingBox(const double time0,
                               const double time1) const noexcept {
  const auto center0 = this->ComputeCenter(time0);
  const auto center1 = this->ComputeCenter(time1);

  const AABB box0{center0 - Vector3{mRadius, mRadius, mRadius},
                  center0 + Vector3{mRadius, mRadius, mRadius}};
  const AABB box1{center1 - Vector3{mRadius, mRadius, mRadius},
                  center1 + Vector3{mRadius, mRadius, mRadius}};

  return {box0, box1};
}

std::optional<HitRecord>
MovingSphere::Hit(const Ray &ray, Math::Interval tInterval) const noexcept {
  const auto center = this->ComputeCenter(ray.Time());

  const auto oc = center - ray.Origin(); // Vector from ray origin to center
  const auto a = ray.Direction().LengthSquared();
  // b = -2h, where h = ray.Direction() . oc,
  // substituting b = -2h in the quadratic formula, simplifies the equation
  const auto h = Vector3::Dot(ray.Direction(), oc);
  const auto c = oc.LengthSquared() - mRadius * mRadius;

  const auto discriminant = h * h - a * c;
  if (discriminant < 0)
    return std::nullopt;

  const auto sqrtD = std::sqrt(discriminant);
  auto t = (h - sqrtD) / a;

  if (!tInterval.Surrounds(t)) {
    t = (h + sqrtD) / a;
    if (!tInterval.Surrounds(t))
      return std::nullopt;
  }
  const auto intersection = ray.At(t); // Intersection point
  const auto outwardNormal =
      (intersection - center) /
      mRadius; // Points from center to intersection point (outward)

  const auto [u, v] = ComputeSphereUV(outwardNormal);
  const bool frontFace = Vector3::Dot(ray.Direction(), outwardNormal) <
                         0; // True if ray originates from outside the sphere

  // We flip normal = -normal, if ray originates from inside,
  // to keep the sign of the dot product consistent
  return HitRecord{.intersection = intersection,
                   .normal = frontFace ? outwardNormal : -outwardNormal,
                   .material = mMaterial,
                   .t = t,
                   .u = u,
                   .v = v,
                   .frontFace = frontFace};
}

void MovingSphere::Accept(HittableVisitor &visitor) const noexcept {
  visitor.Visit(*this);
}

const Point3 &MovingSphere::Center0() const noexcept { return mCenter0; }

const Point3 &MovingSphere::Center1() const noexcept { return mCenter1; }

double MovingSphere::Radius() const noexcept { return mRadius; }

double MovingSphere::Time0() const noexcept { return mTime0; }

double MovingSphere::Time1() const noexcept { return mTime1; }

Parallelogram::Parallelogram(const std::shared_ptr<Material> material,
                             const Point3 &Q, const Vector3 &u,
                             const Vector3 &v) noexcept
    : Primitive{material}, mQ{Q}, mU{u}, mV{v}, mD{Vector3::Dot(mN, Q)} {
  const auto n = Vector3::Cross(u, v);
  mN = Vector3::Normalize(n);
  mW = n / n.LengthSquared();
  mD = Vector3::Dot(mN, Q);
}

AABB Parallelogram::BoundingBox(double, double) const noexcept {
  const auto diagonal1Box = AABB{mQ, mQ + mU + mV};
  const auto diagonal2Box = AABB{mQ + mU, mQ + mV};
  return {diagonal1Box, diagonal2Box};
}

std::optional<HitRecord>
Parallelogram::Hit(const Ray &ray, Math::Interval tInterval) const noexcept {
  const auto denominator = Vector3::Dot(mN, ray.Direction());

  if (std::fabs(denominator) < Math::kEpsilon)
    return std::nullopt;

  const auto t = (mD - Vector3::Dot(mN, ray.Origin())) / denominator;

  if (!tInterval.Contains(t))
    return std::nullopt;

  const auto intersection = ray.At(t);
  const auto planarHitPointVector = intersection - mQ;

  const auto alpha = Vector3::Dot(mW, Vector3::Cross(planarHitPointVector, mV));
  const auto beta = Vector3::Dot(mW, Vector3::Cross(mU, planarHitPointVector));

  const auto unitInterval = Math::Interval{0, 1};
  if (!(unitInterval.Contains(alpha) && unitInterval.Contains(beta)))
    return std::nullopt;

  const auto frontFace = Vector3::Dot(mN, ray.Direction()) < 0;

  return HitRecord{
      .intersection = intersection,
      .normal = frontFace ? mN : -mN,
      .material = mMaterial,
      .t = t,
      .u = alpha,
      .v = beta,
      .frontFace = frontFace,
  };
}

void Parallelogram::Accept(HittableVisitor &visitor) const noexcept {
  visitor.Visit(*this);
}

const Point3 &Parallelogram::Q() const noexcept { return mQ; }

const Point3 &Parallelogram::U() const noexcept { return mU; }

const Point3 &Parallelogram::V() const noexcept { return mV; }

const Vector3 &Parallelogram::Normal() const noexcept { return mN; }

const Vector3 &Parallelogram::W() const noexcept { return mW; }

double Parallelogram::Displacement() const noexcept { return mD; }

AABB HittableList::BoundingBox(const double time0,
                               const double time1) const noexcept {
  auto box = AABB::Empty();

  for (const auto &object : mObjects)
    box = {box, object->BoundingBox(time0, time1)};

  return box;
}

std::optional<HitRecord>
HittableList::Hit(const Ray &ray, Math::Interval tInterval) const noexcept {
  std::optional<HitRecord> record = std::nullopt;

  for (const auto &object : mObjects)
    if (auto temp = object->Hit(ray, tInterval)) {
      record = temp;
      tInterval.SetMax(temp->t);
    }

  return record;
}

void HittableList::Accept(HittableVisitor &visitor) const noexcept {
  visitor.Visit(*this);
}

const std::vector<std::unique_ptr<Hittable>> &
HittableList::Objects() const noexcept {
  return mObjects;
}

std::vector<std::unique_ptr<Hittable>> HittableList::TakeObjects() noexcept {
  return std::move(mObjects);
}

void HittableList::Add(std::unique_ptr<Hittable> hittable) noexcept {
  mObjects.emplace_back(std::move(hittable));
}

void HittableList::Clear() noexcept { mObjects.clear(); }

BVHNode::BVHNode(std::unique_ptr<Hittable> left,
                 std::unique_ptr<Hittable> right, const AABB &box) noexcept
    : mLeft{std::move(left)}, mRight{std::move(right)}, mBox{box} {}

std::unique_ptr<Hittable>
BVHNode::BuildImpl(std::span<std::unique_ptr<Hittable>> hittables,
                   const double time0, const double time1) noexcept {
  const auto n = hittables.size();

  if (n == 0)
    return std::make_unique<NullHittable>();

  if (n == 1)
    return std::move(hittables[0]);

  if (n == 2) {
    const auto box = AABB{hittables[0]->BoundingBox(time0, time1),
                          hittables[1]->BoundingBox(time0, time1)};
    return std::unique_ptr<BVHNode>{
        new BVHNode{std::move(hittables[0]), std::move(hittables[1]), box}};
  }

  auto allBox = AABB::Empty();
  for (const auto &hittable : hittables)
    allBox = {allBox, hittable->BoundingBox(time0, time1)};

  const auto axis = allBox.LongestAxis();

  const auto boxComparator =
      [axis, time0, time1](const std::unique_ptr<Hittable> &a,
                           const std::unique_ptr<Hittable> &b) noexcept {
        const auto aBox = a->BoundingBox(time0, time1);
        const auto bBox = b->BoundingBox(time0, time1);

        return aBox.AxisInterval(axis).Midpoint() <
               bBox.AxisInterval(axis).Midpoint();
      };

  const auto mid = n / 2;
  std::ranges::nth_element(hittables, hittables.begin() + mid, boxComparator);

  auto left = BuildImpl(hittables.subspan(0, mid), time0, time1);
  auto right = BuildImpl(hittables.subspan(mid), time0, time1);
  const auto box =
      AABB{left->BoundingBox(time0, time1), right->BoundingBox(time0, time1)};

  return std::unique_ptr<BVHNode>{
      new BVHNode{std::move(left), std::move(right), box}};
}

AABB BVHNode::BoundingBox(double, // time0 unused
                          double  // time1 unused
) const noexcept {
  return mBox;
}

std::optional<HitRecord> BVHNode::Hit(const Ray &ray,
                                      Math::Interval tInterval) const noexcept {
  if (!mBox.Hit(ray, tInterval))
    return std::nullopt;

  const auto leftRecord = mLeft->Hit(ray, tInterval);
  const auto rightRecord = mRight->Hit(ray, tInterval);

  std::optional<HitRecord> result = std::nullopt;

  if (leftRecord && rightRecord)
    result = leftRecord->t < rightRecord->t ? leftRecord : rightRecord;
  else if (leftRecord)
    result = leftRecord;
  else if (rightRecord)
    result = rightRecord;

  return result;
}

void BVHNode::Accept(HittableVisitor &visitor) const noexcept {
  visitor.Visit(*this);
}

const Hittable &BVHNode::Left() const noexcept { return *mLeft; }

const Hittable &BVHNode::Right() const noexcept { return *mRight; }

std::unique_ptr<Hittable> BVHNode::Build(HittableList hittableList,
                                         const double time0,
                                         const double time1) noexcept {
  auto hittables = hittableList.TakeObjects();
  return BuildImpl(hittables, time0, time1);
}
