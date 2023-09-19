#include <webgpu/webgpu.h>

WGPUAdapter requestAdapter(WGPUInstance instance, WGPURequestAdapterOptions const * options);
void inspectAdapter(WGPUAdapter adapter);
WGPUDevice requestDevice(WGPUAdapter adapter, WGPUDeviceDescriptor const * descriptor);