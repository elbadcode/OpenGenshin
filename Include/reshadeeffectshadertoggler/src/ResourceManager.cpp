#include <format>
#include "ResourceManager.h"

using namespace Rendering;
using namespace reshade::api;
using namespace Shim::Resources;
using namespace std;

ResourceShimType ResourceManager::ResolveResourceShimType(const string& stype)
{
    if (stype == "none")
        return ResourceShimType::Resource_Shim_None;
    else if (stype == "srgb")
        return ResourceShimType::Resource_Shim_SRGB;
    else if (stype == "ffxiv")
        return ResourceShimType::Resource_Shim_FFXIV;

    return ResourceShimType::Resource_Shim_None;
}

void ResourceManager::Init()
{

    switch (_shimType)
    {
    case Resource_Shim_None:
        rShim = nullptr;
        break;
    case Resource_Shim_SRGB:
    {
        static ResourceShimSRGB srgbShim;
        rShim = &srgbShim;
    }
        break;
    case Resource_Shim_FFXIV:
    {
        static ResourceShimFFXIV ffxivShim;
        rShim = &ffxivShim;
    }
        break;
    default:
        rShim = nullptr;
        break;
    }

    if (rShim != nullptr && rShim->Init())
    {
        reshade::log_message(reshade::log_level::info, std::format("Resource shim initialized").c_str());
    }
    else
    {
        reshade::log_message(reshade::log_level::info, std::format("No resource shim initialized").c_str());
    }
}

void ResourceManager::InitBackbuffer(swapchain* runtime)
{
    // Create backbuffer resource views
    device* dev = runtime->get_device();

    resource_desc desc = dev->get_resource_desc(runtime->get_back_buffer(0));

    resource_desc dummy_desc = desc;
    dummy_desc.texture.height = 1;
    dummy_desc.texture.width = 1;
    
    bool resCreated = runtime->get_device()->create_resource(dummy_desc, nullptr, resource_usage::render_target, &dummy_res);

    if (resCreated)
    {
        runtime->get_device()->create_resource_view(dummy_res, resource_usage::render_target, resource_view_desc{ desc.texture.format }, &dummy_rtv);
    }
}

void ResourceManager::ClearBackbuffer(reshade::api::swapchain* runtime)
{
    device* dev = runtime->get_device();

    uint32_t count = runtime->get_back_buffer_count();

    if (dummy_res != 0)
    {
        runtime->get_device()->destroy_resource(dummy_res);
        dummy_res = { 0 };
    }

    if (dummy_rtv != 0)
    {
        runtime->get_device()->destroy_resource_view(dummy_rtv);
        dummy_rtv = { 0 };
    }
}

bool ResourceManager::OnCreateSwapchain(reshade::api::swapchain_desc& desc, void* hwnd)
{
    return false;
}

void ResourceManager::OnInitSwapchain(reshade::api::swapchain* swapchain)
{
    InitBackbuffer(swapchain);
}

void ResourceManager::OnDestroySwapchain(reshade::api::swapchain* swapchain)
{
    OnDestroyDevice(swapchain->get_device());
    ClearBackbuffer(swapchain);
}

bool ResourceManager::OnCreateResource(device* device, resource_desc& desc, subresource_data* initial_data, resource_usage initial_state)
{
    bool ret = false;

    if (static_cast<uint32_t>(desc.usage & resource_usage::render_target) &&
        !static_cast<uint32_t>(desc.usage & resource_usage::shader_resource) &&
        desc.type == resource_type::texture_2d)
    {
        desc.usage |= resource_usage::shader_resource;
        ret = true;
    }

    if (rShim != nullptr)
    {
        ret |= rShim->OnCreateResource(device, desc, initial_data, initial_state);
    }
    
    return ret;
}

void ResourceManager::OnInitResource(device* device, const resource_desc& desc, const subresource_data* initData, resource_usage usage, reshade::api::resource handle)
{
    auto& data = device->get_private_data<DeviceDataContainer>();

    if (rShim != nullptr)
    {
        rShim->OnInitResource(device, desc, initData, usage, handle);
    }
}

void ResourceManager::OnDestroyResource(device* device, resource res)
{
    if (rShim != nullptr)
    {
        rShim->OnDestroyResource(device, res);
    }

    if(!in_destroy_device)
    {
        std::shared_lock<shared_mutex> lock_view(view_mutex);

        const auto& views = global_resources.find(res.handle);

        if (views != global_resources.end())
        {
            views->second->state = GlobalResourceState::RESOURCE_INVALID;
        }
    }
}

void ResourceManager::OnDestroyDevice(device* device)
{
    in_destroy_device = true;

    std::unique_lock<shared_mutex> lock_view(view_mutex);
    global_resources.clear();

    DisposePreview(device);

    in_destroy_device = false;
}


bool ResourceManager::OnCreateResourceView(device* device, resource resource, resource_usage usage_type, resource_view_desc& desc)
{
    if (rShim != nullptr)
    {
        return rShim->OnCreateResourceView(device, resource, usage_type, desc);
    }

    return false;
}

void ResourceManager::OnInitResourceView(device* device, resource resource, resource_usage usage_type, const resource_view_desc& desc, resource_view view)
{
}

void ResourceManager::OnDestroyResourceView(device* device, resource_view view)
{
}

const std::shared_ptr<GlobalResourceView>& ResourceManager::GetResourceView(device* device, const ResourceRenderData& data)
{
    return GetResourceView(device, data.resource.handle, data.format);
}

const std::shared_ptr<GlobalResourceView>& ResourceManager::GetResourceView(device* device, uint64_t handle, reshade::api::format format)
{
    static std::shared_ptr<GlobalResourceView> emptyView{ nullptr };
    if (handle == 0)
    {
        return emptyView;
    }

    std::unique_lock<shared_mutex> lock_view(view_mutex);

    auto& res = global_resources[handle];

    if (res == nullptr)
    {
        res = std::make_shared<GlobalResourceView>(device, resource{ handle }, format);
    }

    if (res->state == GlobalResourceState::RESOURCE_INVALID)
    {
        return emptyView;
    }

    if (res->state == GlobalResourceState::RESOURCE_VALID)
        res->state = GlobalResourceState::RESOURCE_USED;

    return res;
}

void ResourceManager::DisposePreview(reshade::api::device* device)
{
    if (preview_res[0] == 0 && preview_res[1] == 0)
        return;

    for (uint32_t i = 0; i < 2; i++)
    {
        if (preview_srv[i] != 0)
        {
            device->destroy_resource_view(preview_srv[i]);
        }

        if (preview_rtv[i] != 0)
        {
            device->destroy_resource_view(preview_rtv[i]);
        }

        if (preview_res[i] != 0)
        {
            device->destroy_resource(preview_res[i]);
        }

        preview_res[i] = resource{ 0 };
        preview_srv[i] = resource_view{ 0 };
        preview_rtv[i] = resource_view{ 0 };
    }
}

void ResourceManager::OnEffectsReloading(effect_runtime* runtime)
{
    effects_reloading = true;
}

void ResourceManager::OnEffectsReloaded(effect_runtime* runtime)
{
    effects_reloading = false;
}

void ResourceManager::CheckResourceViews(reshade::api::effect_runtime* runtime)
{
    if (!effects_reloading)
    {
        std::unique_lock<shared_mutex> lock_view(view_mutex);
        for (auto view = global_resources.begin(); view != global_resources.end();)
        {
            bool referenced = view->second.use_count() > 1;

            // valid but not used or just invalid, dispose
            if (view->second->state != GlobalResourceState::RESOURCE_USED && !referenced)
            {
                view->second->Dispose();
                view = global_resources.erase(view);
                continue;
            }

            if (view->second->state == GlobalResourceState::RESOURCE_USED)
            {
                view->second->state = GlobalResourceState::RESOURCE_VALID;
            }

            view++;
        }
    }
}

void ResourceManager::CheckPreview(reshade::api::command_list* cmd_list, reshade::api::device* device)
{
    if (device == nullptr)
    {
        return;
    }

    DeviceDataContainer& deviceData = device->get_private_data<DeviceDataContainer>();

    if (deviceData.huntPreview.recreate_preview)
    {
        DisposePreview(device);
        resource_desc desc = deviceData.huntPreview.target_desc;
        resource_desc preview_desc[2] = {
            resource_desc(desc.texture.width, desc.texture.height, 1, 1, format_to_typeless(desc.texture.format), 1, memory_heap::gpu_only, resource_usage::copy_dest | resource_usage::copy_source | resource_usage::shader_resource | resource_usage::render_target),
            resource_desc(desc.texture.width, desc.texture.height, 1, 1, format_to_typeless(desc.texture.format), 1, memory_heap::gpu_only, resource_usage::copy_dest | resource_usage::shader_resource | resource_usage::render_target)
        };

        for (uint32_t i = 0; i < 2; i++)
        {
            if (!device->create_resource(preview_desc[i], nullptr, resource_usage::shader_resource, &preview_res[i]))
            {
                reshade::log_message(reshade::log_level::error, "Failed to create preview render target!");
            }

            if (preview_res[i] != 0 && !device->create_resource_view(preview_res[i], resource_usage::shader_resource, resource_view_desc(format_to_default_typed(deviceData.huntPreview.view_format, 0)), &preview_srv[i]))
            {
                reshade::log_message(reshade::log_level::error, "Failed to create preview shader resource view!");
            }

            if (preview_res[i] != 0 && !device->create_resource_view(preview_res[i], resource_usage::render_target, resource_view_desc(format_to_default_typed(deviceData.huntPreview.view_format, 0)), &preview_rtv[i]))
            {
                reshade::log_message(reshade::log_level::error, "Failed to create preview render target view!");
            }
        }
    }
}

void ResourceManager::SetPingPreviewHandles(reshade::api::resource* res, reshade::api::resource_view* rtv, reshade::api::resource_view* srv)
{
    if (preview_res[0] != 0)
    {
        if(res != nullptr)
            *res = preview_res[0];
        if(rtv != nullptr)
            *rtv = preview_rtv[0];
        if(srv != nullptr)
            *srv = preview_srv[0];
    }
}

void ResourceManager::SetPongPreviewHandles(reshade::api::resource* res, reshade::api::resource_view* rtv, reshade::api::resource_view* srv)
{
    if (preview_res[1] != 0)
    {
        if (res != nullptr)
            *res = preview_res[1];
        if (rtv != nullptr)
            *rtv = preview_rtv[1];
        if (srv != nullptr)
            *srv = preview_srv[1];
    }
}

bool ResourceManager::IsCompatibleWithPreviewFormat(reshade::api::effect_runtime* runtime, reshade::api::resource res, reshade::api::format view_format)
{
    if (preview_res[0] == 0 || res == 0)
        return false;

    resource_desc res_desc = runtime->get_device()->get_resource_desc(res);
    resource_desc preview_desc = runtime->get_device()->get_resource_desc(preview_res[0]);
    resource_view_desc preview_view_desc = runtime->get_device()->get_resource_view_desc(preview_srv[0]);

    if ((view_format == preview_view_desc.format) &&
        res_desc.texture.width == preview_desc.texture.width &&
        res_desc.texture.height == preview_desc.texture.height)
    {
        return true;
    }

    return false;
}

EmbeddedResourceData ResourceManager::GetResourceData(uint16_t id) {
    HMODULE hModule = NULL;
    GetModuleHandleEx(
        GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS,
        (LPCTSTR)GetResourceData,
        &hModule);

    HRSRC myResource = ::FindResource(hModule, MAKEINTRESOURCE(id), RT_RCDATA);

    if (myResource != 0)
    {
        DWORD myResourceSize = SizeofResource(hModule, myResource);
        HGLOBAL myResourceData = LoadResource(hModule, myResource);

        if (myResourceData != 0)
        {
            const char* pMyBinaryData = static_cast<const char*>(LockResource(myResourceData));
            return EmbeddedResourceData{ pMyBinaryData, myResourceSize };
        }
    }

    return EmbeddedResourceData{ nullptr, 0 };
}