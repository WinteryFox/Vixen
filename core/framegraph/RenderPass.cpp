#include "RenderPass.h"

#include <utility>

namespace Vixen {
    RenderPass::RenderPass(std::string&& name, const RenderPassType type, std::vector<ResourceNode>&& resources,
                           std::unique_ptr<ExecutorBase>&& executor)
        : name(std::move(name)),
          type(type),
          resources(std::move(resources)),
          executor(std::move(executor)) {}

    void RenderPass::execute() const {
        executor->execute();
    }

    const std::string& RenderPass::getName() const noexcept { return name; }

    RenderPassType RenderPass::getType() const noexcept { return type; }

    const std::vector<ResourceNode>& RenderPass::getResources() const noexcept { return resources; }
}
