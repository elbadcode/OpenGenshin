#pragma once
#include "ConstantCopyMemcpy.h"

namespace Shim
{
    namespace Constants
    {
        class ConstantCopyMemcpyNested final : public virtual ConstantCopyMemcpy {
        public:
            ConstantCopyMemcpyNested();
            ~ConstantCopyMemcpyNested();

            void OnMemcpy(void* dest, void* src, size_t size) override final;
            void OnMapBufferRegion(reshade::api::device * device, reshade::api::resource resource, uint64_t offset, uint64_t size, reshade::api::map_access access, void** data) override final;
            void OnUnmapBufferRegion(reshade::api::device* device, reshade::api::resource resource) override final;
        private:
            std::unordered_map<uint64_t, BufferCopy> _resourceMemoryMapping;
            std::shared_mutex _map_mutex;
        };
    }
}