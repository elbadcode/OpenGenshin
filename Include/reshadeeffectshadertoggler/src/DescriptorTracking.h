/*
 * Copyright (C) 2021 Patrick Mours
 * SPDX-License-Identifier: BSD-3-Clause OR MIT
 */

#pragma once

#include <vector>
#include <concurrent_vector.h>
#include <concurrent_unordered_map.h>
#include "reshade.hpp"

 /// <summary>
 /// An instance of this is automatically created for all devices and can be queried with <c>device->get_private_data&lt;descriptor_tracking&gt;()</c> (assuming descriptor tracking was registered via <see cref="descriptor_tracking::register_events"/>).
 /// </summary>
class __declspec(uuid("33319e83-387c-448e-881c-7e68fc2e52c4")) descriptor_tracking
{
public:
    enum class descriptor_data_type : uint32_t
    {
        sampler = 0,
        sampler_with_resource_view = 1,
        shader_resource_view = 2,
        unordered_access_view = 3,
        constant_buffer = 6,
        shader_storage_buffer = 7,
        push_constant = 8
    };

    struct descriptor_data
    {
        reshade::api::descriptor_type type;
        reshade::api::sampler_with_resource_view sampler_and_view;
        reshade::api::sampler sampler;
        reshade::api::resource_view view;
        reshade::api::buffer_range constant;
    };

    /// <summary>
    /// Registers all the necessary add-on events for descriptor tracking to work.
    /// </summary>
    static void register_events(bool track_descriptors = true);
    /// <summary>
    /// Unregisters all the necessary add-on events for descriptor tracking to work.
    /// </summary>
    static void unregister_events(bool track_descriptors = true);

    /// <summary>
    /// Gets the sampler in a descriptor set at the specified offset.
    /// </summary>
    reshade::api::sampler get_sampler(reshade::api::descriptor_heap heap, uint32_t offset) const;
    /// <summary>
    /// Gets the shader resource view in a descriptor set at the specified offset.
    /// </summary>
    reshade::api::resource_view get_shader_resource_view(reshade::api::descriptor_heap heap, uint32_t offset) const;
    /// <summary>
    /// Gets the buffer range in a descriptor set at the specified offset.
    /// </summary>
    reshade::api::buffer_range get_buffer_range(reshade::api::descriptor_heap heap, uint32_t offset) const;

    void set_all_descriptors(reshade::api::descriptor_heap heap, uint32_t offset, uint32_t count, std::vector<descriptor_tracking::descriptor_data>& descriptor_list, uint32_t list_offset) const;

    /// <summary>
    /// Gets the description that was used to create the specified pipeline layout parameter.
    /// </summary>
    reshade::api::pipeline_layout_param get_pipeline_layout_param(reshade::api::pipeline_layout layout, uint32_t param) const;


private:
    void register_pipeline_layout(reshade::api::pipeline_layout layout, uint32_t count, const reshade::api::pipeline_layout_param* params);
    void unregister_pipeline_layout(reshade::api::pipeline_layout layout);

    static void on_init_pipeline_layout(reshade::api::device* device, uint32_t count, const reshade::api::pipeline_layout_param* params, reshade::api::pipeline_layout layout);
    static void on_destroy_pipeline_layout(reshade::api::device* device, reshade::api::pipeline_layout layout);

    static bool on_copy_descriptor_tables(reshade::api::device* device, uint32_t count, const reshade::api::descriptor_table_copy* copies);
    static bool on_update_descriptor_tables(reshade::api::device* device, uint32_t count, const reshade::api::descriptor_table_update* updates);

    struct descriptor_heap_data
    {
        concurrency::concurrent_vector<descriptor_data> descriptors;
    };
    struct descriptor_heap_hash : std::hash<uint64_t>
    {
        size_t operator()(reshade::api::descriptor_heap handle) const
        {
            return std::hash<uint64_t>::operator()(handle.handle);
        }
    };

    struct pipeline_layout_data
    {
        std::vector<reshade::api::pipeline_layout_param> params;
        std::vector<std::vector<reshade::api::descriptor_range>> ranges;
    };
    struct pipeline_layout_hash : std::hash<uint64_t>
    {
        size_t operator()(reshade::api::pipeline_layout handle) const
        {
            return std::hash<uint64_t>::operator()(handle.handle);
        }
    };

    struct pipeline_hash : std::hash<uint64_t>
    {
        size_t operator()(reshade::api::pipeline handle) const
        {
            return std::hash<uint64_t>::operator()(handle.handle);
        }
    };

    concurrency::concurrent_unordered_map<reshade::api::descriptor_heap, descriptor_heap_data, descriptor_heap_hash> heaps;
    concurrency::concurrent_unordered_map<reshade::api::pipeline_layout, pipeline_layout_data, pipeline_layout_hash> layouts;
    concurrency::concurrent_unordered_map<reshade::api::pipeline, reshade::api::pipeline_layout, pipeline_hash> pipelines;
};
