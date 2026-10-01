#include "native_gpu.hpp"

#ifdef WARLOCK_HAS_NATIVE_WEBGPU
#include <webgpu/webgpu.h>
#include <algorithm>
#include <chrono>
#include <cstring>
#include <fstream>
#include <iterator>
#include <limits>
#include <sstream>
#include <thread>

#ifndef WARLOCK_WEBGPU_SHADER_PATH
#define WARLOCK_WEBGPU_SHADER_PATH "src/sim/webgpu/combat.wgsl"
#endif

namespace warlock {
namespace {
struct AdapterResult { WGPUAdapter value = nullptr; bool done = false; WGPURequestAdapterStatus status{}; };
struct DeviceResult { WGPUDevice value = nullptr; bool done = false; WGPURequestDeviceStatus status{}; };
struct MapResult { bool done = false; WGPUMapAsyncStatus status{}; };

void adapter_callback(WGPURequestAdapterStatus status, WGPUAdapter adapter, WGPUStringView,
                      void* user, void*) {
    auto& r = *static_cast<AdapterResult*>(user); r.value = adapter; r.status = status; r.done = true;
}
void device_callback(WGPURequestDeviceStatus status, WGPUDevice device, WGPUStringView,
                     void* user, void*) {
    auto& r = *static_cast<DeviceResult*>(user); r.value = device; r.status = status; r.done = true;
}
void map_callback(WGPUMapAsyncStatus status, WGPUStringView, void* user, void*) {
    auto& r = *static_cast<MapResult*>(user); r.status = status; r.done = true;
}
WGPUStringView strview(const char* text) { return {text, WGPU_STRLEN}; }

template <typename Result>
void wait_future(WGPUInstance instance, WGPUFuture, Result& result) {
    while (!result.done) {
        wgpuInstanceProcessEvents(instance);
        if (!result.done) std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }
}

WGPUBuffer create_buffer(WGPUDevice device, uint64_t size, WGPUBufferUsage usage, const char* label) {
    WGPUBufferDescriptor desc = WGPU_BUFFER_DESCRIPTOR_INIT;
    desc.label = strview(label); desc.size = size; desc.usage = usage;
    return wgpuDeviceCreateBuffer(device, &desc);
}
}

bool run_native_webgpu(const std::vector<Config>& configs, uint32_t replicas, uint32_t seed, uint32_t step_us,
                       std::vector<State>& states, double& elapsed_seconds,
                       std::string& error) {
    using Clock = std::chrono::steady_clock;
    const auto start = Clock::now();
    if (configs.empty() || replicas == 0 || uint64_t(configs.size()) * replicas * sizeof(State) > 128ull * 1024ull * 1024ull ||
        uint64_t(configs.size()) * sizeof(Config) > 128ull * 1024ull * 1024ull) {
        error = "GPU batch exceeds the 128 MiB per-buffer limit"; return false;
    }
    std::ifstream shader_file(WARLOCK_WEBGPU_SHADER_PATH, std::ios::binary);
    std::string shader;
    if (shader_file) {
        shader = {std::istreambuf_iterator<char>(shader_file), {}};
    } else {
        std::ifstream fallback_file("/assets/combat.wgsl", std::ios::binary);
        if (fallback_file) {
            shader = {std::istreambuf_iterator<char>(fallback_file), {}};
        } else {
            std::ifstream fallback2("assets/combat.wgsl", std::ios::binary);
            if (fallback2) {
                shader = {std::istreambuf_iterator<char>(fallback2), {}};
            } else {
                error = "Cannot open WebGPU shader: " WARLOCK_WEBGPU_SHADER_PATH;
                return false;
            }
        }
    }

    WGPUInstanceDescriptor instance_desc = WGPU_INSTANCE_DESCRIPTOR_INIT;
    WGPUInstance instance = wgpuCreateInstance(&instance_desc);
    if (!instance) { error = "Cannot create WebGPU instance"; return false; }
    WGPUAdapter adapter = nullptr; WGPUDevice device = nullptr;
    WGPUQueue queue = nullptr; WGPUShaderModule module = nullptr;
    WGPUComputePipeline pipeline = nullptr; WGPUBindGroupLayout layout = nullptr;
    WGPUBindGroup bind_group = nullptr; WGPUBuffer config_buffer = nullptr;
    WGPUBuffer params_buffer = nullptr; WGPUBuffer state_buffer = nullptr;
    WGPUBuffer read_buffer = nullptr;
    auto cleanup = [&] {
        if (read_buffer) wgpuBufferRelease(read_buffer);
        if (state_buffer) wgpuBufferRelease(state_buffer);
        if (params_buffer) wgpuBufferRelease(params_buffer);
        if (config_buffer) wgpuBufferRelease(config_buffer);
        if (bind_group) wgpuBindGroupRelease(bind_group);
        if (layout) wgpuBindGroupLayoutRelease(layout);
        if (pipeline) wgpuComputePipelineRelease(pipeline);
        if (module) wgpuShaderModuleRelease(module);
        if (queue) wgpuQueueRelease(queue);
        if (device) { wgpuDeviceDestroy(device); wgpuDeviceRelease(device); }
        if (adapter) wgpuAdapterRelease(adapter);
        wgpuInstanceRelease(instance);
    };

    AdapterResult ar;
    WGPURequestAdapterOptions adapter_options = WGPU_REQUEST_ADAPTER_OPTIONS_INIT;
    adapter_options.powerPreference = WGPUPowerPreference_HighPerformance;
#ifndef __EMSCRIPTEN__
    // The desktop renderer owns a GLFW/OpenGL context. Letting wgpu-native
    // probe its default backend list can enter EGL on that same display and
    // abort the process with BadAccess. The native runtime is bundled for
    // Linux with Vulkan support, so request Vulkan directly.
    adapter_options.backendType = WGPUBackendType_Vulkan;
#endif
    WGPURequestAdapterCallbackInfo acb = WGPU_REQUEST_ADAPTER_CALLBACK_INFO_INIT;
    acb.mode = WGPUCallbackMode_AllowProcessEvents; acb.callback = adapter_callback; acb.userdata1 = &ar;
    wait_future(instance, wgpuInstanceRequestAdapter(instance, &adapter_options, acb), ar);
    if (ar.status != WGPURequestAdapterStatus_Success || !ar.value) {
        error = "No usable WebGPU adapter"; cleanup(); return false;
    }
    adapter = ar.value;
    DeviceResult dr;
    WGPUDeviceDescriptor device_desc = WGPU_DEVICE_DESCRIPTOR_INIT;
    WGPURequestDeviceCallbackInfo dcb = WGPU_REQUEST_DEVICE_CALLBACK_INFO_INIT;
    dcb.mode = WGPUCallbackMode_AllowProcessEvents; dcb.callback = device_callback; dcb.userdata1 = &dr;
    wait_future(instance, wgpuAdapterRequestDevice(adapter, &device_desc, dcb), dr);
    if (dr.status != WGPURequestDeviceStatus_Success || !dr.value) {
        error = "Cannot create WebGPU device"; cleanup(); return false;
    }
    device = dr.value; queue = wgpuDeviceGetQueue(device);

    WGPUShaderSourceWGSL wgsl = WGPU_SHADER_SOURCE_WGSL_INIT;
    wgsl.code = {shader.data(), shader.size()};
    WGPUShaderModuleDescriptor module_desc = WGPU_SHADER_MODULE_DESCRIPTOR_INIT;
    module_desc.nextInChain = &wgsl.chain;
    module = wgpuDeviceCreateShaderModule(device, &module_desc);
    WGPUConstantEntry constants[4]{};
    constants[0].key = strview("FIXED"); constants[0].value = step_us != 0 ? 1.0 : 0.0;
    constants[1].key = strview("STEP_US"); constants[1].value = step_us;
    constants[2].key = strview("CHUNK"); constants[2].value = step_us != 0 ? 4096.0 : 256.0;
    constants[3].key = strview("GROUP_SIZE"); constants[3].value = 64.0;
    WGPUComputePipelineDescriptor pipeline_desc = WGPU_COMPUTE_PIPELINE_DESCRIPTOR_INIT;
    pipeline_desc.compute.module = module; pipeline_desc.compute.entryPoint = strview("simulate");
    pipeline_desc.compute.constants = constants; pipeline_desc.compute.constantCount = 4;
    pipeline = wgpuDeviceCreateComputePipeline(device, &pipeline_desc);
    if (!module || !pipeline) { error = "Cannot compile WebGPU compute shader"; cleanup(); return false; }
    layout = wgpuComputePipelineGetBindGroupLayout(pipeline, 0);

    const uint32_t candidate_count = static_cast<uint32_t>(configs.size());
    const uint32_t total_states = candidate_count * replicas;
    const uint64_t config_bytes = uint64_t(candidate_count) * sizeof(Config);
    const uint64_t state_bytes = uint64_t(total_states) * sizeof(State);
    config_buffer = create_buffer(device, config_bytes, WGPUBufferUsage_Storage | WGPUBufferUsage_CopyDst, "warlock-configs");
    params_buffer = create_buffer(device, 16, WGPUBufferUsage_Uniform | WGPUBufferUsage_CopyDst, "warlock-params");
    state_buffer = create_buffer(device, state_bytes, WGPUBufferUsage_Storage | WGPUBufferUsage_CopySrc | WGPUBufferUsage_CopyDst, "warlock-states");
    read_buffer = create_buffer(device, state_bytes, WGPUBufferUsage_CopyDst | WGPUBufferUsage_MapRead, "warlock-readback");
    std::vector<State> initial(total_states); std::memset(initial.data(), 0, initial.size() * sizeof(State));
    const uint32_t params[4] = {total_states, replicas, seed, 0};
    wgpuQueueWriteBuffer(queue, config_buffer, 0, configs.data(), config_bytes);
    wgpuQueueWriteBuffer(queue, params_buffer, 0, params, sizeof(params));
    wgpuQueueWriteBuffer(queue, state_buffer, 0, initial.data(), state_bytes);

    WGPUBindGroupEntry entries[3]{};
    entries[0].binding = 0; entries[0].buffer = config_buffer; entries[0].size = config_bytes;
    entries[1].binding = 1; entries[1].buffer = params_buffer; entries[1].size = sizeof(params);
    entries[2].binding = 2; entries[2].buffer = state_buffer; entries[2].size = state_bytes;
    WGPUBindGroupDescriptor bg_desc = WGPU_BIND_GROUP_DESCRIPTOR_INIT;
    bg_desc.layout = layout; bg_desc.entryCount = 3; bg_desc.entries = entries;
    bind_group = wgpuDeviceCreateBindGroup(device, &bg_desc);

    float max_duration = 0.0f;
    for (const Config& config : configs) max_duration = std::max(max_duration, config.duration);
    const uint32_t rounds = step_us != 0
        ? std::max(1u, static_cast<uint32_t>(std::ceil((std::ceil(max_duration * 1000000.0 / step_us) + 1.0) / 4096.0)))
        : std::max(1u, static_cast<uint32_t>((std::ceil(max_duration) * 4.0 + 8.0 + 255.0) / 256.0));
    WGPUCommandEncoderDescriptor encoder_desc = WGPU_COMMAND_ENCODER_DESCRIPTOR_INIT;
    WGPUCommandEncoder encoder = wgpuDeviceCreateCommandEncoder(device, &encoder_desc);
    for (uint32_t i = 0; i < rounds; ++i) {
        WGPUComputePassDescriptor pass_desc = WGPU_COMPUTE_PASS_DESCRIPTOR_INIT;
        WGPUComputePassEncoder pass = wgpuCommandEncoderBeginComputePass(encoder, &pass_desc);
        wgpuComputePassEncoderSetPipeline(pass, pipeline);
        wgpuComputePassEncoderSetBindGroup(pass, 0, bind_group, 0, nullptr);
        wgpuComputePassEncoderDispatchWorkgroups(pass, (total_states + 63u) / 64u, 1, 1);
        wgpuComputePassEncoderEnd(pass); wgpuComputePassEncoderRelease(pass);
    }
    wgpuCommandEncoderCopyBufferToBuffer(encoder, state_buffer, 0, read_buffer, 0, state_bytes);
    WGPUCommandBuffer command = wgpuCommandEncoderFinish(encoder, nullptr);
    wgpuCommandEncoderRelease(encoder);
    wgpuQueueSubmit(queue, 1, &command); wgpuCommandBufferRelease(command);
    MapResult mr;
    WGPUBufferMapCallbackInfo mcb = WGPU_BUFFER_MAP_CALLBACK_INFO_INIT;
    mcb.mode = WGPUCallbackMode_AllowProcessEvents; mcb.callback = map_callback; mcb.userdata1 = &mr;
    wait_future(instance, wgpuBufferMapAsync(read_buffer, WGPUMapMode_Read, 0, state_bytes, mcb), mr);
    if (mr.status != WGPUMapAsyncStatus_Success) {
        error = "WebGPU result readback failed"; cleanup(); return false;
    }
    const void* mapped = wgpuBufferGetConstMappedRange(read_buffer, 0, state_bytes);
    if (!mapped) { error = "WebGPU result buffer could not be mapped"; cleanup(); return false; }
    states.resize(total_states); std::memcpy(states.data(), mapped, state_bytes);
    wgpuBufferUnmap(read_buffer);
    cleanup();
    elapsed_seconds = std::chrono::duration<double>(Clock::now() - start).count();
    return true;
}
}
#else
namespace warlock {
bool run_native_webgpu(const std::vector<Config>&, uint32_t, uint32_t, uint32_t, std::vector<State>&,
                       double&, std::string& error) {
    error = "Native WebGPU runtime is not available in this build";
    return false;
}
}
#endif
