#pragma once

#include <cstddef>
#include <vector>

#include "FrameGraphResourceView.h"

namespace Vixen {
    class RenderingDevice;

    class FrameGraphResourceStorage final {
        enum class Ownership { Empty, Imported, Owned };

        RenderingDevice* device;

        std::vector<ResourceObject> resources;

        std::vector<Ownership> ownership;

        void requireEmpty(std::size_t index) const;

    public:
        FrameGraphResourceStorage(RenderingDevice& device, std::size_t resourceCount);

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
