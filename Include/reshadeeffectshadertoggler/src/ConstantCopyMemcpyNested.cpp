#include "ConstantCopyMemcpyNested.h"

using namespace Shim::Constants;
using namespace reshade::api;
using namespace std;

ConstantCopyMemcpyNested::ConstantCopyMemcpyNested()
{
    _instance = this;
}

ConstantCopyMemcpyNested::~ConstantCopyMemcpyNested()
{

}

void ConstantCopyMemcpyNested::OnMapBufferRegion(device* device, resource resource, uint64_t offset, uint64_t size, map_access access, void** data)
{
    if (access == map_access::write_discard || access == map_access::write_only)
    {
        resource_desc desc = device->get_resource_desc(resource);
        if (desc.heap == memory_heap::cpu_to_gpu && static_cast<uint32_t>(desc.usage & resource_usage::constant_buffer))
        {
            unique_lock<shared_mutex> lock(_map_mutex);
            _resourceMemoryMapping[resource.handle] = BufferCopy{ resource.handle, *data, nullptr, offset, size, desc.buffer.size };
        }
    }
}

void ConstantCopyMemcpyNested::OnUnmapBufferRegion(device* device, resource resource)
{

    resource_desc desc = device->get_resource_desc(resource);
    if (desc.heap == memory_heap::cpu_to_gpu && static_cast<uint32_t>(desc.usage & resource_usage::constant_buffer))
    {
        unique_lock<shared_mutex> lock(_map_mutex);
        _resourceMemoryMapping.erase(resource.handle);
    }
}

void ConstantCopyMemcpyNested::OnMemcpy(void* volatile dest, void* src, size_t size)
{
    shared_lock<shared_mutex> lock(_map_mutex);
    if (_resourceMemoryMapping.size() > 0)
    {
        for (auto& [_,buffer] : _resourceMemoryMapping)
        {
            if (dest >= buffer.destination && static_cast<uintptr_t>(reinterpret_cast<intptr_t>(dest)) <= reinterpret_cast<intptr_t>(buffer.destination) + buffer.bufferSize - buffer.offset)
            {
                SetHostConstantBuffer(buffer.resource, src, size, reinterpret_cast<intptr_t>(dest) - reinterpret_cast<intptr_t>(buffer.destination), buffer.bufferSize);
                break;
            }
        }
    }
}