#include "FrameGraphResourceStorage.h"

#include <stdexcept>
#include <utility>

#include "Node.h"
#include "ResourceSlot.h"
#include "core/rendering/RenderingDevice.h"

namespace Vixen {
    void FrameGraphResourceStorage::requireEmpty(const std::size_t index) const {
        if (index >= slots.size())
            throw std::out_of_range{"Frame-graph resource index is out of range"};

        if (slots[index].ownership != Ownership::Empty)
            throw std::logic_error{"Frame-graph resource slot is already populated"};
    }

    void FrameGraphResourceStorage::requireType(const std::size_t index, const ResourceType type) const {
        if (slots[index].type != type)
            throw std::logic_error{"Resource is not of the expected type"};
    }

    void FrameGraphResourceStorage::requireOwnedLifetime(const std::size_t index) const {
        if (slots[index].lifetime == ResourceLifetime::Imported)
            throw std::logic_error{"An imported resource slot cannot adopt an owned object"};
    }

    void FrameGraphResourceStorage::requireImportedLifetime(const std::size_t index) const {
        if (slots[index].lifetime != ResourceLifetime::Imported)
            throw std::logic_error{"A transient or persistent resource slot cannot adopt an imported object"};
    }

    FrameGraphResourceStorage::FrameGraphResourceStorage(
        RenderingDevice& device,
        const std::span<const ResourceNode> nodes
    ) : device(device) {
        slots.reserve(nodes.size());
        for (const auto& node : nodes)
            slots.push_back({
                .type = node.type,
                .lifetime = node.lifetime,
                .latestVersion = node.latestVersion,
                .object = std::monostate{},
                .ownership = Ownership::Empty
            });
    }

    FrameGraphResourceStorage::FrameGraphResourceStorage(
        FrameGraphResourceStorage&& other
    ) noexcept : device(other.device),
                 slots(std::move(other.slots)) {}

    FrameGraphResourceStorage::~FrameGraphResourceStorage() {
        reset();
    }

    void FrameGraphResourceStorage::setOwned(const std::size_t index, Image* image) {
        requireEmpty(index);
        requireType(index, ResourceType::Image);
        requireOwnedLifetime(index);

        if (image == nullptr)
            throw std::invalid_argument{"An owned frame-graph image cannot be null"};

        slots[index].object = image;
        slots[index].ownership = Ownership::Owned;
    }

    void FrameGraphResourceStorage::setOwned(const std::size_t index, Buffer* buffer) {
        requireEmpty(index);
        requireType(index, ResourceType::Buffer);
        requireOwnedLifetime(index);

        if (buffer == nullptr)
            throw std::invalid_argument{"An owned frame-graph buffer cannot be null"};

        slots[index].object = buffer;
        slots[index].ownership = Ownership::Owned;
    }

    void FrameGraphResourceStorage::setImported(const std::size_t index, Image* image) {
        requireEmpty(index);
        requireType(index, ResourceType::Image);
        requireImportedLifetime(index);

        if (image == nullptr)
            throw std::invalid_argument{"An imported frame-graph image cannot be null"};

        slots[index].object = image;
        slots[index].ownership = Ownership::Imported;
    }

    void FrameGraphResourceStorage::setImported(const std::size_t index, Buffer* buffer) {
        requireEmpty(index);
        requireType(index, ResourceType::Buffer);
        requireImportedLifetime(index);

        if (buffer == nullptr)
            throw std::invalid_argument{"An imported frame-graph buffer cannot be null"};

        slots[index].object = buffer;
        slots[index].ownership = Ownership::Imported;
    }

    void FrameGraphResourceStorage::reset() {
        for (std::size_t i = slots.size(); i-- > 0;) {
            if (slots[i].ownership != Ownership::Owned)
                continue;

            if (const auto image = std::get_if<Image*>(&slots[i].object))
                device.deferDestroy(*image);

            else if (const auto buffer = std::get_if<Buffer*>(&slots[i].object))
                device.deferDestroy(*buffer);
        }

        slots.clear();
    }

    std::size_t FrameGraphResourceStorage::size() const noexcept {
        return slots.size();
    }

    bool FrameGraphResourceStorage::isFullyResolved() const noexcept {
        for (const auto& slot : slots) {
            if (slot.ownership == Ownership::Empty)
                return false;

            if (slot.lifetime == ResourceLifetime::Imported) {
                if (slot.ownership != Ownership::Imported)
                    return false;
            } else if (slot.ownership != Ownership::Owned) {
                return false;
            }

            switch (slot.type) {
                case ResourceType::Image: {
                    const auto image = std::get_if<Image*>(&slot.object);
                    if (image == nullptr || *image == nullptr)
                        return false;
                    break;
                }

                case ResourceType::Buffer: {
                    const auto buffer = std::get_if<Buffer*>(&slot.object);
                    if (buffer == nullptr || *buffer == nullptr)
                        return false;
                    break;
                }

                default:
                    return false;
            }
        }

        return true;
    }

    FrameGraphResourceView FrameGraphResourceStorage::getResources() const noexcept {
        return FrameGraphResourceView{slots};
    }
}
