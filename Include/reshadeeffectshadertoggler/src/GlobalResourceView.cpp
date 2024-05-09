#include "GlobalResourceView.h"

using namespace Rendering;
using namespace reshade::api;

GlobalResourceView::GlobalResourceView(reshade::api::device* d, reshade::api::resource r, reshade::api::format format)
{
    device = d;
    resource_handle = r.handle;
    rtv = { 0 };
    rtv_srgb = { 0 };
    srv = { 0 };
    srv_srgb = { 0 };
    state = GlobalResourceState::RESOURCE_VALID;

    resource_desc desc = device->get_resource_desc(r);

    if ((static_cast<uint32_t>(desc.usage) & static_cast<uint32_t>(resource_usage::render_target) || static_cast<uint32_t>(desc.usage) & static_cast<uint32_t>(resource_usage::shader_resource)) && desc.type == resource_type::texture_2d)
    {
        reshade::api::format f = format == reshade::api::format::unknown ? desc.texture.format : format;

        reshade::api::format format_non_srgb = format_to_default_typed(f, 0);
        reshade::api::format format_srgb = format_to_default_typed(f, 1);

        if (static_cast<uint32_t>(desc.usage & resource_usage::render_target))
        {
            device->create_resource_view(r, resource_usage::render_target, resource_view_desc(format_non_srgb), &rtv);
            device->create_resource_view(r, resource_usage::render_target, resource_view_desc(format_srgb), &rtv_srgb);
        }

        if (static_cast<uint32_t>(desc.usage & resource_usage::shader_resource) && IsValidShaderResource(desc.texture.format))
        {
            device->create_resource_view(r, resource_usage::shader_resource, resource_view_desc(format_non_srgb), &srv);
            device->create_resource_view(r, resource_usage::shader_resource, resource_view_desc(format_srgb), &srv_srgb);
        }
    }
}

GlobalResourceView::~GlobalResourceView()
{
    Dispose();
}

inline bool GlobalResourceView::IsValidShaderResource(reshade::api::format format)
{
    return format != reshade::api::format::intz;
}

void GlobalResourceView::Dispose()
{
    if (device == nullptr)
    {
        return;
    }

    if (rtv != 0)
    {
        device->destroy_resource_view(rtv);
        rtv = { 0 };
    }

    if (rtv_srgb != 0)
    {
        device->destroy_resource_view(rtv_srgb);
        rtv_srgb = { 0 };
    }

    if (srv != 0)
    {
        device->destroy_resource_view(srv);
        srv = { 0 };
    }

    if (srv_srgb != 0)
    {
        device->destroy_resource_view(srv_srgb);
        srv_srgb = { 0 };
    }
}