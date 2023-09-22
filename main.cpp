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

    std::cout << "🚚 Requesting adapter..." << std::endl;
    Surface surface = glfwGetWGPUSurface(instance, window);
	RequestAdapterOptions adapterOpts{};
    adapterOpts.compatibleSurface = surface;
    Adapter adapter = instance.requestAdapter(adapterOpts);
    std::cout << "✅ Got adapter: " << adapter << std::endl;

    // Get supported limits
	SupportedLimits supportedLimits;
	adapter.getLimits(&supportedLimits);

    std::cout << "🚚 Requesting device..." << std::endl;
    // Create required limits
    RequiredLimits requiredLimits = Default;
	// We use at most 1 vertex attribute for now
	requiredLimits.limits.maxVertexAttributes = 1;
	// We should also tell that we use 1 vertex buffers
	requiredLimits.limits.maxVertexBuffers = 1;
	// Maximum size of a buffer is 6 vertices of 2 float each
	requiredLimits.limits.maxBufferSize = 6 * 2 * sizeof(float);
	// Maximum stride between 2 consecutive vertices in the vertex buffer
	requiredLimits.limits.maxVertexBufferArrayStride = 2 * sizeof(float);
	// This must be set even if we do not use storage buffers for now
	requiredLimits.limits.minStorageBufferOffsetAlignment = supportedLimits.limits.minStorageBufferOffsetAlignment;
	// This must be set even if we do not use uniform buffers for now
	requiredLimits.limits.minUniformBufferOffsetAlignment = supportedLimits.limits.minUniformBufferOffsetAlignment;

    // Setup device
    DeviceDescriptor deviceDesc{};
    deviceDesc.label = "My device";
    deviceDesc.requiredFeaturesCount = 0;
    deviceDesc.requiredLimits = &requiredLimits;
    deviceDesc.defaultQueue.label = "My default queue";

    Device device = adapter.requestDevice(deviceDesc);
    std::cout << "✅ Got device: " << device << std::endl;

	adapter.getLimits(&supportedLimits);
	std::cout << "ℹ️ adapter.maxVertexAttributes: " << supportedLimits.limits.maxVertexAttributes << std::endl;

	device.getLimits(&supportedLimits);
	std::cout << "ℹ️ device.maxVertexAttributes: " << supportedLimits.limits.maxVertexAttributes << std::endl;

    // Setup device error callback
    auto onDeviceError = [](ErrorType type, char const * message) {
        std::cout << "Uncaptured device error: type " << type;
        if (message) std::cout << " (" << message << ")";
        std::cout << std::endl;
    };
    device.setUncapturedErrorCallback(onDeviceError);

    Queue queue = device.getQueue();

    std::cout << "🚚 Creating swapchain..." << std::endl;
    SwapChainDescriptor swapChainDesc = {};
    swapChainDesc.width = SCREEN_WIDTH;
    swapChainDesc.height = SCREEN_HEIGHT;
#if WEBGPU_BACKEND_DAWN
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
    std::cout << "✅ Swapchain: " << swapChain << std::endl;

	std::cout << "🚚 Creating shader module..." << std::endl;
	const char* shaderSource = R"(
@vertex
fn vs_main(@location(0) in_vertex_position: vec2f) -> @builtin(position) vec4f {
	return vec4f(in_vertex_position, 0.0, 1.0);
}


@fragment
fn fs_main() -> @location(0) vec4f {
    return vec4f(0.0, 0.4, 1.0, 1.0);
}
)";
    // Setup shader module
    ShaderModuleDescriptor shaderDesc;
#ifdef WEBGPU_BACKEND_WGPU
    shaderDesc.hintCount = 0;
    shaderDesc.hints = nullptr;
#endif
	// Use the extension mechanism to load a WGSL shader source code
	ShaderModuleWGSLDescriptor shaderCodeDesc;
	// Set the chained struct's header
	shaderCodeDesc.chain.next = nullptr;
	shaderCodeDesc.chain.sType = SType::ShaderModuleWGSLDescriptor;
	// Connect the chain
	shaderDesc.nextInChain = &shaderCodeDesc.chain;

	// Setup the actual payload of the shader code descriptor
	shaderCodeDesc.code = shaderSource;

    ShaderModule shaderModule = device.createShaderModule(shaderDesc);
	std::cout << "✅ Shader module: " << shaderModule << std::endl;

	std::cout << "🚚 Creating render pipeline..." << std::endl;
	// Vertex fetch
	VertexAttribute vertexAttrib;
	// == Per attribute ==
	// Corresponds to @location(...)
	vertexAttrib.shaderLocation = 0;
	// Means vec2<f32> in the shader
	vertexAttrib.format = VertexFormat::Float32x2;
	// Index of the first element
	vertexAttrib.offset = 0;

	VertexBufferLayout vertexBufferLayout;
	// [...] Build vertex buffer layout
	vertexBufferLayout.attributeCount = 1;
	vertexBufferLayout.attributes = &vertexAttrib;
	// == Common to attributes from the same buffer ==
	vertexBufferLayout.arrayStride = 2 * sizeof(float);
	vertexBufferLayout.stepMode = VertexStepMode::Vertex;

    // Setup render pipeline
    RenderPipelineDescriptor pipelineDesc{};
    // Setup vertex shader
    pipelineDesc.vertex.bufferCount = 1;
    pipelineDesc.vertex.buffers = &vertexBufferLayout;
    pipelineDesc.vertex.module = shaderModule;
    pipelineDesc.vertex.entryPoint = "vs_main";
    pipelineDesc.vertex.constantCount = 0;
    pipelineDesc.vertex.constants = nullptr;
    // Each sequence of 3 vertices is considered as a triangle
    pipelineDesc.primitive.topology = PrimitiveTopology::TriangleList;
    // We'll see later how to specify the order in which vertices should be
    // connected. When not specified, vertices are considered sequentially.
    pipelineDesc.primitive.stripIndexFormat = IndexFormat::Undefined;
    // The face orientation is defined by assuming that when looking
    // from the front of the face, its corner vertices are enumerated
    // in the counter-clockwise (CCW) order.
    pipelineDesc.primitive.frontFace = FrontFace::CCW;
    // But the face orientation does not matter much because we do not
    // cull (i.e. "hide") the faces pointing away from us (which is often
    // used for optimization).
    pipelineDesc.primitive.cullMode = CullMode::None; // Should probably be set to front for optimization, but set to none for simplicity / debugging purposes

    // Setup fragment shader
    FragmentState fragmentState;
    fragmentState.module = shaderModule;
    fragmentState.entryPoint = "fs_main";
    fragmentState.constantCount = 0;
    fragmentState.constants = nullptr;
    pipelineDesc.fragment = &fragmentState;

    // Setup blending
    BlendState blendState;
    // Usual alpha blending for the color
	blendState.color.srcFactor = BlendFactor::SrcAlpha;
	blendState.color.dstFactor = BlendFactor::OneMinusSrcAlpha;
	blendState.color.operation = BlendOperation::Add;
    // We leave the target alpha untouched
	blendState.alpha.srcFactor = BlendFactor::Zero;
	blendState.alpha.dstFactor = BlendFactor::One;
	blendState.alpha.operation = BlendOperation::Add;

    ColorTargetState colorTarget;
    colorTarget.format = swapChainFormat;
    colorTarget.blend = &blendState;
    colorTarget.writeMask = ColorWriteMask::All; // We could write to only some of the color channels.
    // We have only one target because our render pass has only one output color attachment.
    fragmentState.targetCount = 1;
    fragmentState.targets = &colorTarget;

    pipelineDesc.depthStencil = nullptr;

    // Multi-sampling
	// Samples per pixel
	pipelineDesc.multisample.count = 1;
	// Default value for the mask, meaning "all bits on"
	pipelineDesc.multisample.mask = ~0u;
	// Default value as well (irrelevant for count = 1 anyways)
	pipelineDesc.multisample.alphaToCoverageEnabled = false;

    RenderPipeline pipeline = device.createRenderPipeline(pipelineDesc);
    std::cout << "✅ Render pipeline: " << pipeline << std::endl;

    // Static vertex buffer
    std::vector<float> vertexData = {
        -0.5, -0.5,
        +0.5, -0.5,
        +0.0, +0.5,

        -0.55f, -0.5,
        -0.05f, +0.5,
        -0.55f, +0.5,
    };
    int vertexCount = static_cast<int>(vertexData.size() / 2);

    // Upload geometry to the GPU via buffer
	BufferDescriptor bufferDesc;
	bufferDesc.size = vertexData.size() * sizeof(float);
	bufferDesc.usage = BufferUsage::CopyDst | BufferUsage::Vertex;
	bufferDesc.mappedAtCreation = false;
	Buffer vertexBuffer = device.createBuffer(bufferDesc);

    queue.writeBuffer(vertexBuffer, 0, vertexData.data(), bufferDesc.size);

    std::cout << "🔄 Starting main loop" << pipeline << std::endl;
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

        // In its overall outline, drawing a triangle is as simple as this:
		// Select which render pipeline to use
		renderPass.setPipeline(pipeline);
		
        // Set vertex buffer while encoding the render pass
		renderPass.setVertexBuffer(0, vertexBuffer, 0, vertexData.size() * sizeof(float));

		// We use the `vertexCount` variable instead of hard-coding the vertex count
		renderPass.draw(vertexCount, 1, 0, 0);

        renderPass.end();
        nextTexture.release();

        CommandBufferDescriptor cmdBufferDescriptor = {};
        cmdBufferDescriptor.nextInChain = nullptr;
        cmdBufferDescriptor.label = "Command buffer";
        CommandBuffer command = encoder.finish(cmdBufferDescriptor);
        
        queue.submit(1, &command);

        // We can tell the swap chain to present the next texture.
        swapChain.present();
        // renderPass.release();
        // encoder.release();

#ifdef WEBGPU_BACKEND_DAWN
		// Check for pending error callbacks
		device.tick();
#endif
    }

    glfwDestroyWindow(window);
    glfwTerminate();

    swapChain.release();
    surface.release();
    adapter.release();
    device.release();
    instance.release();
    queue.release();

    return 0;
}