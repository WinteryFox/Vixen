#include "RenderPass.h"

namespace Vixen {
    RenderPass::RenderPass(
        std::string name,
        const RenderPassType type,
        std::vector<ResourceUsage> resourceUsages,
        std::vector<RenderAttachment> colorAttachments,
        std::optional<RenderAttachment> depthStencilAttachment,
        ExecuteCallback executeCallback
    ) : name(std::move(name)),
        type(type),
        resourceUsages(std::move(resourceUsages)),
        colorAttachments(std::move(colorAttachments)),
        depthStencilAttachment(std::move(depthStencilAttachment)),
        executeCallback(std::move(executeCallback)) {}

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
}
