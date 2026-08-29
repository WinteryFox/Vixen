add_library(
        Vixen
        STATIC
        ${IMGUI_SOURCES}
        DisplayServer.cpp
        IndexFormat.h
        PrimitiveTopology.h
        VertexAttribute.h
        AttachmentInfo.h
        LoadAction.h
        StoreAction.h
        buffer/BufferUsage.h
        Application.cpp
        Application.h
        RenderingContextDriver.h
        RenderingDeviceDriver.cpp
        RenderingDeviceDriver.h
        error/Macros.h
        error/CantCreateError.h
        buffer/Buffer.h
        MemoryAllocationType.h
        ImageDataFormat.h
        image/ImageSwizzle.h
        image/ImageLayout.h
        image/ImageAspectFlags.h
        image/ImageFormat.h
        image/ImageSamples.h
        image/ImageType.h
        image/Image.h
        image/ImageUsage.h
        image/Sampler.h
        image/SamplerState.h
        image/SamplerFilter.h
        image/SamplerRepeatMode.h
        image/CompareOperator.h
        image/SamplerBorderColor.h
        Bitmask.h
        buffer/Buffer.cpp
        command/CommandPool.h
        command/CommandBuffer.h
        command/Semaphore.h
        command/Fence.h
        shader/Shader.h
        shader/ShaderUniform.h
        shader/ShaderUniformType.h
        shader/ShaderStage.h
        WindowFlags.h
        WindowMode.h
        VSyncMode.h
        RenderingDriver.h
        WindowMouseMode.h
        WindowCursor.h
        shader/ShaderSource.h
        shader/ShaderLanguage.h
        command/CommandQueue.h
        shader/ShaderStageData.h
        Swapchain.h
        Surface.h
        QueueFamilyFlags.h
        Window.h
        command/CommandBufferType.h
        buffer/BufferCopyRegion.h
        image/ImageCopyRegion.h
        image/ImageSubresourceLayers.h
        buffer/BufferImageCopyRegion.h
        image/ImageSubresourceRange.h
        PipelineStageFlags.h
        MemoryBarrier.h
        BufferBarrier.h
        ImageBarrier.h
        BarrierAccessFlags.h
        Framebuffer.h
        error/Error.h
        Frame.h
        DeferredRelease.h
        RenderingDevice.cpp
        RenderingDevice.h
        DriverDevice.h
        framegraph/RenderPass.h
        ClearValue.h
        error/SwapchainError.h
        framegraph/FrameGraph.cpp
        framegraph/FrameGraph.h
        RenderingContextDriver.cpp
        framegraph/RenderPass.cpp
        framegraph/RenderPassType.h
        framegraph/Resource.h
        framegraph/FrameGraphError.h
        framegraph/ResourceStateMapping.h
        framegraph/ResourceStateMapping.cpp
        framegraph/BarrierBatch.h
        framegraph/BarrierBatch.cpp
        framegraph/Node.h
        buffer/BufferFormat.h
        framegraph/FrameGraphResourceView.h
        framegraph/FrameGraphResourceStorage.h
        framegraph/FrameGraphResourceStorage.cpp
        framegraph/RenderPassContext.h
        error/Shader.h
        framegraph/ResourceSlot.h
        error/ResourceCreationError.h
)
vixen_configure_target(Vixen)
target_compile_definitions(
        Vixen
        PUBLIC
        ENGINE_NAME="${ENGINE_NAME}"
        ENGINE_VERSION_MAJOR=${ENGINE_VERSION_MAJOR}
        ENGINE_VERSION_MINOR=${ENGINE_VERSION_MINOR}
        ENGINE_VERSION_PATCH=${ENGINE_VERSION_PATCH}
        ENGINE_VERSION_STATUS="$<IF:$<CONFIG:Release>,stable,dev>"
        ENGINE_VERSION="${ENGINE_VERSION_MAJOR}.${ENGINE_VERSION_MINOR}.${ENGINE_VERSION_PATCH}.$<IF:$<CONFIG:Release>,stable,dev>"
)

if (ENABLE_VULKAN)
    target_compile_definitions(Vixen PRIVATE VULKAN_ENABLED)
endif ()
if (ENABLE_D3D12)
    target_compile_definitions(Vixen PRIVATE D3D12_ENABLED)
endif ()
if (ENABLE_OPENGL)
    target_compile_definitions(Vixen PRIVATE OPENGL_ENABLED)
endif ()
if (CMAKE_SYSTEM_NAME STREQUAL "Darwin")
    target_compile_definitions(Vixen PRIVATE MACOS_ENABLED)
elseif (CMAKE_SYSTEM_NAME STREQUAL "iOS")
    target_compile_definitions(Vixen PRIVATE IOS_ENABLED)
endif ()

target_link_libraries(
        Vixen
        PUBLIC
        glfw
        assimp::assimp
        glm::glm
        spdlog
        spirv-cross-core
        spirv-cross-cpp
        glslang::glslang
        glslang::glslang-default-resource-limits
        glslang::SPIRV
        Threads::Threads
)
target_include_directories(
        Vixen
        PUBLIC
        ${CMAKE_SOURCE_DIR}
        ${CMAKE_CURRENT_SOURCE_DIR}
)
target_include_directories(
        Vixen
        SYSTEM PUBLIC
        ${CMAKE_SOURCE_DIR}/thirdparty/imgui
        ${CMAKE_SOURCE_DIR}/thirdparty/glslang/SPIRV
        ${CMAKE_SOURCE_DIR}/thirdparty/glslang/glslang
        ${CMAKE_SOURCE_DIR}/thirdparty/SPIRV-Cross
        ${CMAKE_SOURCE_DIR}/thirdparty/SPIRV-Cross/include
        ${CMAKE_SOURCE_DIR}/thirdparty/stb
)

if (ENABLE_VULKAN)
    target_link_libraries(
            Vixen
            PUBLIC
            volk::volk
    )
endif ()
