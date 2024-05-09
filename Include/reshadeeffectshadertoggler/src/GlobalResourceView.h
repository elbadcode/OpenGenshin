#pragma once

#include <reshade.hpp>
#include <reshade_api.hpp>

namespace Rendering
{
    enum class GlobalResourceState : uint32_t
    {
        RESOURCE_INVALID = 0,
        RESOURCE_VALID = 1,
        RESOURCE_USED = 2,
    };

    class GlobalResourceView final
    {
    public:
        GlobalResourceView() = delete;
        GlobalResourceView(reshade::api::device*, reshade::api::resource, reshade::api::format);

        ~GlobalResourceView();

        void Dispose();

        uint64_t resource_handle;
        reshade::api::resource_view rtv;
        reshade::api::resource_view rtv_srgb;
        reshade::api::resource_view srv;
        reshade::api::resource_view srv_srgb;
        GlobalResourceState state;

    private:
        static inline bool IsValidShaderResource(reshade::api::format);

        reshade::api::device* device;
    };
}