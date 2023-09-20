#include <iostream>
#include <vector>

#define WEBGPU_CPP_IMPLEMENTATION
#include "webgpu/webgpu.hpp"

#include <glfw3webgpu.h>
#include <GLFW/glfw3.h>

using namespace wgpu;

const int SCREEN_WIDTH = 640;
const int SCREEN_HEIGHT = 480;
const char* SCREEN_TITLE = "Learn WebGPU";

int main (int, char**) {
    std::cout << "Starting application... 🚀" << std::endl;

	InstanceDescriptor instanceDesc{};
	Instance instance = createInstance(instanceDesc);

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
    Surface surface = glfwGetWGPUSurface(instance, window);
	RequestAdapterOptions adapterOpts{};
    adapterOpts.compatibleSurface = surface;
    Adapter adapter = instance.requestAdapter(adapterOpts);

    // Setup device
    DeviceDescriptor deviceDesc{};
    deviceDesc.label = "My device";
    deviceDesc.requiredLimits = nullptr;
    deviceDesc.defaultQueue.label = "My default queue";
    Device device = adapter.requestDevice(deviceDesc);

    Queue queue = device.getQueue();

    // Setup device error callback
    auto onDeviceError = [](ErrorType type, char const * message) {
        std::cout << "Uncaptured device error: type " << type;
        if (message) std::cout << " (" << message << ")";
        std::cout << std::endl;
    };
    device.setUncapturedErrorCallback(onDeviceError);

    // Setup swap chain
    SwapChainDescriptor swapChainDesc = {};
    swapChainDesc.width = SCREEN_WIDTH;
    swapChainDesc.height = SCREEN_HEIGHT;
#if WGPU_DAWN
    TextureFormat swapChainFormat = WGPUTextureFormat_BGRA8Unorm; // getPreferredFormat is not implemented in Dawn yet
#else
    TextureFormat swapChainFormat = surface.getPreferredFormat(adapter);
#endif
    swapChainDesc.format = swapChainFormat;
    // Like buffers, textures are allocated for a specific usage. In our case,
	// we will use them as the target of a Render Pass so it needs to be created
	// with the `RenderAttachment` usage flag.
    swapChainDesc.usage = TextureUsage::RenderAttachment;
    // FIFO stands for "first in, first out", meaning that the presented
	// texture is always the oldest one, like a regular queue.
    swapChainDesc.presentMode = PresentMode::Fifo;
    SwapChain swapChain = device.createSwapChain(surface, swapChainDesc);

    while (!glfwWindowShouldClose(window)) {
        // Check whether the user clicked on the close button (and any other
        // mouse/key event, which we don't use so far)
        glfwPollEvents();

        // Get the next available swap chain texture
        TextureView nextTexture = swapChain.getCurrentTextureView();

        if (!nextTexture) {
            // Texture might be null, if for example the window has been resized
            break;
        }

		CommandEncoderDescriptor commandEncoderDesc{};
		commandEncoderDesc.label = "Command Encoder";
		CommandEncoder encoder = device.createCommandEncoder(commandEncoderDesc);

        // Describe a render pass, which targets the texture view
        RenderPassDescriptor renderPassDesc{};

        RenderPassColorAttachment renderPassColorAttachment = {};
        // The attachment is tighed to the view returned by the swap chain, so that
		// the render pass draws directly on screen.
        renderPassColorAttachment.view = nextTexture;
        // Not relevant here because we do not use multi-sampling
        renderPassColorAttachment.resolveTarget = nullptr;
        renderPassColorAttachment.loadOp = LoadOp::Clear;
        renderPassColorAttachment.storeOp = StoreOp::Store;
        renderPassColorAttachment.clearValue = Color{ 0.9, 0.1, 0.2, 1.0 };
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
        RenderPassEncoder renderPass = encoder.beginRenderPass(renderPassDesc);
        renderPass.end();
        nextTexture.release();

        CommandBufferDescriptor cmdBufferDescriptor = {};
        cmdBufferDescriptor.nextInChain = nullptr;
        cmdBufferDescriptor.label = "Command buffer";
        CommandBuffer command = encoder.finish(cmdBufferDescriptor);
        queue.submit(1, &command);

        // We can tell the swap chain to present the next texture.
        swapChain.present();
    }

    swapChain.release();
    surface.release();
    adapter.release();
    device.release();
    instance.release();

    glfwDestroyWindow(window);
    glfwTerminate();

    return 0;
}