#pragma once

#include <expected>
#include <functional>
#include <string>
#include <utility>
#include <vector>

#include "FrameGraphResourceStorage.h"
#include "Node.h"
#include "RenderPass.h"
#include "RenderPassType.h"

namespace Vixen {
    struct FrameGraphError;
    class Buffer;
    struct Image;

    class FrameGraph final {
        struct ResourceVersion {
            std::optional<uint32_t> producer;
            std::vector<uint32_t> consumers;

            bool initializedExternally = false;
        };

        enum class DependencyType {
            ReadAfterRead,
            ReadAfterWrite,
            WriteAfterRead,
            WriteAfterWrite,
            ExplicitOrdering
        };

        struct Dependency {
            ResourceId handle;
            DependencyType type = DependencyType::ExplicitOrdering;
        };

        struct PassEdge {
            uint32_t pass;
            std::vector<Dependency> dependencies;
        };

        struct PassDependencies {
            std::vector<PassEdge> predecessors;
            std::vector<PassEdge> successors;
        };

        struct DependencyPlan {
            std::vector<std::vector<ResourceVersion>> resources;
            std::vector<PassDependencies> passes;
            std::vector<uint32_t> executionOrder;
        };

        std::vector<ResourceNode> nodes;

        FrameGraphResourceStorage storage;

        std::vector<RenderPass> renderPasses;

        FrameGraph(std::vector<ResourceNode>&& nodes, FrameGraphResourceStorage&& storage,
                   std::vector<RenderPass>&& renderPasses);

    public:
        FrameGraph(const FrameGraph& other) = delete;
        FrameGraph& operator=(const FrameGraph& other) = delete;

        FrameGraph(FrameGraph&& other) noexcept;
        FrameGraph& operator=(FrameGraph&& other) noexcept = delete;

        ~FrameGraph() = default;

        void execute(RenderPassContext& context);

        class Builder {
            std::vector<ResourceNode> nodes;

            std::vector<RenderPass> renderPasses;

            template <typename PassData, typename Setup, typename Execute>
            Builder& addPass(
                std::string name,
                const RenderPassType type,
                Setup&& setup,
                Execute&& execute
            ) {
                std::vector<uint32_t> versionsBefore;
                versionsBefore.reserve(nodes.size());
                for (const auto& resource : nodes)
                    versionsBefore.push_back(resource.latestVersion);

                try {
                    PassData data{};

                    RenderPass::Builder passBuilder{
                        nodes,
                        std::move(name),
                        type
                    };

                    std::invoke(std::forward<Setup>(setup), passBuilder, data);

                    renderPasses.emplace_back(
                        std::move(passBuilder).build<PassData>(
                            std::move(data),
                            std::forward<Execute>(execute)
                        )
                    );
                } catch (...) {
                    for (std::size_t i = 0; i < versionsBefore.size(); ++i)
                        nodes[i].latestVersion = versionsBefore[i];

                    throw;
                }

                return *this;
            }

            ResourceId addResource(
                std::string name,
                ResourceType type,
                ResourceLifetime lifetime,
                ResourceDescription description,
                ImportedResource importedResource = {},
                std::optional<ResourceState> initialState = std::nullopt,
                std::optional<ResourceState> finalState = std::nullopt
            );

        public:
            Builder() = default;

            Builder(const Builder& other) = delete;
            Builder& operator=(const Builder& other) = delete;

            Builder(Builder&& other) noexcept = default;
            Builder& operator=(Builder&& other) noexcept = default;

            ~Builder() = default;

            template <typename PassData, typename Setup, typename Execute>
            Builder& addGraphicsPass(
                std::string name,
                Setup&& setup,
                Execute&& execute
            ) {
                return addPass<PassData>(
                    std::move(name),
                    RenderPassType::Graphics,
                    std::forward<Setup>(setup),
                    std::forward<Execute>(execute)
                );
            }

            template <typename PassData, typename Setup, typename Execute>
            Builder& addComputePass(
                std::string name,
                Setup&& setup,
                Execute&& execute
            ) {
                return addPass<PassData>(
                    std::move(name),
                    RenderPassType::Compute,
                    std::forward<Setup>(setup),
                    std::forward<Execute>(execute)
                );
            }

            ImageHandle createImage(
                std::string name,
                ImageResourceDescription description,
                ResourceLifetime lifetime = ResourceLifetime::Transient
            );

            BufferHandle createBuffer(
                std::string name,
                BufferFormat description,
                ResourceLifetime lifetime = ResourceLifetime::Transient
            );

            ImageHandle importImage(
                std::string name,
                Image& image,
                ImageState initialState,
                ImageState finalState
            );

            BufferHandle importBuffer(
                std::string name,
                Buffer& buffer,
                BufferState initialState,
                BufferState finalState
            );

            [[nodiscard]] auto build(RenderingDevice& device) && -> std::expected<FrameGraph, FrameGraphError>;
        };
    };
} // namespace Vixen
