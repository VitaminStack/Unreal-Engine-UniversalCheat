#include <Windows.h>
#include <iostream>

#include "Helper/Logger.h"  // <---- Logger einbinden

#include "SDK/Engine_classes.hpp"
#include "Overlay/OverlayCord.h"
#include "Overlay/Overlay.h"
#include "MinHook/MinHook.h"
#include "Hooks/hooks.hpp"
#include "Hooks/utils/utils.hpp"

// Basic.cpp was added to the VS project
// Engine_functions.cpp was added to the VS project

bool DLLrunning = true;
bool MainHacking = false;

bool ImGUIDiscordRender = false;
bool DiscordRender = false;
bool TransRender = false;
bool TestRender = false;


//HOOKRENDERING
bool HookRender = true;
RenderingBackend_t RenderModule = RenderingBackend_t::DIRECTX12;

void OpenConsole()
{
    AllocConsole();

    // stdout
    FILE* fp_out;
    freopen_s(&fp_out, "CONOUT$", "w", stdout);
    // stderr
    FILE* fp_err;
    freopen_s(&fp_err, "CONOUT$", "w", stderr);
    // stdin
    FILE* fp_in;
    freopen_s(&fp_in, "CONIN$", "r", stdin);

    // C++ Streams auch umleiten
    std::ios::sync_with_stdio(true);
    std::cout.clear();
    std::cerr.clear();
    std::clog.clear();

    // Optional: Titel setzen
    SetConsoleTitleA("Overlay/Debug Console");

    // Testausgabe für dich:
    LOG_INFO("Console started successfully\n");
}


DWORD WINAPI OverlayThread(LPVOID lpParameter)
{
    LOG_INFO("OverlayThread started (TID: %d)", GetCurrentThreadId());

    if (ImGUIDiscordRender)
    {
        try {
            LOG_INFO("Initializing ImGUI-Discord-Render overlay");
            ImGuiOverlay overlay(800, 600, 1234, true); // Letztes Argument aktiviert Debugging

            while (DLLrunning) { // WICHTIG: auf DLLrunning prüfen!
                overlay.Render();
                overlay.SendToOverlay();
                Sleep(16); // 60 FPS
            }
            LOG_INFO("ImGUIDiscordRender finished.");
        }
        catch (const std::exception& e) {
            LOG_ERROR("Error in ImGUIDiscordRender: %s", e.what());
        }
    }
    else if (DiscordRender)
    {
        LOG_INFO("Searching for target process for DiscordRender...");
        const char* targetExe = "SCUM.exe";

        DWORD targetProcessId = GetProcessIDByName(targetExe);
        if (!targetProcessId) {
            LOG_ERROR("Process not found: %s", targetExe);
            return -1;
        }
        LOG_INFO("Found process ID: %u", targetProcessId);

        // Verbindung zu OverlayCord herstellen
        LOG_INFO("Connecting to OverlayCord backend...");
        OverlayCord::Communication::ConnectedProcessInfo processInfo = { 0 };
        processInfo.ProcessId = targetProcessId;

        if (!OverlayCord::Communication::ConnectToProcess(processInfo)) {
            LOG_ERROR("Error: Connection to overlay backend failed");
            return -1;
        }

        LOG_INFO("Successfully connected with mapped address: 0x%p", processInfo.MappedAddress);

        // **Render Loop starten** – sollte intern auch auf DLLrunning prüfen!
        while (DLLrunning) {
            StartRenderLoop(processInfo); // Passe ggf. die interne Loop an!
            Sleep(10); // Notfall-Fallback
        }

        // Verbindung trennen, wenn der Render-Loop beendet ist
        LOG_INFO("Render loop ended, disconnecting...");
        OverlayCord::Communication::DisconnectFromProcess(processInfo);
        LOG_INFO("Disconnected");
    }
    else if (TransRender || TestRender)
    {
        LOG_INFO("Starting TransRender...");
        while (DLLrunning) {
            RenderOverlay();
            Sleep(10);
        }
        LOG_INFO("TransRender finished.");
    }
    else if (HookRender)
    {
        LOG_INFO("Starting HookRender...");
        if (U::GetRenderingBackend() == NONE) {
            LOG_INFO("[!] Looks like you forgot to set a backend. Will unload after pressing enter...");
            std::cin.get();
            return 0;
        }

        H::Init();

        // Idle/Loop, bis DLL entladen werden soll
        while (DLLrunning) {
            Sleep(50);
        }
        LOG_INFO("HookRender finished.");
    }
    else
    {
        LOG_ERROR("No valid render module selected.");
        return -1;
    }

    LOG_INFO("OverlayThread ending.");
    return 0;
}


DWORD MainThread(HMODULE Module)
{
    
    LOG_INFO("MainThread started (TID: %d)", GetCurrentThreadId());

    if (MainHacking)
    {
        LOG_INFO("Starting MainHacking...");
        SDK::UEngine* Engine = SDK::UEngine::GetEngine();
        if (!Engine) {
            LOG_ERROR("UEngine pointer is null");
            return 0;
        }
        SDK::UWorld* World = SDK::UWorld::GetWorld();
        if (!World || !World->OwningGameInstance) {
            LOG_ERROR("World or OwningGameInstance invalid");
            return 0;
        }
        auto& LocalPlayers = World->OwningGameInstance->LocalPlayers;
        if (LocalPlayers.Num() <= 0 || !LocalPlayers[0]) {
            LOG_ERROR("LocalPlayer not found");
            return 0;
        }
        SDK::APlayerController* MyController = LocalPlayers[0]->PlayerController;
        if (!MyController) {
            LOG_ERROR("PlayerController not found");
            return 0;
        }
        if (Engine->ConsoleClass)
            LOG_INFO("Engine found: %s", Engine->ConsoleClass->GetFullName());
        else
            LOG_INFO("Engine found but ConsoleClass is null");

        for (int i = 0; i < SDK::UObject::GObjects->Num(); i++)
        {
            SDK::UObject* Obj = SDK::UObject::GObjects->GetByIndex(i);

            if (!Obj)
                continue;

            if (Obj->IsDefaultObject())
                continue;

            if (Obj->IsA(SDK::APawn::StaticClass()) || Obj->HasTypeFlag(SDK::EClassCastFlags::Pawn))
            {
                LOG_DEBUG("Pawn found: %s", Obj->GetFullName());
            }
        }

        SDK::ULevel* Level = World->PersistentLevel;
        SDK::TArray<SDK::AActor*>& Actors = Level->Actors;

        for (SDK::AActor* Actor : Actors)
        {
            if (!Actor || !Actor->IsA(SDK::EClassCastFlags::Pawn) || !Actor->IsA(SDK::APawn::StaticClass()))
                continue;

            SDK::APawn* Pawn = static_cast<SDK::APawn*>(Actor);
            // Du könntest hier Details loggen, falls gewünscht
        }

        LOG_INFO("Adjusting InputSettings...");
        SDK::UInputSettings::GetDefaultObj()->ConsoleKeys[0].KeyName = SDK::UKismetStringLibrary::Conv_StringToName(L"F2");

        SDK::UObject* NewObject = SDK::UGameplayStatics::SpawnObject(Engine->ConsoleClass, Engine->GameViewport);
        Engine->GameViewport->ViewportConsole = static_cast<SDK::UConsole*>(NewObject);
        LOG_INFO("MainHacking completed.");
    }


    while (DLLrunning) {
        // Beenden der DLL durch Tastendruck
        if (GetAsyncKeyState(VK_END) & 1) {
            LOG_INFO("Stopping overlay and MainThread via key press");
            DLLrunning = false;
            running = false;
            break;
        }

        Sleep(50);
    }

    LOG_INFO("MainThread ending.");
    Sleep(500);
    FreeConsole();
    FreeLibraryAndExitThread(Module, 0);
    return 0;
}

BOOL APIENTRY DllMain(HMODULE hModule, DWORD reason, LPVOID lpReserved)
{
    static HANDLE hOverlayThread = NULL;
    static HANDLE hMainThread = NULL;

    if (reason == DLL_PROCESS_ATTACH) {
        // Setze Rendering-Backend direkt hier, ODER später per Config/Detection
        U::SetRenderingBackend(RenderModule); // Oder dynamisch nach Bedarf/Detection

        // Konsole+Logger zentral initialisieren
        OpenConsole();
        LOG_INFO("DLL_PROCESS_ATTACH - Starting threads");
        LOG_INFO("Rendering backend: %s", U::RenderingBackendToStr(RenderModule));

        if (U::GetRenderingBackend() == NONE) {
            LOG_ERROR("No rendering backend set. DLL will not continue.");
            return FALSE;
        }

        // Initialize MinHook only once
        MH_STATUS mhStatus = MH_Initialize();
        if (mhStatus != MH_OK && mhStatus != MH_ERROR_ALREADY_INITIALIZED) {
            LOG_ERROR("MinHook initialization failed: %s", MH_StatusToString(mhStatus));
            return FALSE;
        }

        // Threads starten
        hOverlayThread = CreateThread(0, 0, (LPTHREAD_START_ROUTINE)OverlayThread, hModule, 0, 0);
        if (!hOverlayThread) {
            LOG_ERROR("Failed to create OverlayThread");
            return FALSE;
        }
        hMainThread = CreateThread(0, 0, (LPTHREAD_START_ROUTINE)MainThread, hModule, 0, 0);
        if (!hMainThread) {
            LOG_ERROR("Failed to create MainThread");
            return FALSE;
        }
    }
    else if (reason == DLL_PROCESS_DETACH && !lpReserved) {
        LOG_INFO("DLL_PROCESS_DETACH - Stopping threads");
        DLLrunning = false; // Threads beenden
        MH_Uninitialize();  // MinHook freigeben

        // Optional: Auf Thread-Ende warten
        if (hOverlayThread) WaitForSingleObject(hOverlayThread, 3000);
        if (hMainThread) WaitForSingleObject(hMainThread, 3000);

        // Optional: Hook/Cleanup wie bei UniversalHookX
        H::Free();
        FreeConsole();

    }
    return TRUE;
}