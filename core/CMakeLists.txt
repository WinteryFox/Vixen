add_library(
        VixenCore
        STATIC
        ${IMGUI_SOURCES}
        Application.cpp
        buffer/Buffer.cpp
        DisplayServer.cpp
        framegraph/BarrierBatch.cpp
        framegraph/FrameGraph.cpp
        framegraph/FrameGraphResourceStorage.cpp
        framegraph/RenderPass.cpp
        framegraph/ResourceStateMapping.cpp
        RenderingContextDriver.cpp
        RenderingDevice.cpp
        RenderingDeviceDriver.cpp
)
target_sources(
        VixenCore
        PUBLIC
        FILE_SET public_headers
        TYPE HEADERS
        BASE_DIRS ${CMAKE_CURRENT_SOURCE_DIR}
        FILES
        Application.h
        AttachmentInfo.h
        BarrierAccessFlags.h
        Bitmask.h
        Blending.h
        buffer/Buffer.h
        buffer/BufferCopyRegion.h
        buffer/BufferFormat.h
        buffer/BufferImageCopyRegion.h
        buffer/BufferUsage.h
        BufferBarrier.h
        ClearValue.h
        command/CommandBuffer.h
        command/CommandBufferType.h
        command/CommandPool.h
        command/CommandQueue.h
        command/Fence.h
        command/Semaphore.h
        DeferredRelease.h
        DisplayServer.h
        DriverDevice.h
        error/CantCreateError.h
        error/Error.h
        error/Macros.h
        error/ResourceCreationError.h
        error/Shader.h
        error/SwapchainError.h
        Frame.h
        Framebuffer.h
        framegraph/BarrierBatch.h
        framegraph/FrameGraph.h
        framegraph/FrameGraphError.h
        framegraph/FrameGraphExecutionError.h
        framegraph/FrameGraphResourceStorage.h
        framegraph/FrameGraphResourceView.h
        framegraph/Node.h
        framegraph/RenderPass.h
        framegraph/RenderPassContext.h
        framegraph/RenderPassType.h
        framegraph/Resource.h
        framegraph/ResourceSlot.h
        framegraph/ResourceStateMapping.h
        image/CompareOperator.h
        image/Image.h
        image/ImageAspectFlags.h
        image/ImageCopyRegion.h
        image/ImageFormat.h
        image/ImageLayout.h
        image/ImageSamples.h
        image/ImageSubresourceLayers.h
        image/ImageSubresourceRange.h
        image/ImageSwizzle.h
        image/ImageType.h
        image/ImageUsage.h
        image/ImageView.h
        image/Sampler.h
        image/SamplerBorderColor.h
        image/SamplerFilter.h
        image/SamplerRepeatMode.h
        image/SamplerState.h
        ImageBarrier.h
        ImageDataFormat.h
        IndexFormat.h
        LoadAction.h
        MemoryAllocationType.h
        MemoryBarrier.h
        Monitor.h
        PipelineStageFlags.h
        PrimitiveTopology.h
        QueueFamilyFlags.h
        Rectangle.h
        RenderingContextDriver.h
        RenderingDevice.h
        RenderingDeviceDriver.h
        RenderingDriver.h
        shader/Shader.h
        shader/ShaderLanguage.h
        shader/ShaderSource.h
        shader/ShaderStage.h
        shader/ShaderStageData.h
        shader/ShaderUniform.h
        shader/ShaderUniformType.h
        StoreAction.h
        Surface.h
        Swapchain.h
        VertexAttribute.h
        VSyncMode.h
        Window.h
        WindowCursor.h
        WindowFlags.h
        WindowMode.h
        WindowMouseMode.h
)
vixen_configure_target(VixenCore)
target_compile_definitions(
        VixenCore
        PUBLIC
        ENGINE_NAME="${ENGINE_NAME}"
        ENGINE_VERSION_MAJOR=${ENGINE_VERSION_MAJOR}
        ENGINE_VERSION_MINOR=${ENGINE_VERSION_MINOR}
        ENGINE_VERSION_PATCH=${ENGINE_VERSION_PATCH}
        ENGINE_VERSION_STATUS="$<IF:$<CONFIG:Release>,stable,dev>"
        ENGINE_VERSION="${ENGINE_VERSION_MAJOR}.${ENGINE_VERSION_MINOR}.${ENGINE_VERSION_PATCH}.$<IF:$<CONFIG:Release>,stable,dev>"
)

if (ENABLE_VULKAN)
    target_compile_definitions(VixenCore PRIVATE VULKAN_ENABLED)
endif ()
if (ENABLE_D3D12)
    target_compile_definitions(VixenCore PRIVATE D3D12_ENABLED)
endif ()
if (ENABLE_OPENGL)
    target_compile_definitions(VixenCore PRIVATE OPENGL_ENABLED)
endif ()
if (CMAKE_SYSTEM_NAME STREQUAL "Darwin")
    target_compile_definitions(VixenCore PRIVATE MACOS_ENABLED)
elseif (CMAKE_SYSTEM_NAME STREQUAL "iOS")
    target_compile_definitions(VixenCore PRIVATE IOS_ENABLED)
endif ()

target_link_libraries(
        VixenCore
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
        VixenCore
        PUBLIC
        ${CMAKE_SOURCE_DIR}
        ${CMAKE_CURRENT_SOURCE_DIR}
)
target_include_directories(
        VixenCore
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
            VixenCore
            PUBLIC
            volk::volk
    )
endif ()
