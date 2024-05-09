/*
 * Copyright (C) 2021 Patrick Mours
 * SPDX-License-Identifier: BSD-3-Clause OR MIT
 */

#include <algorithm>
#include "reshade.hpp"
#include "DescriptorTracking.h"

using namespace reshade::api;

sampler descriptor_tracking::get_sampler(descriptor_heap heap, uint32_t offset) const
{
    const descriptor_heap_data& heap_data = heaps.at(heap);

    if (offset < heap_data.descriptors.size())
    {
        const descriptor_data& descriptor = heap_data.descriptors[offset];

        if (descriptor.type == descriptor_type::sampler)
            return descriptor.sampler;
        else if (descriptor.type == descriptor_type::sampler_with_resource_view)
            return descriptor.sampler_and_view.sampler;
    }

    return { 0 };
}
resource_view descriptor_tracking::get_shader_resource_view(descriptor_heap heap, uint32_t offset) const
{
    const descriptor_heap_data& heap_data = heaps.at(heap);

    if (offset < heap_data.descriptors.size())
    {
        const descriptor_data& descriptor = heap_data.descriptors[offset];

        if (descriptor.type == descriptor_type::shader_resource_view)
            return descriptor.view;
        else if (descriptor.type == descriptor_type::sampler_with_resource_view)
            return descriptor.sampler_and_view.view;
    }

    return { 0 };
}
buffer_range descriptor_tracking::get_buffer_range(descriptor_heap heap, uint32_t offset) const
{
    const descriptor_heap_data& heap_data = heaps.at(heap);

    if (offset < heap_data.descriptors.size())
    {
        const descriptor_data& descriptor = heap_data.descriptors[offset];

        if (descriptor.type == descriptor_type::constant_buffer)
            return descriptor.constant;
    }

    return { 0 };
}

void descriptor_tracking::set_all_descriptors(reshade::api::descriptor_heap heap, uint32_t offset, uint32_t count, std::vector<descriptor_tracking::descriptor_data>& descriptor_list, uint32_t list_offset) const
{
    const descriptor_heap_data& heap_data = heaps.at(heap);

    size_t mcount = std::min(static_cast<size_t>(offset) + count, heap_data.descriptors.size());

    for (uint32_t i = offset; i < mcount; i++)
    {
        descriptor_list[list_offset + i - offset] = heap_data.descriptors[i];
    }
}

pipeline_layout_param descriptor_tracking::get_pipeline_layout_param(pipeline_layout layout, uint32_t param) const
{
    const pipeline_layout_data& layout_data = layouts.at(layout);

    return layout_data.params[param];
}

void descriptor_tracking::register_pipeline_layout(pipeline_layout layout, uint32_t count, const pipeline_layout_param* params)
{
    pipeline_layout_data& layout_data = layouts[layout];
    layout_data.params.assign(params, params + count);
    layout_data.ranges.resize(count);

    for (uint32_t i = 0; i < count; ++i)
    {
        if (params[i].type == pipeline_layout_param_type::descriptor_table)
        {
            layout_data.ranges[i].assign(params[i].descriptor_table.ranges, params[i].descriptor_table.ranges + params[i].descriptor_table.count);
            layout_data.params[i].descriptor_table.ranges = layout_data.ranges[i].data();
        }
    }
}
void descriptor_tracking::unregister_pipeline_layout(pipeline_layout layout)
{
    pipeline_layout_data& layout_data = layouts[layout];
    layout_data.params.clear();
    layout_data.ranges.clear();
}

static void on_init_device(device* device)
{
    device->create_private_data<descriptor_tracking>();
}
static void on_destroy_device(device* device)
{
    device->destroy_private_data<descriptor_tracking>();
}

void descriptor_tracking::on_init_pipeline_layout(device* device, uint32_t count, const pipeline_layout_param* params, pipeline_layout layout)
{
    descriptor_tracking& ctx = device->get_private_data<descriptor_tracking>();
    ctx.register_pipeline_layout(layout, count, params);
}
void descriptor_tracking::on_destroy_pipeline_layout(device* device, pipeline_layout layout)
{
    descriptor_tracking& ctx = device->get_private_data<descriptor_tracking>();
    ctx.unregister_pipeline_layout(layout);
}

bool descriptor_tracking::on_copy_descriptor_tables(device* device, uint32_t count, const descriptor_table_copy* copies)
{
    descriptor_tracking& ctx = device->get_private_data<descriptor_tracking>();

    for (uint32_t i = 0; i < count; ++i)
    {
        const descriptor_table_copy& copy = copies[i];

        uint32_t src_offset;
        descriptor_heap src_heap;
        device->get_descriptor_heap_offset(copy.source_table, copy.source_binding, copy.source_array_offset, &src_heap, &src_offset);

        uint32_t dst_offset;
        descriptor_heap dst_heap;
        device->get_descriptor_heap_offset(copy.dest_table, copy.dest_binding, copy.dest_array_offset, &dst_heap, &dst_offset);

        descriptor_heap_data& src_pool_data = ctx.heaps[src_heap];
        descriptor_heap_data& dst_pool_data = ctx.heaps[dst_heap];

        if (dst_offset + copy.count > dst_pool_data.descriptors.size())
            dst_pool_data.descriptors.grow_to_at_least(dst_offset + copy.count);

        for (uint32_t k = 0; k < copy.count; ++k)
        {
            dst_pool_data.descriptors[dst_offset + k] = src_pool_data.descriptors[src_offset + k];
        }
    }

    return false;
}

bool descriptor_tracking::on_update_descriptor_tables(device* device, uint32_t count, const descriptor_table_update* updates)
{
    descriptor_tracking& ctx = device->get_private_data<descriptor_tracking>();

    for (uint32_t i = 0; i < count; ++i)
    {
        const descriptor_table_update& update = updates[i];

        uint32_t offset;
        descriptor_heap heap;
        device->get_descriptor_heap_offset(update.table, update.binding, update.array_offset, &heap, &offset);

        descriptor_heap_data& heap_data = ctx.heaps[heap];

        if (offset + update.count > heap_data.descriptors.size())
            heap_data.descriptors.grow_to_at_least(offset + update.count);

        for (uint32_t k = 0; k < update.count; ++k)
        {
            descriptor_data& descriptor = heap_data.descriptors[offset + k];

            descriptor.type = update.type;

            switch (update.type)
            {
            case descriptor_type::sampler:
                descriptor.sampler = static_cast<const sampler*>(update.descriptors)[k];
                break;
            case descriptor_type::sampler_with_resource_view:
                descriptor.sampler_and_view = static_cast<const sampler_with_resource_view*>(update.descriptors)[k];
                descriptor.view = descriptor.sampler_and_view.view;
                descriptor.sampler = descriptor.sampler_and_view.sampler;
                break;
            case descriptor_type::shader_resource_view:
            case descriptor_type::unordered_access_view:
                descriptor.view = static_cast<const resource_view*>(update.descriptors)[k];
                break;
            case descriptor_type::constant_buffer:
            case descriptor_type::shader_storage_buffer:
                descriptor.constant = static_cast<const buffer_range*>(update.descriptors)[k];
            }
        }
    }

    return false;
}

void descriptor_tracking::register_events(bool track_descriptors)
{
    reshade::register_event<reshade::addon_event::init_device>(on_init_device);
    reshade::register_event<reshade::addon_event::destroy_device>(on_destroy_device);
    reshade::register_event<reshade::addon_event::init_pipeline_layout>(on_init_pipeline_layout);
    reshade::register_event<reshade::addon_event::destroy_pipeline_layout>(on_destroy_pipeline_layout);

    if (track_descriptors)
    {
        reshade::register_event<reshade::addon_event::copy_descriptor_tables>(on_copy_descriptor_tables);
        reshade::register_event<reshade::addon_event::update_descriptor_tables>(on_update_descriptor_tables);
    }
}
void descriptor_tracking::unregister_events(bool track_descriptors)
{
    reshade::unregister_event<reshade::addon_event::init_device>(on_init_device);
    reshade::unregister_event<reshade::addon_event::destroy_device>(on_destroy_device);
    reshade::unregister_event<reshade::addon_event::init_pipeline_layout>(on_init_pipeline_layout);
    reshade::unregister_event<reshade::addon_event::destroy_pipeline_layout>(on_destroy_pipeline_layout);

    if (track_descriptors)
    {
        reshade::unregister_event<reshade::addon_event::copy_descriptor_tables>(on_copy_descriptor_tables);
        reshade::unregister_event<reshade::addon_event::update_descriptor_tables>(on_update_descriptor_tables);
    }
}
