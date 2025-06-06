#pragma once

#include "SDK/Engine_classes.hpp"
#include <Windows.h>
#include <iostream>
#include <directxmath.h>
#include <d3dx9.h>
#include <string>
#include <codecvt>
#include <locale>
#include "ImGui/imgui.h"
#include <cmath>  // Für sqrtf

namespace VectorUtils {
    // ✅ Berechnet die euklidische Distanz zwischen zwei Punkten
    float CalculateDistance(const SDK::FVector& A, const SDK::FVector& B);

    // ✅ Berechnet den kürzesten Abstand von einem Punkt zu einer Linie (Ray)
    float CalculateDistanceToRay(const SDK::FVector& RayStart, const SDK::FVector& RayEnd, const SDK::FVector& Point);

    // ✅ Vektor-Subtraktion
    SDK::FVector Subtract(const SDK::FVector& A, const SDK::FVector& B);

    // ✅ Vektor-Länge (Magnitude)
    float Size(const SDK::FVector& Vec);

    // ✅ Dot-Produkt von zwei Vektoren
    float DotProduct(const SDK::FVector& A, const SDK::FVector& B);

    // ✅ Normalisiert einen Vektor
    SDK::FVector Normalize(const SDK::FVector& Vec);

    // ✅ Berechnet den Kreuzprodukt (Cross Product)
    SDK::FVector CrossProduct(const SDK::FVector& A, const SDK::FVector& B);
}
// ✅ Vektor-Strukturen
struct Vector2 {
    float x, y;
};
class Vector3 {
public:
    float x, y, z;

    Vector3() : x(0), y(0), z(0) {}
    Vector3(float X, float Y, float Z) : x(X), y(Y), z(Z) {}

    // Operatoren für Vektor-Berechnungen
    Vector3 operator+(const Vector3& v) const;
    Vector3 operator-(const Vector3& v) const;
    Vector3 operator*(float scalar) const;
    Vector3 Cross(const Vector3& v) const;
    float Dot(const Vector3& v) const;
    float Length() const;
    float DistTo(const Vector3& v) const;
};
struct Vector4 {
    float x, y, z, w;
    Vector4 Conjugate() const;
    Vector4 operator*(const Vector4& other) const;
};

// ✅ Kamera-Klasse
class Cam {
public:
    SDK::FVector CamPos;
    SDK::FRotator Rotation;
    float Fov;

    void UpdateCam(SDK::APlayerCameraManager cam);
};

// ✅ Mathe- und Konvertierungsfunktionen
D3DXMATRIX ToMatrix(const Vector3& Rotation);
bool UEWorldToScreen(const Vector3& worldLoc, Vector2& screenPos, Vector3 Rotation, Vector3 CamPos, float FOV, int ScreenHöhe, int ScreenBreite);

// ✅ String- und Name-Konvertierung
SDK::FName CreateFName(UC::FString Name);
UC::FString ConvertToFString(const char* CharArray);
void LoadLevels(SDK::UWorld* World);


const char* floatToConstChar(float value);
const char* combineConstChars(const char* str1, const char* str2);
float CalcMiddlePos(float vScreenX, const char* Text);
void PrintActorType(SDK::AActor* Actor);

class FPSLimiter {
public:
    // Konstruktor
    FPSLimiter(float fps = 60.0f);

    // Setzt das Ziel-FPS-Limit
    void setTargetFPS(float fps);

    // Begrenzt die FPS
    void cap(ImGuiIO& io);

private:
    LARGE_INTEGER frequency;   // High-Precision Timer Frequenz
    LARGE_INTEGER frameStart;  // Startzeit des Frames
    LARGE_INTEGER frameEnd;    // Endzeit des Frames

    float targetFPS;           // Ziel-FPS
    double targetFrameTime;    // Ziel-Framezeit in Millisekunden

    // Berechnet die verstrichene Zeit in Millisekunden
    double getElapsedTime(LARGE_INTEGER start, LARGE_INTEGER end);
};

struct CheatOption {
    std::string Name;
    bool Enabled;
    bool LastState;
    std::function<void(bool)> ToggleAction;
    std::function<void()> ResetAction;

    CheatOption(const std::string& name, std::function<void(bool)> action, std::function<void()> reset = []() {}, bool defaultEnabled = false);
};
class Cheats {
public:
    static void Initialize(SDK::APlayerController* Controller);
    static void ApplyCheats();
    static void ResetCheats();
    static std::vector<CheatOption>& GetCheatList();

    static void ActivateCheat(const std::string& cheatName, bool enable);

    // ✅ Neu: Flyhack-Steuerung (wird im ApplyCheats aufgerufen)
    static void FlyhackControl(SDK::APlayerController* Controller);
    static void DisableRequiredAmmoForShoot(SDK::APlayerController* MyController, bool enable);

private:
    static std::vector<CheatOption> CheatList;
    static bool FlyhackActive;  // Status des Flyhacks
    static bool IsReadable(void* ptr);
};
class Raycaster {
public:
    struct RaycastResult {
        bool bHit = false;
        SDK::FVector StartPoint;
        SDK::FVector EndPoint;
        SDK::FHitResult HitResult;
    };

    static RaycastResult PerformRaycast(SDK::UWorld* World, SDK::APlayerController* Controller, float Distance);
    static bool CustomLineTrace(SDK::UWorld* World, const SDK::FVector& Start, const SDK::FVector& End, SDK::FHitResult& OutHit);
    static void DrawDebugRay(ImDrawList* drawlist, SDK::APlayerController* Controller);
    static const std::vector<RaycastResult>& GetRaycastHistory();

private:
    static inline std::vector<RaycastResult> RaycastHistory;
    static constexpr size_t MaxStoredRays = 50;  // History Limit
};
class SimpleESP {
public:
    static bool DrawActorESP(SDK::AActor* Actor, SDK::APlayerController* Controller, ImDrawList* drawlist, float DistCap);

private:
    static SDK::FVector GetActorPosition(SDK::AActor* Actor);
    static bool IsActorVisible(const Vector2& screenPos, int screenWidth, int screenHeight);
    static void RenderActorInfo(ImDrawList* drawlist, const Vector2& screenPos, ImU32 color, const std::string& ActorName, float Dist);
};


// ==========================================
// PROBLEM: Bitfelder haben keine Adresse!
// ==========================================

// Das funktioniert NICHT:
// bool bCanBeDamaged : 1;  // Bitfeld!
// &Controller->Character->bCanBeDamaged;  // ❌ Fehler!

// ==========================================
// LÖSUNG 1: Erweiterte PointerChecks mit Bitfeld-Support
// ==========================================

// PointerChecks.hpp - Erweiterte Version
#pragma once
#include <Windows.h>
#include <cstdio>

class PointerChecks {
public:
    // Bestehende Funktionen...
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

    // ✅ NEU: Sichere Zuweisung für Bitfelder und normale Variablen
    template<typename ObjectType, typename ValueType>
    static inline bool SafeAssign(ObjectType* object, ValueType ObjectType::* member,
        const ValueType& value, const char* name = "Unknown") {
        if (!IsValidPtr(object, name)) {
            return false;
        }

        __try {
            object->*member = value;  // Direkte Zuweisung über Member-Pointer
            printf("[POINTER_CHECK] %s: Successfully assigned value\n", name);
            return true;
        }
        __except (EXCEPTION_EXECUTE_HANDLER) {
            printf("[POINTER_CHECK] %s: Exception during assignment\n", name);
            return false;
        }
    }

    // ✅ NEU: Vereinfachte Bitfeld-Zuweisung
    template<typename T>
    static inline bool SafeBitfieldWrite(T* object, const char* name,
        std::function<void(T*)> setter) {
        if (!IsValidPtr(object, name)) {
            return false;
        }

        __try {
            setter(object);
            printf("[POINTER_CHECK] %s: Bitfield successfully modified\n", name);
            return true;
        }
        __except (EXCEPTION_EXECUTE_HANDLER) {
            printf("[POINTER_CHECK] %s: Exception during bitfield modification\n", name);
            return false;
        }
    }

    // ✅ SafeRead für sichere Speicher-Lesevorgänge
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

    // Standard SafeWrite für normale Pointer
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





void DrawDebugText(SDK::UCanvas* Canvas);