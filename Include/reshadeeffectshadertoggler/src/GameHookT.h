#pragma once

#include <reshade_api.hpp>
#include <reshade_api_device.hpp>
#include <reshade_api_pipeline.hpp>
#include <unordered_map>
#include <MinHook.h>
#include <string>

#pragma warning(push)
#pragma warning(disable : 4005)
#include <sigmatch.hpp>
#pragma warning(pop)

struct HostBufferData;
struct param_3_struct;
struct param_2_struct;
struct ResourceData;
struct ID3D11Resource;

using sig_memcpy = void* (__fastcall)(void*, void*, size_t);
using sig_ffxiv_cbload = int64_t(__fastcall)(ResourceData*, param_2_struct*, param_3_struct, HostBufferData*);
using sig_nier_replicant_cbload = void(__fastcall)(intptr_t p1, intptr_t* p2, uintptr_t p3);
using sig_ffxiv_texture_create = void(__fastcall)(uintptr_t*, uintptr_t*);
using sig_ffxiv_textures_recreate = uintptr_t(__fastcall)(uintptr_t);
using sig_ffxiv_textures_create = uintptr_t(__fastcall)(uintptr_t);

namespace Shim
{
    class GameHook {
    protected:
        static bool _hooked;
    };

    template<typename T>
    class GameHookT : public GameHook {
    public:
        static bool Hook(T** original, T* detour, const sigmatch::signature& sig);
        static bool Unhook();
        static std::string GetExecutableName();
        static T* InstallHook(void* target, T* callback);
        static T* InstallApiHook(LPCWSTR pszModule, LPCSTR pszProcName, T* callback);
    };
}