#pragma once

struct HostBufferData;
struct param_3_struct;
struct param_2_struct;
struct ResourceData;
struct ID3D11Resource;

using sig_memcpy = void* (__fastcall)(void*, void*, size_t);
using sig_ffxiv_cbload = int64_t(__fastcall)(ResourceData*, param_2_struct*, param_3_struct, HostBufferData*);
using sig_nier_replicant_cbload = void(__fastcall)(intptr_t p1, intptr_t* p2, uintptr_t p3);