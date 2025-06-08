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
    printf("[Konsole] erfolgreich gestartet!\n");
}


DWORD WINAPI OverlayThread(LPVOID lpParameter)
{
    LOG_INFO("OverlayThread gestartet (TID: %d)", GetCurrentThreadId());

    if (ImGUIDiscordRender)
    {
        try {
            LOG_INFO("ImGUI-Discord-Render-Overlay wird initialisiert");
            ImGuiOverlay overlay(800, 600, 1234, true); // Letztes Argument aktiviert Debugging

            while (DLLrunning) { // WICHTIG: auf DLLrunning prüfen!
                overlay.Render();
                overlay.SendToOverlay();
                Sleep(16); // 60 FPS
            }
            LOG_INFO("ImGUIDiscordRender beendet.");
        }
        catch (const std::exception& e) {
            LOG_ERROR("Fehler im ImGUIDiscordRender: %s", e.what());
        }
    }
    else if (DiscordRender)
    {
        LOG_INFO("Suche nach Zielprozess für DiscordRender...");
        const char* targetExe = "SCUM.exe";

        DWORD targetProcessId = GetProcessIDByName(targetExe);
        if (!targetProcessId) {
            LOG_ERROR("Process not found: %s", targetExe);
            return -1;
        }
        LOG_INFO("Gefundene Prozess ID: %u", targetProcessId);

        // Verbindung zu OverlayCord herstellen
        LOG_INFO("Verbindung zum OverlayCord Backend wird aufgebaut...");
        OverlayCord::Communication::ConnectedProcessInfo processInfo = { 0 };
        processInfo.ProcessId = targetProcessId;

        if (!OverlayCord::Communication::ConnectToProcess(processInfo)) {
            LOG_ERROR("Fehler: Verbindung zum Overlay Backend fehlgeschlagen");
            return -1;
        }

        LOG_INFO("Erfolgreich verbunden mit mapped address: 0x%p", processInfo.MappedAddress);

        // **Render Loop starten** – sollte intern auch auf DLLrunning prüfen!
        while (DLLrunning) {
            StartRenderLoop(processInfo); // Passe ggf. die interne Loop an!
            Sleep(10); // Notfall-Fallback
        }

        // Verbindung trennen, wenn der Render-Loop beendet ist
        LOG_INFO("Render-Loop beendet, trenne Verbindung...");
        OverlayCord::Communication::DisconnectFromProcess(processInfo);
        LOG_INFO("Verbindung getrennt");
    }
    else if (TransRender)
    {
        LOG_INFO("TransRender wird gestartet...");
        // Falls RenderOverlay() blockiert, baue eine Schleife drumherum!
        while (DLLrunning) {
            RenderOverlay();
            Sleep(10);
        }
        LOG_INFO("TransRender beendet.");
    }
    else if (HookRender)
    {
        LOG_INFO("HookRender wird gestartet...");
        if (U::GetRenderingBackend() == NONE) {
            LOG_INFO("[!] Looks like you forgot to set a backend. Will unload after pressing enter...");
            std::cin.get();
            return 0;
        }

        MH_Initialize();
        H::Init();

        // Idle/Loop, bis DLL entladen werden soll
        while (DLLrunning) {
            Sleep(50);
        }
        LOG_INFO("HookRender beendet.");
    }
    else
    {
        LOG_ERROR("Kein gültiges Render-Modul ausgewählt.");
        return -1;
    }

    LOG_INFO("OverlayThread wird beendet.");
    return 0;
}


DWORD MainThread(HMODULE Module)
{
    
    LOG_INFO("MainThread gestartet (TID: %d)", GetCurrentThreadId());

    if (MainHacking)
    {
        LOG_INFO("Starte MainHacking...");
        SDK::UEngine* Engine = SDK::UEngine::GetEngine();
        SDK::UWorld* World = SDK::UWorld::GetWorld();
        SDK::APlayerController* MyController = World->OwningGameInstance->LocalPlayers[0]->PlayerController;
        LOG_INFO("Engine gefunden: %s", Engine->ConsoleClass->GetFullName());

        for (int i = 0; i < SDK::UObject::GObjects->Num(); i++)
        {
            SDK::UObject* Obj = SDK::UObject::GObjects->GetByIndex(i);

            if (!Obj)
                continue;

            if (Obj->IsDefaultObject())
                continue;

            if (Obj->IsA(SDK::APawn::StaticClass()) || Obj->HasTypeFlag(SDK::EClassCastFlags::Pawn))
            {
                LOG_DEBUG("Pawn gefunden: %s", Obj->GetFullName());
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

        LOG_INFO("Passe InputSettings an...");
        SDK::UInputSettings::GetDefaultObj()->ConsoleKeys[0].KeyName = SDK::UKismetStringLibrary::Conv_StringToName(L"F2");

        SDK::UObject* NewObject = SDK::UGameplayStatics::SpawnObject(Engine->ConsoleClass, Engine->GameViewport);
        Engine->GameViewport->ViewportConsole = static_cast<SDK::UConsole*>(NewObject);
        LOG_INFO("MainHacking abgeschlossen.");
    }


    while (DLLrunning) {
        // Beenden der DLL durch Tastendruck
        if (GetAsyncKeyState(VK_END) & 1) {
            LOG_INFO("Beende Overlay & MainThread durch Tastendruck");
            DLLrunning = false;
            running = false;
            break;
        }

        Sleep(50);
    }

    LOG_INFO("MainThread wird beendet.");
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
        LOG_INFO("DLL_PROCESS_ATTACH - Threads werden gestartet");
        printf("[+] Rendering backend: %s\n", U::RenderingBackendToStr(RenderModule));

        if (U::GetRenderingBackend() == NONE) {
            LOG_ERROR("Kein Rendering-Backend gesetzt. DLL wird nicht weiter ausgeführt.");
            return FALSE;
        }

        // MinHook einmal initialisieren
        MH_Initialize();

        // Threads starten
        hOverlayThread = CreateThread(0, 0, (LPTHREAD_START_ROUTINE)OverlayThread, hModule, 0, 0);
        hMainThread = CreateThread(0, 0, (LPTHREAD_START_ROUTINE)MainThread, hModule, 0, 0);
    }
    else if (reason == DLL_PROCESS_DETACH && !lpReserved) {
        LOG_INFO("DLL_PROCESS_DETACH - Threads werden beendet");
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