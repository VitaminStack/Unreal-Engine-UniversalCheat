#include "Cheese.h"

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
        "Godmode_Simple",
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

    // Unlimited Ammo
    CheeseList.emplace_back(
        "Unl Ammo",
        [Controller](bool enable) {
            if (!PointerChecks::IsValidPtr(Controller, "Controller")) return;
            if (!PointerChecks::IsValidPtr(Controller->Character, "Controller->Character")) return;

            uintptr_t characterBase = reinterpret_cast<uintptr_t>(Controller->Character);
            uintptr_t itemInHandsPtrAddress = characterBase + 0x17D8;

            if (!PointerChecks::IsReadable(reinterpret_cast<void*>(itemInHandsPtrAddress), sizeof(uintptr_t))) {
                printf("[UNL_AMMO] ItemInHands pointer address not readable\n");
                return;
            }

            uintptr_t itemInHandsPtr;
            if (!PointerChecks::SafeRead(reinterpret_cast<void*>(itemInHandsPtrAddress), itemInHandsPtr, "ItemInHands Pointer")) {
                return;
            }

            if (!itemInHandsPtr) {
                printf("[UNL_AMMO] ItemInHands is null - no weapon equipped\n");
                return;
            }

            if (!PointerChecks::IsReadable(reinterpret_cast<void*>(itemInHandsPtr), 0x1400)) {
                printf("[UNL_AMMO] ItemInHands object not fully readable\n");
                return;
            }

            uintptr_t requiredAmmoAddress = itemInHandsPtr + 0xAF8;
            uintptr_t spreadAddress = itemInHandsPtr + 0x1381;
            uintptr_t durabilityAddress = itemInHandsPtr + 0x13f4;

            if (PointerChecks::SafeWrite(reinterpret_cast<void*>(requiredAmmoAddress),
                enable ? 0 : 1, "RequiredAmmoFlag")) {
                printf("[UNL_AMMO] RequiredAmmo flag %s\n", enable ? "disabled" : "enabled");
            }

            if (PointerChecks::SafeWrite(reinterpret_cast<void*>(spreadAddress),
                static_cast<byte>(enable ? 0 : 1), "Spread")) {
                printf("[UNL_AMMO] Spread %s\n", enable ? "disabled" : "enabled");
            }

            if (enable) {
                if (PointerChecks::SafeWrite(reinterpret_cast<void*>(durabilityAddress),
                    0.0f, "Durability")) {
                    printf("[UNL_AMMO] Durability set to 0.0\n");
                }
            }
        },
        [Controller]() {
            if (!PointerChecks::IsValidPtr(Controller, "Controller")) return;
            if (!PointerChecks::IsValidPtr(Controller->Character, "Controller->Character")) return;

            uintptr_t characterBase = reinterpret_cast<uintptr_t>(Controller->Character);
            uintptr_t itemInHandsPtrAddress = characterBase + 0x17D8;

            uintptr_t itemInHandsPtr;
            if (!PointerChecks::SafeRead(reinterpret_cast<void*>(itemInHandsPtrAddress), itemInHandsPtr, "ItemInHands Pointer Reset")) {
                return;
            }

            if (!itemInHandsPtr) {
                printf("[UNL_AMMO] Reset: ItemInHands is null\n");
                return;
            }

            uintptr_t requiredAmmoAddress = itemInHandsPtr + 0xAF8;
            if (PointerChecks::SafeWrite(reinterpret_cast<void*>(requiredAmmoAddress), 1, "RequiredAmmoFlag Reset")) {
                printf("[UNL_AMMO] Reset to default successfully\n");
            }
        }
    );

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
