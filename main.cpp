#include <iostream>
#include <vector>
#include <webgpu/webgpu.hpp>
#include <glfw3webgpu.h>
#include <GLFW/glfw3.h>
#include "webgpu-utils.h"

const int SCREEN_WIDTH = 640;
const int SCREEN_HEIGHT = 480;
const char* SCREEN_TITLE = "Learn WebGPU";

int main (int, char**) {
    std::cout << "Starting application... 🚀" << std::endl;

	WGPUInstanceDescriptor desc = {};
	desc.nextInChain = nullptr;
	WGPUInstance instance = wgpuCreateInstance(&desc);

	// Check if the WebGPU instance was created successfully
	if (!instance) {
		std::cerr << "Could not initialize WebGPU!" << std::endl;
		return 1;
	}

    // Initialize GLFW
    if (!glfwInit()) {
		std::cerr << "Could not initialize GLFW!" << std::endl;
		return 1;
	}
    
    // Don't initialize any particular graphics API by default. We do that manually.
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API); 
    // Don't allow the window to be resized
    glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);
    // Create window
    GLFWwindow* window = glfwCreateWindow(SCREEN_WIDTH, SCREEN_HEIGHT, SCREEN_TITLE, NULL, NULL);
    if (!window) {
        std::cerr << "Could not open window!" << std::endl;
        glfwTerminate();
        return 1;
    }

    // Setup adapter
    WGPUSurface surface = glfwGetWGPUSurface(instance, window);
    WGPURequestAdapterOptions adapterOpts = {};
    adapterOpts.nextInChain = nullptr;
    adapterOpts.compatibleSurface = surface;
    WGPUAdapter adapter = requestAdapter(instance, &adapterOpts);

    // Setup device
    WGPUDeviceDescriptor deviceDesc = {};
    deviceDesc.nextInChain = nullptr;
    deviceDesc.label = "My device";
    deviceDesc.requiredFeaturesCount = 0;
    deviceDesc.requiredLimits = nullptr;
    deviceDesc.defaultQueue.nextInChain = nullptr;
    deviceDesc.defaultQueue.label = "My default queue";
    WGPUDevice device = requestDevice(adapter, &deviceDesc);

    WGPUQueue queue = wgpuDeviceGetQueue(device);

    // Setup device error callback
    auto onDeviceError = [](WGPUErrorType type, char const * message, void * /* pUserData */) {
        std::cout << "Uncaptured device error: type " << type;
        if (message) std::cout << " (" << message << ")";
        std::cout << std::endl;
    };
    wgpuDeviceSetUncapturedErrorCallback(device, onDeviceError, nullptr);

    // Setup swap chain
    WGPUSwapChainDescriptor swapChainDesc = {};
    swapChainDesc.nextInChain = nullptr;
    swapChainDesc.width = SCREEN_WIDTH;
    swapChainDesc.height = SCREEN_HEIGHT;
    WGPUTextureFormat swapChainFormat = WGPUTextureFormat_BGRA8Unorm; // wgpuSurfaceGetPreferredFormat is not implemented in Dawn yet
    swapChainDesc.format = swapChainFormat;
    // Like buffers, textures are allocated for a specific usage. In our case,
	// we will use them as the target of a Render Pass so it needs to be created
	// with the `RenderAttachment` usage flag.
    swapChainDesc.usage = WGPUTextureUsage_RenderAttachment;
    // FIFO stands for "first in, first out", meaning that the presented
	// texture is always the oldest one, like a regular queue.
    swapChainDesc.presentMode = WGPUPresentMode_Fifo;
    WGPUSwapChain swapChain = wgpuDeviceCreateSwapChain(device, surface, &swapChainDesc);

    while (!glfwWindowShouldClose(window)) {
        // Check whether the user clicked on the close button (and any other
        // mouse/key event, which we don't use so far)
        glfwPollEvents();

        // Get the next available swap chain texture
        WGPUTextureView nextTexture = wgpuSwapChainGetCurrentTextureView(swapChain);

        if (!nextTexture) {
            // Texture might be null, if for example the window has been resized
            break;
        }

		WGPUCommandEncoderDescriptor commandEncoderDesc;
		commandEncoderDesc.label = "Command Encoder";
		WGPUCommandEncoder encoder = wgpuDeviceCreateCommandEncoder(device, &commandEncoderDesc);

        // Describe a render pass, which targets the texture view
        WGPURenderPassDescriptor renderPassDesc = {};

        WGPURenderPassColorAttachment renderPassColorAttachment = {};
        // The attachment is tighed to the view returned by the swap chain, so that
		// the render pass draws directly on screen.
        renderPassColorAttachment.view = nextTexture;
        // Not relevant here because we do not use multi-sampling
        renderPassColorAttachment.resolveTarget = nullptr;
        renderPassColorAttachment.loadOp = WGPULoadOp_Clear;
        renderPassColorAttachment.storeOp = WGPUStoreOp_Store;
        renderPassColorAttachment.clearValue = WGPUColor{ 0.9, 0.1, 0.2, 1.0 };
        renderPassDesc.colorAttachmentCount = 1;
        renderPassDesc.colorAttachments = &renderPassColorAttachment;
        // No depth buffer for now
        renderPassDesc.depthStencilAttachment = nullptr;
        // We do not use timers for now neither
        renderPassDesc.timestampWriteCount = 0;
        renderPassDesc.timestampWrites = nullptr;
        renderPassDesc.nextInChain = nullptr;

		// Create a render pass. We end it immediately because we use its built-in
		// mechanism for clearing the screen when it begins (see descriptor).
        WGPURenderPassEncoder renderPass = wgpuCommandEncoderBeginRenderPass(encoder, &renderPassDesc);
        wgpuRenderPassEncoderEnd(renderPass);

        wgpuTextureViewRelease(nextTexture);

        WGPUCommandBufferDescriptor cmdBufferDescriptor = {};
        cmdBufferDescriptor.nextInChain = nullptr;
        cmdBufferDescriptor.label = "Command buffer";
        WGPUCommandBuffer command = wgpuCommandEncoderFinish(encoder, &cmdBufferDescriptor);
        wgpuQueueSubmit(queue, 1, &command);

        // We can tell the swap chain to present the next texture.
        wgpuSwapChainPresent(swapChain);
    }

    wgpuSwapChainRelease(swapChain);
    wgpuSurfaceRelease(surface);
    wgpuAdapterRelease(adapter);
    wgpuDeviceRelease(device);
	wgpuInstanceRelease(instance);

    glfwDestroyWindow(window);
    glfwTerminate();

    return 0;
}