#include "headers/Scene.hpp"
#include "headers/Colour.hpp"
#include "headers/Config.hpp"
#include "headers/Hittable.hpp"
#include "headers/Material.hpp"
#include "headers/Math.hpp"
#include "headers/Random.hpp"
#include "headers/Texture.hpp"
#include "headers/Vector3.hpp"
#include <array>
#include <expected>
#include <memory>
#include <span>
#include <utility>

namespace {
std::expected<Scene::Data, std::string> BouncingSpheres() noexcept {
  constexpr int gridRange = 10;
  constexpr double airRefractiveIndex = 1.0;
  constexpr double motionStart = 0.0;
  constexpr double motionEnd = 1.0;

  HittableList world = {};

  const auto checkerTexture = std::make_shared<CheckerTexture>(
      0.32, Colour{0.2, 0.3, 0.1}, Colour{0.9, 0.9, 0.9});
  const auto groundMaterial = std::make_shared<Lambertian>(checkerTexture);
  world.Add(
      std::make_unique<Sphere>(groundMaterial, Point3{0, -1000, 0}, 1000));

  for (int a = -gridRange; a < gridRange; ++a) {
    for (int b = -gridRange; b < gridRange; ++b) {
      const double chooseMat = Random::Double();

      const auto center =
          Point3{a + 0.9 * Random::Double(), 0.2, b + 0.9 * Random::Double()};

      if ((center - Point3{4, 0.2, 0}).Length() <= 0.9)
        continue;

      std::shared_ptr<Material> mat;

      if (chooseMat < 0.8) {
        const auto albedo = Random::Colour() * Random::Colour();
        mat = std::make_shared<Lambertian>(albedo);
        const auto center1 = center + Vector3{0, 0.5 * Random::Double(), 0};
        world.Add(std::make_unique<MovingSphere>(mat, center, center1, 0.2,
                                                 motionStart, motionEnd));
      } else if (chooseMat < 0.95) {
        const auto albedo = Colour(0.5, 0.5, 0.5) + (Random::Colour() * 0.5);
        const auto fuzz = 0.5 * Random::Double();
        mat = std::make_shared<Metal>(albedo, fuzz);
        world.Add(std::make_unique<Sphere>(mat, center, 0.2));
      } else {
        mat = std::make_shared<Dielectric>(1.5, airRefractiveIndex);
        world.Add(std::make_unique<Sphere>(mat, center, 0.2));
      }
    }
  }

  world.Add(std::make_unique<Sphere>(
      std::make_shared<Dielectric>(1.5, airRefractiveIndex), Point3{0, 1, 0},
      1.0));
  world.Add(std::make_unique<Sphere>(
      std::make_shared<Lambertian>(Colour{0.4, 0.2, 0.1}), Point3{-4, 1, 0},
      1.0));
  world.Add(std::make_unique<Sphere>(
      std::make_shared<Metal>(Colour{0.7, 0.6, 0.5}, 0.0), Point3{4, 1, 0},
      1.0));

  return Scene::Data{
      .sceneConfig =
          {
              .camera = {.lookFrom = {13, 2, 3},
                         .lookAt = {0, 0, 0},
                         .viewAngle = Math::ToRadians(20),
                         .defocusAngle = Math::ToRadians(0.0),
                         .focusDistance = 10.0},
              .image = {.width = 1200, .aspectRatio = 16.0 / 9.0},
              .motionStart = motionStart,
              .motionEnd = motionEnd,
          },
      .environment = {.skyColour = {0.5, 0.7, 1}, .groundColour = {1, 1, 1}},
      .world = std::move(world)};
}

std::expected<Scene::Data, std::string> CheckeredSpheres() noexcept {
  HittableList world;

  const auto checkerTexture = std::make_shared<CheckerTexture>(
      0.32, Colour{0.2, 0.3, 0.1}, Colour{0.9, 0.9, 0.9});
  const auto checkerMaterial = std::make_shared<Lambertian>(checkerTexture);

  world.Add(std::make_unique<Sphere>(checkerMaterial, Point3{0, -10, 0}, 10));
  world.Add(std::make_unique<Sphere>(checkerMaterial, Point3{0, 10, 0}, 10));

  constexpr double motionStart = 0.0;
  constexpr double motionEnd = 1.0;

  return Scene::Data{
      .sceneConfig =
          {
              .camera = {.lookFrom = {13, 2, 3},
                         .lookAt = {0, 0, 0},
                         .viewAngle = Math::ToRadians(20),
                         .defocusAngle = Math::ToRadians(0.0),
                         .focusDistance = 10.0},
              .image = {.width = 400, .aspectRatio = 16.0 / 9.0},
              .motionStart = motionStart,
              .motionEnd = motionEnd,
          },
      .environment = {.skyColour = {0.5, 0.7, 1}, .groundColour = {1, 1, 1}},
      .world = std::move(world)};
}

std::expected<Scene::Data, std::string> Earth() noexcept {
  HittableList world;

  auto imageTexture = ImageTexture::Create(EARTH_IMAGE);
  if (!imageTexture)
    return std::unexpected(imageTexture.error());

  const auto earthTexture =
      std::make_shared<ImageTexture>(std::move(imageTexture.value()));
  const auto earthSurface = std::make_shared<Lambertian>(earthTexture);

  world.Add(std::make_unique<Sphere>(earthSurface, Point3{0, 0, 0}, 2));

  constexpr double motionStart = 0.0;
  constexpr double motionEnd = 1.0;

  return Scene::Data{
      .sceneConfig =
          {
              .camera = {.lookFrom = {0, 0, 12},
                         .lookAt = {0, 0, 0},
                         .viewAngle = Math::ToRadians(20),
                         .defocusAngle = Math::ToRadians(0.0),
                         .focusDistance = 10.0},
              .image = {.width = 400, .aspectRatio = 16.0 / 9.0},
              .motionStart = motionStart,
              .motionEnd = motionEnd,
          },
      .environment = {.skyColour = {0.5, 0.7, 1}, .groundColour = {1, 1, 1}},
      .world = std::move(world)};
}

std::expected<Scene::Data, std::string> PerlinSpheres() noexcept {
  HittableList world;

  const auto perlinTexture = std::make_shared<NoiseTexture>(4, 10, 7);
  const auto perlinMaterial = std::make_shared<Lambertian>(perlinTexture);

  world.Add(
      std::make_unique<Sphere>(perlinMaterial, Point3{0, -1000, 0}, 1000));
  world.Add(std::make_unique<Sphere>(perlinMaterial, Point3{0, 2, 0}, 2));

  constexpr double motionStart = 0.0;
  constexpr double motionEnd = 1.0;

  return Scene::Data{
      .sceneConfig =
          {
              .camera = {.lookFrom = {13, 2, 3},
                         .lookAt = {0, 0, 0},
                         .viewAngle = Math::ToRadians(20),
                         .defocusAngle = Math::ToRadians(0.0),
                         .focusDistance = 10.0},
              .image = {.width = 400, .aspectRatio = 16.0 / 9.0},
              .motionStart = motionStart,
              .motionEnd = motionEnd,
          },
      .environment = {.skyColour = {0.5, 0.7, 1}, .groundColour = {1, 1, 1}},
      .world = std::move(world)};
}

std::expected<Scene::Data, std::string> Quads() noexcept {
  HittableList world;

  const auto leftRed = std::make_shared<Lambertian>(Colour{1.0, 0.2, 0.2});
  const auto backGreen = std::make_shared<Lambertian>(Colour{0.2, 1.0, 0.2});
  const auto rightBlue = std::make_shared<Lambertian>(Colour{0.2, 0.2, 1.0});
  const auto upperOrange = std::make_shared<Lambertian>(Colour{1.0, 0.5, 0.0});
  const auto lowerTeal = std::make_shared<Lambertian>(Colour{0.2, 0.8, 0.8});

  world.Add(std::make_unique<Parallelogram>(
      leftRed, Point3{-3, -2, 5}, Vector3{0, 0, -4}, Vector3{0, 4, 0}));
  world.Add(std::make_unique<Parallelogram>(
      backGreen, Point3{-2, -2, 0}, Vector3{4, 0, 0}, Vector3{0, 4, 0}));
  world.Add(std::make_unique<Parallelogram>(
      rightBlue, Point3{3, -2, 1}, Vector3{0, 0, 4}, Vector3{0, 4, 0}));
  world.Add(std::make_unique<Parallelogram>(
      upperOrange, Point3{-2, 3, 1}, Vector3{4, 0, 0}, Vector3{0, 0, 4}));
  world.Add(std::make_unique<Parallelogram>(
      lowerTeal, Point3{-2, -3, 5}, Vector3{4, 0, 0}, Vector3{0, 0, -4}));

  constexpr double motionStart = 0.0;
  constexpr double motionEnd = 1.0;

  return Scene::Data{
      .sceneConfig =
          {
              .camera = {.lookFrom = {0, 0, 9},
                         .lookAt = {0, 0, 0},
                         .viewAngle = Math::ToRadians(80),
                         .defocusAngle = Math::ToRadians(0.0),
                         .focusDistance = 10.0},
              .image = {.width = 400, .aspectRatio = 1.0},
              .motionStart = motionStart,
              .motionEnd = motionEnd,
          },
      .environment = {.skyColour = {0.5, 0.7, 1}, .groundColour = {1, 1, 1}},
      .world = std::move(world)};
}

std::expected<Scene::Data, std::string> SimpleLight() noexcept {
  HittableList world;

  const auto perlinTexture = std::make_shared<NoiseTexture>(4, 10, 7);
  const auto perlinMaterial = std::make_shared<Lambertian>(perlinTexture);

  world.Add(
      std::make_unique<Sphere>(perlinMaterial, Point3{0, -1000, 0}, 1000));
  world.Add(std::make_unique<Sphere>(perlinMaterial, Point3{0, 2, 0}, 2));

  // The light colour value exceeds 1 to be brighter than the background.
  const auto light = std::make_shared<DiffuseLight>(Colour{4, 4, 4});
  world.Add(std::make_unique<Sphere>(light, Point3{0, 7, 0}, 2));
  world.Add(std::make_unique<Parallelogram>(
      light, Point3{3, 1, -2}, Vector3{2, 0, 0}, Vector3{0, 2, 0}));

  constexpr double motionStart = 0.0;
  constexpr double motionEnd = 1.0;

  return Scene::Data{
      .sceneConfig = {.camera = {.lookFrom = {26, 3, 6},
                                 .lookAt = {0, 2, 0},
                                 .viewAngle = Math::ToRadians(20),
                                 .defocusAngle = Math::ToRadians(0.0),
                                 .focusDistance = 10.0},
                      .image = {.width = 400, .aspectRatio = 16.0 / 9.0},
                      .motionStart = motionStart,
                      .motionEnd = motionEnd},
      .environment = {.skyColour = {0, 0, 0}, .groundColour = {0, 0, 0}},
      .world = std::move(world)};
}

std::expected<Scene::Data, std::string> CornellBox() noexcept {
  const auto box =
      [](const Point3 &a, const Point3 &b,
         const std::shared_ptr<Material> material) noexcept -> HittableList {
    const auto min = Point3{std::min(a.x(), b.x()), std::min(a.y(), b.y()),
                            std::min(a.z(), b.z())};

    const auto max = Point3{std::max(a.x(), b.x()), std::max(a.y(), b.y()),
                            std::max(a.z(), b.z())};

    const auto dx = Vector3{max.x() - min.x(), 0, 0};
    const auto dy = Vector3{0, max.y() - min.y(), 0};
    const auto dz = Vector3{0, 0, max.z() - min.z()};

    auto front =
        Parallelogram{material, Point3{min.x(), min.y(), max.z()}, dx, dy};
    auto back =
        Parallelogram{material, Point3{max.x(), min.y(), min.z()}, -dx, dy};
    auto right =
        Parallelogram{material, Point3{max.x(), min.y(), max.z()}, -dz, dy};
    auto left =
        Parallelogram{material, Point3{min.x(), min.y(), min.z()}, dz, dy};
    auto top =
        Parallelogram{material, Point3{min.x(), max.y(), max.z()}, -dz, dx};
    auto bottom =
        Parallelogram{material, Point3{min.x(), min.y(), min.z()}, dx, dz};

    return {std::move(front), std::move(back), std::move(right),
            std::move(left),  std::move(top),  std::move(bottom)};
  };

  const auto red = std::make_shared<Lambertian>(Colour{.65, .05, .05});
  const auto white = std::make_shared<Lambertian>(Colour{.73, .73, .73});
  const auto green = std::make_shared<Lambertian>(Colour{.12, .45, .15});
  const auto light = std::make_shared<DiffuseLight>(Colour{15, 15, 15});

  HittableList world;

  world.Add(std::make_unique<Parallelogram>(
      green, Point3{555, 0, 0}, Vector3{0, 555, 0}, Vector3{0, 0, 555}));
  world.Add(std::make_unique<Parallelogram>(
      red, Point3{0, 0, 0}, Vector3{0, 555, 0}, Vector3{0, 0, 555}));
  world.Add(std::make_unique<Parallelogram>(
      light, Point3{343, 554, 332}, Vector3{-130, 0, 0}, Vector3{0, 0, -105}));
  world.Add(std::make_unique<Parallelogram>(
      white, Point3{0, 0, 0}, Vector3{555, 0, 0}, Vector3{0, 0, 555}));
  world.Add(std::make_unique<Parallelogram>(
      white, Point3{555, 555, 555}, Vector3{-555, 0, 0}, Vector3{0, 0, -555}));
  world.Add(std::make_unique<Parallelogram>(
      white, Point3{0, 0, 555}, Vector3{555, 0, 0}, Vector3{0, 555, 0}));

  auto box1 = std::unique_ptr<Hittable>(std::make_unique<HittableList>(
      box(Point3{}, Point3{165, 330, 165}, white)));
  box1 = std::make_unique<RotateY>(std::move(box1), Math::ToRadians(15));
  box1 = std::make_unique<Translate>(std::move(box1), Vector3{265, 0, 295});
  world.Add(std::move(box1));

  auto box2 = std::unique_ptr<Hittable>(std::make_unique<HittableList>(
      box(Point3{}, Point3{165, 165, 165}, white)));
  box2 = std::make_unique<RotateY>(std::move(box2), Math::ToRadians(-18));
  box2 = std::make_unique<Translate>(std::move(box2), Vector3{130, 0, 65});
  world.Add(std::move(box2));

  constexpr double motionStart = 0.0;
  constexpr double motionEnd = 1.0;

  return Scene::Data{
      .sceneConfig = {.camera = {.lookFrom = {278, 278, -800},
                                 .lookAt = {278, 278, 0},
                                 .viewAngle = Math::ToRadians(40),
                                 .defocusAngle = Math::ToRadians(0),
                                 .focusDistance = 10},
                      .image = {.width = 600, .aspectRatio = 1.0},
                      .motionStart = motionStart,
                      .motionEnd = motionEnd},
      .environment = {.skyColour = {0, 0, 0}, .groundColour = {0, 0, 0}},
      .world = std::move(world)};
}

std::expected<Scene::Data, std::string> CornellSmoke() noexcept {
  const auto box =
      [](const Point3 &a, const Point3 &b,
         const std::shared_ptr<Material> material) noexcept -> HittableList {
    const auto min = Point3{std::min(a.x(), b.x()), std::min(a.y(), b.y()),
                            std::min(a.z(), b.z())};

    const auto max = Point3{std::max(a.x(), b.x()), std::max(a.y(), b.y()),
                            std::max(a.z(), b.z())};

    const auto dx = Vector3{max.x() - min.x(), 0, 0};
    const auto dy = Vector3{0, max.y() - min.y(), 0};
    const auto dz = Vector3{0, 0, max.z() - min.z()};

    auto front =
        Parallelogram{material, Point3{min.x(), min.y(), max.z()}, dx, dy};
    auto back =
        Parallelogram{material, Point3{max.x(), min.y(), min.z()}, -dx, dy};
    auto right =
        Parallelogram{material, Point3{max.x(), min.y(), max.z()}, -dz, dy};
    auto left =
        Parallelogram{material, Point3{min.x(), min.y(), min.z()}, dz, dy};
    auto top =
        Parallelogram{material, Point3{min.x(), max.y(), max.z()}, -dz, dx};
    auto bottom =
        Parallelogram{material, Point3{min.x(), min.y(), min.z()}, dx, dz};

    return {std::move(front), std::move(back), std::move(right),
            std::move(left),  std::move(top),  std::move(bottom)};
  };

  const auto red = std::make_shared<Lambertian>(Colour{.65, .05, .05});
  const auto white = std::make_shared<Lambertian>(Colour{.73, .73, .73});
  const auto green = std::make_shared<Lambertian>(Colour{.12, .45, .15});

  const auto light = std::make_shared<DiffuseLight>(Colour{7, 7, 7});

  HittableList world;

  world.Add(std::make_unique<Parallelogram>(
      green, Point3{555, 0, 0}, Vector3{0, 555, 0}, Vector3{0, 0, 555}));
  world.Add(std::make_unique<Parallelogram>(
      red, Point3{0, 0, 0}, Vector3{0, 555, 0}, Vector3{0, 0, 555}));
  world.Add(std::make_unique<Parallelogram>(
      light, Point3{113, 554, 127}, Vector3{330, 0, 0}, Vector3{0, 0, 305}));
  world.Add(std::make_unique<Parallelogram>(
      white, Point3{0, 0, 0}, Vector3{555, 0, 0}, Vector3{0, 0, 555}));
  world.Add(std::make_unique<Parallelogram>(
      white, Point3{555, 555, 555}, Vector3{-555, 0, 0}, Vector3{0, 0, -555}));
  world.Add(std::make_unique<Parallelogram>(
      white, Point3{0, 0, 555}, Vector3{555, 0, 0}, Vector3{0, 555, 0}));

  auto box1 = std::unique_ptr<Hittable>(std::make_unique<HittableList>(
      box(Point3{}, Point3{165, 330, 165}, white)));
  box1 = std::make_unique<RotateY>(std::move(box1), Math::ToRadians(15));
  box1 = std::make_unique<Translate>(std::move(box1), Vector3{265, 0, 295});
  world.Add(
      std::make_unique<ConstantMedium>(std::move(box1), Colour{0, 0, 0}, 0.01));

  auto box2 = std::unique_ptr<Hittable>(std::make_unique<HittableList>(
      box(Point3{}, Point3{165, 165, 165}, white)));
  box2 = std::make_unique<RotateY>(std::move(box2), Math::ToRadians(-18));
  box2 = std::make_unique<Translate>(std::move(box2), Vector3{130, 0, 65});
  world.Add(
      std::make_unique<ConstantMedium>(std::move(box2), Colour{1, 1, 1}, 0.01));

  constexpr double motionStart = 0.0;
  constexpr double motionEnd = 1.0;

  return Scene::Data{
      .sceneConfig = {.camera = {.lookFrom = {278, 278, -800},
                                 .lookAt = {278, 278, 0},
                                 .viewAngle = Math::ToRadians(40),
                                 .defocusAngle = Math::ToRadians(0),
                                 .focusDistance = 10},
                      .image = {.width = 600, .aspectRatio = 1.0},
                      .motionStart = motionStart,
                      .motionEnd = motionEnd},
      .environment = {.skyColour = {0, 0, 0}, .groundColour = {0, 0, 0}},
      .world = std::move(world)};
}

std::expected<Scene::Data, std::string> RTOWCoverPage() noexcept {
  constexpr int gridRange = 10;
  constexpr double airRefractiveIndex = 1.0;
  constexpr double motionStart = 0.0;
  constexpr double motionEnd = 1.0;

  HittableList world;

  const auto groundMaterial =
      std::make_shared<Lambertian>(Colour{0.5, 0.5, 0.5});
  world.Add(
      std::make_unique<Sphere>(groundMaterial, Point3{0, -1000, 0}, 1000));

  for (int a = -gridRange; a < gridRange; ++a) {
    for (int b = -gridRange; b < gridRange; ++b) {
      const double chooseMat = Random::Double();

      const auto center =
          Point3{a + 0.9 * Random::Double(), 0.2, b + 0.9 * Random::Double()};

      if ((center - Point3{4, 0.2, 0}).Length() <= 0.9)
        continue;

      std::shared_ptr<Material> mat;

      if (chooseMat < 0.8) {
        const auto albedo = Random::Colour() * Random::Colour();
        mat = std::make_shared<Lambertian>(albedo);
        world.Add(std::make_unique<Sphere>(mat, center, 0.2));
      } else if (chooseMat < 0.95) {
        const auto albedo = Colour(0.5, 0.5, 0.5) + (Random::Colour() * 0.5);
        const auto fuzz = 0.5 * Random::Double();
        mat = std::make_shared<Metal>(albedo, fuzz);
        world.Add(std::make_unique<Sphere>(mat, center, 0.2));
      } else {
        mat = std::make_shared<Dielectric>(1.5, airRefractiveIndex);
        world.Add(std::make_unique<Sphere>(mat, center, 0.2));
      }
    }
  }

  world.Add(std::make_unique<Sphere>(
      std::make_shared<Dielectric>(1.5, airRefractiveIndex), Point3{0, 1, 0},
      1.0));
  world.Add(std::make_unique<Sphere>(
      std::make_shared<Lambertian>(Colour{0.4, 0.2, 0.1}), Point3{-4, 1, 0},
      1.0));
  world.Add(std::make_unique<Sphere>(
      std::make_shared<Metal>(Colour{0.7, 0.6, 0.5}, 0.0), Point3{4, 1, 0},
      1.0));

  return Scene::Data{
      .sceneConfig = {.camera = {.lookFrom = {13, 2, 3},
                                 .lookAt = {0, 0, 0},
                                 .viewAngle = Math::ToRadians(20),
                                 .defocusAngle = Math::ToRadians(0.0),
                                 .focusDistance = 10.0},
                      .image = {.width = 1200, .aspectRatio = 16.0 / 9.0},
                      .motionStart = motionStart,
                      .motionEnd = motionEnd},
      .environment = {.skyColour = {0.5, 0.7, 1}, .groundColour = {1, 1, 1}},
      .world = std::move(world)};
}

std::expected<Scene::Data, std::string> RTNWCoverPage() noexcept {
  const auto box =
      [](const Point3 &a, const Point3 &b,
         const std::shared_ptr<Material> material) noexcept -> HittableList {
    const auto min = Point3{std::min(a.x(), b.x()), std::min(a.y(), b.y()),
                            std::min(a.z(), b.z())};
    const auto max = Point3{std::max(a.x(), b.x()), std::max(a.y(), b.y()),
                            std::max(a.z(), b.z())};

    const auto dx = Vector3{max.x() - min.x(), 0, 0};
    const auto dy = Vector3{0, max.y() - min.y(), 0};
    const auto dz = Vector3{0, 0, max.z() - min.z()};

    auto front =
        Parallelogram{material, Point3{min.x(), min.y(), max.z()}, dx, dy};
    auto back =
        Parallelogram{material, Point3{max.x(), min.y(), min.z()}, -dx, dy};
    auto right =
        Parallelogram{material, Point3{max.x(), min.y(), max.z()}, -dz, dy};
    auto left =
        Parallelogram{material, Point3{min.x(), min.y(), min.z()}, dz, dy};
    auto top =
        Parallelogram{material, Point3{min.x(), max.y(), max.z()}, -dz, dx};
    auto bottom =
        Parallelogram{material, Point3{min.x(), min.y(), min.z()}, dx, dz};

    return {std::move(front), std::move(back), std::move(right),
            std::move(left),  std::move(top),  std::move(bottom)};
  };

  constexpr double motionStart = 0.0;
  constexpr double motionEnd = 1.0;

  HittableList world;

  // earth sphere
  auto earthImage = ImageTexture::Create(EARTH_IMAGE);

  if (!earthImage)
    return std::unexpected(earthImage.error());

  const auto earthTexture =
      std::make_shared<ImageTexture>(std::move(earthImage.value()));
  world.Add(std::make_unique<Sphere>(std::make_shared<Lambertian>(earthTexture),
                                     Point3{400, 200, 400}, 100));

  // ground boxes
  const auto ground = std::make_shared<Lambertian>(Colour{0.48, 0.83, 0.53});
  constexpr int boxesPerSide = 20;
  auto groundBoxes = HittableList{};

  for (int i = 0; i < boxesPerSide; ++i)
    for (int j = 0; j < boxesPerSide; ++j) {
      constexpr double w = 100.0;
      const double x0 = -1000.0 + i * w;
      const double z0 = -1000.0 + j * w;
      const double x1 = x0 + w;
      const double y1 = Random::Double(Math::Interval{1, 101});
      const double z1 = z0 + w;
      groundBoxes.Add(std::make_unique<HittableList>(
          box(Point3{x0, 0, z0}, Point3{x1, y1, z1}, ground)));
    }

  world.Add(BVHNode::Build(std::move(groundBoxes), motionStart, motionEnd));

  // light
  const auto light = std::make_shared<DiffuseLight>(Colour{7, 7, 7});
  world.Add(std::make_unique<Parallelogram>(
      light, Point3{123, 554, 147}, Vector3{300, 0, 0}, Vector3{0, 0, 265}));

  // moving sphere
  const auto sphereMaterial =
      std::make_shared<Lambertian>(Colour{0.7, 0.3, 0.1});
  const auto center1 = Point3{400, 400, 200};
  const auto center2 = center1 + Vector3{30, 0, 0};
  world.Add(std::make_unique<MovingSphere>(sphereMaterial, center1, center2, 50,
                                           motionStart, motionEnd));

  // dielectric sphere
  world.Add(std::make_unique<Sphere>(std::make_shared<Dielectric>(1.5, 1.0),
                                     Point3{260, 150, 45}, 50));

  // metal sphere
  world.Add(std::make_unique<Sphere>(
      std::make_shared<Metal>(Colour{0.8, 0.8, 0.9}, 1.0), Point3{0, 150, 145},
      50));

  // blue sphere
  const auto boundary1Mat = std::make_shared<Dielectric>(1.5, 1.0);
  world.Add(std::make_unique<Sphere>(boundary1Mat, Point3{360, 150, 145}, 70));
  world.Add(std::make_unique<ConstantMedium>(
      std::make_unique<Sphere>(boundary1Mat, Point3{360, 150, 145}, 70),
      Colour{0.2, 0.4, 0.9}, 0.2));

  // mist
  world.Add(std::make_unique<ConstantMedium>(
      std::make_unique<Sphere>(std::make_shared<Dielectric>(1.5, 1.0),
                               Point3{0, 0, 0}, 5000),
      Colour{1, 1, 1}, 0.0001));

  // perlin noise sphere
  const auto perlinTexture = std::make_shared<NoiseTexture>(0.2, 10, 7);
  world.Add(std::make_unique<Sphere>(
      std::make_shared<Lambertian>(perlinTexture), Point3{220, 280, 300}, 80));

  // sphere cluster
  const auto white = std::make_shared<Lambertian>(Colour{0.73, 0.73, 0.73});
  constexpr int clusterSize = 1000;
  auto clusterBoxes = HittableList{};
  for (int j = 0; j < clusterSize; ++j)
    clusterBoxes.Add(
        std::make_unique<Sphere>(white,
                                 Point3{Random::Double(Math::Interval{0, 165}),
                                        Random::Double(Math::Interval{0, 165}),
                                        Random::Double(Math::Interval{0, 165})},
                                 10));

  auto cluster =
      BVHNode::Build(std::move(clusterBoxes), motionStart, motionEnd);
  cluster = std::make_unique<RotateY>(std::move(cluster), Math::ToRadians(15));
  cluster =
      std::make_unique<Translate>(std::move(cluster), Vector3{-100, 270, 395});
  world.Add(std::move(cluster));

  return Scene::Data{
      .sceneConfig = {.camera = {.lookFrom = {478, 278, -600},
                                 .lookAt = {278, 278, 0},
                                 .viewAngle = Math::ToRadians(40),
                                 .defocusAngle = Math::ToRadians(0),
                                 .focusDistance = 10},
                      .image = {.width = 800, .aspectRatio = 1.0},
                      .motionStart = motionStart,
                      .motionEnd = motionEnd},
      .environment = {.skyColour = {0, 0, 0}, .groundColour = {0, 0, 0}},
      .world = std::move(world)};
}

template <std::expected<Scene::Data, std::string> (*Generator)()>
std::expected<Scene::Data, std::string> Validator() noexcept {
  auto data = Generator();

  if (!data)
    return std::unexpected(data.error());

  if (!(data->sceneConfig.image.width > 0 &&
        data->sceneConfig.image.aspectRatio > 0))
    return std::unexpected("imageWidth and aspectRatio must be non-negative");

  if (data->sceneConfig.motionStart < 0)
    return std::unexpected("shutterOpenTime must be non-negative");

  if (data->sceneConfig.motionEnd < data->sceneConfig.motionStart)
    return std::unexpected("shutterCloseTime must not precede open time");

  if ((data->sceneConfig.camera.lookFrom - data->sceneConfig.camera.lookAt)
          .NearZero())
    return std::unexpected(
        "lookFrom and lookAt cannot be the exact same point");

  if (data->sceneConfig.camera.focusDistance < 0)
    return std::unexpected("focusDistance must be positive");

  if (data->sceneConfig.camera.viewAngle < 0)
    return std::unexpected("viewAngle must be positive");

  return data;
}
} // namespace

std::span<const SceneCatalog::SceneEntry> SceneCatalog::Get() noexcept {
  static constexpr std::array entries = {
      SceneEntry{.name = "Bouncing Spheres",
                 .create = Validator<BouncingSpheres>},
      SceneEntry{.name = "Checkered Spheres",
                 .create = Validator<CheckeredSpheres>},
      SceneEntry{.name = "Earth", .create = Validator<Earth>},
      SceneEntry{.name = "Perlin Spheres", .create = Validator<PerlinSpheres>},
      SceneEntry{.name = "Quads", .create = Validator<Quads>},
      SceneEntry{.name = "Simple Light", .create = Validator<SimpleLight>},
      SceneEntry{.name = "Cornell Box", .create = Validator<CornellBox>},
      SceneEntry{.name = "Cornell Smoke", .create = Validator<CornellSmoke>},
      SceneEntry{.name = "RTOW Cover Page", .create = Validator<RTOWCoverPage>},
      SceneEntry{.name = "RTNW Cover Page", .create = Validator<RTNWCoverPage>},
  };

  return entries;
}
