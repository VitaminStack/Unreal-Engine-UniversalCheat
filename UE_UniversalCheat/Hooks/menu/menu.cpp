#include "menu.hpp"

#include "../../imgui/imgui.h"
#include "../../imgui/imgui_impl_win32.h"
#include "../../Helper/Cheese.h"
#include "../../Helper/Helper.h"
#include "../../ImGui/imgui_internal.h"

namespace ig = ImGui;

int ValidEntsLevel;
int AllEntsLevel;
static int selectedLevelIndex = 0;
static std::vector<std::string> levelNames;
static bool PawnFilterEnabled = true;

namespace Menu {
    void InitializeContext(HWND hwnd) {
        if (ig::GetCurrentContext( ))
            return;

        ImGui::CreateContext( );
        ImGui_ImplWin32_Init(hwnd);

        ImGuiIO& io = ImGui::GetIO( );
        io.IniFilename = io.LogFilename = nullptr;
    }

    void Render()
    {
        if (!bShowMenu)
            return;

        ImGuiIO& IO = ImGui::GetIO();
        ImDrawList* drawlist = ImGui::GetForegroundDrawList();

        // Prüfe Engine, World, Controller
        SDK::UEngine* Engine = SDK::UEngine::GetEngine();
        SDK::UWorld* World = SDK::UWorld::GetWorld();
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
            Cheese::ActivateCheese("Godmode", true);
            Cheese::ActivateCheese("Flyhack", false);
            initialized = true;
        }



        enum class ActorArrayOffset {
            PrimaryActors = 0x98,
            SecondaryActors = 0xA8
        };
        
        ImGui::Begin("MainMenu");

        // FEHLERANZEIGE bei fehlender World/Controller
        if (!World)
            ImGui::TextColored(ImVec4(1, 0, 0, 1), "[!] UWorld nicht gefunden!");
        if (!worldValid)
            ImGui::TextColored(ImVec4(1, 0.5, 0, 1), "[!] World->OwningGameInstance oder LocalPlayers invalid!");
        if (worldValid && !MyController)
            ImGui::TextColored(ImVec4(1, 1, 0, 1), "[!] PlayerController nicht gefunden!");
        static ActorArrayOffset selectedArrayOffset = ActorArrayOffset::PrimaryActors;

        

        // FEHLERANZEIGE bei fehlender World/Controller
        if (!World)
            ImGui::TextColored(ImVec4(1, 0, 0, 1), "[!] UWorld nicht gefunden!");
        if (!worldValid)
            ImGui::TextColored(ImVec4(1, 0.5, 0, 1), "[!] World->OwningGameInstance oder LocalPlayers invalid!");
        if (worldValid && !MyController)
            ImGui::TextColored(ImVec4(1, 1, 0, 1), "[!] PlayerController nicht gefunden!");

        ImGui::Text("%.3f ms/frame (%.1f FPS)", 1000.0f / IO.Framerate, IO.Framerate);

        static float fps = 50.0f;
        static float Distcap = 3000.0f;
        ImGui::SliderFloat("FPS", &fps, 1.f, 170.f);
        ImGui::SliderFloat("ESP Distance", &Distcap, 2.f, 20000.0f);

        

        // Level-Auswahl nur bei gültigen Daten
        if (worldValid && LevelArr.size() > 0 && selectedLevelIndex < LevelArr.size()) {
            if (ImGui::BeginCombo("Select Level", levelNames[selectedLevelIndex].c_str())) {
                for (int i = 0; i < levelNames.size(); i++) {
                    bool isSelected = (selectedLevelIndex == i);
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
        int currentSelection = (selectedArrayOffset == ActorArrayOffset::PrimaryActors) ? 0 : 1;
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

        ImGui::Text("All Entities: %d", AllEntsLevel);
        ImGui::Text("Valid Entities: %d", ValidEntsLevel);
        ImGui::Checkbox("Render only Pawns", &PawnFilterEnabled);

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
        if (worldValid && MyController && LevelArr.size() > 0 && selectedLevelIndex < LevelArr.size()) {
            SDK::ULevel* CurrentLevel = LevelArr[selectedLevelIndex];
            SDK::TArray<SDK::AActor*>* ActiveActorArray = reinterpret_cast<SDK::TArray<SDK::AActor*>*>(
                reinterpret_cast<uintptr_t>(CurrentLevel) + static_cast<uintptr_t>(selectedArrayOffset)
                );

            AllEntsLevel = 0;
            ValidEntsLevel = 0;
            static Cam gameCam;
            gameCam.UpdateCam(MyController->PlayerCameraManager);

            for (int i = 0; i < ActiveActorArray->Num(); i++) {
                SDK::AActor* Actor = (*ActiveActorArray)[i];
                if (Actor) {
                    AllEntsLevel++;
                    if (SimpleESP::DrawActorESP(Actor, gameCam, drawlist, Distcap, PawnFilterEnabled)) {
                        ValidEntsLevel++;
                    }
                }
            }
            Cheese::ApplyCheese();
        }
    }

} // namespace Menu
