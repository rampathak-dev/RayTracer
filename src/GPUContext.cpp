#include "headers/GPUContext.hpp"
#include "headers/ShaderInterop.hpp"
#include "webgpu/webgpu.h"
#include <algorithm>
#include <atomic>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <expected>
#include <format>
#include <fstream>
#include <iterator>
#include <memory>
#include <print>
#include <string>
#include <string_view>
#include <thread>
#include <utility>

namespace {
enum class GroupZeroBindings : std::uint32_t {
  Camera = 0,
  Environment,
  RenderSettings,
  ImageDimensions,
  TextureList,
  PixelData,
  PerlinNoiseValues,
  PerlinPermutations,
  MaterialList,
  VolumeList,
  ObjectList,
  NodeList,
  OutputBuffer
};

struct BindingDescriptor {
  GroupZeroBindings binding;
  WGPUBufferBindingType type;
  std::uint64_t minBindingSize;
  GPUContext::BufferHandles::Handle GPUContext::BufferHandles::*member;
};

constexpr std::array kBindingDescriptors = {
    BindingDescriptor{GroupZeroBindings::Camera, WGPUBufferBindingType_Uniform,
                      sizeof(ShaderInterop::Camera),
                      &GPUContext::BufferHandles::camera},
    BindingDescriptor{GroupZeroBindings::Environment,
                      WGPUBufferBindingType_Uniform,
                      sizeof(ShaderInterop::Environment),
                      &GPUContext::BufferHandles::environment},
    BindingDescriptor{GroupZeroBindings::RenderSettings,
                      WGPUBufferBindingType_Uniform,
                      sizeof(ShaderInterop::RenderSettings),
                      &GPUContext::BufferHandles::renderSettings},
    BindingDescriptor{GroupZeroBindings::ImageDimensions,
                      WGPUBufferBindingType_Uniform,
                      sizeof(ShaderInterop::ImageDimensions),
                      &GPUContext::BufferHandles::imageDimensions},
    BindingDescriptor{GroupZeroBindings::TextureList,
                      WGPUBufferBindingType_ReadOnlyStorage,
                      sizeof(ShaderInterop::TextureListHeader) +
                          sizeof(ShaderInterop::Texture),
                      &GPUContext::BufferHandles::textureList},
    BindingDescriptor{
        GroupZeroBindings::PixelData, WGPUBufferBindingType_ReadOnlyStorage,
        sizeof(ShaderInterop::WGPUPixelDataHeader) + sizeof(std::uint32_t),
        &GPUContext::BufferHandles::pixelData},
    BindingDescriptor{GroupZeroBindings::PerlinNoiseValues,
                      WGPUBufferBindingType_ReadOnlyStorage,
                      sizeof(ShaderInterop::PerlinNoiseValuesHeader) +
                          sizeof(ShaderInterop::Vec4f),
                      &GPUContext::BufferHandles::perlinNoiseValues},
    BindingDescriptor{GroupZeroBindings::PerlinPermutations,
                      WGPUBufferBindingType_ReadOnlyStorage,
                      sizeof(ShaderInterop::PerlinPermutationsHeader) +
                          sizeof(std::uint32_t),
                      &GPUContext::BufferHandles::perlinPermutations},
    BindingDescriptor{GroupZeroBindings::MaterialList,
                      WGPUBufferBindingType_ReadOnlyStorage,
                      sizeof(ShaderInterop::MaterialListHeader) +
                          sizeof(ShaderInterop::Material),
                      &GPUContext::BufferHandles::materialList},
    BindingDescriptor{
        GroupZeroBindings::VolumeList, WGPUBufferBindingType_ReadOnlyStorage,
        sizeof(ShaderInterop::VolumeListHeader) + sizeof(ShaderInterop::Volume),
        &GPUContext::BufferHandles::volumeList},
    BindingDescriptor{
        GroupZeroBindings::ObjectList, WGPUBufferBindingType_ReadOnlyStorage,
        sizeof(ShaderInterop::ObjectListHeader) + sizeof(ShaderInterop::Object),
        &GPUContext::BufferHandles::objectList},
    BindingDescriptor{GroupZeroBindings::NodeList,
                      WGPUBufferBindingType_ReadOnlyStorage,
                      sizeof(ShaderInterop::BVHNodeListHeader) +
                          sizeof(ShaderInterop::BVHNode),
                      &GPUContext::BufferHandles::nodeList},
    BindingDescriptor{GroupZeroBindings::OutputBuffer,
                      WGPUBufferBindingType_Storage, 0,
                      &GPUContext::BufferHandles::outputBuffer},
};

std::expected<
    std::unique_ptr<WGPUShaderModuleImpl, decltype(&wgpuShaderModuleRelease)>,
    std::string>
CreateWGPUShaderModule(const WGPUDevice device,
                       const std::string &filePath) noexcept {
  std::ifstream file{filePath};
  if (!file)
    return std::unexpected(std::string{"Unavailable to read file at: "} +
                           filePath);

  const auto sourceStr = std::string{std::istreambuf_iterator<char>{file},
                                     std::istreambuf_iterator<char>{}};

  auto source = WGPUShaderSourceWGSL{
      .chain = {.sType = WGPUSType_ShaderSourceWGSL},
      .code = {.data = sourceStr.c_str(), .length = sourceStr.length()}};

  const auto descriptor = WGPUShaderModuleDescriptor{
      .nextInChain = &source.chain,
      .label = GPUContext::ToWGPUStringView("ComputeShader")};

  auto module =
      std::unique_ptr<WGPUShaderModuleImpl, decltype(&wgpuShaderModuleRelease)>{
          wgpuDeviceCreateShaderModule(device, &descriptor),
          &wgpuShaderModuleRelease};

  if (!module)
    return std::unexpected("Failed to create WGPUShaderModule");

  return module;
}

std::expected<std::unique_ptr<WGPUComputePipelineImpl,
                              decltype(&wgpuComputePipelineRelease)>,
              std::string>
CreateWGPUComputePipeline(const WGPUDevice device,
                          const WGPUBindGroupLayout bindGroupLayout,
                          const std::string &shaderFilePath) noexcept {
  const auto pipelineLayoutDescriptor = WGPUPipelineLayoutDescriptor{
      .label = GPUContext::ToWGPUStringView("PipelineLayout"),
      .bindGroupLayoutCount = 1,
      .bindGroupLayouts = &bindGroupLayout,
  };
  const auto pipelineLayout =
      std::unique_ptr<WGPUPipelineLayoutImpl,
                      decltype(&wgpuPipelineLayoutRelease)>{
          wgpuDeviceCreatePipelineLayout(device, &pipelineLayoutDescriptor),
          &wgpuPipelineLayoutRelease};

  if (!pipelineLayout)
    return std::unexpected("Failed to create WGPUPipelineLayout");

  const std::array constants = {WGPUConstantEntry{
      .key = GPUContext::ToWGPUStringView("kGroupSize"),
      .value = static_cast<double>(ShaderInterop::kComputePassWorkGroupSize)}};

  const auto module = CreateWGPUShaderModule(device, shaderFilePath);

  if (!module)
    return std::unexpected(module.error());

  const auto descriptor = WGPUComputePipelineDescriptor{
      .label = GPUContext::ToWGPUStringView("ComputePipeline"),
      .layout = pipelineLayout.get(),
      .compute = {.module = module->get(),
                  .entryPoint = GPUContext::ToWGPUStringView("CSMain"),
                  .constantCount = constants.size(),
                  .constants = constants.data()}};

  const auto pipeline = wgpuDeviceCreateComputePipeline(device, &descriptor);

  if (!pipeline)
    return std::unexpected("Failed to create WGPUComputePipeline");

  return std::unique_ptr<WGPUComputePipelineImpl,
                         decltype(&wgpuComputePipelineRelease)>{
      pipeline, &wgpuComputePipelineRelease};
}

template <typename CPUData>
std::expected<GPUContext::BufferHandles::Handle, std::string>
CreateWGPUUniformBuffer(const WGPUDevice device, const WGPUStringView label,
                        const CPUData &data) noexcept {
  const auto bufferDescriptor =
      WGPUBufferDescriptor{.label = label,
                           .usage = WGPUBufferUsage_Uniform,
                           .size = static_cast<std::uint64_t>(sizeof(CPUData)),
                           .mappedAtCreation = true};
  const auto buffer = wgpuDeviceCreateBuffer(device, &bufferDescriptor);

  if (!buffer)
    return std::unexpected(
        std::format("Failed to create WGPUBuffer: {}",
                    std::string_view{label.data, label.length}));

  const auto range = static_cast<std::byte *>(
      wgpuBufferGetMappedRange(buffer, 0, sizeof(CPUData)));

  std::memcpy(range, &data, sizeof(CPUData));

  wgpuBufferUnmap(buffer);
  return GPUContext::BufferHandles::Handle{buffer, &wgpuBufferRelease};
}

template <typename Header, typename Element>
std::expected<GPUContext::BufferHandles::Handle, std::string>
CreateWGPUStorageBuffer(const WGPUDevice device, const WGPUStringView label,
                        const Header &header,
                        const std::vector<Element> &elements) noexcept {
  const auto descriptor = WGPUBufferDescriptor{
      .label = label,
      .usage = WGPUBufferUsage_Storage,
      .size = static_cast<std::uint64_t>(sizeof(Header) +
                                         elements.size() * sizeof(Element)),
      .mappedAtCreation = true};

  const auto buffer = wgpuDeviceCreateBuffer(device, &descriptor);

  if (!buffer)
    return std::unexpected(
        std::format("Failed to create WGPUBuffer: {}",
                    std::string_view{label.data, label.length}));

  const auto range = static_cast<std::byte *>(
      wgpuBufferGetMappedRange(buffer, 0, descriptor.size));

  std::memcpy(range, &header, sizeof(Header));
  std::memcpy(range + sizeof(Header), elements.data(),
              elements.size() * sizeof(Element));

  wgpuBufferUnmap(buffer);
  return GPUContext::BufferHandles::Handle{buffer, &wgpuBufferRelease};
}

std::expected<GPUContext::BufferHandles::Handle, std::string>
CreateWGPUStorageBuffer(const WGPUDevice device, const WGPUStringView label,
                        const std::size_t size,
                        const WGPUBufferUsage usage) noexcept {
  const auto descriptor =
      WGPUBufferDescriptor{.label = label,
                           .usage = WGPUBufferUsage_Storage | usage,
                           .size = size,
                           .mappedAtCreation = false};

  const auto buffer = wgpuDeviceCreateBuffer(device, &descriptor);

  if (!buffer)
    return std::unexpected(
        std::format("Failed to create WGPUBuffer: {}",
                    std::string_view{label.data, label.length}));

  return std::unique_ptr<WGPUBufferImpl, decltype(&wgpuBufferRelease)>{
      buffer, &wgpuBufferRelease};
}
} // namespace

std::expected<std::unique_ptr<WGPUInstanceImpl, decltype(&wgpuInstanceRelease)>,
              std::string>
GPUContext::CreateWGPUInstance(
    const WGPUInstanceDescriptor *const descriptor) noexcept {
  const auto instance = wgpuCreateInstance(descriptor);

  if (!instance)
    return std::unexpected("Failed to create WGPUInstance");

  return std::unique_ptr<WGPUInstanceImpl, decltype(&wgpuInstanceRelease)>{
      instance, &wgpuInstanceRelease};
}

std::expected<std::unique_ptr<WGPUAdapterImpl, decltype(&wgpuAdapterRelease)>,
              std::string>
GPUContext::RequestWGPUAdapterSync(
    const WGPUInstance instance,
    const WGPURequestAdapterOptions *const options) noexcept {

  struct UserData {
    WGPUAdapter adapter = nullptr;
    std::atomic_bool completed = false;
  };

  const auto onAdapterRequestEnded =
      [](const WGPURequestAdapterStatus status, const WGPUAdapter adapter,
         const WGPUStringView message, void *const userData,
         void *const // userData2 unused
         ) noexcept {
        auto &data = *static_cast<UserData *>(userData);

        if (status == WGPURequestAdapterStatus_Success) {
          data.adapter = adapter;
          if (message.data && message.length > 0)
            std::println(stdout, "Adapter report: {}\n",
                         std::string_view{message.data, message.length});
        } else {
          std::println(stderr, "Failed to acquire adapter");
          if (message.data && message.length > 0)
            std::println(stderr, "Reason: {}",
                         std::string_view{message.data, message.length});

          std::println(stderr);
        }

        data.completed = true;
      };

  auto data = UserData{};
  const auto adapterCallBackInfo =
      WGPURequestAdapterCallbackInfo{.mode = WGPUCallbackMode_AllowSpontaneous,
                                     .callback = onAdapterRequestEnded,
                                     .userdata1 = &data};

  wgpuInstanceRequestAdapter(instance, options, adapterCallBackInfo);
  while (!data.completed) {
    wgpuInstanceProcessEvents(instance);
    std::this_thread::yield();
  }

  if (!data.adapter)
    return std::unexpected("Failed to acquire WGPUAdapter");

  return std::unique_ptr<WGPUAdapterImpl, decltype(&wgpuAdapterRelease)>{
      data.adapter, &wgpuAdapterRelease};
}

std::expected<WGPULimits, std::string>
GPUContext::RequestWGPUDeviceRequiredLimitsSync(
    const WGPUAdapter adapter) noexcept {
  auto nativeLimits = WGPULimits{};
  const bool success = wgpuAdapterGetLimits(adapter, &nativeLimits);

  if (success) {
#ifndef NDEBUG
    std::println(stdout, "Adapter Limits");
    std::println(stdout, "  maxTextureDimension1D: {}",
                 nativeLimits.maxTextureDimension1D);
    std::println(stdout, "  maxTextureDimension2D: {}",
                 nativeLimits.maxTextureDimension2D);
    std::println(stdout, "  maxTextureDimension3D: {}",
                 nativeLimits.maxTextureDimension3D);
    std::println(stdout, "  maxTextureArrayLayers: {}\n",
                 nativeLimits.maxTextureArrayLayers);
#endif
  } else
    return std::unexpected("Failed to acquire adapter limits");

#ifndef NDEBUG
  auto supportedFeatures = WGPUSupportedFeatures{};
  wgpuAdapterGetFeatures(adapter, &supportedFeatures);

  if (supportedFeatures.features && supportedFeatures.featureCount > 0) {
    std::println(stdout, "Adapter Features");

    for (std::size_t i = 0; i < supportedFeatures.featureCount; ++i)
      std::println(stdout, "  {}",
                   std::to_underlying(supportedFeatures.features[i]));

    std::println(stdout);
  }

  wgpuSupportedFeaturesFreeMembers(supportedFeatures);

  auto adapterInfo = WGPUAdapterInfo{};
  const auto status = wgpuAdapterGetInfo(adapter, &adapterInfo);

  if (status == WGPUStatus_Success) {
    std::println(stdout, "Adapter Properties");
    std::println(stdout, "  vendorID: {}", adapterInfo.vendorID);

    if (adapterInfo.vendor.data && adapterInfo.vendor.length > 0)
      std::println(
          stdout, "  vendorName: {}",
          std::string_view{adapterInfo.vendor.data, adapterInfo.vendor.length});

    if (adapterInfo.architecture.data && adapterInfo.architecture.length > 0)
      std::println(stdout, "  architecture: {}",
                   std::string_view{adapterInfo.architecture.data,
                                    adapterInfo.architecture.length});

    std::println(stdout, "  deviceID: {}", adapterInfo.deviceID);

    if (adapterInfo.device.data && adapterInfo.device.length > 0)
      std::println(
          stdout, "  deviceName: {}",
          std::string_view{adapterInfo.device.data, adapterInfo.device.length});

    if (adapterInfo.description.data && adapterInfo.description.length > 0)
      std::println(stdout, "  driverDescription: {}",
                   std::string_view{adapterInfo.description.data,
                                    adapterInfo.description.length});

    std::println(stdout, "  adapterType: {}",
                 std::to_underlying(adapterInfo.adapterType));
    std::println(stdout, "  backendType: {}\n",
                 std::to_underlying(adapterInfo.backendType));

    wgpuAdapterInfoFreeMembers(adapterInfo);
  }
#endif

  const auto &requiredLimits = nativeLimits;
  return requiredLimits;
}

std::expected<std::unique_ptr<WGPUDeviceImpl, decltype(&wgpuDeviceRelease)>,
              std::string>
GPUContext::CreateWGPUDevice(
    const WGPUInstance instance, const WGPUAdapter adapter,
    const WGPUDeviceDescriptor *const descriptor) noexcept {

  struct UserData {
    WGPUDevice device = nullptr;
    std::atomic_bool completed = false;
  };

  const auto onDeviceRequestEnded =
      [](const WGPURequestDeviceStatus status, const WGPUDevice device,
         const WGPUStringView message, void *const userData1,
         void *const // userData2 unused
         ) noexcept {
        auto &data = *static_cast<UserData *>(userData1);

        if (status == WGPURequestDeviceStatus_Success) {
          data.device = device;
          if (message.data && message.length > 0)
            std::println(stdout, "Device info: {}\n",
                         std::string_view(message.data, message.length));
        } else {
          std::println(stderr, "Could not get WGPUDevice (Status: {})",
                       std::to_underlying(status));
          if (message.data && message.length > 0)
            std::println(stderr, "Reason: {}",
                         std::string_view(message.data, message.length));

          std::println(stderr);
        }

        data.completed = true;
      };

  auto data = UserData{};
  const auto deviceCallBackInfo =
      WGPURequestDeviceCallbackInfo{.mode = WGPUCallbackMode_AllowSpontaneous,
                                    .callback = onDeviceRequestEnded,
                                    .userdata1 = &data};

  wgpuAdapterRequestDevice(adapter, descriptor, deviceCallBackInfo);
  while (!data.completed) {
    wgpuInstanceProcessEvents(instance);
    std::this_thread::yield();
  }

  if (!data.device)
    return std::unexpected("Failed to create WGPUDevice");

  return std::unique_ptr<WGPUDeviceImpl, decltype(&wgpuDeviceRelease)>{
      data.device, &wgpuDeviceRelease};
}

std::expected<std::unique_ptr<WGPUQueueImpl, decltype(&wgpuQueueRelease)>,
              std::string>
GPUContext::RequestWGPUQueueSync(const WGPUDevice device) noexcept {
  const auto queue = wgpuDeviceGetQueue(device);
  if (!queue)
    return std::unexpected("Failed to create WGPUQueue");

  return std::unique_ptr<WGPUQueueImpl, decltype(&wgpuQueueRelease)>{
      queue, &wgpuQueueRelease};
}

std::expected<GPUContext::ComputePipelineAndBindGroup, std::string>
GPUContext::CreateComputePipelineAndBindGroup(
    const WGPUDevice device,
    const GPUContext::BufferHandles &bufferSet) noexcept {
  auto layoutEntries = std::vector<WGPUBindGroupLayoutEntry>{};

  for (const auto &descriptor : kBindingDescriptors)
    layoutEntries.push_back({.binding = std::to_underlying(descriptor.binding),
                             .visibility = WGPUShaderStage_Compute,
                             .buffer = {
                                 .type = descriptor.type,
                                 .minBindingSize = descriptor.minBindingSize,
                             }});

  const auto layoutDescriptor = WGPUBindGroupLayoutDescriptor{
      .label = ToWGPUStringView("BindGroupLayout"),
      .entryCount = layoutEntries.size(),
      .entries = layoutEntries.data()};

  const auto bindGroupLayout =
      std::unique_ptr<WGPUBindGroupLayoutImpl,
                      decltype(&wgpuBindGroupLayoutRelease)>{
          wgpuDeviceCreateBindGroupLayout(device, &layoutDescriptor),
          &wgpuBindGroupLayoutRelease};

  if (!bindGroupLayout)
    return std::unexpected("Failed to create WGPUBindGroupLayout");

  auto pipeline =
      CreateWGPUComputePipeline(device, bindGroupLayout.get(), SHADER_PATH);

  if (!pipeline)
    return std::unexpected(pipeline.error());

  auto bindGroupEntries = std::vector<WGPUBindGroupEntry>{};

  for (const auto &descriptor : kBindingDescriptors)
    bindGroupEntries.push_back(
        {.binding = std::to_underlying(descriptor.binding),
         .buffer = (bufferSet.*descriptor.member).get(),
         .size = wgpuBufferGetSize((bufferSet.*descriptor.member).get())});

  const auto bindGroupDescriptor = WGPUBindGroupDescriptor{
      .label = GPUContext::ToWGPUStringView("BindGroup"),
      .layout = bindGroupLayout.get(),
      .entryCount = bindGroupEntries.size(),
      .entries = bindGroupEntries.data(),
  };

  auto bindGroup =
      std::unique_ptr<WGPUBindGroupImpl, decltype(&wgpuBindGroupRelease)>{
          wgpuDeviceCreateBindGroup(device, &bindGroupDescriptor),
          &wgpuBindGroupRelease};

  if (!bindGroup)
    return std::unexpected("Failed to create WGPUBindGroup");

  return ComputePipelineAndBindGroup{.pipeline = std::move(pipeline.value()),
                                     .bindGroup = std::move(bindGroup)};
}

std::expected<GPUContext::BufferHandles, std::string>
GPUContext::CreateBufferHandles(
    const WGPUDevice device, const ShaderInterop::Camera &camera,
    const ShaderInterop::Environment &environment,
    const ShaderInterop::RenderSettings &renderSettings,
    const ShaderInterop::ImageDimensions &imageDimensions,
    const GPUContext::ShaderPayload &payload,
    const std::size_t outputBufferSize) noexcept {
  auto cameraBuffer = CreateWGPUUniformBuffer(
      device, GPUContext::ToWGPUStringView("Camera"), camera);

  if (!cameraBuffer)
    return std::unexpected(cameraBuffer.error());

  auto environmentBuffer = CreateWGPUUniformBuffer(
      device, GPUContext::ToWGPUStringView("Environment"), environment);

  if (!environmentBuffer)
    return std::unexpected(environmentBuffer.error());

  auto renderSettingsBuffer = CreateWGPUUniformBuffer(
      device, GPUContext::ToWGPUStringView("RenderSettings"), renderSettings);

  if (!renderSettingsBuffer)
    return std::unexpected(renderSettingsBuffer.error());

  auto imageDimensionsBuffer = CreateWGPUUniformBuffer(
      device, GPUContext::ToWGPUStringView("ImageDimensions"), imageDimensions);

  if (!imageDimensionsBuffer)
    return std::unexpected(imageDimensionsBuffer.error());

  auto textureListHeader = ShaderInterop::TextureListHeader{
      .count = static_cast<std::uint32_t>(payload.textures.size())};

  // emplace_back() if empty to ensure at least 1 element
  // otherwise the buffer won't meet the minBindingSize that was defined in the
  // bind group layout
  auto textureListBuffer = CreateWGPUStorageBuffer(
      device, GPUContext::ToWGPUStringView("TextureList"), textureListHeader,
      payload.textures.empty() ? std::vector<ShaderInterop::Texture>{{}}
                               : payload.textures);

  if (!textureListBuffer)
    return std::unexpected(textureListBuffer.error());

  const auto pixelDataHeader = ShaderInterop::WGPUPixelDataHeader{
      .maxOffset = static_cast<std::uint32_t>(
          std::max(0, static_cast<int>(payload.pixelData.size()) - 1))};

  auto pixelDataBuffer = CreateWGPUStorageBuffer(
      device, GPUContext::ToWGPUStringView("PixelData"), pixelDataHeader,
      payload.pixelData.empty() ? std::vector<std::uint32_t>{0}
                                : payload.pixelData);

  if (!pixelDataBuffer)
    return std::unexpected(pixelDataBuffer.error());

  const auto perlinNoiseValuesHeader = ShaderInterop::PerlinNoiseValuesHeader{
      .maxOffset = static_cast<std::uint32_t>(
          std::max(0, static_cast<int>(payload.perlinNoiseValues.size()) - 1)),
      .pointCount = static_cast<std::uint32_t>(Perlin::kPointCount)};

  auto perlinNoiseValuesBuffer = CreateWGPUStorageBuffer(
      device, GPUContext::ToWGPUStringView("PerlinNoiseValues"),
      perlinNoiseValuesHeader,
      payload.perlinNoiseValues.empty() ? std::vector<ShaderInterop::Vec4f>{{}}
                                        : payload.perlinNoiseValues);

  if (!perlinNoiseValuesBuffer)
    return std::unexpected(perlinNoiseValuesBuffer.error());

  const auto perlinPermutationsHeader = ShaderInterop::PerlinPermutationsHeader{
      .maxOffset = static_cast<std::uint32_t>(std::max(
          0, static_cast<int>(payload.perlinPermutations.size()) - 1))};

  auto perlinPermutationsBuffer = CreateWGPUStorageBuffer(
      device, GPUContext::ToWGPUStringView("PerlinPermutations"),
      perlinPermutationsHeader,
      payload.perlinPermutations.empty() ? std::vector<std::uint32_t>{0}
                                         : payload.perlinPermutations);

  if (!perlinPermutationsBuffer)
    return std::unexpected(perlinPermutationsBuffer.error());

  const auto materialListHeader = ShaderInterop::MaterialListHeader{
      .count = static_cast<std::uint32_t>(payload.materials.size())};

  auto materialListBuffer = CreateWGPUStorageBuffer(
      device, GPUContext::ToWGPUStringView("MaterialList"), materialListHeader,
      payload.materials.empty() ? std::vector<ShaderInterop::Material>{{}}
                                : payload.materials);

  if (!materialListBuffer)
    return std::unexpected(materialListBuffer.error());

  const auto volumeListHeader = ShaderInterop::VolumeListHeader{
      .count = static_cast<std::uint32_t>(payload.volumes.size())};

  auto volumeListBuffer = CreateWGPUStorageBuffer(
      device, GPUContext::ToWGPUStringView("VolumeList"), volumeListHeader,
      payload.volumes.empty() ? std::vector<ShaderInterop::Volume>{{}}
                              : payload.volumes);

  if (!volumeListBuffer)
    return std::unexpected(volumeListBuffer.error());

  const auto objectListHeader = ShaderInterop::ObjectListHeader{
      .count = static_cast<std::uint32_t>(payload.objects.size())};

  auto objectListBuffer = CreateWGPUStorageBuffer(
      device, GPUContext::ToWGPUStringView("ObjectList"), objectListHeader,
      payload.objects.empty() ? std::vector<ShaderInterop::Object>{{}}
                              : payload.objects);

  if (!objectListBuffer)
    return std::unexpected(objectListBuffer.error());

  const auto nodeListHeader = ShaderInterop::BVHNodeListHeader{
      .count = static_cast<std::uint32_t>(payload.nodes.size()),
      .rootIndex = payload.rootNodeIndex};

  auto nodeListBuffer = CreateWGPUStorageBuffer(
      device, GPUContext::ToWGPUStringView("BVHNodeList"), nodeListHeader,
      payload.nodes.empty() ? std::vector<ShaderInterop::BVHNode>{{}}
                            : payload.nodes);

  if (!nodeListBuffer)
    return std::unexpected(nodeListBuffer.error());

  auto outputBuffer = CreateWGPUStorageBuffer(
      device, GPUContext::ToWGPUStringView("OutputBuffer"), outputBufferSize,
      WGPUBufferUsage_CopySrc);

  if (!outputBuffer)
    return std::unexpected(outputBuffer.error());

  return GPUContext::BufferHandles{
      .camera = std::move(cameraBuffer.value()),
      .environment = std::move(environmentBuffer.value()),
      .renderSettings = std::move(renderSettingsBuffer.value()),
      .imageDimensions = std::move(imageDimensionsBuffer.value()),
      .textureList = std::move(textureListBuffer.value()),
      .pixelData = std::move(pixelDataBuffer.value()),
      .perlinNoiseValues = std::move(perlinNoiseValuesBuffer.value()),
      .perlinPermutations = std::move(perlinPermutationsBuffer.value()),
      .materialList = std::move(materialListBuffer.value()),
      .objectList = std::move(objectListBuffer.value()),
      .volumeList = std::move(volumeListBuffer.value()),
      .nodeList = std::move(nodeListBuffer.value()),
      .outputBuffer = std::move(outputBuffer.value()),
  };
}
