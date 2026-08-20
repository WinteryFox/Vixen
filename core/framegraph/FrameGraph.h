#pragma once

#include <functional>
#include <string>
#include <utility>
#include <vector>

#include "Node.h"
#include "RenderPass.h"
#include "RenderPassType.h"

namespace Vixen {
    class Buffer;
    struct Image;

    class FrameGraph final {
        std::vector<ResourceNode> resources;

        std::vector<RenderPass> renderPasses;

        FrameGraph(std::vector<ResourceNode>&& resources, std::vector<RenderPass>&& renderPasses);

    public:
        FrameGraph(const FrameGraph& other) = delete;

        FrameGraph(FrameGraph&& other) noexcept = default;

        FrameGraph& operator=(const FrameGraph& other) = delete;

        FrameGraph& operator=(FrameGraph&& other) noexcept = default;

        ~FrameGraph() = default;

        class Builder {
            std::vector<ResourceNode> resources;

            std::vector<RenderPass> renderPasses;

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

            Builder(Builder&& other) noexcept = default;

            Builder& operator=(const Builder& other) = delete;

            Builder& operator=(Builder&& other) noexcept = default;

            ~Builder() = default;

            template <typename PassData, typename Setup, typename Execute>
            Builder& addGraphicsPass(
                std::string name,
                Setup&& setup,
                Execute&& execute
            ) {
                PassData data{};

                RenderPass::Builder passBuilder{
                    resources,
                    std::move(name),
                    RenderPassType::Graphics
                };

                std::invoke(std::forward<Setup>(setup), passBuilder, data);

                renderPasses.emplace_back(
                    std::move(passBuilder).build<PassData>(
                        std::move(data),
                        std::forward<Execute>(execute)
                    )
                );

                return *this;
            }

            template <typename PassData, typename Setup, typename Execute>
            Builder& addComputePass(
                std::string name,
                Setup&& setup,
                Execute&& execute
            ) {
                PassData data{};

                RenderPass::Builder passBuilder{
                    resources,
                    std::move(name),
                    RenderPassType::Compute
                };

                std::invoke(std::forward<Setup>(setup), passBuilder, data);

                renderPasses.emplace_back(
                    std::move(passBuilder).build<PassData>(
                        std::move(data),
                        std::forward<Execute>(execute)
                    )
                );

                return *this;
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

            [[nodiscard]] FrameGraph build() && {
                return FrameGraph{
                    std::move(resources),
                    std::move(renderPasses)
                };
            }
        };
    };
} // namespace Vixen
