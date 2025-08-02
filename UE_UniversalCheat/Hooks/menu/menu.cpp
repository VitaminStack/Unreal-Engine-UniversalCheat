#include "menu.hpp"

#include "../../imgui/imgui.h"
#include "../../imgui/imgui_impl_win32.h"
#include "../../Helper/Cheese.h"
#include "../../Helper/Helper.h"
#include "../../ImGui/imgui_internal.h"
#include <atomic>
#include <thread>
#include <chrono>
#include <mutex>

namespace ig = ImGui;

enum class ActorArrayOffset {
    PrimaryActors = 0x98,
    SecondaryActors = 0xA8
};

static std::atomic<int> ValidEntsLevel{0};
static std::atomic<int> AllEntsLevel{0};

static std::atomic<int> selectedLevelIndex{0};
static std::atomic<ActorArrayOffset> selectedArrayOffset{ActorArrayOffset::PrimaryActors};
static std::atomic<bool> PawnFilterEnabled{true};
static std::atomic<float> Distcap{3000.0f};
static std::atomic<int> ScreenW{0};
static std::atomic<int> ScreenH{0};

static std::atomic<bool> runEntityThread{true};
static std::thread entityThread;

static void EntityUpdateLoop()
{
    Cam gameCam;
    int scanCounter = 0;
    constexpr int scanInterval = 20;
    while (runEntityThread)
    {
        SDK::UEngine* Engine = SDK::UEngine::GetEngine();
        SDK::UWorld* World = SDK::UWorld::GetWorld();
        if (!World && Engine && Engine->GameViewport)
            World = Engine->GameViewport->World;
        SDK::APlayerController* MyController = nullptr;
        bool worldValid = SimpleESP::IsWorldValid(World);
        if (worldValid)
            MyController = World->OwningGameInstance->LocalPlayers[0]->PlayerController;

        if (worldValid && MyController)
        {
            int lvlIdx = selectedLevelIndex.load();
            SDK::ULevel* CurrentLevel = World->Levels.IsValidIndex(lvlIdx) ? World->Levels[lvlIdx] : nullptr;
            if (CurrentLevel)
            {
                ActorArrayOffset arrOff = selectedArrayOffset.load();
                auto* ActorArray = reinterpret_cast<SDK::TArray<SDK::AActor*>*>(
                    reinterpret_cast<std::uintptr_t>(CurrentLevel) + static_cast<std::uintptr_t>(arrOff));
                if (PointerChecks::IsValidPtr(ActorArray, "ActorArray") && ActorArray->IsValid())
                {
                    gameCam.UpdateCam(MyController->PlayerCameraManager);

                    if (++scanCounter % scanInterval == 0)
                    {
                        AllEntsLevel = ActorArray->Num();
                        for (int i = 0; i < ActorArray->Num(); ++i)
                        {
                            if (SDK::AActor* a = (*ActorArray)[i])
                            {
                                if (!PointerChecks::IsValidPtr(a, "AActor") ||
                                    !PointerChecks::IsValidPtr(a->Class, "ActorClass"))
                                    continue;

                                if (PawnFilterEnabled.load() && !a->IsA(SDK::APawn::StaticClass()))
                                    continue;

                                g_EntityCache.Add(a);
                            }
                        }
                    }

                    g_EntityCache.Refresh(gameCam.CamPos,
                        gameCam.Rotation,
                        gameCam.Fov,
                        ScreenW.load(),
                        ScreenH.load(),
                        Distcap.load(),
                        PawnFilterEnabled.load());

                    ValidEntsLevel = static_cast<int>(g_EntityCache.DrawList().size());
                }
            }
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }
}

namespace Menu {
    void InitializeContext(HWND hwnd) {
        if (ig::GetCurrentContext( ))
            return;

        ImGui::CreateContext( );
        ImGui_ImplWin32_Init(hwnd);

        ImGuiIO& io = ImGui::GetIO( );
        io.IniFilename = io.LogFilename = nullptr;
    }

    void RenderGameMenu()
    {
        if (!bShowMenu)
            return;

        ImGuiIO& IO = ImGui::GetIO();
        ImDrawList* drawlist = ImGui::GetForegroundDrawList();
        ScreenW = static_cast<int>(IO.DisplaySize.x);
        ScreenH = static_cast<int>(IO.DisplaySize.y);

        static std::once_flag threadFlag;
        std::call_once(threadFlag, []() {
            entityThread = std::thread(EntityUpdateLoop);
            entityThread.detach();
        });

        // Prüfe Engine, World, Controller
        SDK::UEngine* Engine = SDK::UEngine::GetEngine();
        SDK::UWorld* World = SDK::UWorld::GetWorld();
        if (!World && Engine && Engine->GameViewport)
            World = Engine->GameViewport->World;
        SDK::APlayerController* MyController = nullptr;
        bool worldValid = SimpleESP::IsWorldValid(World);

        if (worldValid)
            MyController = World->OwningGameInstance->LocalPlayers[0]->PlayerController;

        // Level-Daten immer neu befüllen, falls World da ist
        static std::vector<SDK::ULevel*> LevelArr;
        static std::vector<std::string> levelNames;
        LevelArr.clear();
        levelNames.clear();

        if (worldValid) {

            for (int i = 0; i < World->Levels.Num(); i++) {
                SDK::ULevel* Level = World->Levels[i];
                if (Level) {
                    LevelArr.push_back(Level);
                    levelNames.push_back(Level->GetFullName());
                }
            }
        }

        // Cheats initialisieren (nur falls MyController vorhanden)
        static bool initialized = false;
        if (!initialized && MyController) {
            Cheese::Initialize(MyController);
            Cheese::ActivateCheese("Godmode", false);
            Cheese::ActivateCheese("Flyhack", false);
            Cheese::ActivateCheese("Unl Ammo", false);
            Cheese::ActivateCheese("No Spread", false);
            Cheese::ActivateCheese("Inf Durability", false);

            initialized = true;
        }



        ImGui::Begin("MainMenu");

        // FEHLERANZEIGE bei fehlender World/Controller
        if (!World)
            ImGui::TextColored(ImVec4(1, 0, 0, 1), "[!] UWorld nicht gefunden!");
        if (!worldValid)
            ImGui::TextColored(ImVec4(1, 0.5, 0, 1), "[!] World->OwningGameInstance oder LocalPlayers invalid!");
        if (worldValid && !MyController)
            ImGui::TextColored(ImVec4(1, 1, 0, 1), "[!] PlayerController nicht gefunden!");

        ImGui::Text("%.3f ms/frame (%.1f FPS)", 1000.0f / IO.Framerate, IO.Framerate);

        float distTemp = Distcap.load();
        if (ImGui::SliderFloat("ESP Distance", &distTemp, 2.f, 20000.0f))
            Distcap = distTemp;

        

        // Level-Auswahl nur bei gültigen Daten
        int selIdx = selectedLevelIndex.load();
        if (worldValid && LevelArr.size() > 0 && selIdx < static_cast<int>(LevelArr.size())) {
            if (ImGui::BeginCombo("Select Level", levelNames[selIdx].c_str())) {
                for (int i = 0; i < levelNames.size(); i++) {
                    bool isSelected = (selIdx == i);
                    if (ImGui::Selectable(levelNames[i].c_str(), isSelected))
                        selectedLevelIndex = i;
                    if (isSelected)
                        ImGui::SetItemDefaultFocus();
                }
                ImGui::EndCombo();
            }
        }

        // Actor Array Auswahl (immer anzeigen)
        const char* arrayOptions[] = { "Primary Actors (0x98)", "Secondary Actors (0xA8)" };
        ActorArrayOffset arrOff = selectedArrayOffset.load();
        int currentSelection = (arrOff == ActorArrayOffset::PrimaryActors) ? 0 : 1;
        if (ImGui::BeginCombo("Select Actor Array", arrayOptions[currentSelection])) {
            for (int i = 0; i < IM_ARRAYSIZE(arrayOptions); i++) {
                bool isSelected = (currentSelection == i);
                if (ImGui::Selectable(arrayOptions[i], isSelected))
                    selectedArrayOffset = (i == 0) ? ActorArrayOffset::PrimaryActors : ActorArrayOffset::SecondaryActors;
                if (isSelected)
                    ImGui::SetItemDefaultFocus();
            }
            ImGui::EndCombo();
        }

        ImGui::Text("All Entities: %d", AllEntsLevel.load());
        ImGui::Text("Valid Entities: %d", ValidEntsLevel.load());
        bool pawnTemp = PawnFilterEnabled.load();
        if (ImGui::Checkbox("Render only Pawns", &pawnTemp))
            PawnFilterEnabled = pawnTemp;

        // Cheats anzeigen, aber nur togglen wenn initialisiert
        for (auto& cheat : Cheese::GetCheeseList()) {
            bool old = cheat.Enabled;
            if (ImGui::Checkbox(cheat.Name.c_str(), &cheat.Enabled) && initialized)
                cheat.ToggleAction(cheat.Enabled);
            else
                cheat.Enabled = old; // falls nicht initialisiert, Wert zurücksetzen
        }
        ImGui::End();

        // ESP nur bei validen Daten
        if (worldValid && MyController)
        {
            auto& dynList = g_EntityCache.DrawList();
            auto& statList = g_EntityCache.StaticDrawList();

            for (size_t i = 0; i < dynList.size(); ++i)
            {
                const auto& dyn = dynList[i];
                const auto* st = statList[i];

                std::string distanceText = std::to_string(dyn.distance);
                distanceText = distanceText.substr(0, distanceText.find('.') + 2);
                drawlist->AddText(ImVec2(CalcMiddlePos(dyn.screenPos.x, st->label.c_str()), dyn.screenPos.y), st->color, st->label.c_str());
                drawlist->AddText(ImVec2(CalcMiddlePos(dyn.screenPos.x, (distanceText + "m").c_str()), dyn.screenPos.y + 15), st->color, (distanceText + "m").c_str());
            }

            // restliche Features (Cheats etc.)
            Cheese::ApplyCheese();
        }
    }

    void RenderDebugMenu()
    {
        ImGui::Begin("DebugMenu");
        ImGui::Text("Overlay running");
        ImGui::End();
    }

    void Render()
    {
        RenderGameMenu();
    }

} // namespace Menu
