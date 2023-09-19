#include <iostream>
#include <vector>
#include <webgpu/webgpu.h>
#include <glfw3webgpu.h>
#include <GLFW/glfw3.h>
#include "webgpu-utils.h"

const int SCREEN_WIDTH = 640;
const int SCREEN_HEIGHT = 480;
const char* SCREEN_TITLE = "Learn WebGPU";

int main (int, char**) {
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

    while (!glfwWindowShouldClose(window)) {
        // Check whether the user clicked on the close button (and any other
        // mouse/key event, which we don't use so far)
        glfwPollEvents();
    }

    wgpuSurfaceRelease(surface);
    wgpuAdapterRelease(adapter);
    wgpuDeviceRelease(device);
	wgpuInstanceRelease(instance);

    glfwDestroyWindow(window);
    glfwTerminate();

    return 0;
}