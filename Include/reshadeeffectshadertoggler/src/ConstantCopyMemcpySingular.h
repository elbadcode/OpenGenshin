#pragma once
#include "ConstantCopyMemcpy.h"

namespace Shim
{
    namespace Constants
    {
        class ConstantCopyMemcpySingular final : public virtual ConstantCopyMemcpy {
        public:
            ConstantCopyMemcpySingular();
            ~ConstantCopyMemcpySingular();

            void OnMemcpy(void* dest, void* src, size_t size) override final;
            void OnMapBufferRegion(reshade::api::device* device, reshade::api::resource resource, uint64_t offset, uint64_t size, reshade::api::map_access access, void** data) override final;
            void OnUnmapBufferRegion(reshade::api::device* device, reshade::api::resource resource) override final;
        private:
            static BufferCopy _bufferCopy;
        };
    }
}