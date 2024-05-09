#pragma once

#include "RenderingManager.h"

namespace Rendering
{
    class __declspec(novtable) RenderingQueueManager final
    {
    public:
        RenderingQueueManager(AddonImGui::AddonUIData& data, ResourceManager& rManager);
        ~RenderingQueueManager();

        void RescheduleGroups(CommandListDataContainer& commandListData, DeviceDataContainer& deviceData);
        void ClearQueue(CommandListDataContainer& commandListData, const uint64_t pipelineChange, const uint64_t location) const;
        void ClearQueue(CommandListDataContainer& commandListData, const uint64_t pipelineChange) const;
        void CheckCallForCommandList(reshade::api::command_list* commandList);
    private:
        AddonImGui::AddonUIData& uiData;
        ResourceManager& resourceManager;

        void _RescheduleGroups(ShaderData& sData, CommandListDataContainer& commandListData, DeviceDataContainer& deviceData);
        void _CheckCallForCommandList(ShaderData& sData, CommandListDataContainer& commandListData, DeviceDataContainer& deviceData, RuntimeDataContainer& runtimeData) const;
    };
}