#pragma once

#include <cstddef>
#include <vector>

#include "FrameGraphResourceView.h"

namespace Vixen {
    struct ResourceNode;
    class RenderingDevice;

    class FrameGraphResourceStorage final {
        RenderingDevice* device;

        std::vector<FrameGraphResourceSlot> slots;

        void requireEmpty(std::size_t index) const;

        void requireType(std::size_t index, ResourceType type) const;

        void requireOwnership(std::size_t index, Ownership ownership) const;

    public:
        FrameGraphResourceStorage(RenderingDevice& device, std::span<const ResourceNode> nodes);

        FrameGraphResourceStorage(const FrameGraphResourceStorage&) = delete;
        FrameGraphResourceStorage& operator=(const FrameGraphResourceStorage&) = delete;

        FrameGraphResourceStorage(FrameGraphResourceStorage&& other) noexcept;
        FrameGraphResourceStorage& operator=(FrameGraphResourceStorage&& other) = delete;

        ~FrameGraphResourceStorage();

        void setOwned(std::size_t index, Image* image);

        void setOwned(std::size_t index, Buffer* buffer);

        void setImported(std::size_t index, Image* image);

        void setImported(std::size_t index, Buffer* buffer);

        void reset();

        [[nodiscard]] FrameGraphResourceView getResources() const noexcept;
    };
}
