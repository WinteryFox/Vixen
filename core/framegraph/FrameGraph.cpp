#include "FrameGraph.h"

#include <algorithm>
#include <stdexcept>
#include <utility>

#include "buffer/Buffer.h"
#include "image/Image.h"

namespace Vixen {
    FrameGraph::FrameGraph(std::vector<ResourceNode>&& resources, std::vector<RenderPass>&& renderPasses)
        : resources(std::move(resources)),
          renderPasses(std::move(renderPasses)) {}

    ResourceId FrameGraph::Builder::addResource(
        std::string name,
        const ResourceType type,
        const ResourceLifetime lifetime,
        ResourceDescription description,
        ImportedResource importedResource,
        std::optional<ResourceState> initialState,
        std::optional<ResourceState> finalState
    ) {
        if (name.empty())
            throw std::invalid_argument{"Frame graph resource name must not be empty"};

        if (std::ranges::any_of(resources, [&](const ResourceNode& resource) {
            return resource.name == name;
        }))
            throw std::invalid_argument{"A frame graph resource named '" + name + "' already exists"};

        if (resources.size() >= ResourceId::Invalid)
            throw std::overflow_error{"Frame graph resource limit exceeded"};

        const bool imported = lifetime == ResourceLifetime::Imported;
        const bool hasImportedResource = !std::holds_alternative<std::monostate>(importedResource);
        if (imported != hasImportedResource ||
            imported != initialState.has_value() ||
            imported != finalState.has_value())
            throw std::logic_error{"Invalid frame graph resource ownership metadata"};

        if (imported) {
            const bool importedTypeMatches = type == ResourceType::Image
                                                 ? std::holds_alternative<Image*>(importedResource) &&
                                                 std::holds_alternative<ImageState>(*initialState) &&
                                                 std::holds_alternative<ImageState>(*finalState)
                                                 : std::holds_alternative<Buffer*>(importedResource) &&
                                                 std::holds_alternative<BufferState>(*initialState) &&
                                                 std::holds_alternative<BufferState>(*finalState);
            if (!importedTypeMatches)
                throw std::logic_error{"Imported frame graph resource metadata has the wrong type"};
        }

        const auto index = static_cast<uint32_t>(resources.size());

        resources.push_back({
            .name = std::move(name),
            .type = type,
            .lifetime = lifetime,
            .description = description,
            .latestVersion = 0,
            .importedResource = importedResource,
            .initialState = initialState,
            .finalState = finalState
        });

        return ResourceId{
            .index = index,
            .version = 0
        };
    }

    ImageHandle FrameGraph::Builder::createImage(
        std::string name,
        ImageResourceDescription description,
        const ResourceLifetime lifetime
    ) {
        if (lifetime == ResourceLifetime::Imported)
            throw std::invalid_argument{"Imported images must be added through importImage"};

        if (description.format.width == 0 || description.format.height == 0 || description.format.depth == 0 ||
            description.format.layerCount == 0 || description.format.mipmapCount == 0)
            throw std::invalid_argument{"Frame graph image dimensions, layer count, and mipmap count must be nonzero"};

        if (description.format.usage.empty())
            throw std::invalid_argument{"Frame graph image usage must not be empty"};

        return ImageHandle{
            .id = addResource(
                std::move(name),
                ResourceType::Image,
                lifetime,
                description
            )
        };
    }

    BufferHandle FrameGraph::Builder::createBuffer(
        std::string name,
        BufferFormat description,
        const ResourceLifetime lifetime
    ) {
        if (lifetime == ResourceLifetime::Imported)
            throw std::invalid_argument{"Imported buffers must be added through importBuffer"};

        if (description.count == 0 || description.stride == 0)
            throw std::invalid_argument{"Frame graph buffer count and stride must be nonzero"};

        if (description.usage.empty())
            throw std::invalid_argument{"Frame graph buffer usage must not be empty"};

        return BufferHandle{
            .id = addResource(
                std::move(name),
                ResourceType::Buffer,
                lifetime,
                description
            )
        };
    }

    ImageHandle FrameGraph::Builder::importImage(
        std::string name,
        Image& image,
        ImageState initialState,
        ImageState finalState
    ) {
        return ImageHandle{
            .id = addResource(
                std::move(name),
                ResourceType::Image,
                ResourceLifetime::Imported,
                ImageResourceDescription{
                    .format = image.format,
                    .view = image.view
                },
                &image,
                initialState,
                finalState
            )
        };
    }

    BufferHandle FrameGraph::Builder::importBuffer(
        std::string name,
        Buffer& buffer,
        BufferState initialState,
        BufferState finalState
    ) {
        return BufferHandle{
            .id = addResource(
                std::move(name),
                ResourceType::Buffer,
                ResourceLifetime::Imported,
                BufferFormat{
                    .count = buffer.getCount(),
                    .stride = buffer.getStride(),
                    .usage = buffer.getUsage()
                },
                &buffer,
                initialState,
                finalState
            )
        };
    }
}
