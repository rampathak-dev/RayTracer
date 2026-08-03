#include "headers/Colour.hpp"
#include "headers/GPUContext.hpp"
#include "headers/RayTracer.hpp"
#include "headers/RenderJob.hpp"
#include "headers/ShaderInterop.hpp"
#include "headers/ShaderPayloadBuilder.hpp"
#include "webgpu/webgpu.h"
#include "webgpu/wgpu.h"
#include <atomic>
#include <cstddef>
#include <cstdint>
#include <expected>
#include <format>
#include <memory>
#include <print>
#include <string>
#include <string_view>
#include <thread>
#include <utility>

namespace {
class GPURayTracer : public RayTracer {
private:
  std::unique_ptr<WGPUDeviceImpl, decltype(&wgpuDeviceRelease)> mDevice = {
      nullptr, &wgpuDeviceRelease};
  std::unique_ptr<WGPUQueueImpl, decltype(&wgpuQueueRelease)> mQueue = {
      nullptr, &wgpuQueueRelease};
  GPUContext::ComputePipelineAndBindGroup mPipelineBindGroup = {};
  RenderData::ImageDimensions mImageDimensions = {};
  GPUContext::BufferHandles mBufferHandles = {};

  GPURayTracer(const RenderData::ImageDimensions &,
               std::unique_ptr<WGPUDeviceImpl, decltype(&wgpuDeviceRelease)>,
               std::unique_ptr<WGPUQueueImpl, decltype(&wgpuQueueRelease)>,
               GPUContext::BufferHandles,
               GPUContext::ComputePipelineAndBindGroup) noexcept;

  std::expected<void, std::string> DispatchCompute() const noexcept;
  std::expected<Image, std::string> ReadBackImage() const noexcept;

public:
  GPURayTracer() = delete;
  ~GPURayTracer() noexcept override = default;
  GPURayTracer(const GPURayTracer &) = delete;
  GPURayTracer &operator=(const GPURayTracer &) = delete;
  GPURayTracer(GPURayTracer &&) = default;
  GPURayTracer &operator=(GPURayTracer &&) = default;

  std::expected<Image, std::string> Render() const noexcept override;

  static std::expected<std::unique_ptr<RayTracer>, std::string>
      Create(RenderJob) noexcept;
};

GPURayTracer::GPURayTracer(
    const RenderData::ImageDimensions &dimensions,
    std::unique_ptr<WGPUDeviceImpl, decltype(&wgpuDeviceRelease)> device,
    std::unique_ptr<WGPUQueueImpl, decltype(&wgpuQueueRelease)> queue,
    GPUContext::BufferHandles handles,
    GPUContext::ComputePipelineAndBindGroup pipelineBindGroup) noexcept
    : mDevice{std::move(device)}, mQueue{std::move(queue)},
      mPipelineBindGroup{std::move(pipelineBindGroup)},
      mImageDimensions{dimensions}, mBufferHandles{std::move(handles)} {}

std::expected<void, std::string>
GPURayTracer::DispatchCompute() const noexcept {
  const auto commandEncoderDescriptor = WGPUCommandEncoderDescriptor{
      .label = GPUContext::ToWGPUStringView("CommandEncoder")};

  const auto commandEncoder = std::unique_ptr<
      WGPUCommandEncoderImpl, decltype(&wgpuCommandEncoderRelease)>{
      wgpuDeviceCreateCommandEncoder(mDevice.get(), &commandEncoderDescriptor),
      &wgpuCommandEncoderRelease};

  if (!commandEncoder)
    return std::unexpected("Failed to create WGPUCommandEncoder");

  const auto computePassDescriptor = WGPUComputePassDescriptor{
      .label = GPUContext::ToWGPUStringView("ComputePass")};

  const auto computePass =
      std::unique_ptr<WGPUComputePassEncoderImpl,
                      decltype(&wgpuComputePassEncoderRelease)>{
          wgpuCommandEncoderBeginComputePass(commandEncoder.get(),
                                             &computePassDescriptor),
          &wgpuComputePassEncoderRelease};

  if (!computePass)
    return std::unexpected("Failed to create WGPUComputePassEncoder");

  wgpuComputePassEncoderSetPipeline(computePass.get(),
                                    mPipelineBindGroup.pipeline.get());
  wgpuComputePassEncoderSetBindGroup(
      computePass.get(), 0, mPipelineBindGroup.bindGroup.get(), 0, nullptr);

  const auto workGroupSizeX = static_cast<std::uint32_t>(
      (mImageDimensions.width + ShaderInterop::kComputePassWorkGroupSize - 1) /
      ShaderInterop::kComputePassWorkGroupSize);

  const auto workGroupSizeY = static_cast<std::uint32_t>(
      (mImageDimensions.height + ShaderInterop::kComputePassWorkGroupSize - 1) /
      ShaderInterop::kComputePassWorkGroupSize);

  wgpuComputePassEncoderDispatchWorkgroups(computePass.get(), workGroupSizeX,
                                           workGroupSizeY, 1);

  wgpuComputePassEncoderEnd(computePass.get());

  const auto commandBufferDescriptor = WGPUCommandBufferDescriptor{
      .label = GPUContext::ToWGPUStringView("CommandBuffer")};

  const auto commandBuffer = std::unique_ptr<
      WGPUCommandBufferImpl, decltype(&wgpuCommandBufferRelease)>{
      wgpuCommandEncoderFinish(commandEncoder.get(), &commandBufferDescriptor),
      &wgpuCommandBufferRelease};

  if (!commandBuffer)
    return std::unexpected("Failed to create WGPUCommandBuffer");

  const auto commandBufferPtr = commandBuffer.get();

  wgpuQueueSubmit(mQueue.get(), 1, &commandBufferPtr);
  wgpuDevicePoll(mDevice.get(), false, nullptr);

  return std::expected<void, std::string>{};
}

std::expected<Image, std::string> GPURayTracer::ReadBackImage() const noexcept {
  const auto copyEncoderDescriptor = WGPUCommandEncoderDescriptor{
      .label = GPUContext::ToWGPUStringView("CopyEncoder")};

  const auto copyEncoder =
      std::unique_ptr<WGPUCommandEncoderImpl,
                      decltype(&wgpuCommandEncoderRelease)>{
          wgpuDeviceCreateCommandEncoder(mDevice.get(), &copyEncoderDescriptor),
          &wgpuCommandEncoderRelease};

  const auto stagingBufferDescriptor = WGPUBufferDescriptor{
      .label = GPUContext::ToWGPUStringView("StagingBuffer"),
      .usage = WGPUBufferUsage_CopyDst | WGPUBufferUsage_MapRead,
      .size = wgpuBufferGetSize(mBufferHandles.outputBuffer.get()),
  };

  const auto stagingBuffer =
      std::unique_ptr<WGPUBufferImpl, decltype(&wgpuBufferRelease)>{
          wgpuDeviceCreateBuffer(mDevice.get(), &stagingBufferDescriptor),
          &wgpuBufferRelease};

  if (!stagingBuffer)
    return std::unexpected("Failed to create WGPUBuffer: stagingBuffer");

  wgpuCommandEncoderCopyBufferToBuffer(
      copyEncoder.get(), mBufferHandles.outputBuffer.get(), 0,
      stagingBuffer.get(), 0, stagingBufferDescriptor.size);

  const auto copyCommandBuffer =
      std::unique_ptr<WGPUCommandBufferImpl,
                      decltype(&wgpuCommandBufferRelease)>{
          wgpuCommandEncoderFinish(copyEncoder.get(), nullptr),
          &wgpuCommandBufferRelease};

  const auto copyCommandBufferPtr = copyCommandBuffer.get();
  wgpuQueueSubmit(mQueue.get(), 1, &copyCommandBufferPtr);
  wgpuDevicePoll(mDevice.get(), true, nullptr);

  struct UserData {
    std::atomic_bool completed = false;
    WGPUMapAsyncStatus status = WGPUMapAsyncStatus_Success;
  };

  const auto onBufferMapped = [](const WGPUMapAsyncStatus status,
                                 const WGPUStringView, void *const userData1,
                                 void *const // userData2 unused
                                 ) noexcept -> void {
    auto &data = *static_cast<UserData *>(userData1);
    data.status = status;
    data.completed = true;
  };

  auto data = UserData{};

  const auto callbackInfo =
      WGPUBufferMapCallbackInfo{.mode = WGPUCallbackMode_AllowSpontaneous,
                                .callback = onBufferMapped,
                                .userdata1 = &data};

  wgpuBufferMapAsync(stagingBuffer.get(), WGPUMapMode_Read, 0,
                     stagingBufferDescriptor.size, callbackInfo);

  while (!data.completed) {
    wgpuDevicePoll(mDevice.get(), false, nullptr);
    std::this_thread::yield();
  }

  if (data.status != WGPUMapAsyncStatus_Success)
    return std::unexpected(
        std::format("Failed to map WGPUBuffer: stagingBuffer (Status: {})",
                    std::to_underlying(data.status)));

  const auto range =
      static_cast<const ShaderInterop::Vec4f *>(wgpuBufferGetConstMappedRange(
          stagingBuffer.get(), 0, stagingBufferDescriptor.size));

  auto pixels = std::vector<SRGBColour>{};

  const auto imageResolution = mImageDimensions.width * mImageDimensions.height;
  pixels.reserve(imageResolution);

  for (std::size_t i = 0; i < imageResolution; ++i)
    pixels.push_back(ShaderInterop::ToSRGBColour(range[i]));

  wgpuBufferUnmap(stagingBuffer.get());

  return Image{.dimensions = mImageDimensions, .pixels = pixels};
}

std::expected<Image, std::string> GPURayTracer::Render() const noexcept {
  if (const auto result = this->DispatchCompute(); !result)
    return std::unexpected(result.error());

  return this->ReadBackImage();
}

std::expected<std::unique_ptr<RayTracer>, std::string>
GPURayTracer::Create(RenderJob job) noexcept {
  const WGPUInstanceDescriptor instanceDescriptor = {};
  const auto instance = GPUContext::CreateWGPUInstance(&instanceDescriptor);
  if (!instance)
    return std::unexpected(instance.error());

  const auto adapterOptions = WGPURequestAdapterOptions{};
  const auto adapter =
      GPUContext::RequestWGPUAdapterSync(instance->get(), &adapterOptions);

  if (!adapter)
    return std::unexpected(adapter.error());

  const auto requiredLimits =
      GPUContext::RequestWGPUDeviceRequiredLimitsSync(adapter->get());

  if (!requiredLimits)
    return std::unexpected(requiredLimits.error());

  const auto onDeviceError = [](const WGPUDevice *const, // device unused
                                const WGPUErrorType type,
                                const WGPUStringView message, void *const,
                                void *const // userData unused
                                ) noexcept {
    std::println(stderr, "Uncaptured Device error, type: {}",
                 std::to_underlying(type));
    if (message.data)
      std::println(stderr, "Message: {}",
                   std::string_view{message.data, message.length});
  };

  const auto onDeviceLost = [](const WGPUDevice *const, // device unused
                               const WGPUDeviceLostReason reason,
                               const WGPUStringView message, void *const,
                               void *const // userData unused
                               ) noexcept {
    std::print(stderr, "Reason: {}, ", std::to_underlying(reason));
    if (message.data)
      std::println(stderr, "({})",
                   std::string_view{message.data, message.length});
  };

  const auto deviceDescriptor = WGPUDeviceDescriptor{
      .label = {GPUContext::ToWGPUStringView("Device")},
      .requiredLimits = &requiredLimits.value(),
      .defaultQueue = {.label = GPUContext::ToWGPUStringView("Queue")},
      .deviceLostCallbackInfo = {.callback = onDeviceLost},
      .uncapturedErrorCallbackInfo = {.callback = onDeviceError},
  };

  auto device = GPUContext::CreateWGPUDevice(instance->get(), adapter->get(),
                                             &deviceDescriptor);

  if (!device)
    return std::unexpected(device.error());

  auto queue = GPUContext::RequestWGPUQueueSync(device->get());
  if (!queue)
    return std::unexpected(queue.error());

  auto builder =
      ShaderPayloadBuilder{*job.world, job.renderSettings.shutterOpenTime,
                           job.renderSettings.shutterCloseTime};

  const auto payload = builder.TakePayload();

  auto handles = GPUContext::CreateBufferHandles(
      device->get(), job.camera, job.environment, job.renderSettings,
      job.imageDimensions, payload,
      job.imageDimensions.width * job.imageDimensions.height *
          sizeof(ShaderInterop::Vec4f));

  auto pipelineBindGroup = GPUContext::CreateComputePipelineAndBindGroup(
      device->get(), handles.value());

  if (!pipelineBindGroup)
    return std::unexpected(std::move(pipelineBindGroup.error()));

  return std::unique_ptr<RayTracer>{new GPURayTracer(
      job.imageDimensions, std::move(device.value()), std::move(queue.value()),
      std::move(handles.value()), std::move(pipelineBindGroup.value()))};
}

struct Registrar {
  Registrar() noexcept {
    RayTracer::Register(RenderMode::GPU, &GPURayTracer::Create);
  }
} registrar;
} // namespace
