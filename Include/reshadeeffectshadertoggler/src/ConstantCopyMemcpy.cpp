#include <cstring>
#include <MinHook.h>
#include "ConstantCopyMemcpy.h"

using namespace Shim;
using namespace Shim::Constants;
using namespace std;

sig_memcpy* ConstantCopyMemcpy::org_memcpy = nullptr;
ConstantCopyMemcpy* ConstantCopyMemcpy::_instance = nullptr;

ConstantCopyMemcpy::ConstantCopyMemcpy()
{

}

ConstantCopyMemcpy::~ConstantCopyMemcpy()
{
}

bool ConstantCopyMemcpy::Init()
{
    return Hook(&org_memcpy, detour_memcpy, ""_sig);
}

bool ConstantCopyMemcpy::UnInit()
{
    return MH_Uninitialize() == MH_OK;
}


bool ConstantCopyMemcpy::HookStatic(sig_memcpy** original, sig_memcpy* detour)
{
    string exe = GameHookT<sig_memcpy>::GetExecutableName();
    if (exe.length() == 0)
        return false;

    for (const auto& sig : memcpy_static)
    {
        sigmatch::this_process_target target;
        sigmatch::search_result result = target.in_module(exe).search(sig);

        for (const std::byte* address : result.matches()) {
            void* adr = static_cast<void*>(const_cast<std::byte*>(address));
            *original = GameHookT<sig_memcpy>::InstallHook(adr, detour);

            // Assume signature is unique
            if (*original != nullptr)
            {
                return true;
            }
        }
    }

    return false;
}

bool ConstantCopyMemcpy::HookDynamic(sig_memcpy** original, sig_memcpy* detour)
{
    for (const auto& [mod,procName] : memcpy_dynamic)
    {
        *original = GameHookT<sig_memcpy>::InstallApiHook(mod.c_str(), procName.c_str(), detour);

        // Pick first hit and hope for the best
        if (*original != nullptr)
        {
            return true;
        }
    }

    return false;
}

bool ConstantCopyMemcpy::Hook(sig_memcpy** original, sig_memcpy* detour, const sigmatch::signature& sig)
{
    // Try hooking statically linked memcpy first, then look into dynamically linked ones
    if (HookStatic(original, detour) || HookDynamic(original, detour))
    {
        return !(MH_EnableHook(MH_ALL_HOOKS) != MH_OK);
    }

    return false;
}

bool ConstantCopyMemcpy::Unhook()
{
    if (MH_DisableHook(MH_ALL_HOOKS) != MH_OK)
    {
        return false;
    }

    return true;
}

void* __fastcall ConstantCopyMemcpy::detour_memcpy(void* dest, void* src, size_t size)
{
    _instance->OnMemcpy(dest, src, size);

    return org_memcpy(dest, src, size);
}