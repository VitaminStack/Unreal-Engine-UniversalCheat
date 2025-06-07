#include <Windows.h>
#include <iostream>

#include "SDK/Engine_classes.hpp"
#include "OverlayCord.h"
#include "Overlay.h"
#include "DXHook.h"

// Basic.cpp was added to the VS project
// Engine_functions.cpp was added to the VS project

bool DLLrunning = true;
bool MainHacking = false;

bool ImGUIDiscordRender = false;
bool DiscordRender = false;
bool TransRender = false;
bool DXRender = true;

DX12Hook* g_pDXHookInstance = nullptr;

void OpenConsole() {
    /* Code to open a console window */
    AllocConsole();
    FILE* Dummy;
    freopen_s(&Dummy, "CONOUT$", "w", stdout);
    freopen_s(&Dummy, "CONIN$", "r", stdin);
}

DWORD WINAPI OverlayThread(LPVOID lpParameter)
{
    if (ImGUIDiscordRender)
    {
        try {
            ImGuiOverlay overlay(800, 600, 1234, true); // Letztes Argument aktiviert Debugging

            while (true) {
                overlay.Render();
                overlay.SendToOverlay();
                Sleep(16); // 60 FPS
            }
        }
        catch (const std::exception& e) {
            printf("Fehler: %s\n", e.what());
        }

    }
    else if (DiscordRender)
    {
        printf("[>] Searching for target window...\n");
        const char* targetExe = "SCUM.exe";

        DWORD targetProcessId = GetProcessIDByName(targetExe);
        if (!targetProcessId) {
            printf("[!] Process not found: %s\n", targetExe);
            return -1;
        }
        printf("[+] Found process ID: %u\n", targetProcessId);

        // Verbindung zu OverlayCord herstellen
        printf("[>] Connecting to process overlay backend...\n");
        OverlayCord::Communication::ConnectedProcessInfo processInfo = { 0 };
        processInfo.ProcessId = targetProcessId;

        if (!OverlayCord::Communication::ConnectToProcess(processInfo)) {
            printf("[!] Failed to connect to overlay backend\n");
            return -1;
        }

        printf("[+] Connected with mapped address: 0x%p\n", processInfo.MappedAddress);

        // **Render Loop starten**
        StartRenderLoop(processInfo);

        // Verbindung trennen, wenn der Render-Loop beendet ist
        printf("[>] Cleaning up...\n");
        OverlayCord::Communication::DisconnectFromProcess(processInfo);
        printf("[+] Disconnected\n");

    }
    else if (TransRender)
    {
        RenderOverlay();
    }
    else if (DXRender)
    {
        // Neuen Hook-Context global verfügbar machen
        static DX12Hook* pDX12Hook = nullptr;
        if (!pDX12Hook) {
            pDX12Hook = new DX12Hook();
            if (!pDX12Hook->Init()) {
                printf("[DX12Hook] DX12 initialization failed!\n");
                delete pDX12Hook;
                pDX12Hook = nullptr;
                return -1;
            }
            printf("[DX12Hook] Hook successfully initialized!\n");
        }

        // Hauptloop, läuft bis DLLrunning == false
        while (DLLrunning) {
            Sleep(100);
        }

        // Cleanup & Unhook
        if (pDX12Hook) {
            pDX12Hook->Shutdown();
            delete pDX12Hook;
            pDX12Hook = nullptr;
            printf("[DX12Hook] Shutdown & Unhooked.\n");
        }
    }



    return 0;
}

DWORD MainThread(HMODULE Module)
{
    OpenConsole();


    if (MainHacking)
    {
        SDK::UEngine* Engine = SDK::UEngine::GetEngine();
        SDK::UWorld* World = SDK::UWorld::GetWorld();
        SDK::APlayerController* MyController = World->OwningGameInstance->LocalPlayers[0]->PlayerController;
        std::cout << Engine->ConsoleClass->GetFullName() << std::endl;

        for (int i = 0; i < SDK::UObject::GObjects->Num(); i++)
        {
            SDK::UObject* Obj = SDK::UObject::GObjects->GetByIndex(i);

            if (!Obj)
                continue;

            if (Obj->IsDefaultObject())
                continue;

            if (Obj->IsA(SDK::APawn::StaticClass()) || Obj->HasTypeFlag(SDK::EClassCastFlags::Pawn))
            {
                std::cout << Obj->GetFullName() << "\n";
            }
        }

        SDK::ULevel* Level = World->PersistentLevel;
        SDK::TArray<SDK::AActor*>& Actors = Level->Actors;

        for (SDK::AActor* Actor : Actors)
        {
            if (!Actor || !Actor->IsA(SDK::EClassCastFlags::Pawn) || !Actor->IsA(SDK::APawn::StaticClass()))
                continue;

            SDK::APawn* Pawn = static_cast<SDK::APawn*>(Actor);

        }

        SDK::UInputSettings::GetDefaultObj()->ConsoleKeys[0].KeyName = SDK::UKismetStringLibrary::Conv_StringToName(L"F2");

        SDK::UObject* NewObject = SDK::UGameplayStatics::SpawnObject(Engine->ConsoleClass, Engine->GameViewport);
        Engine->GameViewport->ViewportConsole = static_cast<SDK::UConsole*>(NewObject);

    }


    while (DLLrunning) {




        // Beenden der DLL durch Tastendruck
        if (GetAsyncKeyState(VK_END) & 1) {
            printf("[INFO] Beende Overlay & MainThread...\n");
            DLLrunning = false;
            running = false;
            break;
        }

        Sleep(50);
    }

    Sleep(500);
    FreeConsole();
    FreeLibraryAndExitThread(Module, 0);
    return 0;
}

BOOL APIENTRY DllMain(HMODULE hModule, DWORD reason, LPVOID lpReserved)
{
    switch (reason)
    {
    case DLL_PROCESS_ATTACH:
        CreateThread(0, 0, (LPTHREAD_START_ROUTINE)OverlayThread, hModule, 0, 0);
        CreateThread(0, 0, (LPTHREAD_START_ROUTINE)MainThread, hModule, 0, 0);
        break;
    case DLL_PROCESS_DETACH:
        DLLrunning = false;  // Beende beide Threads
        break;
    }
    return TRUE;
}