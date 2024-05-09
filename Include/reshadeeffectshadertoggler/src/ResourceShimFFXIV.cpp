#include "ResourceShimFFXIV.h"
#include "PipelinePrivateData.h"

using namespace Shim::Resources;
using namespace reshade::api;
using namespace std;

sig_ffxiv_texture_create* ResourceShimFFXIV::org_ffxiv_texture_create = nullptr;
sig_ffxiv_textures_create* ResourceShimFFXIV::org_ffxiv_textures_create = nullptr;
sig_ffxiv_textures_recreate* ResourceShimFFXIV::org_ffxiv_textures_recreate = nullptr;

uintptr_t* ResourceShimFFXIV::ffxiv_texture = nullptr;

uintptr_t ResourceShimFFXIV::ffxiv_recreation_struct = 0;

uintptr_t ResourceShimFFXIV::ffxiv_creation_struct = 0;
int32_t ResourceShimFFXIV::ffxiv_current_creation_index = -1;
unordered_set<uintptr_t> ResourceShimFFXIV::ffxiv_created_resources;

bool ResourceShimFFXIV::Init()
{
    return
        GameHookT<sig_ffxiv_textures_recreate>::Hook(&org_ffxiv_textures_recreate, detour_ffxiv_textures_recreate, ffxiv_textures_recreate) &&
        GameHookT<sig_ffxiv_texture_create>::Hook(&org_ffxiv_texture_create, detour_ffxiv_texture_create, ffxiv_texture_create) &&
        GameHookT<sig_ffxiv_textures_create>::Hook(&org_ffxiv_textures_create, detour_ffxiv_textures_create, ffxiv_textures_create);
}

bool ResourceShimFFXIV::UnInit()
{
    return MH_Uninitialize() == MH_OK;
}

bool ResourceShimFFXIV::OnCreateResource(reshade::api::device* device, reshade::api::resource_desc& desc, reshade::api::subresource_data* initial_data, reshade::api::resource_usage initial_state)
{
    if (static_cast<uint32_t>(desc.usage & resource_usage::render_target) && desc.type == resource_type::texture_2d)
    {
        // Resources are recreated
        if (ffxiv_recreation_struct != 0 && ffxiv_texture != nullptr)
        {
            for (uint32_t i = 0; i < RT_OFFSET_LIST.size(); i++)
            {
                if (*reinterpret_cast<uintptr_t*>(ffxiv_recreation_struct + RT_OFFSET_LIST[i]) == reinterpret_cast<uintptr_t>(ffxiv_texture))
                {
                    switch (RT_OFFSET_LIST[i])
                    {
                    case RT_OFFSET::RT_UI:
                    {
                        DeviceDataContainer& dev = device->get_private_data<DeviceDataContainer>();
                        resource_desc d = device->get_resource_desc(dev.current_runtime->get_current_back_buffer());

                        desc.texture.format = format_to_typeless(d.texture.format);
                    }
                    return true;
                    case RT_OFFSET::RT_NORMALS:
                    case RT_OFFSET::RT_NORMALS_DECAL:
                        desc.texture.format = format::r16g16b16a16_unorm;
                        return true;
                    default:
                        return false;
                    }
                }
            }
        }

        // Resources are created the first time and assigned to some overarching struct. We iterate using the fixed order of initialization,
        // which is pretty ugly, but we make do...
        if (ffxiv_creation_struct != 0 && ffxiv_texture != nullptr)
        {
            DeviceDataContainer& dev = device->get_private_data<DeviceDataContainer>();
            effect_runtime* runtime = dev.current_runtime;

            uint32_t w, h;
            runtime->get_screenshot_width_and_height(&w, &h);

            if (desc.texture.width == w && desc.texture.height == h && desc.texture.format == reshade::api::format::b8g8r8a8_unorm)
            {
                ffxiv_current_creation_index++;
                uintptr_t rt_offset = RT_OFFSET_LIST[ffxiv_current_creation_index];
                ffxiv_created_resources.emplace(reinterpret_cast<uintptr_t>(ffxiv_texture));

                switch (rt_offset)
                {
                case RT_OFFSET::RT_UI:
                {
                    DeviceDataContainer& dev = device->get_private_data<DeviceDataContainer>();
                    resource_desc d = device->get_resource_desc(dev.current_runtime->get_current_back_buffer());
                
                    desc.texture.format = format_to_typeless(d.texture.format);
                }
                    return true;
                case RT_OFFSET::RT_NORMALS:
                case RT_OFFSET::RT_NORMALS_DECAL:
                    desc.texture.format = format::r16g16b16a16_unorm;
                    return true;
                default:
                    return false;
                }
            }
        }
    }

    return false;
}


void ResourceShimFFXIV::OnDestroyResource(reshade::api::device* device, reshade::api::resource res)
{

}

void ResourceShimFFXIV::OnInitResource(reshade::api::device* device, const reshade::api::resource_desc& desc, const reshade::api::subresource_data* initData, reshade::api::resource_usage usage, reshade::api::resource handle)
{

}

bool ResourceShimFFXIV::OnCreateResourceView(reshade::api::device* device, reshade::api::resource resource, reshade::api::resource_usage usage_type, reshade::api::resource_view_desc& desc)
{
    const resource_desc texture_desc = device->get_resource_desc(resource);
    if(!static_cast<uint32_t>(texture_desc.usage & resource_usage::render_target) || texture_desc.type != resource_type::texture_2d || ffxiv_texture == nullptr)
        return false;

    if (desc.type == resource_view_type::unknown)
    {
        desc.type = texture_desc.texture.depth_or_layers > 1 ? resource_view_type::texture_2d_array : resource_view_type::texture_2d;
        desc.texture.first_level = 0;
        desc.texture.level_count = (usage_type == resource_usage::shader_resource) ? UINT32_MAX : 1;
        desc.texture.first_layer = 0;
        desc.texture.layer_count = (usage_type == resource_usage::shader_resource) ? UINT32_MAX : 1;
    }

    if (ffxiv_recreation_struct != 0)
    {
        for (uint32_t i = 0; i < RT_OFFSET_LIST.size(); i++)
        {
            if (*reinterpret_cast<uintptr_t*>(ffxiv_recreation_struct + RT_OFFSET_LIST[i]) == reinterpret_cast<uintptr_t>(ffxiv_texture))
            {
                switch (RT_OFFSET_LIST[i])
                {
                case RT_OFFSET::RT_UI:
                {
                    resource_desc d = device->get_resource_desc(resource);
                    desc.format = format_to_default_typed(d.texture.format, 0);
                }
                return true;
                case RT_OFFSET::RT_NORMALS:
                case RT_OFFSET::RT_NORMALS_DECAL:
                    desc.format = format::r16g16b16a16_unorm;
                    return true;
                default:
                    return false;
                }
            }
        }
    }

    if (ffxiv_creation_struct != 0 && ffxiv_created_resources.contains(reinterpret_cast<uintptr_t>(ffxiv_texture)))
    {
        DeviceDataContainer& dev = device->get_private_data<DeviceDataContainer>();
        effect_runtime* runtime = dev.current_runtime;

        uint32_t w, h;
        runtime->get_screenshot_width_and_height(&w, &h);
        resource_desc d = device->get_resource_desc(resource);

        uintptr_t rt_offset = RT_OFFSET_LIST[ffxiv_current_creation_index];

        switch (rt_offset)
        {
        case RT_OFFSET::RT_UI:
        {
            desc.format = format_to_default_typed(d.texture.format, 0);
        }
        return true;
        case RT_OFFSET::RT_NORMALS:
        case RT_OFFSET::RT_NORMALS_DECAL:
            desc.format = format::r16g16b16a16_unorm;
            return true;
        default:
            return false;
        }
    }

    return false;
}

void __fastcall ResourceShimFFXIV::detour_ffxiv_texture_create(uintptr_t* param_1, uintptr_t* param_2)
{
    ffxiv_texture = param_1;

    org_ffxiv_texture_create(param_1, param_2);

    ffxiv_texture = nullptr;
}

uintptr_t __fastcall ResourceShimFFXIV::detour_ffxiv_textures_recreate(uintptr_t param_1)
{
    ffxiv_recreation_struct = param_1;

    uintptr_t ret = org_ffxiv_textures_recreate(param_1);

    ffxiv_recreation_struct = 0;

    return ret;
}

uintptr_t __fastcall ResourceShimFFXIV::detour_ffxiv_textures_create(uintptr_t param_1)
{
    ffxiv_creation_struct = param_1;
    ffxiv_current_creation_index = -1;
    ffxiv_created_resources.clear();

    uintptr_t ret = org_ffxiv_textures_create(param_1);

    ffxiv_creation_struct = 0;

    return ret;
}