#pragma once

#include <string>
#include <vector>
#include <functional>
#include <Windows.h>
#include <cstdio>
#include "SDK/Engine_classes.hpp"

// =============================
// PointerChecks (safe memory)
// =============================
class PointerChecks {
public:
    static inline bool IsReadable(void* ptr, size_t size = sizeof(void*)) {
        if (!ptr) {
            printf("[POINTER_CHECK] Null pointer detected\n");
            return false;
        }
        MEMORY_BASIC_INFORMATION mbi;
        if (VirtualQuery(ptr, &mbi, sizeof(mbi)) == 0) {
            printf("[POINTER_CHECK] VirtualQuery failed for address: 0x%p\n", ptr);
            return false;
        }
        if (mbi.State != MEM_COMMIT ||
            !(mbi.Protect & (PAGE_READONLY | PAGE_READWRITE | PAGE_EXECUTE_READ | PAGE_EXECUTE_READWRITE))) {
            printf("[POINTER_CHECK] Memory not accessible at address: 0x%p\n", ptr);
            return false;
        }
        return true;
    }

    template<typename T>
    static inline bool IsValidPtr(T* ptr, const char* name = "Unknown") {
        if (!ptr) {
            printf("[POINTER_CHECK] %s: Null pointer\n", name);
            return false;
        }
        if (!IsReadable(ptr, sizeof(T))) {
            printf("[POINTER_CHECK] %s: Pointer 0x%p not readable\n", name, ptr);
            return false;
        }
        return true;
    }

    template<typename T>
    static inline bool SafeRead(void* address, T& output, const char* name = "Unknown") {
        if (!IsReadable(address, sizeof(T))) {
            printf("[POINTER_CHECK] %s: Cannot read from address 0x%p\n", name, address);
            return false;
        }
        __try {
            output = *reinterpret_cast<T*>(address);
            printf("[POINTER_CHECK] %s: Successfully read from address 0x%p\n", name, address);
            return true;
        }
        __except (EXCEPTION_EXECUTE_HANDLER) {
            printf("[POINTER_CHECK] %s: Exception reading from address 0x%p\n", name, address);
            return false;
        }
    }
    template<typename T>
    static inline bool SafeWrite(void* address, const T& value, const char* name = "Unknown") {
        if (!IsReadable(address, sizeof(T))) {
            printf("[POINTER_CHECK] %s: Cannot write to address 0x%p\n", name, address);
            return false;
        }
        __try {
            *reinterpret_cast<T*>(address) = value;
            printf("[POINTER_CHECK] %s: Successfully wrote to address 0x%p\n", name, address);
            return true;
        }
        __except (EXCEPTION_EXECUTE_HANDLER) {
            printf("[POINTER_CHECK] %s: Exception writing to address 0x%p\n", name, address);
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

