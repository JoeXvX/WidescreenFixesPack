#pragma once
#include <stdafx.h>
#include "ppc.hpp"

class Dolphin
{
public:
    static inline uintptr_t GameMemoryStart = 0;
    static inline uintptr_t GameMemoryEnd = 0;
    static inline uintptr_t ImageBase = 0;
private:
    static inline auto _SetIsThrottlerTempDisabled = (void(__fastcall*)(bool disable))(nullptr);
    static inline auto _MenuBarClearCache = (void(__fastcall*)())(nullptr);

public:
    static hook::pattern Pattern()
    {
        return hook::pattern("0F B6 C8 E8 ? ? ? ? 33 D2");
    }

    static void SetIsThrottlerTempDisabled(bool disable)
    {
        if (!_SetIsThrottlerTempDisabled)
        {
            auto pattern = hook::pattern("0F B6 C8 E8 ? ? ? ? 33 D2");
            if (!pattern.empty())
                _SetIsThrottlerTempDisabled = (void(__fastcall*)(bool disable))(injector::GetBranchDestination(pattern.get_first(3)).as_int());
        }
        else
            return _SetIsThrottlerTempDisabled(disable);
    }

    static void MenuBarClearCache()
    {
        if (!_MenuBarClearCache)
        {
            auto pattern = hook::pattern("45 33 C9 45 33 C0 33 D2");
            if (!pattern.empty())
            {
                for (size_t i = 0; i < pattern.size(); i++)
                {
                    auto range_pattern = hook::pattern((uintptr_t)pattern.get(i).get<uintptr_t>(0), (uintptr_t)pattern.get(i).get<uintptr_t>(200), "45 ? ? ? 8D");
                    if (!range_pattern.empty())
                    {
                        std::string_view str(injector::ReadRelativeOffset(range_pattern.get(0).get<uintptr_t>(6)).get_raw<char>());
                        if (str == "Clear Cache")
                        {
                            _MenuBarClearCache = (void(__fastcall*)())(injector::ReadRelativeOffset(pattern.get(i).get<uintptr_t>(22)).as_int());
                            break;
                        }
                    }
                }
            }
        }
        else
            return _MenuBarClearCache();
    }

    static void FindEmulatorMemory()
    {
        while (GameMemoryStart == 0)
        {
            uintptr_t curAddr = 0;
            do
            {
                MEMORY_BASIC_INFORMATION MemoryInf;
                if (VirtualQuery((LPCVOID)curAddr, &MemoryInf, sizeof(MemoryInf)) == 0) break;
                if (MemoryInf.AllocationProtect == PAGE_READWRITE && MemoryInf.State == MEM_COMMIT &&
                    MemoryInf.Protect == PAGE_READWRITE && MemoryInf.Type == MEM_MAPPED && MemoryInf.RegionSize == 0x2000000)
                {
                    if (GameMemoryStart == 0)
                    {
                        GameMemoryStart = (uintptr_t)MemoryInf.BaseAddress;
                        GameMemoryEnd = GameMemoryStart + 0x01800000;
                        ImageBase = 0x80000000;
                        break;
                    }
                }
                curAddr += MemoryInf.RegionSize;
            } while (true);
        }
    }

    static bool MemoryValid()
    {
        static MEMORY_BASIC_INFORMATION MemoryInf;
        if (GameMemoryStart == 0 || VirtualQuery((LPCVOID)GameMemoryStart, &MemoryInf, sizeof(MemoryInf)) == 0 || MemoryInf.AllocationProtect != PAGE_READWRITE)
        {
            GameMemoryStart = 0;
            FindEmulatorMemory();
            return false;
        }
        return true;
    }

    template <typename T>
    static bool MemoryValid(T addr)
    {
        static MEMORY_BASIC_INFORMATION MemoryInf;
        if (addr == 0 || VirtualQuery((LPCVOID)addr, &MemoryInf, sizeof(MemoryInf)) == 0 || MemoryInf.AllocationProtect != PAGE_READWRITE)
            return false;
        return true;
    }

    static std::string_view GameID()
    {
        return std::string_view(reinterpret_cast<char*>(GameMemoryStart));
    }
};
