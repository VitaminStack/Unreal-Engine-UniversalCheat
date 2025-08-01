#pragma once

#include "../SDK/Engine_classes.hpp"
#include <Windows.h>
#include <iostream>
#include <directxmath.h>
#include <d3dx9.h>
#include <string>
#include <codecvt>
#include <locale>
#include "../ImGui/imgui.h"
#include <cmath>  // Für sqrtf
#include <unordered_map>
#include <shared_mutex>
#include <array>
#include <atomic>
#include "Cheese.h"

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

    void UpdateCam(SDK::APlayerCameraManager* cam) {
        if (!cam) return;
        CamPos = cam->GetCameraLocation();
        Rotation = cam->GetCameraRotation();
        Fov = cam->GetFOVAngle();
    }
};

// ✅ Mathe- und Konvertierungsfunktionen
D3DXMATRIX ToMatrix(const Vector3& Rotation);
bool UEWorldToScreen(const Vector3& worldLoc, Vector2& screenPos, Vector3 Rotation, Vector3 CamPos, float FOV, int ScreenHöhe, int ScreenBreite);

extern int Screen_w;
extern int Screen_h;

// ✅ String- und Name-Konvertierung
SDK::FName CreateFName(UC::FString Name);
UC::FString ConvertToFString(const char* CharArray);
void LoadLevels(SDK::UWorld* World);
void DrawDebugText(SDK::UCanvas* Canvas);

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
    static bool DrawActorESP(SDK::AActor* Actor, const Cam& camera, ImDrawList* drawlist, float DistCap, bool onlyPawns);
    static bool IsWorldValid(SDK::UWorld* World);

private:
    static SDK::FVector GetActorPosition(SDK::AActor* Actor);
    static bool IsActorVisible(const Vector2& screenPos, int screenWidth, int screenHeight);
    static void RenderActorInfo(ImDrawList* drawlist, const Vector2& screenPos, ImU32 color, const std::string& ActorName, float Dist);
};






struct CachedEntityStatic          // selten verändert
{
    const SDK::AActor* actor = nullptr;
    std::string         label;     // bereits UTF-8
    ImU32               color = IM_COL32_WHITE;
};

struct CachedEntityDynamic         // jedes Frame neu
{
    SDK::FVector worldPos;
    Vector2      screenPos;
    float        distance = 0.f; // Meter
};

class EntityCache
{
public:
    /* Game-Thread */ void Add(const SDK::AActor* actor);
    /* Game-Thread */ void Remove(const SDK::AActor* actor);

    /* Update-Thread – pro Frame aufrufen */
    void Refresh(const SDK::FVector& camPos,
        const SDK::FRotator& camRot,
        float fov, int screenW, int screenH);

    /* Render-Thread – lock-frei */
    const std::vector<CachedEntityDynamic>& DrawList() const;
    const std::vector<const CachedEntityStatic*>& StaticDrawList() const;
    bool GetWorldPos(const SDK::AActor* actor, SDK::FVector& out) const;
    bool SetWorldPos(const SDK::AActor* actor, const SDK::FVector& newPos) const;

private:
    std::unordered_map<const SDK::AActor*, CachedEntityStatic> statics_;
    mutable std::shared_mutex staticsMx_;

    std::array<std::vector<CachedEntityDynamic>, 2> dynBuf_;
    std::array<std::vector<const CachedEntityStatic*>, 2> statPtrBuf_;
    std::atomic<uint32_t> writeIdx_{ 0 };
};

extern EntityCache g_EntityCache;