#include "headers/RayTracer.hpp"
#include "headers/RenderJob.hpp"
#include <cstddef>
#include <expected>
#include <map>
#include <memory>
#include <string>
#include <utility>

namespace {
std::map<RenderMode, RayTracer::FactoryFunction> &GetRegistry() noexcept {
  static std::map<RenderMode, RayTracer::FactoryFunction> registry;
  return registry;
}
} // namespace

void RayTracer::Register(const RenderMode mode,
                         const FactoryFunction factory) noexcept {
  GetRegistry()[mode] = factory;
}

std::expected<std::unique_ptr<RayTracer>, std::string>
RayTracer::Create(const RenderMode renderMode, RenderJob renderJob) noexcept {
  const auto &registry = GetRegistry();

  auto it = registry.find(renderMode);

  if (it == registry.end())
    return std::unexpected(std::string{"Unexpected RenderMode: "} +
                           std::to_string(std::to_underlying(renderMode)));

  return it->second(std::move(renderJob));
}
