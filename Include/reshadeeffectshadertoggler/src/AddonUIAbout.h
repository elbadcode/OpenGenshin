#pragma once
#include <windows.h>
#include <tuple>
#include <reshade.hpp>
#include "resource.h"

static std::string GetResourceData(uint16_t id) {
    HMODULE hModule = NULL;
    GetModuleHandleEx(
        GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS,
        (LPCTSTR)GetResourceData,
        &hModule);

    HRSRC myResource = ::FindResource(hModule, MAKEINTRESOURCE(id), RT_RCDATA);

    if (myResource != 0)
    {
        DWORD myResourceSize = SizeofResource(hModule, myResource);
        HGLOBAL myResourceData = LoadResource(hModule, myResource);

        if (myResourceData != 0)
        {
            const char* pMyBinaryData = static_cast<const char*>(LockResource(myResourceData));
            return std::string(pMyBinaryData, myResourceSize);
        }
    }

    return "";
}

static void DisplayAbout()
{
    static bool wndOpen = false;

    ImGui::PushItemWidth(ImGui::GetWindowWidth());
    if (ImGui::Button("About", ImVec2(ImGui::GetWindowWidth(), 0.0f)))
    {
        wndOpen = !wndOpen;
    }
    ImGui::PopItemWidth();

    if (wndOpen)
    {
        ImGui::SetNextWindowSize({ 500, 800 }, ImGuiCond_Once);
        if (ImGui::Begin("About##Wnd", &wndOpen))
        {
            if (ImGui::BeginChild("About##Licenses", { 0, 0 }, true, ImGuiWindowFlags_AlwaysAutoResize))
            {
                auto lic_REST = GetResourceData(LICENSE_REST);
                ImGui::TextWrapped("%s", lic_REST.c_str());

                if (ImGui::CollapsingHeader("ReShade"))
                {
                    auto lic_ReShade = GetResourceData(LICENSE_RESHADE);
                    ImGui::TextWrapped("%s", lic_ReShade.c_str());
                }

                if (ImGui::CollapsingHeader("MinHook"))
                {
                    auto lic_MinHook = GetResourceData(LICENSE_MINHOOK);
                    ImGui::TextWrapped("%s", lic_MinHook.c_str());
                }

                if (ImGui::CollapsingHeader("SigMatch"))
                {
                    auto lic_SigMatch = GetResourceData(LICENSE_SIGMATCH);
                    ImGui::TextWrapped("%s", lic_SigMatch.c_str());
                }

                if (ImGui::CollapsingHeader("Robin-Map"))
                {
                    auto lic_RobinMap = GetResourceData(LICENSE_ROBINMAP);
                    ImGui::TextWrapped("%s", lic_RobinMap.c_str());
                }

                if (ImGui::CollapsingHeader("ImGui"))
                {
                    auto lic_ImGui = GetResourceData(LICENSE_IMGUI);
                    ImGui::TextWrapped("%s", lic_ImGui.c_str());
                }

                ImGui::EndChild();
            }

            ImGui::End();
        }
    }
}