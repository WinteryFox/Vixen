#pragma once

#include <memory>
#include <string>
#include <glm/glm.hpp>

#include "Node.h"
#include "RenderPassType.h"

namespace Vixen {
    class RenderPass {
        class ExecutorBase {
        public:
            virtual ~ExecutorBase() = default;

            virtual void execute() = 0;
        };

        template <typename Data, typename Execute>
        class Executor : public ExecutorBase {
            Data data;

            Execute callback;

        public:
            Executor(Data&& data, Execute&& callback)
                : data(std::move(data)),
                  callback(std::move(callback)) {}

            void execute() override {
                std::invoke(callback, data);
            }
        };

        std::string name;

        RenderPassType type;

        std::vector<ResourceNode> resources;

        std::unique_ptr<ExecutorBase> executor;

    public:
        RenderPass(std::string&& name, RenderPassType type, std::vector<ResourceNode>&& resources,
                   std::unique_ptr<ExecutorBase>&& executor);

        RenderPass(const RenderPass& other) = delete;

        RenderPass(RenderPass&& other) noexcept = delete;

        RenderPass& operator=(const RenderPass& other) = delete;

        RenderPass& operator=(RenderPass&& other) noexcept = delete;

        ~RenderPass() = default;

        void execute() const;

        [[nodiscard]] const std::string& getName() const noexcept;

        [[nodiscard]] RenderPassType getType() const noexcept;

        [[nodiscard]] const std::vector<ResourceNode>& getResources() const noexcept;

        class Builder {
            std::string name;

            RenderPassType type;

            std::vector<ResourceNode> resources;

        public:
            Builder(std::string&& name, const RenderPassType type)
                : name(std::move(name)),
                  type(type) {}

            Builder(const Builder& other) = delete;

            Builder(Builder&& other) noexcept = default;

            Builder& operator=(const Builder& other) = delete;

            Builder& operator=(Builder&& other) noexcept = default;

            ~Builder() = default;

            Builder& importImage(const std::string& name) {

            }

            template <typename Data, typename Execute>
            RenderPass build(
                Data&& data,
                Execute&& execute
            ) && {
                using StoredData = std::decay_t<Data>;
                using StoredExecute = std::decay_t<Execute>;

                static_assert(
                    std::is_invocable_v<StoredExecute&, StoredData&>,
                    "RenderPass execute function must be invocable with PassData"
                );

                auto ex = std::make_unique<Executor<StoredData, StoredExecute>>(
                    std::forward<Data>(data),
                    std::forward<Execute>(execute)
                );

                return RenderPass(
                    std::move(name),
                    type,
                    std::move(resources),
                    std::move(ex)
                );
            }
        };
    };
} // namespace Vixen
