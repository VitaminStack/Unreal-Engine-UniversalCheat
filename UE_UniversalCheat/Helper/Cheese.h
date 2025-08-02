#pragma once

#include <string>
#include <vector>
#include <functional>
#include <Windows.h>
#include <cstdio>
#include "../SDK/Engine_classes.hpp"


// =============================
// PointerChecks (safe memory)
// =============================
class PointerChecks {
public:
    static inline bool IsReadable(const void* ptr, size_t size = sizeof(void*))
    {
        // 1:1 die gleiche Logik wie unten – nur Parameter ist const
        if (!ptr) {
            return false;
        }
        MEMORY_BASIC_INFORMATION mbi;
        if (VirtualQuery(ptr, &mbi, sizeof(mbi)) == 0) {
            return false;
        }
        if (mbi.State != MEM_COMMIT ||
            !(mbi.Protect & (PAGE_READONLY | PAGE_READWRITE |
                PAGE_EXECUTE_READ | PAGE_EXECUTE_READWRITE))) {
            return false;
        }
        return true;
    }
    static inline bool IsReadable(void* ptr, size_t size = sizeof(void*)) {
        return IsReadable(static_cast<const void*>(ptr), size);
    }

    template<typename T>
    static inline bool IsValidPtr(T* ptr, const char* name = "Unknown") {
        if (!ptr) {
            return false;
        }
        if (!IsReadable(ptr, sizeof(T))) {
            return false;
        }
        return true;
    }

    template<typename T>
    static inline bool SafeRead(void* address, T& output, const char* name = "Unknown") {
        if (!IsReadable(address, sizeof(T))) {
            return false;
        }
        __try {
            output = *reinterpret_cast<T*>(address);
            return true;
        }
        __except (EXCEPTION_EXECUTE_HANDLER) {
            return false;
        }
    }
    template<typename T>
    static inline bool SafeWrite(void* address, const T& value, const char* name = "Unknown") {
        if (!IsReadable(address, sizeof(T))) {
            return false;
        }
        __try {
            *reinterpret_cast<T*>(address) = value;
            return true;
        }
        __except (EXCEPTION_EXECUTE_HANDLER) {
            return false;
        }
    }
};

// =============================
// Cheese/ Cheat core
// =============================

struct CheeseOption {
    std::string Name;
    bool Enabled;
    bool LastState;
    std::function<void(bool)> ToggleAction;
    std::function<void()> ResetAction;

    CheeseOption(const std::string& name,
        std::function<void(bool)> action,
        std::function<void()> reset = []() {},
        bool defaultEnabled = false)
        : Name(name), Enabled(defaultEnabled), LastState(!defaultEnabled),
        ToggleAction(action), ResetAction(reset) {
        if (defaultEnabled) {
            ToggleAction(true);
            LastState = true;
        }
    }
};

class Cheese {
public:
    static void Initialize(SDK::APlayerController* Controller);
    static void ApplyCheese();
    static void ResetCheese();
    static std::vector<CheeseOption>& GetCheeseList();
    static void ActivateCheese(const std::string& name, bool enable);
    static void FlyhackControl(SDK::APlayerController* Controller);
private:
    static std::vector<CheeseOption> CheeseList;
    static bool FlyhackActive;
};

