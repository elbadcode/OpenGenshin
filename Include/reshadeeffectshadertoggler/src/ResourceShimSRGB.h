#pragma once

#include <reshade_api.hpp>
#include <reshade_api_device.hpp>
#include <reshade_api_pipeline.hpp>
#include <unordered_map>
#include <shared_mutex>

#include "ResourceShim.h"

namespace Shim
{
    namespace Resources
    {
        class ResourceShimSRGB final : public virtual ResourceShim {
        public:
            virtual bool Init() override final;
            virtual bool UnInit() override final;

            virtual bool OnCreateResource(reshade::api::device* device, reshade::api::resource_desc& desc, reshade::api::subresource_data* initial_data, reshade::api::resource_usage initial_state) override final;
            virtual void OnDestroyResource(reshade::api::device* device, reshade::api::resource res) override final;
            virtual void OnInitResource(reshade::api::device* device, const reshade::api::resource_desc& desc, const reshade::api::subresource_data* initData, reshade::api::resource_usage usage, reshade::api::resource handle) override final;
            virtual bool OnCreateResourceView(reshade::api::device* device, reshade::api::resource resource, reshade::api::resource_usage usage_type, reshade::api::resource_view_desc& desc) override final;
        private:
            bool _IsSRGB(reshade::api::format value);
            bool _HasSRGB(reshade::api::format value);

            std::unordered_map<const reshade::api::resource_desc*, reshade::api::format> s_resourceFormatTransient;
            std::unordered_map<uint64_t, reshade::api::format> s_resourceFormat;
            std::shared_mutex resource_mutex;
        };
    }
}
