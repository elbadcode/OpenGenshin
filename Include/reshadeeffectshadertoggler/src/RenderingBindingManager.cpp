#include "RenderingBindingManager.h"
#include "Util.h"

using namespace Rendering;
using namespace ShaderToggler;
using namespace reshade::api;
using namespace std;

RenderingBindingManager::RenderingBindingManager(AddonImGui::AddonUIData& data, ResourceManager& rManager, ToggleGroupResourceManager& tgResources) : uiData(data), resourceManager(rManager), toggleGroupResources(tgResources)
{
}

RenderingBindingManager::~RenderingBindingManager()
{

}

void RenderingBindingManager::InitTextureBingings(effect_runtime* runtime)
{
    if (runtime == nullptr || runtime->get_device() == nullptr)
        return;

    DeviceDataContainer& data = runtime->get_device()->get_private_data<DeviceDataContainer>();

    // Init empty texture
    CreateTextureBinding(runtime, &empty_res, &empty_srv, &empty_rtv, reshade::api::format::r8g8b8a8_unorm);
}

void RenderingBindingManager::DisposeTextureBindings(device* device)
{
    DeviceDataContainer& data = device->get_private_data<DeviceDataContainer>();

    unique_lock<shared_mutex> lock(data.binding_mutex);

    if (empty_res != 0)
    {
        device->destroy_resource(empty_res);
        empty_res = { 0 };
    }

    if (empty_rtv != 0)
    {
        device->destroy_resource_view(empty_rtv);
        empty_rtv = { 0 };
    }

    if (empty_srv != 0)
    {
        device->destroy_resource_view(empty_srv);
        empty_srv = { 0 };
    }
}

bool RenderingBindingManager::_CreateTextureBinding(reshade::api::effect_runtime* runtime,
    reshade::api::resource* res,
    reshade::api::resource_view* srv,
    reshade::api::resource_view* rtv,
    reshade::api::format format,
    uint32_t width,
    uint32_t height,
    uint16_t levels)
{
    runtime->get_command_queue()->wait_idle();

    reshade::api::resource_usage res_usage = resource_usage::copy_dest | resource_usage::shader_resource | resource_usage::render_target;

    if (*res == 0 && !runtime->get_device()->create_resource(
        resource_desc(width, height, 1, levels, format, 1, memory_heap::gpu_only, res_usage),
        nullptr, resource_usage::shader_resource, res))
    {
        reshade::log_message(reshade::log_level::error, "Failed to create texture binding resource!");
        return false;
    }

    if (*srv == 0 && !runtime->get_device()->create_resource_view(*res, resource_usage::shader_resource, resource_view_desc(format_to_default_typed(format, 0)), srv))
    {
        reshade::log_message(reshade::log_level::error, "Failed to create texture binding resource view!");
        return false;
    }

    if (*rtv == 0 && !runtime->get_device()->create_resource_view(*res, resource_usage::render_target, resource_view_desc(format_to_default_typed(format, 0)), rtv))
    {
        reshade::log_message(reshade::log_level::error, "Failed to create texture binding resource view!");
        return false;
    }

    return true;
}

bool RenderingBindingManager::CreateTextureBinding(effect_runtime* runtime, resource* res, resource_view* srv, resource_view* rtv, const resource_desc& desc)
{
    return _CreateTextureBinding(runtime, res, srv, rtv, desc.texture.format, desc.texture.width, desc.texture.height, desc.texture.levels);
}

bool RenderingBindingManager::CreateTextureBinding(effect_runtime* runtime, resource* res, resource_view* srv, resource_view* rtv, reshade::api::format format)
{
    uint32_t frame_width, frame_height;
    runtime->get_screenshot_width_and_height(&frame_width, &frame_height);

    return _CreateTextureBinding(runtime, res, srv, rtv, format, frame_width, frame_height, 1);
}

uint32_t RenderingBindingManager::UpdateTextureBinding(effect_runtime* runtime, ToggleGroup* group, resource res, const resource_desc& desc, reshade::api::format viewformat)
{
    DeviceDataContainer& data = runtime->get_device()->get_private_data<DeviceDataContainer>();
    GroupResource& groupResource = group->GetGroupResource(ShaderToggler::GroupResourceType::RESOURCE_BINDING);

    // Switch from game's buffer to internal copy
    if (!groupResource.owning)
    {
        groupResource.target_description = desc;
        groupResource.view_format = viewformat;
        groupResource.g_res = { 0 };
        groupResource.owning = true;
    }

    // Copy format changed, recreate internal buffer
    if (!toggleGroupResources.IsCompatibleWithGroupFormat(runtime->get_device(), ShaderToggler::GroupResourceType::RESOURCE_BINDING, res, group))
    {
        groupResource.target_description = desc;
        groupResource.view_format = viewformat;
        groupResource.state = ShaderToggler::GroupResourceState::RESOURCE_INVALID;
        runtime->update_texture_bindings(group->getTextureBindingName().c_str(), empty_srv, empty_srv);

        return 0;
    }

    if (groupResource.state == ShaderToggler::GroupResourceState::RESOURCE_RECREATED || groupResource.state == ShaderToggler::GroupResourceState::RESOURCE_CLEARED)
    {
        runtime->update_texture_bindings(group->getTextureBindingName().c_str(), groupResource.srv, groupResource.srv);
        groupResource.state = ShaderToggler::GroupResourceState::RESOURCE_VALID;
    }

    return 1;
}

void RenderingBindingManager::_QueueOrDequeue(
    command_list* cmd_list,
    DeviceDataContainer& deviceData,
    CommandListDataContainer& commandListData,
    binding_queue& queue,
    unordered_set<ToggleGroup*>& immediateQueue,
    uint64_t callLocation,
    uint32_t layoutIndex,
    uint64_t action)
{
    for (auto it = queue.begin(); it != queue.end();)
    {
        auto& [group, data] = *it;
        // Set views during draw call since we can be sure the correct ones are bound at that point
        if (!callLocation && data.resource == 0)
        {
            ResourceViewData active_data = RenderingManager::GetCurrentResourceView(cmd_list, deviceData, group, commandListData, layoutIndex, action);

            if (active_data.resource != 0)
            {
                data.resource = active_data.resource;
                data.format = active_data.format;
            }
            else if (group->getRequeueAfterRTMatchingFailure())
            {
                // Leave loaded up in the effect/bind list and re-issue command on RT change
                it++;
                continue;
            }
            else
            {
                it = queue.erase(it);
                continue;
            }
        }

        // Queue updates depending on the place their supposed to be called at
        if (data.resource != 0 && (!callLocation && !data.invocationLocation || callLocation & data.invocationLocation))
        {
            immediateQueue.insert(group);
        }

        it++;
    }
}

void RenderingBindingManager::_UpdateTextureBindings(command_list* cmd_list,
    DeviceDataContainer& deviceData,
    const binding_queue& bindingsToUpdate,
    vector<ToggleGroup*>& removalList,
    const unordered_set<ToggleGroup*>& toUpdateBindings)
{
    effect_runtime* runtime = deviceData.current_runtime;

    if (runtime == nullptr)
        return;

    auto& runtimeData = runtime->get_private_data<RuntimeDataContainer>();

    for (auto& [group, bindingData] : bindingsToUpdate)
    {
        if (toUpdateBindings.contains(group) && !deviceData.bindingsUpdated.contains(group))
        {
            if (bindingData.resource == 0)
            {
                continue;
            }

            GroupResource& bindingResource = group->GetGroupResource(ShaderToggler::GroupResourceType::RESOURCE_BINDING);

            if (!group->getCopyTextureBinding())
            {
                const std::shared_ptr<GlobalResourceView>& view = resourceManager.GetResourceView(runtime->get_device(), bindingData);

                if (view == nullptr || view->srv == 0)
                {
                    return;
                }

                resource_desc resDesc = runtime->get_device()->get_resource_desc(bindingData.resource);

                resource target_res = bindingResource.g_res == nullptr ? resource{ 0 } : resource{ bindingResource.g_res->resource_handle };

                if (target_res != bindingData.resource || bindingResource.state == ShaderToggler::GroupResourceState::RESOURCE_CLEARED)
                {
                    runtime->update_texture_bindings(group->getTextureBindingName().c_str(), view->srv, view->srv_srgb);

                    bindingResource.g_res = view;
                    bindingResource.view_format = bindingData.format;
                    bindingResource.target_description = resDesc;
                    bindingResource.owning = false;
                    bindingResource.state = ShaderToggler::GroupResourceState::RESOURCE_VALID;
                }
            }
            else
            {
                resource_desc resDesc = runtime->get_device()->get_resource_desc(bindingData.resource);

                uint32_t retUpdate = UpdateTextureBinding(runtime, group, bindingData.resource, resDesc, bindingData.format);

                resource target_res = bindingResource.res;

                if (retUpdate && target_res != 0)
                {
                    cmd_list->copy_resource(bindingData.resource, target_res);

                    if (group->getFlipBufferBinding() && bindingResource.rtv != 0 && runtimeData.specialEffects[REST_FLIP].technique != 0)
                    {
                        deviceData.current_runtime->render_technique(runtimeData.specialEffects[REST_FLIP].technique, cmd_list, bindingResource.rtv, bindingResource.rtv_srgb);
                    }
                }
            }

            deviceData.bindingsUpdated.emplace(group);
            removalList.push_back(group);
        }
    }
}

void RenderingBindingManager::UpdateTextureBindings(command_list* cmd_list, uint64_t callLocation, uint64_t invocation)
{
    if (cmd_list == nullptr || cmd_list->get_device() == nullptr)
    {
        return;
    }

    device* device = cmd_list->get_device();
    CommandListDataContainer& commandListData = cmd_list->get_private_data<CommandListDataContainer>();
    DeviceDataContainer& deviceData = device->get_private_data<DeviceDataContainer>();

    // Remove call location from queue
    commandListData.commandQueue &= ~(invocation << (callLocation * MATCH_DELIMITER));

    unique_lock<shared_mutex> mtx(deviceData.binding_mutex);

    if (deviceData.current_runtime == nullptr || (commandListData.ps.bindingsToUpdate.size() == 0 && commandListData.vs.bindingsToUpdate.size() == 0 && commandListData.cs.bindingsToUpdate.size() == 0)) {
        return;
    }

    unordered_set<ToggleGroup*> psToUpdateBindings;
    unordered_set<ToggleGroup*> vsToUpdateBindings;
    unordered_set<ToggleGroup*> csToUpdateBindings;

    if (invocation & MATCH_BINDING_PS)
    {
        _QueueOrDequeue(cmd_list, deviceData, commandListData, commandListData.ps.bindingsToUpdate, psToUpdateBindings, callLocation, 0, MATCH_BINDING_PS);
    }

    if (invocation & MATCH_BINDING_VS)
    {
        _QueueOrDequeue(cmd_list, deviceData, commandListData, commandListData.vs.bindingsToUpdate, vsToUpdateBindings, callLocation, 1, MATCH_BINDING_VS);
    }

    if (invocation & MATCH_BINDING_CS)
    {
        _QueueOrDequeue(cmd_list, deviceData, commandListData, commandListData.cs.bindingsToUpdate, csToUpdateBindings, callLocation, 2, MATCH_BINDING_CS);
    }

    if (psToUpdateBindings.size() == 0 && vsToUpdateBindings.size() == 0 && csToUpdateBindings.size() == 0)
    {
        return;
    }

    vector<ToggleGroup*> psRemovalList;
    vector<ToggleGroup*> vsRemovalList;
    vector<ToggleGroup*> csRemovalList;

    if (psToUpdateBindings.size() > 0)
    {
        _UpdateTextureBindings(cmd_list, deviceData, commandListData.ps.bindingsToUpdate, psRemovalList, psToUpdateBindings);
    }
    if (vsToUpdateBindings.size() > 0)
    {
        _UpdateTextureBindings(cmd_list, deviceData, commandListData.vs.bindingsToUpdate, vsRemovalList, vsToUpdateBindings);
    }
    if (csToUpdateBindings.size() > 0)
    {
        _UpdateTextureBindings(cmd_list, deviceData, commandListData.cs.bindingsToUpdate, csRemovalList, csToUpdateBindings);
    }
    mtx.unlock();

    for (auto& g : psRemovalList)
    {
        commandListData.ps.bindingsToUpdate.erase(g);
    }

    for (auto& g : vsRemovalList)
    {
        commandListData.vs.bindingsToUpdate.erase(g);
    }

    for (auto& g : csRemovalList)
    {
        commandListData.cs.bindingsToUpdate.erase(g);
    }

}

void RenderingBindingManager::ClearUnmatchedTextureBindings(reshade::api::command_list* cmd_list)
{
    DeviceDataContainer& data = cmd_list->get_device()->get_private_data<DeviceDataContainer>();

    shared_lock<shared_mutex> mtx(data.binding_mutex);

    static const float clearColor[4] = { 0.0f, 0.0f, 0.0f, 0.0f };

    for (auto& groupData : uiData.GetToggleGroups())
    {
        ToggleGroup& group = groupData.second;
        GroupResource& resources = group.GetGroupResource(ShaderToggler::GroupResourceType::RESOURCE_BINDING);

        if (!data.bindingsUpdated.contains(&group) && (resources.clear_on_miss() && empty_srv != 0 && resources.state != ShaderToggler::GroupResourceState::RESOURCE_CLEARED))
        {
            data.current_runtime->update_texture_bindings(group.getTextureBindingName().c_str(), empty_srv, empty_srv);
            resources.state = ShaderToggler::GroupResourceState::RESOURCE_CLEARED;

            if (!resources.owning)
            {
                resources.g_res = { 0 };
            }
        }
    }

    if (!data.huntPreview.matched && uiData.GetToggleGroupIdShaderEditing() >= 0)
    {
        resource_view rtv_ping = resource_view{ 0 };
        resource_view rtv_pong = resource_view{ 0 };

        resourceManager.SetPingPreviewHandles(nullptr, &rtv_ping, nullptr);
        resourceManager.SetPongPreviewHandles(nullptr, &rtv_pong, nullptr);

        if (rtv_ping != 0)
        {
            cmd_list->clear_render_target_view(rtv_ping, clearColor);
        }
        if (rtv_pong != 0)
        {
            cmd_list->clear_render_target_view(rtv_pong, clearColor);
        }
    }
}