#pragma once

#include <cstdint>
#include <expected>
#include <functional>
#include <optional>
#include <string>
#include <unordered_set>
#include <utility>
#include <vector>

#include "FrameGraphResourceStorage.h"
#include "Node.h"
#include "RenderPass.h"
#include "RenderPassType.h"

namespace Vixen {
    enum class FrameGraphErrorCode;
    struct FrameGraphError;
    class Buffer;
    struct Image;

    class FrameGraph final {
        struct VersionAccess {
            uint32_t pass;
            ResourceState state;
        };

        struct ResourceVersion {
            std::optional<VersionAccess> producer;
            std::vector<VersionAccess> consumers;

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

            friend bool operator==(const Dependency& lhs, const Dependency& rhs) = default;
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

        DependencyPlan dependencyPlan;

        FrameGraphResourceStorage storage;

        std::vector<RenderPass> renderPasses;

        FrameGraph(
            std::vector<ResourceNode>&& nodes,
            DependencyPlan&& dependencyPlan,
            FrameGraphResourceStorage&& storage,
            std::vector<RenderPass>&& renderPasses
        );

    public:
        FrameGraph(const FrameGraph& other) = delete;
        FrameGraph& operator=(const FrameGraph& other) = delete;

        FrameGraph(FrameGraph&& other) noexcept;
        FrameGraph& operator=(FrameGraph&& other) noexcept = delete;

        ~FrameGraph() = default;

        void execute(RenderPassContext& context);

        class Builder {
            std::vector<ResourceNode> nodes;

            std::unordered_set<std::string> resourceNames;

            std::vector<RenderPass> renderPasses;

            [[nodiscard]] FrameGraphError resourceError(
                FrameGraphErrorCode code,
                std::string message,
                uint32_t resourceIndex,
                std::optional<uint32_t> version = std::nullopt
            ) const;

            [[nodiscard]] FrameGraphError passError(
                FrameGraphErrorCode code,
                std::string message,
                uint32_t passIndex,
                std::optional<uint32_t> resourceIndex = std::nullopt,
                std::optional<uint32_t> resourceVersion = std::nullopt
            ) const;

            [[nodiscard]] auto validateAndInitializeResources(DependencyPlan& plan) const
                -> std::expected<void, FrameGraphError>;

            [[nodiscard]] auto recordPassUsages(
                DependencyPlan& plan,
                std::vector<uint32_t>& declaredVersions
            ) const -> std::expected<void, FrameGraphError>;

            [[nodiscard]] auto validateAttachments(
                const DependencyPlan& plan
            ) const -> std::expected<void, FrameGraphError>;

            [[nodiscard]] auto validateVersionTable(
                const DependencyPlan& plan,
                const std::vector<uint32_t>& declaredVersions
            ) const -> std::expected<void, FrameGraphError>;

            static void buildDependencyEdges(DependencyPlan& plan);

            [[nodiscard]] auto buildExecutionOrder(DependencyPlan& plan) const
                -> std::expected<void, FrameGraphError>;

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

            /**
             * Compiles the declarations collected by the builder into an executable dependency plan.
             * Compilation proceeds through the following phases:
             *
             * 1. Validate resource declarations and initialize the version table for each resource.
             * 2. Validate pass resource usages and record each version's producer and consumers.
             * 3. Verify that declaration-time version cursors and producer records are complete.
             * 4. Convert resource accesses into dependencies between passes.
             * 5. Produce a stable topological execution order and reject dependency cycles.
             *
             * @return The compiled FrameGraph::DependencyPlan, or a FrameGraphError describing the
             * first compilation failure.
             */
            [[nodiscard]] auto compile() const -> std::expected<DependencyPlan, FrameGraphError>;

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
