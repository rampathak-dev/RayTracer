#pragma once

#include "ShaderInterop.hpp"
#include "webgpu/webgpu.h"
#include <cstddef>
#include <cstdint>
#include <expected>
#include <memory>
#include <string>
#include <vector>

namespace GPUContext {
struct ShaderPayload {
  std::uint32_t rootNodeIndex = 0;
  std::vector<ShaderInterop::Texture> textures = {};
  std::vector<std::uint32_t> pixelData = {};
  std::vector<ShaderInterop::Vec4f> perlinNoiseValues = {};
  std::vector<std::uint32_t> perlinPermutations = {};
  std::vector<ShaderInterop::Material> materials = {};
  std::vector<ShaderInterop::Object> objects = {};
  std::vector<ShaderInterop::Volume> volumes = {};
  std::vector<ShaderInterop::BVHNode> nodes = {};
};

struct BufferHandles {
  using Handle = std::unique_ptr<WGPUBufferImpl, decltype(&wgpuBufferRelease)>;

  Handle camera = {nullptr, &wgpuBufferRelease};
  Handle environment = {nullptr, &wgpuBufferRelease};
  Handle renderSettings = {nullptr, &wgpuBufferRelease};
  Handle imageDimensions = {nullptr, &wgpuBufferRelease};
  Handle textureList = {nullptr, &wgpuBufferRelease};
  Handle pixelData = {nullptr, &wgpuBufferRelease};
  Handle perlinNoiseValues = {nullptr, &wgpuBufferRelease};
  Handle perlinPermutations = {nullptr, &wgpuBufferRelease};
  Handle materialList = {nullptr, &wgpuBufferRelease};
  Handle objectList = {nullptr, &wgpuBufferRelease};
  Handle volumeList = {nullptr, &wgpuBufferRelease};
  Handle nodeList = {nullptr, &wgpuBufferRelease};
  Handle outputBuffer = {nullptr, &wgpuBufferRelease};
};

struct ComputePipelineAndBindGroup {
  std::unique_ptr<WGPUComputePipelineImpl,
                  decltype(&wgpuComputePipelineRelease)>
      pipeline = {nullptr, &wgpuComputePipelineRelease};
  std::unique_ptr<WGPUBindGroupImpl, decltype(&wgpuBindGroupRelease)>
      bindGroup = {nullptr, &wgpuBindGroupRelease};
};

template <std::size_t n>
[[nodiscard]]
constexpr WGPUStringView ToWGPUStringView(const char (&str)[n]) noexcept {
  return WGPUStringView{.data = str, .length = n - 1};
}

[[nodiscard]] std::expected<
    std::unique_ptr<WGPUInstanceImpl, decltype(&wgpuInstanceRelease)>,
    std::string>
CreateWGPUInstance(const WGPUInstanceDescriptor *) noexcept;

[[nodiscard]] std::expected<
    std::unique_ptr<WGPUAdapterImpl, decltype(&wgpuAdapterRelease)>,
    std::string>
RequestWGPUAdapterSync(WGPUInstance,
                       const WGPURequestAdapterOptions *) noexcept;

[[nodiscard]] std::expected<WGPULimits, std::string>
    RequestWGPUDeviceRequiredLimitsSync(WGPUAdapter) noexcept;

[[nodiscard]] std::expected<
    std::unique_ptr<WGPUDeviceImpl, decltype(&wgpuDeviceRelease)>, std::string>
CreateWGPUDevice(WGPUInstance, WGPUAdapter,
                 const WGPUDeviceDescriptor *) noexcept;

[[nodiscard]] std::expected<
    std::unique_ptr<WGPUQueueImpl, decltype(&wgpuQueueRelease)>, std::string>
    RequestWGPUQueueSync(WGPUDevice) noexcept;

[[nodiscard]] std::expected<BufferHandles, std::string> CreateBufferHandles(
    WGPUDevice, const ShaderInterop::Camera &,
    const ShaderInterop::Environment &, const ShaderInterop::RenderSettings &,
    const ShaderInterop::ImageDimensions &, const ShaderPayload &,
    std::size_t outputBufferSize) noexcept;

[[nodiscard]] std::expected<ComputePipelineAndBindGroup, std::string>
CreateComputePipelineAndBindGroup(WGPUDevice, const BufferHandles &) noexcept;
} // namespace GPUContext
