///////////////////////////////////////////////////////////////////////
//
// Part of ShaderToggler, a shader toggler add on for Reshade 5+ which allows you
// to define groups of shaders to toggle them on/off with one key press
// 
// (c) Frans 'Otis_Inf' Bouma.
//
// All rights reserved.
// https://github.com/FransBouma/ShaderToggler
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are met :
//
//  * Redistributions of source code must retain the above copyright notice, this
//	  list of conditions and the following disclaimer.
//
//  * Redistributions in binary form must reproduce the above copyright notice,
//    this list of conditions and the following disclaimer in the documentation
//    and / or other materials provided with the distribution.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
// AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
// DISCLAIMED.IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
// FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
// DAMAGES(INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
// SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
// CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
// OR TORT(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
// OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
/////////////////////////////////////////////////////////////////////////
#pragma once

#include <reshade_api.hpp>

#include "stdafx.h"

// Mostly from Reshade, see https://github.com/crosire/reshade/blob/main/source/input.cpp
namespace ShaderToggler
{
    static std::string vkCodeToString(uint8_t vkCode)
    {
        static const char* keyboard_keys[256] = {
            "", "Left Mouse", "Right Mouse", "Cancel", "Middle Mouse", "X1 Mouse", "X2 Mouse", "", "Backspace", "Tab", "", "", "Clear", "Enter", "", "",
            "Shift", "Control", "Alt", "Pause", "Caps Lock", "", "", "", "", "", "", "Escape", "", "", "", "",
            "Space", "Page Up", "Page Down", "End", "Home", "Left Arrow", "Up Arrow", "Right Arrow", "Down Arrow", "Select", "", "", "Print Screen", "Insert", "Delete", "Help",
            "0", "1", "2", "3", "4", "5", "6", "7", "8", "9", "", "", "", "", "", "",
            "", "A", "B", "C", "D", "E", "F", "G", "H", "I", "J", "K", "L", "M", "N", "O",
            "P", "Q", "R", "S", "T", "U", "V", "W", "X", "Y", "Z", "Left Windows", "Right Windows", "Apps", "", "Sleep",
            "Numpad 0", "Numpad 1", "Numpad 2", "Numpad 3", "Numpad 4", "Numpad 5", "Numpad 6", "Numpad 7", "Numpad 8", "Numpad 9", "Numpad *", "Numpad +", "", "Numpad -", "Numpad Decimal", "Numpad /",
            "F1", "F2", "F3", "F4", "F5", "F6", "F7", "F8", "F9", "F10", "F11", "F12", "F13", "F14", "F15", "F16",
            "F17", "F18", "F19", "F20", "F21", "F22", "F23", "F24", "", "", "", "", "", "", "", "",
            "Num Lock", "Scroll Lock", "", "", "", "", "", "", "", "", "", "", "", "", "", "",
            "Left Shift", "Right Shift", "Left Control", "Right Control", "Left Menu", "Right Menu", "Browser Back", "Browser Forward", "Browser Refresh", "Browser Stop", "Browser Search", "Browser Favorites", "Browser Home", "Volume Mute", "Volume Down", "Volume Up",
            "Next Track", "Previous Track", "Media Stop", "Media Play/Pause", "Mail", "Media Select", "Launch App 1", "Launch App 2", "", "", "OEM ;", "OEM +", "OEM ,", "OEM -", "OEM .", "OEM /",
            "OEM ~", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "",
            "", "", "", "", "", "", "", "", "", "", "", "OEM [", "OEM \\", "OEM ]", "OEM '", "OEM 8",
            "", "", "OEM <", "", "", "", "", "", "", "", "", "", "", "", "", "",
            "", "", "", "", "", "", "Attn", "CrSel", "ExSel", "Erase EOF", "Play", "Zoom", "", "PA1", "OEM Clear", ""
        };

        return keyboard_keys[vkCode];
    }

    static bool areKeysPressed(uint32_t keys, reshade::api::effect_runtime * runtime)
    {
        uint8_t key0 = keys & 0xFF;
        uint8_t key1 = (keys >> 8) & 0xFF;
        uint8_t key2 = (keys >> 16) & 0xFF;
        uint8_t key3 = (keys >> 24) & 0xFF;

        bool ret = runtime->is_key_pressed(key0);

        ret = ret && (key1 == 0 || runtime->is_key_down(0x11));
        ret = ret && (key2 == 0 || runtime->is_key_down(0x10));
        ret = ret && (key3 == 0 || runtime->is_key_down(0x12));

        return ret;
    }


    static std::string reshade_key_name(const uint32_t keys)
    {
        uint8_t key0 = keys & 0xFF;
        uint8_t key1 = (keys >> 8) & 0xFF;
        uint8_t key2 = (keys >> 16) & 0xFF;
        uint8_t key3 = (keys >> 24) & 0xFF;

        assert(key0 != VK_CONTROL && key0 != VK_SHIFT && key0 != VK_MENU);

        return (key1 ? "Ctrl + " : std::string()) + (key2 ? "Shift + " : std::string()) + (key3 ? "Alt + " : std::string()) + vkCodeToString(key0);
    }


    static uint8_t reshade_last_key_pressed(const reshade::api::effect_runtime* runtime)
    {
        for (uint32_t i = VK_XBUTTON2 + 1; i < 256; i++)
            if (runtime->is_key_pressed(static_cast<uint8_t>(i)))
                return i;
        return 0;
    }
}

