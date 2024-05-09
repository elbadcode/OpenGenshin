#include "ResourceShimSRGB.h"

using namespace Shim::Resources;
using namespace reshade::api;
using namespace std;

bool ResourceShimSRGB::_IsSRGB(reshade::api::format value)
{
    switch (value)
    {
    case format::r8g8b8a8_unorm_srgb:
    case format::r8g8b8x8_unorm_srgb:
    case format::b8g8r8a8_unorm_srgb:
    case format::b8g8r8x8_unorm_srgb:
        return true;
    default:
        return false;
    }

    return false;
}


bool ResourceShimSRGB::_HasSRGB(reshade::api::format value)
{
    switch (value)
    {
    case format::r8g8b8a8_typeless:
    case format::r8g8b8a8_unorm:
    case format::r8g8b8a8_unorm_srgb:
    case format::r8g8b8x8_unorm:
    case format::r8g8b8x8_unorm_srgb:
    case format::b8g8r8a8_typeless:
    case format::b8g8r8a8_unorm:
    case format::b8g8r8a8_unorm_srgb:
    case format::b8g8r8x8_typeless:
    case format::b8g8r8x8_unorm:
    case format::b8g8r8x8_unorm_srgb:
        return true;
    default:
        return false;
    }

    return false;
}

bool ResourceShimSRGB::Init()
{
    return true;
}

bool ResourceShimSRGB::UnInit()
{
    return true;
}

bool ResourceShimSRGB::OnCreateResource(reshade::api::device* device, reshade::api::resource_desc& desc, reshade::api::subresource_data* initial_data, reshade::api::resource_usage initial_state)
{
    if (static_cast<uint32_t>(desc.usage & resource_usage::render_target) && desc.type == resource_type::texture_2d)
    {
        if (_HasSRGB(desc.texture.format)) {
            unique_lock<shared_mutex> lock(resource_mutex);

            s_resourceFormatTransient.emplace(&desc, desc.texture.format);

            desc.texture.format = format_to_typeless(desc.texture.format);

            return true;
        }
    }

    return false;
}


void ResourceShimSRGB::OnDestroyResource(reshade::api::device* device, reshade::api::resource res)
{
    unique_lock<shared_mutex> lock(resource_mutex);

    s_resourceFormat.erase(res.handle);
}

void ResourceShimSRGB::OnInitResource(reshade::api::device* device, const reshade::api::resource_desc& desc, const reshade::api::subresource_data* initData, reshade::api::resource_usage usage, reshade::api::resource handle)
{
    if (static_cast<uint32_t>(desc.usage & resource_usage::render_target) && desc.type == resource_type::texture_2d)
    {
        unique_lock<shared_mutex> lock(resource_mutex);

        if (s_resourceFormatTransient.contains(&desc))
        {
            reshade::api::format orgFormat = s_resourceFormatTransient.at(&desc);
            s_resourceFormat.emplace(handle.handle, orgFormat);
            s_resourceFormatTransient.erase(&desc);
        }
    }
}

bool ResourceShimSRGB::OnCreateResourceView(reshade::api::device* device, reshade::api::resource resource, reshade::api::resource_usage usage_type, reshade::api::resource_view_desc& desc)
{
    const resource_desc texture_desc = device->get_resource_desc(resource);
    if (!static_cast<uint32_t>(texture_desc.usage & resource_usage::render_target) || texture_desc.type != resource_type::texture_2d)
        return false;

    shared_lock<shared_mutex> lock(resource_mutex);
    if (s_resourceFormat.contains(resource.handle))
    {
        // Set original resource format in case the game uses that as a basis for creating it's views
        if (desc.format == format_to_typeless(desc.format) && format_to_typeless(desc.format) != format_to_default_typed(desc.format) ||
            desc.format == reshade::api::format::unknown) {

            reshade::api::format rFormat = s_resourceFormat.at(resource.handle);

            // The game may try to re-use the format setting of a previous resource that we had already set to typeless. Try default format in that case.
            desc.format = format_to_typeless(rFormat) == rFormat ? format_to_default_typed(rFormat) : rFormat;

            if (desc.type == resource_view_type::unknown)
            {
                desc.type = texture_desc.texture.depth_or_layers > 1 ? resource_view_type::texture_2d_array : resource_view_type::texture_2d;
                desc.texture.first_level = 0;
                desc.texture.level_count = (usage_type == resource_usage::shader_resource) ? UINT32_MAX : 1;
                desc.texture.first_layer = 0;
                desc.texture.layer_count = (usage_type == resource_usage::shader_resource) ? UINT32_MAX : 1;
            }

            return true;
        }
    }

    return false;
}
