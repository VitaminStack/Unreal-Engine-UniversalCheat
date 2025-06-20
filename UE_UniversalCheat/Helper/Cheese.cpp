﻿#include "Cheese.h"

// Static Members (jetzt exakt wie in Cheese.h!)
std::vector<CheeseOption> Cheese::CheeseList;
bool Cheese::FlyhackActive = false;


// 1. Cheese initialisieren
void Cheese::Initialize(SDK::APlayerController* Controller) {
    CheeseList.clear();
    const SDK::EMovementMode DefaultMovementMode = SDK::EMovementMode::MOVE_Walking;

    // PointerChecks wie gehabt
    if (!PointerChecks::IsValidPtr(Controller, "Controller")) {
        printf("[Cheese] Controller pointer invalid - aborting initialization\n");
        return;
    }

    // Godmode
    CheeseList.emplace_back(
        "Godmode",
        [Controller](bool enable) {
            if (!PointerChecks::IsValidPtr(Controller, "Controller")) return;
            if (!PointerChecks::IsValidPtr(Controller->Character, "Controller->Character")) return;
            __try {
                Controller->Character->bCanBeDamaged = !enable;
                printf("[GODMODE_SIMPLE] %s successfully\n", enable ? "Enabled" : "Disabled");
            }
            __except (EXCEPTION_EXECUTE_HANDLER) {
                printf("[GODMODE_SIMPLE] Exception occurred during assignment\n");
            }
        },
        [Controller]() {
            if (!PointerChecks::IsValidPtr(Controller, "Controller")) return;
            if (!PointerChecks::IsValidPtr(Controller->Character, "Controller->Character")) return;
            __try {
                Controller->Character->bCanBeDamaged = true;
                printf("[GODMODE_SIMPLE] Reset to default successfully\n");
            }
            __except (EXCEPTION_EXECUTE_HANDLER) {
                printf("[GODMODE_SIMPLE] Exception occurred during reset\n");
            }
        }
    );

    // Flyhack
    CheeseList.emplace_back(
        "Flyhack",
        [Controller, DefaultMovementMode](bool enable) {
            if (!PointerChecks::IsValidPtr(Controller, "Controller")) return;
            if (!PointerChecks::IsValidPtr(Controller->Character, "Controller->Character")) return;
            if (!PointerChecks::IsValidPtr(Controller->Character->CharacterMovement, "CharacterMovement")) return;

            SDK::UCharacterMovementComponent* Movement = Controller->Character->CharacterMovement;

            __try {
                if (enable) {
                    Movement->SetMovementMode(SDK::EMovementMode::MOVE_Flying, 5);
                    FlyhackActive = true;
                    printf("[FLYHACK] Enabled successfully\n");
                }
                else {
                    Movement->SetMovementMode(DefaultMovementMode, 0);
                    FlyhackActive = false;
                    printf("[FLYHACK] Disabled successfully\n");
                }
            }
            __except (EXCEPTION_EXECUTE_HANDLER) {
                printf("[FLYHACK] Exception occurred during SetMovementMode\n");
            }
        },
        [Controller, DefaultMovementMode]() {
            if (!PointerChecks::IsValidPtr(Controller, "Controller")) return;
            if (!PointerChecks::IsValidPtr(Controller->Character, "Controller->Character")) return;
            if (!PointerChecks::IsValidPtr(Controller->Character->CharacterMovement, "CharacterMovement")) return;
            __try {
                Controller->Character->CharacterMovement->SetMovementMode(DefaultMovementMode, 0);
                FlyhackActive = false;
                printf("[FLYHACK] Reset to default successfully\n");
            }
            __except (EXCEPTION_EXECUTE_HANDLER) {
                printf("[FLYHACK] Exception occurred during reset\n");
            }
        }
    );

    CheeseList.emplace_back(
        "Unl Ammo",
        [Controller](bool enable)
        {
            if (!PointerChecks::IsValidPtr(Controller, "Controller") ||
                !PointerChecks::IsValidPtr(Controller->Character, "Character"))
                return;

            uintptr_t itemInHands = 0;
            uintptr_t itemPtrAddr = reinterpret_cast<uintptr_t>(Controller->Character) + 0x17D8;
            if (!PointerChecks::SafeRead(reinterpret_cast<void*>(itemPtrAddr), itemInHands, "ItemInHands"))
                return;

            uintptr_t requiredAmmoAddr = itemInHands + 0xAF8;
            PointerChecks::SafeWrite(reinterpret_cast<void*>(requiredAmmoAddr),
                enable ? 0 : 1, "RequiredAmmoFlag");
        },
        [Controller]()        // reset
        {
            if (!PointerChecks::IsValidPtr(Controller, "Controller") ||
                !PointerChecks::IsValidPtr(Controller->Character, "Character"))
                return;

            uintptr_t itemInHands = 0;
            uintptr_t itemPtrAddr = reinterpret_cast<uintptr_t>(Controller->Character) + 0x17D8;
            if (!PointerChecks::SafeRead(reinterpret_cast<void*>(itemPtrAddr), itemInHands, "ItemInHands"))
                return;

            uintptr_t requiredAmmoAddr = itemInHands + 0xAF8;
            PointerChecks::SafeWrite(reinterpret_cast<void*>(requiredAmmoAddr),
                1, "RequiredAmmoFlag Reset");
        });


    // ---------------------------------------------------------------------------
    // 2) No-Spread  – separater Cheat
    // ---------------------------------------------------------------------------
    static byte g_savedSpread = 1;      // behält Originalwert
    CheeseList.emplace_back(
        "No Spread",
        [Controller](bool enable)
        {
            if (!PointerChecks::IsValidPtr(Controller, "Controller") ||
                !PointerChecks::IsValidPtr(Controller->Character, "Character"))
                return;

            uintptr_t itemInHands = 0;
            uintptr_t itemPtrAddr = reinterpret_cast<uintptr_t>(Controller->Character) + 0x17D8;
            if (!PointerChecks::SafeRead(reinterpret_cast<void*>(itemPtrAddr), itemInHands, "ItemInHands"))
                return;

            uintptr_t spreadAddr = itemInHands + 0x1381;

            if (enable)
            {
                PointerChecks::SafeRead(reinterpret_cast<void*>(spreadAddr),
                    g_savedSpread, "SaveSpread");
                PointerChecks::SafeWrite(reinterpret_cast<void*>(spreadAddr),
                    static_cast<byte>(0), "DisableSpread");
            }
            else               // Toggle auf OFF ⇒ alten Wert zurück
            {
                PointerChecks::SafeWrite(reinterpret_cast<void*>(spreadAddr),
                    g_savedSpread, "RestoreSpread");
            }
        },
        [Controller]()         // Reset beim Deaktivieren aller Cheats
        {
            // einfach das Toggle-OFF auslösen → alter Wert wird zurückgeschrieben
        });


    // ---------------------------------------------------------------------------
    // 3) Infinite Durability  – separater Cheat
    // ---------------------------------------------------------------------------
    static float g_savedDurability = 1.f;
    CheeseList.emplace_back(
        "Inf Durability",
        [Controller](bool enable)
        {
            if (!PointerChecks::IsValidPtr(Controller, "Controller") ||
                !PointerChecks::IsValidPtr(Controller->Character, "Character"))
                return;

            uintptr_t itemInHands = 0;
            uintptr_t itemPtrAddr = reinterpret_cast<uintptr_t>(Controller->Character) + 0x17D8;
            if (!PointerChecks::SafeRead(reinterpret_cast<void*>(itemPtrAddr), itemInHands, "ItemInHands"))
                return;

            uintptr_t durAddr = itemInHands + 0x13F4;

            if (enable)
            {
                PointerChecks::SafeRead(reinterpret_cast<void*>(durAddr),
                    g_savedDurability, "SaveDurability");
                PointerChecks::SafeWrite(reinterpret_cast<void*>(durAddr),
                    0.0f, "SetDurabilityZero");
            }
            else
            {
                PointerChecks::SafeWrite(reinterpret_cast<void*>(durAddr),
                    g_savedDurability, "RestoreDurability");
            }
        },
        [Controller]()         // Reset
        {
            // Toggle-OFF genügt, daher leer
        });
  

    printf("[Cheese] Initialization completed with %zu Cheese loaded\n", CheeseList.size());
}

// 2. Cheese anwenden
void Cheese::ApplyCheese() {
    SDK::UEngine* Engine = SDK::UEngine::GetEngine();
    if (!PointerChecks::IsValidPtr(Engine, "UEngine")) return;

    SDK::UWorld* World = SDK::UWorld::GetWorld();
    if (!PointerChecks::IsValidPtr(World, "UWorld")) return;

    if (!PointerChecks::IsValidPtr(World->OwningGameInstance, "OwningGameInstance")) return;

    auto& LocalPlayers = World->OwningGameInstance->LocalPlayers;
    if (LocalPlayers.Num() <= 0 || !PointerChecks::IsValidPtr(LocalPlayers[0], "LocalPlayer[0]")) return;

    SDK::APlayerController* Controller = LocalPlayers[0]->PlayerController;
    if (!PointerChecks::IsValidPtr(Controller, "PlayerController")) return;

    for (auto& cheese : CheeseList) {
        if (cheese.Enabled != cheese.LastState) {
            cheese.ToggleAction(cheese.Enabled);
            cheese.LastState = cheese.Enabled;
        }
    }
    if (FlyhackActive) {
        FlyhackControl(Controller);
    }
}

// 3. Reset-Funktion
void Cheese::ResetCheese() {
    for (auto& cheese : CheeseList) {
        if (cheese.Enabled) {
            cheese.ResetAction();
            cheese.Enabled = false;
            cheese.LastState = false;
        }
    }
}

// 4. Zugriff auf Cheese-Liste
std::vector<CheeseOption>& Cheese::GetCheeseList() {
    return CheeseList;
}

// 5. Manuelles Aktivieren eines Cheese
void Cheese::ActivateCheese(const std::string& cheeseName, bool enable) {
    for (auto& cheese : CheeseList) {
        if (cheese.Name == cheeseName) {
            cheese.Enabled = enable;
            cheese.ToggleAction(enable);
            cheese.LastState = enable;
            break;
        }
    }
}

// 6. Flyhack-Steuerung
void Cheese::FlyhackControl(SDK::APlayerController* Controller) {
    if (!PointerChecks::IsValidPtr(Controller, "Controller")) return;
    if (!PointerChecks::IsValidPtr(Controller->Character, "Controller->Character")) return;
    if (!PointerChecks::IsValidPtr(Controller->Character->CharacterMovement, "CharacterMovement")) return;

    SDK::UCharacterMovementComponent* Movement = Controller->Character->CharacterMovement;
    SDK::FVector Velocity = Movement->Velocity;
    float Speed = 600.0f;
    float BoostMultiplier = 6.0f;

    if (GetAsyncKeyState(VK_SHIFT) & 0x8000) {
        if (!PointerChecks::IsValidPtr(Controller->PlayerCameraManager, "PlayerCameraManager")) return;
        SDK::FVector ForwardVector = Controller->PlayerCameraManager->GetActorForwardVector();
        Velocity += ForwardVector * Speed * BoostMultiplier * 0.056f;
    }
    if (GetAsyncKeyState(VK_SPACE) & 0x8000) {
        Velocity.Z += Speed * 0.056f;
    }
    if (GetAsyncKeyState(VK_CONTROL) & 0x8000) {
        Velocity.Z -= Speed * 0.036f;
    }
    Movement->Velocity = Velocity;
}
