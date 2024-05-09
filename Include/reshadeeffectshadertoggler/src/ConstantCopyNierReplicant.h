/*
    Because I'm playing it right now.
*/

#pragma once

#include <reshade_api.hpp>
#include <reshade_api_device.hpp>
#include <reshade_api_pipeline.hpp>
#include <unordered_map>
#include <vector>
#include <shared_mutex>
#include "ConstantCopyBase.h"
#include "GameHookT.h"

using namespace sigmatch_literals;

static const sigmatch::signature nier_replicant_cbload = "48 89 5C 24 ?? 48 89 74 24 ?? 57 48 83 EC 40 80 B9 ?? ?? ?? ?? 00 48 8B F2 41 8B F8"_sig;

namespace Shim 
{
    namespace Constants
    {
        class ConstantCopyNierReplicant final : public virtual ConstantCopyBase {
        public:
            ConstantCopyNierReplicant();
            ~ConstantCopyNierReplicant();

            bool Init() override final;
            bool UnInit() override final;

            void OnUpdateBufferRegion(reshade::api::device* device, const void* data, reshade::api::resource resource, uint64_t offset, uint64_t size) override final {};
            void OnMapBufferRegion(reshade::api::device* device, reshade::api::resource resource, uint64_t offset, uint64_t size, reshade::api::map_access access, void** data) override final;
            void OnUnmapBufferRegion(reshade::api::device* device, reshade::api::resource resource) override final;
        private:
            static sig_nier_replicant_cbload* org_nier_replicant_cbload;
            static void __fastcall detour_nier_replicant_cbload(intptr_t p1, intptr_t* p2, uintptr_t p3);

            static void* Origin;
            static size_t Size;
        };
    }
}