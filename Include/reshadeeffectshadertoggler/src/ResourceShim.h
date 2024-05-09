#pragma once

#include <reshade_api.hpp>
#include <reshade_api_device.hpp>
#include <reshade_api_pipeline.hpp>
#include <unordered_map>

namespace Shim {
    namespace Resources
    {
        class ResourceShim {
        public:
            virtual bool Init() = 0;
            virtual bool UnInit() = 0;

            virtual bool OnCreateResource(reshade::api::device* device, reshade::api::resource_desc& desc, reshade::api::subresource_data* initial_data, reshade::api::resource_usage initial_state) = 0;
            virtual void OnDestroyResource(reshade::api::device* device, reshade::api::resource res) = 0;
            virtual void OnInitResource(reshade::api::device* device, const reshade::api::resource_desc& desc, const reshade::api::subresource_data* initData, reshade::api::resource_usage usage, reshade::api::resource handle) = 0;
            virtual bool OnCreateResourceView(reshade::api::device* device, reshade::api::resource resource, reshade::api::resource_usage usage_type, reshade::api::resource_view_desc& desc) = 0;
        };
    }
}
