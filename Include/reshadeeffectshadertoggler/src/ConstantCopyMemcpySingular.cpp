#include "ConstantCopyMemcpySingular.h"

using namespace Shim::Constants;
using namespace reshade::api;
using namespace std;

BufferCopy ConstantCopyMemcpySingular::_bufferCopy;

ConstantCopyMemcpySingular::ConstantCopyMemcpySingular()
{
    _instance = this;
}

ConstantCopyMemcpySingular::~ConstantCopyMemcpySingular() {}

void ConstantCopyMemcpySingular::OnMapBufferRegion(device* device, resource resource, uint64_t offset, uint64_t size, map_access access, void** data)
{
    if (access == map_access::write_discard || access == map_access::write_only)
    {
        resource_desc desc = device->get_resource_desc(resource);
        const auto& it = deviceToHostConstantBuffer.find(resource.handle);

        if (it != deviceToHostConstantBuffer.end())
        {
            auto& [_, buf] = *it;
            _bufferCopy.resource = resource.handle;
            _bufferCopy.destination = *data;
            _bufferCopy.size = size;
            _bufferCopy.offset = offset;
            _bufferCopy.bufferSize = desc.buffer.size;
            _bufferCopy.hostDestination = buf.data();
        }
    }
}

void ConstantCopyMemcpySingular::OnUnmapBufferRegion(device* device, resource resource)
{
    _bufferCopy.resource = 0;
    _bufferCopy.destination = nullptr;
}

void ConstantCopyMemcpySingular::OnMemcpy(void* dest, void* src, size_t size)
{
    uintptr_t destPtr = reinterpret_cast<uintptr_t>(dest);
    uintptr_t destinationPtr = reinterpret_cast<uintptr_t>(_bufferCopy.destination);

    if (_bufferCopy.resource != 0 &&
        destPtr >= destinationPtr &&
        destPtr <= destinationPtr + _bufferCopy.bufferSize - _bufferCopy.offset)
    {
        memcpy(_bufferCopy.hostDestination, src, size);
    }
}