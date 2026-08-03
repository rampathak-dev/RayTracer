#include "headers/Config.hpp"
#include "headers/ImageExport.hpp"
#include "headers/RayTracer.hpp"
#include "headers/Scene.hpp"
#include <chrono>
#include <cstddef>
#include <cstdio>
#include <expected>
#include <format>
#include <iostream>
#include <istream>
#include <memory>
#include <print>
#include <span>
#include <sstream>
#include <string>
#include <type_traits>
#include <utility>

namespace {
struct UserInput {
  Config::Rendering renderConfig = {};
  std::size_t sceneNumber = 1;
  RenderMode renderMode = RenderMode::CPU;
};

std::expected<UserInput, std::string> QueryAndValidateUserInput() noexcept {
  auto input = UserInput{};

  const auto catalog = SceneCatalog::Get();

  for (std::size_t i = 0; i < catalog.size(); ++i)
    std::println(stdout, "{}. {}", i + 1, catalog[i].name);

  std::println(stdout);

  const auto prompt =
      [](const std::string_view message,
         auto &inputVar) noexcept -> std::expected<void, std::string> {
    std::print(stdout, "{}", message);

    auto input = std::string{};
    std::getline(std::cin, input);

    auto stream = std::istringstream{input};

    if (!(stream >> inputVar))
      return std::unexpected("Invalid input");

    stream >> std::ws;

    if (!stream.eof())
      return std::unexpected("Unexpected token");

    std::println(stdout);

    return std::expected<void, std::string>{};
  };

  if (const auto result =
          prompt(std::format("Select Scene [1 - {}]: ", catalog.size()),
                 input.sceneNumber);
      !result)
    return std::unexpected(result.error());

  if (input.sceneNumber < 1 || input.sceneNumber > catalog.size())
    return std::unexpected(
        std::format("Invalid choice '{}' for Scene", input.sceneNumber));

  std::underlying_type_t<RenderMode> rm;
  if (const auto result = prompt("Select RenderMode [1 - CPU, 2 - GPU]: ", rm);
      !result)
    return std::unexpected(result.error());

  if (rm != std::to_underlying(RenderMode::CPU) &&
      rm != std::to_underlying(RenderMode::GPU))
    return std::unexpected(
        std::format("Invalid choice '{}' for RenderMode", rm));

  input.renderMode = static_cast<RenderMode>(rm);

  if (const auto result =
          prompt(std::format("Select Recursion Depth [1 - {}]: ",
                             Config::Rendering::kMaxRecursionDepth),
                 input.renderConfig.samplesPerPixel);
      !result)
    return std::unexpected(result.error());

  if (input.renderConfig.recursionDepth < 1 ||
      input.renderConfig.recursionDepth > Config::Rendering::kMaxRecursionDepth)
    return std::unexpected(
        std::format("Invalid choice '{}' for Recursion Depth",
                    input.renderConfig.recursionDepth));

  if (const auto result =
          prompt(std::format("Select Samples Per Pixel [1 - {}]: ",
                             Config::Rendering::kMaxSamplesPerPixel),
                 input.renderConfig.recursionDepth);
      !result)
    return std::unexpected(result.error());

  if (input.renderConfig.samplesPerPixel < 1 ||
      input.renderConfig.samplesPerPixel >
          Config::Rendering::kMaxSamplesPerPixel)
    return std::unexpected(
        std::format("Invalid choice '{}' for Samples Per Pixel",
                    input.renderConfig.samplesPerPixel));

  return input;
}

std::expected<void, std::string> RunApplication() noexcept {
  auto input = QueryAndValidateUserInput();

  if (!input)
    return std::unexpected(std::move(input.error()));

  auto data = SceneCatalog::Get()[input->sceneNumber - 1].create();

  if (!data) [[unlikely]]
    return std::unexpected(std::move(data.error()));

  auto job = RenderJob{std::move(data.value()), input->renderConfig};

  auto rayTracer = RayTracer::Create(input->renderMode, std::move(job));

  if (!rayTracer) [[unlikely]]
    return std::unexpected(std::move(rayTracer.error()));

  const auto start = std::chrono::high_resolution_clock::now();

  const auto image = rayTracer.value()->Render();

  const auto end = std::chrono::high_resolution_clock::now();

  const std::chrono::duration<double> elapsed = end - start;
  std::println(stdout, "Render time: {:.4f} seconds", elapsed.count());

  if (!image) [[unlikely]]
    return std::unexpected(image.error());

  auto result = ImageExport::PPMAscii("image", image->dimensions.width,
                                      image->dimensions.height, image->pixels);

  if (!result) [[unlikely]]
    return std::unexpected(std::move(result.error()));

  return std::expected<void, std::string>{};
}
} // namespace

int main() {
  const auto result = RunApplication();

  if (!result) {
    std::println(stderr, "{}", result.error());
    return 1;
  }

  return 1;
}
