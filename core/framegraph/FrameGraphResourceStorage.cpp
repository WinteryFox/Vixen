#include "FrameGraphResourceStorage.h"

#include <stdexcept>
#include <utility>

#include "../RenderingDevice.h"

namespace Vixen {
    void FrameGraphResourceStorage::requireEmpty(const std::size_t index) const {
        if (index >= resources.size())
            throw std::out_of_range{"Frame-graph resource index is out of range"};

        if (ownership[index] != Ownership::Empty)
            throw std::logic_error{"Frame-graph resource slot is already populated"};
    }

    FrameGraphResourceStorage::FrameGraphResourceStorage(
        RenderingDevice& device,
        const std::size_t resourceCount
    ) : device(&device),
        resources(resourceCount),
        ownership(resourceCount, Ownership::Empty) {}

    FrameGraphResourceStorage::FrameGraphResourceStorage(
        FrameGraphResourceStorage&& other
    ) noexcept : device(std::exchange(other.device, nullptr)),
                 resources(std::move(other.resources)),
                 ownership(std::move(other.ownership)) {}

    FrameGraphResourceStorage::~FrameGraphResourceStorage() {
        reset();
    }

    void FrameGraphResourceStorage::setOwned(const std::size_t index, Image* image) {
        requireEmpty(index);

        if (image == nullptr)
            throw std::invalid_argument{"An owned frame-graph image cannot be null"};

        resources[index] = image;
        ownership[index] = Ownership::Owned;
    }

    void FrameGraphResourceStorage::setOwned(const std::size_t index, Buffer* buffer) {
        requireEmpty(index);

        if (buffer == nullptr)
            throw std::invalid_argument{"An owned frame-graph buffer cannot be null"};

        resources[index] = buffer;
        ownership[index] = Ownership::Owned;
    }

    void FrameGraphResourceStorage::setImported(const std::size_t index, Image* image) {
        requireEmpty(index);

        if (image == nullptr)
            throw std::invalid_argument{"An imported frame-graph image cannot be null"};

        resources[index] = image;
        ownership[index] = Ownership::Imported;
    }

    void FrameGraphResourceStorage::setImported(const std::size_t index, Buffer* buffer) {
        requireEmpty(index);

        if (buffer == nullptr)
            throw std::invalid_argument{"An imported frame-graph buffer cannot be null"};

        resources[index] = buffer;
        ownership[index] = Ownership::Imported;
    }

    void FrameGraphResourceStorage::reset() {
        if (device == nullptr)
            return;

        for (std::size_t i = resources.size(); i-- > 0;) {
            if (ownership[i] != Ownership::Owned)
                continue;

            if (const auto image = std::get_if<Image*>(&resources[i]))
                device->deferDestroy(*image);

            else if (const auto buffer = std::get_if<Buffer*>(&resources[i]))
                device->deferDestroy(*buffer);
        }

        resources.clear();
        ownership.clear();
    }

    FrameGraphResourceView FrameGraphResourceStorage::getResources() const noexcept {
        return FrameGraphResourceView{resources};
    }
}
