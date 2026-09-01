add_library(
        VixenCore
        STATIC
        ${IMGUI_SOURCES}
        application/Application.cpp
        buffer/Buffer.cpp
        display/DisplayServer.cpp
        framegraph/BarrierBatch.cpp
        framegraph/FrameGraph.cpp
        framegraph/FrameGraphPassResources.cpp
        framegraph/FrameGraphResourceAccess.cpp
        framegraph/FrameGraphResourceStorage.cpp
        framegraph/RenderPass.cpp
        framegraph/ResourceStateMapping.cpp
        rendering/RenderingContextDriver.cpp
        rendering/RenderingDevice.cpp
        rendering/RenderingDeviceDriver.cpp
)
target_sources(
        VixenCore
        PUBLIC
        FILE_SET public_headers
        TYPE HEADERS
        BASE_DIRS ${CMAKE_CURRENT_SOURCE_DIR}
        FILES
        application/Application.h
        buffer/Buffer.h
        buffer/BufferCopyRegion.h
        buffer/BufferFormat.h
        buffer/BufferImageCopyRegion.h
        buffer/BufferUsage.h
        command/CommandBuffer.h
        command/CommandBufferType.h
        command/CommandPool.h
        command/CommandQueue.h
        command/Fence.h
        command/QueueFamilyFlags.h
        command/Semaphore.h
        display/DisplayServer.h
        display/Monitor.h
        display/VSyncMode.h
        display/Window.h
        display/WindowCursor.h
        display/WindowFlags.h
        display/WindowMode.h
        display/WindowMouseMode.h
        error/CantCreateError.h
        error/Error.h
        error/Macros.h
        error/ResourceCreationError.h
        error/Shader.h
        error/SwapchainError.h
        framegraph/BarrierBatch.h
        framegraph/FrameGraph.h
        framegraph/FrameGraphError.h
        framegraph/FrameGraphExecutionError.h
        framegraph/FrameGraphPassResources.h
        framegraph/FrameGraphResourceAccess.h
        framegraph/FrameGraphResourceAccessError.h
        framegraph/FrameGraphResourcePermission.h
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
        image/ImageDataFormat.h
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
        memory/DeferredRelease.h
        memory/MemoryAllocationType.h
        rendering/AttachmentInfo.h
        rendering/Blending.h
        rendering/ClearValue.h
        rendering/DriverDevice.h
        rendering/Frame.h
        rendering/Framebuffer.h
        rendering/IndexFormat.h
        rendering/LoadAction.h
        rendering/PrimitiveTopology.h
        rendering/Rectangle.h
        rendering/RenderingContextDriver.h
        rendering/RenderingDevice.h
        rendering/RenderingDeviceDriver.h
        rendering/RenderingDriver.h
        rendering/StoreAction.h
        rendering/Surface.h
        rendering/Swapchain.h
        rendering/VertexAttribute.h
        shader/Shader.h
        shader/ShaderLanguage.h
        shader/ShaderSource.h
        shader/ShaderStage.h
        shader/ShaderStageData.h
        shader/ShaderUniform.h
        shader/ShaderUniformType.h
        synchronization/BarrierAccessFlags.h
        synchronization/BufferBarrier.h
        synchronization/ImageBarrier.h
        synchronization/MemoryBarrier.h
        synchronization/PipelineStageFlags.h
        utility/Bitmask.h
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
