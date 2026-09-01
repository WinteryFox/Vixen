#include "RenderPass.h"

namespace Vixen {
    RenderPass::RenderPass(
        std::string name,
        const RenderPassType type,
        std::vector<ResourceUsage> resourceUsages,
        std::vector<RenderAttachment> colorAttachments,
        std::optional<RenderAttachment> depthStencilAttachment,
        ExecuteCallback executeCallback,
        glm::vec4 debugLabelColor,
        const bool sideEffecting,
        const bool usesExternallySynchronizedResources
    ) : name(std::move(name)),
        type(type),
        resourceUsages(std::move(resourceUsages)),
        colorAttachments(std::move(colorAttachments)),
        depthStencilAttachment(depthStencilAttachment),
        executeCallback(std::move(executeCallback)),
        debugLabelColor(debugLabelColor),
        sideEffecting(sideEffecting),
        usesExternallySynchronizedResources(usesExternallySynchronizedResources) {}

    void RenderPass::execute(RenderPassContext& context) {
        executeCallback(context);
    }

    const std::string& RenderPass::getName() const noexcept {
        return name;
    }

    RenderPassType RenderPass::getType() const noexcept {
        return type;
    }

    const std::vector<ResourceUsage>& RenderPass::getResourceUsages() const noexcept {
        return resourceUsages;
    }

    const std::vector<RenderAttachment>& RenderPass::getColorAttachments() const noexcept {
        return colorAttachments;
    }

    const std::optional<RenderAttachment>& RenderPass::getDepthStencilAttachment() const noexcept {
        return depthStencilAttachment;
    }

    glm::vec4 RenderPass::getDebugLabelColor() const noexcept {
        return debugLabelColor;
    }

    bool RenderPass::isSideEffecting() const noexcept {
        return sideEffecting;
    }

    bool RenderPass::usesExternalResources() const noexcept {
        return usesExternallySynchronizedResources;
    }
}
