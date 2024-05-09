#pragma once

#include <unordered_map>
#include <charconv>
#include "KeyData.h"
#include "reshade_api.hpp"

namespace ShaderToggler
{
    enum class KeyState : uint32_t
    {
        KEY_STATE_NONE = 0,
        KET_STATE_PRESSED = 1
    };

    class __declspec(novtable) KeyMonitor final
    {
    public:
        static const uint32_t KEY_SCREEN_SHOT = 0;

        void SetKeyMonitor(uint32_t id, uint32_t keys)
        {
            keyMap[id] = keys;
            stateMap[id] = KeyState::KEY_STATE_NONE;
        }

        void PollKeyStates(reshade::api::effect_runtime* runtime)
        {
            for (const auto& keys : keyMap)
            {
                auto& currentState = stateMap[keys.first];

                if (areKeysPressed(keys.second, runtime))
                {
                    currentState = KeyState::KET_STATE_PRESSED;
                }
                else
                {
                    currentState = KeyState::KEY_STATE_NONE;
                }
            }
        }

        KeyState GetKeyState(uint32_t id)
        {
            const auto& state = stateMap.find(id);
            if (state != stateMap.end())
            {
                return state->second;
            }

            return KeyState::KEY_STATE_NONE;
        }

        void Init(reshade::api::effect_runtime* runtime)
        {
            if (initialized)
                return;

            static const size_t bufSize = 64;
            static char buf[bufSize];
            static size_t readSize = bufSize;

            for (auto& configData : reshadeConfigMapping)
            {
                uint32_t keyCombo = 0;

                const auto& [id, data] = configData;
                const auto& [section, name] = data;

                if (reshade::get_config_value(runtime, section.c_str(), name.c_str(), buf, &readSize))
                {
                    const char* delim = ",";
                    char* next_token = NULL;
                    char* token = NULL;
                    uint32_t index = 0;

                    token = strtok_s(buf, delim, &next_token);
                    while (token != NULL)
                    {
                        size_t substringLength = strlen(token);
                        uint32_t key = 0;
                        const auto& [ptr, etc] = std::from_chars(token, token + substringLength, key);
                        key = (key << (index * 8));
                        keyCombo |= key;

                        index++;
                        token = strtok_s(NULL, delim, &next_token);
                    }

                    SetKeyMonitor(configData.first, keyCombo);
                }

                readSize = bufSize;
            }

            initialized = true;
        }
    private:
        bool initialized = false;
        std::unordered_map<uint32_t, uint32_t> keyMap;
        std::unordered_map<uint32_t, KeyState> stateMap;

        const std::unordered_map<uint32_t, std::tuple<std::string, std::string>> reshadeConfigMapping =
        {
            { KEY_SCREEN_SHOT, { "INPUT", "KeyScreenshot" } },
        };
    };
}