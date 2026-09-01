#include "FrameGraphResourceAccess.h"

#include <algorithm>
#include <format>
#include <limits>
#include <string>
#include <utility>

namespace Vixen {
    namespace {
        [[nodiscard]] constexpr std::uint32_t permissionResourceIndex(
            const FrameGraphResourcePermission& permission
        ) noexcept {
            if (permission.input.isValid())
                return permission.input.index;
            if (permission.output.isValid())
                return permission.output.index;
            return ResourceId::Invalid;
        }

        [[nodiscard]] constexpr const char* resourceTypeName(const ResourceType type) noexcept {
            switch (type) {
                case ResourceType::Image:
                    return "image";
                case ResourceType::Buffer:
                    return "buffer";
                default:
                    return "unrecognized resource type";
            }
        }

        [[nodiscard]] constexpr const char* accessName(const ResourceAccess access) noexcept {
            switch (access) {
                case ResourceAccess::Read:
                    return "read";
                case ResourceAccess::Write:
                    return "write";
                case ResourceAccess::ReadWrite:
                    return "read-write";
                default:
                    return "unrecognized access";
            }
        }

        [[nodiscard]] FrameGraphResourceAccessError makeError(
            const FrameGraphResourceAccessErrorCode code,
            std::string message,
            const FrameGraphResourceAccessRequest& request,
            const FrameGraphResourceAccessContext context,
            const FrameGraphResourcePermission* permission = nullptr
        ) {
            FrameGraphResourceAccessError error{
                .code = code,
                .message = std::move(message),
                .resourceVersion = request.handle.version,
                .requestedOperation = request
            };

            if (request.handle.isValid())
                error.resourceIndex = request.handle.index;
            if (context.passIndex.has_value())
                error.passIndex = context.passIndex;
            if (!context.passName.empty())
                error.passName = std::string{context.passName};
            if (!context.resourceName.empty())
                error.resourceName = std::string{context.resourceName};
            if (permission != nullptr)
                error.declaredOperations.push_back(*permission);
            if (context.sideEffecting)
                error.details.emplace_back(
                    "The pass is marked as side-effecting and must remain uncullable"
                );
            if (context.usesExternallySynchronizedResources)
                error.details.emplace_back(
                    "The pass captures external resources whose synchronization remains the caller's responsibility; this does not bypass frame-graph permissions"
                );

            return error;
        }

        [[nodiscard]] constexpr bool isKnownResourceType(const ResourceType type) noexcept {
            return type == ResourceType::Image || type == ResourceType::Buffer;
        }

        [[nodiscard]] constexpr bool isKnownAccess(const ResourceAccess access) noexcept {
            return access == ResourceAccess::Read ||
                access == ResourceAccess::Write ||
                access == ResourceAccess::ReadWrite;
        }

        [[nodiscard]] bool usageMatchesType(
            const FrameGraphResourceUsageKind& usage,
            const ResourceType type
        ) noexcept {
            switch (type) {
                case ResourceType::Image:
                    return std::holds_alternative<ImageUsageBits>(usage);
                case ResourceType::Buffer:
                    return std::holds_alternative<BufferUsageBits>(usage);
                default:
                    return false;
            }
        }
    }

    auto authorizeFrameGraphResourceAccess(
        const std::span<const FrameGraphResourcePermission> permissions,
        const FrameGraphResourceAccessRequest& request,
        const FrameGraphResourceAccessContext context
    ) -> std::expected<const FrameGraphResourcePermission*, FrameGraphResourceAccessError> {
        if (!request.handle.isValid())
            return std::unexpected{
                makeError(
                    FrameGraphResourceAccessErrorCode::InvalidHandle,
                    "Cannot authorize a default-initialized frame-graph resource handle",
                    request,
                    context
                )
            };

        if (!isKnownResourceType(request.type))
            return std::unexpected{
                makeError(
                    FrameGraphResourceAccessErrorCode::InvalidInvariant,
                    "Cannot authorize a frame-graph resource request with an unrecognized ResourceType value",
                    request,
                    context
                )
            };

        if (!isKnownAccess(request.access))
            return std::unexpected{
                makeError(
                    FrameGraphResourceAccessErrorCode::InvalidInvariant,
                    "Cannot authorize a frame-graph resource request with an unrecognized ResourceAccess value",
                    request,
                    context
                )
            };

        const auto permission = std::lower_bound(
            permissions.begin(),
            permissions.end(),
            request.handle.index,
            [](const FrameGraphResourcePermission& candidate, const std::uint32_t resourceIndex) {
                return permissionResourceIndex(candidate) < resourceIndex;
            }
        );

        if (permission == permissions.end() ||
            permissionResourceIndex(*permission) != request.handle.index)
            return std::unexpected{
                makeError(
                    FrameGraphResourceAccessErrorCode::UndeclaredResource,
                    std::format(
                        "Pass did not declare frame-graph resource index {}; every callback resource access must be declared during pass setup",
                        request.handle.index
                    ),
                    request,
                    context
                )
            };

        const auto invalidInputOutputShape =
            (!permission->input.isValid() && !permission->output.isValid()) ||
            (permission->input.isValid() && permission->input.index != request.handle.index) ||
            (permission->output.isValid() && permission->output.index != request.handle.index);

        if (invalidInputOutputShape ||
            !isKnownResourceType(permission->type) ||
            !isKnownAccess(permission->access) ||
            !usageMatchesType(permission->usage, permission->type))
            return std::unexpected{
                makeError(
                    FrameGraphResourceAccessErrorCode::InvalidInvariant,
                    std::format(
                        "Compiled permission for resource index {} is internally inconsistent",
                        request.handle.index
                    ),
                    request,
                    context,
                    &*permission
                )
            };

        if (permission->input.isValid() && permission->output.isValid() &&
            permission->input.index != permission->output.index)
            return std::unexpected{
                makeError(
                    FrameGraphResourceAccessErrorCode::InvalidInvariant,
                    "Compiled read-write permission refers to different input and output resources",
                    request,
                    context,
                    &*permission
                )
            };

        if (permission->type != request.type)
            return std::unexpected{
                makeError(
                    FrameGraphResourceAccessErrorCode::ResourceTypeMismatch,
                    std::format(
                        "Resource index {} was requested as an {}, but this pass declared it as an {}",
                        request.handle.index,
                        resourceTypeName(request.type),
                        resourceTypeName(permission->type)
                    ),
                    request,
                    context,
                    &*permission
                )
            };

        const bool matchesInput = permission->input == request.handle;
        const bool matchesOutput = permission->output == request.handle;
        if (!matchesInput && !matchesOutput)
            return std::unexpected{
                makeError(
                    FrameGraphResourceAccessErrorCode::UndeclaredVersion,
                    std::format(
                        "Pass declared resource index {}, but not logical version {}; physical resource aliasing does not authorize undeclared versions",
                        request.handle.index,
                        request.handle.version
                    ),
                    request,
                    context,
                    &*permission
                )
            };

        if (request.usage.has_value()) {
            if (!usageMatchesType(*request.usage, request.type))
                return std::unexpected{
                    makeError(
                        FrameGraphResourceAccessErrorCode::UsageMismatch,
                        std::format(
                            "The requested usage kind does not match the requested {} resource type",
                            resourceTypeName(request.type)
                        ),
                        request,
                        context,
                        &*permission
                    )
                };

            if (*request.usage != permission->usage)
                return std::unexpected{
                    makeError(
                        FrameGraphResourceAccessErrorCode::UsageMismatch,
                        std::format(
                            "Requested {} usage for resource index {} does not match the usage declared by this pass",
                            resourceTypeName(request.type),
                            request.handle.index
                        ),
                        request,
                        context,
                        &*permission
                    )
                };
        }

        if (request.attachmentRole != FrameGraphAttachmentRole::None &&
            request.attachmentRole != permission->attachmentRole)
            return std::unexpected{
                makeError(
                    FrameGraphResourceAccessErrorCode::UsageMismatch,
                    std::format(
                        "Resource index {} is not declared with the requested attachment role",
                        request.handle.index
                    ),
                    request,
                    context,
                    &*permission
                )
            };

        bool authorized = false;
        switch (request.access) {
            case ResourceAccess::Read:
                switch (permission->access) {
                    case ResourceAccess::Read:
                        authorized = matchesInput;
                        break;
                    case ResourceAccess::Write:
                        authorized = false;
                        break;
                    case ResourceAccess::ReadWrite:
                        authorized = matchesInput || matchesOutput;
                        break;
                    default:
                        break;
                }
                break;

            case ResourceAccess::Write:
                authorized = matchesOutput &&
                    (permission->access == ResourceAccess::Write ||
                     permission->access == ResourceAccess::ReadWrite);
                break;

            case ResourceAccess::ReadWrite:
                authorized = matchesOutput && permission->access == ResourceAccess::ReadWrite;
                break;

            default:
                break;
        }

        if (!authorized)
            return std::unexpected{
                makeError(
                    FrameGraphResourceAccessErrorCode::AccessDenied,
                    std::format(
                        "Pass is not authorized to {} resource index {} version {}; its declaration grants {} access to different endpoint(s)",
                        accessName(request.access),
                        request.handle.index,
                        request.handle.version,
                        accessName(permission->access)
                    ),
                    request,
                    context,
                    &*permission
                )
            };

        return &*permission;
    }
}
