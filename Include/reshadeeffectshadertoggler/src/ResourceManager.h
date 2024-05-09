#pragma once

#include <reshade.hpp>
#include <vector>
#include <unordered_map>
#include <shared_mutex>
#include <functional>
#include "PipelinePrivateData.h"
#include "ResourceShim.h"
#include "ResourceShimSRGB.h"
#include "ResourceShimFFXIV.h"
#include "GlobalResourceView.h"

namespace Rendering
{
    enum ResourceShimType
    {
        Resource_Shim_None = 0,
        Resource_Shim_SRGB,
        Resource_Shim_FFXIV
    };

    static const std::vector<std::string> ResourceShimNames = {
        "none",
        "srgb",
        "ffxiv"
    };

    struct EmbeddedResourceData
    {
        const void* data;
        size_t size;
    };

    class __declspec(novtable) ResourceManager final
    {
    public:
        void InitBackbuffer(reshade::api::swapchain* runtime);
        void ClearBackbuffer(reshade::api::swapchain* runtime);

        bool OnCreateResource(reshade::api::device* device, reshade::api::resource_desc& desc, reshade::api::subresource_data* initial_data, reshade::api::resource_usage initial_state);
        void OnInitResource(reshade::api::device* device, const reshade::api::resource_desc& desc, const reshade::api::subresource_data* initData, reshade::api::resource_usage usage, reshade::api::resource handle);
        void OnDestroyResource(reshade::api::device* device, reshade::api::resource res);
        bool OnCreateResourceView(reshade::api::device* device, reshade::api::resource resource, reshade::api::resource_usage usage_type, reshade::api::resource_view_desc& desc);
        void OnInitResourceView(reshade::api::device* device, reshade::api::resource resource, reshade::api::resource_usage usage_type, const reshade::api::resource_view_desc& desc, reshade::api::resource_view view);
        void OnDestroyResourceView(reshade::api::device* device, reshade::api::resource_view view);
        bool OnCreateSwapchain(reshade::api::swapchain_desc& desc, void* hwnd);
        void OnInitSwapchain(reshade::api::swapchain* swapchain);
        void OnDestroySwapchain(reshade::api::swapchain* swapchain);
        void OnDestroyDevice(reshade::api::device*);

        void SetResourceShim(const std::string& shim) { _shimType = ResolveResourceShimType(shim); }
        void Init();

        void DisposePreview(reshade::api::device* runtime);
        void CheckPreview(reshade::api::command_list* cmd_list, reshade::api::device* device);
        void SetPingPreviewHandles(reshade::api::resource* res, reshade::api::resource_view* rtv, reshade::api::resource_view* srv);
        void SetPongPreviewHandles(reshade::api::resource* res, reshade::api::resource_view* rtv, reshade::api::resource_view* srv);
        bool IsCompatibleWithPreviewFormat(reshade::api::effect_runtime* runtime, reshade::api::resource res, reshade::api::format view_format);

        void OnEffectsReloading(reshade::api::effect_runtime* runtime);
        void OnEffectsReloaded(reshade::api::effect_runtime* runtime);

        const std::shared_ptr<GlobalResourceView>& GetResourceView(reshade::api::device* device, const ResourceRenderData& data);
        const std::shared_ptr<GlobalResourceView>& GetResourceView(reshade::api::device* device, uint64_t handle, reshade::api::format format = reshade::api::format::unknown);
        void CheckResourceViews(reshade::api::effect_runtime* runtime);

        static EmbeddedResourceData GetResourceData(uint16_t id);

        reshade::api::resource dummy_res;
        reshade::api::resource_view dummy_rtv;
    private:
        static ResourceShimType ResolveResourceShimType(const std::string&);

        ResourceShimType _shimType = ResourceShimType::Resource_Shim_None;
        Shim::Resources::ResourceShim* rShim = nullptr;
        bool in_destroy_device = false;

        bool effects_reloading = false;

        std::shared_mutex resource_mutex;
        std::shared_mutex view_mutex;

        reshade::api::resource preview_res[2];
        reshade::api::resource_view preview_rtv[2];
        reshade::api::resource_view preview_srv[2];

        std::unordered_map<uint64_t, std::shared_ptr<GlobalResourceView>> global_resources;
        std::unordered_set<uint64_t> resources;
    };
}