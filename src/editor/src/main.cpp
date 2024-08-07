#ifdef _WIN32

#include <Windows.h>

#endif

#include <VulkanApplication.h>
#include <imgui/imgui.h>
#include <imgui/backends/imgui_impl_vulkan.h>

#include "device/VulkanDevice.h"

int main() {
#ifdef _WIN32
    system(std::format("chcp {}", CP_UTF8).c_str());
#endif

#ifdef DEBUG
    spdlog::set_level(spdlog::level::info);
    spdlog::set_pattern("%Y-%m-%d %T.%e %^%7l%$ %P --- [%t] %1v");
#endif

    try {
        auto vixen = Vixen::VulkanApplication("Vixen Vulkan Test", {1, 0, 0});

        ImGui_ImplVulkan_InitInfo info{
            .Instance = vixen.getInstance()->getInstance(),
            .PhysicalDevice = vixen.getDevice()->getGpu().device,
            .Device = vixen.getDevice()->getDevice(),
            .QueueFamily = vixen.getDevice()->getGraphicsQueueFamily().index,
            .Queue = vixen.getDevice()->getGraphicsQueue(),
            .DescriptorPool = imGuiDescriptorPool,
            .RenderPass = nullptr,
            .MinImageCount = 3,
            .ImageCount = 3,
            .MSAASamples = 8,
            .PipelineCache = nullptr,
            .Subpass = 0,
            .UseDynamicRendering = true,
            .PipelineRenderingCreateInfo = renderingInfo,
            .Allocator = nullptr,
            .CheckVkResultFn = &Vixen::checkVulkanResult,
            .MinAllocationSize = 1024 * 1024
        };

        ImGui_ImplVulkan_Init(&info);

        while (vixen.isRunning()) {
            vixen.update();


            ImGui_ImplVulkan_NewFrame();
            ImGui::NewFrame();
            ImGui::ShowDemoWindow();
            ImGui::Render();
            const auto &drawData = ImGui::GetDrawData();
            ImGui_ImplVulkan_RenderDrawData(drawData, frame.commandBuffer());

            vixen.render();
        }

        ImGui_ImplVulkan_Shutdown();
        ImGui::DestroyContext();
    } catch (const std::runtime_error &e) {
        spdlog::error(e.what());
        throw;
    }

    return EXIT_SUCCESS;
}
