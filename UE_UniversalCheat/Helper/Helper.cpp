#include "helper.h"


Vector3 Vector3::operator+(const Vector3& v) const {
    return Vector3(x + v.x, y + v.y, z + v.z);
}
Vector3 Vector3::operator-(const Vector3& v) const {
    return Vector3(x - v.x, y - v.y, z - v.z);
}
Vector3 Vector3::operator*(float scalar) const {
    return Vector3(x * scalar, y * scalar, z * scalar);
}
Vector3 Vector3::Cross(const Vector3& v) const {
    return Vector3(
        y * v.z - z * v.y,
        z * v.x - x * v.z,
        x * v.y - y * v.x
    );
}
float Vector3::Dot(const Vector3& v) const {
    return (x * v.x + y * v.y + z * v.z);
}
float Vector3::Length() const {
    return sqrtf(x * x + y * y + z * z);
}
float Vector3::DistTo(const Vector3& v) const {
    return (*this - v).Length();
}
Vector4 Vector4::Conjugate() const {
    return { -x, -y, -z, w };
}
Vector4 Vector4::operator*(const Vector4& other) const {
    return {
        w * other.x + x * other.w + y * other.z - z * other.y,
        w * other.y - x * other.z + y * other.w + z * other.x,
        w * other.z + x * other.y - y * other.x + z * other.w,
        w * other.w - x * other.x - y * other.y - z * other.z
    };
}


D3DXMATRIX ToMatrix(const Vector3& Rotation) {
    constexpr float DEG_TO_RAD = 3.1415926535897932f / 180.f;
    float radPitch = Rotation.x * DEG_TO_RAD;
    float radYaw = Rotation.y * DEG_TO_RAD;
    float radRoll = Rotation.z * DEG_TO_RAD;

    const float SP = sinf(radPitch), CP = cosf(radPitch);
    const float SY = sinf(radYaw), CY = cosf(radYaw);
    const float SR = sinf(radRoll), CR = cosf(radRoll);

    D3DMATRIX matrix = {};
    matrix.m[0][0] = CP * CY;
    matrix.m[0][1] = CP * SY;
    matrix.m[0][2] = SP;

    matrix.m[1][0] = SR * SP * CY - CR * SY;
    matrix.m[1][1] = SR * SP * SY + CR * CY;
    matrix.m[1][2] = -SR * CP;

    matrix.m[2][0] = -(CR * SP * CY + SR * SY);
    matrix.m[2][1] = CY * SR - CR * SP * SY;
    matrix.m[2][2] = CR * CP;

    matrix.m[3][3] = 1.f;
    return matrix;
}
bool UEWorldToScreen(const Vector3& worldLoc, Vector2& screenPos, Vector3 Rotation, Vector3 CamPos, float FOV, int screenHeight, int screenWidth) {
    static const float PI = 3.1415926535897932f;
    static float cachedFOV = -1.0f;
    static float fovFactor = 0.0f;

    if (cachedFOV != FOV) {  // FOV nur berechnen, wenn es sich ändert
        fovFactor = tanf(FOV * PI / 360.f);
        cachedFOV = FOV;
    }

    D3DMATRIX tempMatrix = ToMatrix(Rotation);
    Vector3 delta = worldLoc - CamPos;

    Vector3 transformed = {
        delta.Dot({ tempMatrix.m[1][0], tempMatrix.m[1][1], tempMatrix.m[1][2] }),
        delta.Dot({ tempMatrix.m[2][0], tempMatrix.m[2][1], tempMatrix.m[2][2] }),
        delta.Dot({ tempMatrix.m[0][0], tempMatrix.m[0][1], tempMatrix.m[0][2] })
    };

    if (transformed.z < 1.f)
        transformed.z = 1.f;

    const float screenCenterX = screenWidth / 2.0f;
    const float screenCenterY = screenHeight / 2.0f;

    const float scale = screenCenterX / fovFactor;

    screenPos.x = screenCenterX + (transformed.x * scale) / transformed.z;
    screenPos.y = screenCenterY - (transformed.y * scale) / transformed.z;

    return (screenPos.x >= 0 && screenPos.x <= screenWidth && screenPos.y >= 0 && screenPos.y <= screenHeight);
}



SDK::FName CreateFName(UC::FString Name) {
    SDK::FName NewName;
    NewName.ComparisonIndex = SDK::UKismetStringLibrary::Conv_StringToName(Name).ComparisonIndex;
    NewName.Number = 0;
    return NewName;
}
UC::FString ConvertToFString(const char* CharArray) {
    int size_needed = MultiByteToWideChar(CP_UTF8, 0, CharArray, -1, NULL, 0);
    std::wstring wideString(size_needed, 0);
    MultiByteToWideChar(CP_UTF8, 0, CharArray, -1, &wideString[0], size_needed);

    return UC::FString(wideString.c_str());
}


SDK::FVector SimpleESP::GetActorPosition(SDK::AActor* Actor) {
    if (!Actor || !Actor->RootComponent) return { 0, 0, 0 };
    return Actor->RootComponent->RelativeLocation;
}
bool SimpleESP::IsActorVisible(const Vector2& screenPos, int screenWidth, int screenHeight) {
    return screenPos.x >= 0 && screenPos.x <= screenWidth &&
        screenPos.y >= 0 && screenPos.y <= screenHeight;
}
bool SimpleESP::IsWorldValid(SDK::UWorld* World) {
    bool valid = false;
    __try {
        valid = World && World->OwningGameInstance && World->OwningGameInstance->LocalPlayers.Num() > 0;
    }
    __except (EXCEPTION_EXECUTE_HANDLER) {
        valid = false;
    }
    return valid;
}
void SimpleESP::RenderActorInfo(ImDrawList* drawlist, const Vector2& screenPos, ImU32 color, const std::string& ActorName, float Dist) {
    std::string distanceText = std::to_string(Dist);
    distanceText = distanceText.substr(0, distanceText.find('.') + 2);
    drawlist->AddText(ImVec2(CalcMiddlePos(screenPos.x, ActorName.c_str()), screenPos.y), color, ActorName.c_str());
    drawlist->AddText(ImVec2(CalcMiddlePos(screenPos.x, (distanceText + "m").c_str()), screenPos.y + 15), color, (distanceText + "m").c_str());
}
bool SimpleESP::DrawActorESP(
    SDK::AActor* Actor,
    const Cam& camera,
    ImDrawList* drawlist,
    float DistCap,
    bool onlyPawns
) {
    if (!Actor) return false;
    if (onlyPawns && !Actor->IsA(SDK::APawn::StaticClass())) return false;

    std::string ActorName = Actor->GetName();
    float fov = camera.Fov;
    SDK::FVector camPos = camera.CamPos;
    SDK::FRotator rotation = camera.Rotation;
    SDK::FVector actorPos = GetActorPosition(Actor);

    ImU32 color = IM_COL32(255, 255, 0, 255);
    if (!Actor->bActorEnableCollision) return false;

    SDK::APawn* Pawn = nullptr;
    if (Actor->IsA(SDK::APawn::StaticClass())) {
        Pawn = static_cast<SDK::APawn*>(Actor);
        std::string pName = Pawn->GetName();
        color = IM_COL32(255, 0, 0, 255);
        if (Pawn->Controller && Pawn->RootComponent) {
            actorPos = Pawn->RootComponent->RelativeLocation;
        }
    }

    if (actorPos.IsZero()) return false;

    Vector2 screenPos;
    if (!UEWorldToScreen(
        Vector3(actorPos.X, actorPos.Y, actorPos.Z), screenPos,
        Vector3(rotation.Pitch, rotation.Yaw, rotation.Roll),
        Vector3(camPos.X, camPos.Y, camPos.Z), fov, 1440, 2560)) {
        return false;
    }

    if (!IsActorVisible(screenPos, 2560, 1440)) return false;

    float distance = camPos.GetDistanceTo(actorPos) / 100.0f;

    if (distance > 2.f && distance < DistCap) {
        std::string aName = Actor->Class->GetName();
        RenderActorInfo(drawlist, screenPos, color, aName, distance);
        return true;
    }
    return false;
}
//void RenderAllEnts(
//    SDK::TArray<void*>* ActiveEntityArray, // Raw, weil manchmal kein reines TArray<AActor*>
//    const Cam& gameCam,
//    ImDrawList* drawlist,
//    float Distcap,
//    bool PawnFilterEnabled,
//    int& AllEntsLevel,
//    int& ValidEntsLevel)
//{
//    AllEntsLevel = 0;
//    ValidEntsLevel = 0;
//
//    for (int i = 0; i < ActiveEntityArray->Num(); i++) {
//        void* RawPtr = (*ActiveEntityArray)[i];
//        if (!RawPtr) continue;
//        AllEntsLevel++;
//
//        // Vorbereitung für Darstellung
//        std::string entityClass = "Unknown";
//        std::string entityName = "";
//        ImU32 color = IM_COL32(255, 255, 255, 220);
//        SDK::FVector actorPos{};
//        bool rendered = false;
//
//        __try {
//            SDK::AActor* Actor = reinterpret_cast<SDK::AActor*>(RawPtr);
//            if (Actor && Actor->IsA(SDK::AActor::StaticClass())) {
//                entityClass = "AActor";
//                color = IM_COL32(255, 255, 0, 220);
//
//                entityName = Actor->GetName();
//                if (entityName.empty()) entityName = "<no name>";
//
//                actorPos = GetActorPosition(Actor); // Stelle sicher, dass die Funktion sichtbar ist!
//                if (Actor->IsA(SDK::APawn::StaticClass())) {
//                    entityClass = "APawn";
//                    color = IM_COL32(255, 0, 0, 220);
//                }
//                else if (Actor->IsA(SDK::ACharacter::StaticClass())) {
//                    entityClass = "ACharacter";
//                    color = IM_COL32(0, 180, 255, 220);
//                }
//                else if (Actor->IsA(SDK::AStaticMeshActor::StaticClass())) {
//                    entityClass = "AStaticMeshActor";
//                    color = IM_COL32(0, 200, 255, 220);
//                }
//
//                // ESP zeichnen wie gehabt:
//                if (!actorPos.IsZero()) {
//                    Vector2 screenPos;
//                    if (UEWorldToScreen(
//                        Vector3(actorPos.X, actorPos.Y, actorPos.Z), screenPos,
//                        Vector3(gameCam.Rotation.Pitch, gameCam.Rotation.Yaw, gameCam.Rotation.Roll),
//                        Vector3(gameCam.CamPos.X, gameCam.CamPos.Y, gameCam.CamPos.Z),
//                        gameCam.Fov, 1440, 2560)) {
//                        float distance = gameCam.CamPos.GetDistanceTo(actorPos) / 100.0f;
//                        if (distance > 2.f && distance < Distcap) {
//                            char label[128];
//                            sprintf_s(label, "[%s] %s (%.1fm)", entityClass.c_str(), entityName.c_str(), distance);
//                            drawlist->AddText(ImVec2(screenPos.x, screenPos.y), color, label);
//                            rendered = true;
//                            ValidEntsLevel++;
//                        }
//                    }
//                }
//                // Wenn nicht sichtbar oder zu weit, nur Label
//                goto draw_label;
//            }
//
//
//        }
//        __except (EXCEPTION_EXECUTE_HANDLER) {
//            // Exception beim Zugriff – render fallback label unten!
//        }
//
//        // --- Fallback: Immer Adresse/Typ anzeigen ---
//    draw_label:
//        if (!rendered) {
//            // Zeige rohe Adresse
//            ImVec2 screenPos((float)(40 + (i % 20) * 60), (float)(100 + (i / 20) * 16));
//            char buf[128];
//            if (entityName.empty())
//                sprintf_s(buf, "%s: 0x%p", entityClass.c_str(), RawPtr);
//            else
//                sprintf_s(buf, "%s: %s [0x%p]", entityClass.c_str(), entityName.c_str(), RawPtr);
//            drawlist->AddText(screenPos, color, buf);
//        }
//    }
//}





static int selectedLevelIndex = 0;  // Aktuell ausgewählter Level
static std::vector<std::string> levelNames;  // Liste der Levelnamen
void LoadLevels(SDK::UWorld* World) {
    levelNames.clear();  // Vorherige Liste leeren

    if (World) {
        for (int i = 0; i < World->Levels.Num(); i++) {
            SDK::ULevel* Level = World->Levels[i];
            if (Level) {
                levelNames.push_back(Level->GetName());
            }
        }
    }
}
void PrintActorType(SDK::AActor* Actor)
{
    if (!Actor)
        return;  // Null-Pointer-Schutz

    std::string ActorName = Actor->GetName();
    std::string ActorType = "Unknown";  // Fallback, falls kein Typ erkennbar ist

    // 1️⃣ Prüfung: Ist es ein Pawn oder Controller?
    if (Actor->IsA(SDK::EClassCastFlags::Pawn)) {
        ActorType = "Pawn";
    }
    else if (Actor->IsA(SDK::EClassCastFlags::PlayerController)) {
        ActorType = "PlayerController";
    }

    // 2️⃣ Prüfung: Hat der Actor ein StaticMeshComponent?
    auto StaticMeshComp = reinterpret_cast<SDK::UStaticMeshComponent*>(Actor->GetComponentByClass(SDK::UStaticMeshComponent::StaticClass()));
    if (StaticMeshComp) {
        ActorType = "StaticMeshActor";
    }

    // 3️⃣ Prüfung: Hat der Actor ein SkeletalMeshComponent?
    auto SkeletalMeshComp = reinterpret_cast<SDK::USkeletalMeshComponent*>(Actor->GetComponentByClass(SDK::USkeletalMeshComponent::StaticClass()));
    if (SkeletalMeshComp) {
        ActorType = "SkeletalMeshActor";
    }

    // 4️⃣ Prüfung: Ist es ein Blueprint (häufig "BP_" im Namen)?
    if (ActorName.find("BP_") != std::string::npos) {
        ActorType = "BlueprintActor";
    }

    // 5️⃣ Komponentenanalyse: Hat der Actor Primitive Components?
    auto PrimitiveComp = reinterpret_cast<SDK::UPrimitiveComponent*>(Actor->GetComponentByClass(SDK::UPrimitiveComponent::StaticClass()));
    if (PrimitiveComp && ActorType == "Unknown") {
        ActorType = "PrimitiveComponentActor";
    }

    // 6️⃣ Fallback: Falls noch immer unbekannt
    if (ActorType == "Unknown" && Actor->IsA(SDK::EClassCastFlags::Actor)) {
        ActorType = "Generic Actor";
    }

    // Ergebnis ausgeben
    printf("Actor Name: %s | Type: %s\n", ActorName.c_str(), ActorType.c_str());
}



const char* floatToConstChar(float value) {
    static char buffer[32];  // Statischer Puffer, um den Speicher nach Funktionsende zu behalten
    snprintf(buffer, sizeof(buffer), "%.2f", value);  // Float in String umwandeln mit 2 Nachkommastellen
    return buffer;
}
const char* combineConstChars(const char* str1, const char* str2) {
    static char combined[64];  // Statischer Puffer für das kombinierte Ergebnis
    snprintf(combined, sizeof(combined), "%s%s", str1, str2);
    return combined;
}
float CalcMiddlePos(float vScreenX, const char* Text)
{
    float itextX = vScreenX - ((strlen(Text) / 2) * 5);
    return itextX;
}


// Konstruktor: Initialisiert den High-Precision Timer
FPSLimiter::FPSLimiter(float fps) {
    QueryPerformanceFrequency(&frequency);
    QueryPerformanceCounter(&frameStart);
    setTargetFPS(fps);
}
// FPS-Limit setzen
void FPSLimiter::setTargetFPS(float fps) {
    targetFPS = (fps <= 0.0f) ? 170.0f : fps;  // 170 = unbegrenzt
    targetFrameTime = 1000.0 / targetFPS;      // Ziel-Framezeit in ms
}
// FPS-Begrenzung anwenden
void FPSLimiter::cap(ImGuiIO& io) {
    if (targetFPS == 170.0f) return;  // FPS-Limiter deaktiviert

    QueryPerformanceCounter(&frameEnd);
    double elapsedTime = getElapsedTime(frameStart, frameEnd);

    while (elapsedTime < targetFrameTime) {
        QueryPerformanceCounter(&frameEnd);
        elapsedTime = getElapsedTime(frameStart, frameEnd);
        _mm_pause();  // Reduziert CPU-Last während des Wartens
    }

    QueryPerformanceCounter(&frameStart);  // Timer für den nächsten Frame neu starten
}
// Berechnet die verstrichene Zeit in Millisekunden
double FPSLimiter::getElapsedTime(LARGE_INTEGER start, LARGE_INTEGER end) {
    return (double)(end.QuadPart - start.QuadPart) * 1000.0 / (double)frequency.QuadPart;
}


// ✅ Berechnet den Abstand zwischen zwei Punkten
float VectorUtils::CalculateDistance(const SDK::FVector& A, const SDK::FVector& B) {
    return std::sqrt(
        (A.X - B.X) * (A.X - B.X) +
        (A.Y - B.Y) * (A.Y - B.Y) +
        (A.Z - B.Z) * (A.Z - B.Z)
    );
}
// ✅ Berechnet den Abstand eines Punktes zu einem Ray
float VectorUtils::CalculateDistanceToRay(const SDK::FVector& RayStart, const SDK::FVector& RayEnd, const SDK::FVector& Point) {
    SDK::FVector RayDir = Subtract(RayEnd, RayStart);
    SDK::FVector PointDir = Subtract(Point, RayStart);

    float RayLengthSquared = DotProduct(RayDir, RayDir);
    if (RayLengthSquared == 0.0f)
        return CalculateDistance(RayStart, Point);  // Ray hat keine Länge

    float t = DotProduct(PointDir, RayDir) / RayLengthSquared;
    t = std::fmax(0.0f, std::fmin(1.0f, t));  // Clamping auf Ray-Segment

    SDK::FVector ClosestPoint = {
        RayStart.X + t * RayDir.X,
        RayStart.Y + t * RayDir.Y,
        RayStart.Z + t * RayDir.Z
    };

    return CalculateDistance(Point, ClosestPoint);
}
// ✅ Vektor-Subtraktion
SDK::FVector VectorUtils::Subtract(const SDK::FVector& A, const SDK::FVector& B) {
    return { A.X - B.X, A.Y - B.Y, A.Z - B.Z };
}
// ✅ Berechnet die Länge eines Vektors
float VectorUtils::Size(const SDK::FVector& Vec) {
    return std::sqrt(Vec.X * Vec.X + Vec.Y * Vec.Y + Vec.Z * Vec.Z);
}
// ✅ Dot-Produkt
float VectorUtils::DotProduct(const SDK::FVector& A, const SDK::FVector& B) {
    return A.X * B.X + A.Y * B.Y + A.Z * B.Z;
}
// ✅ Normalisiert einen Vektor
SDK::FVector VectorUtils::Normalize(const SDK::FVector& Vec) {
    float Magnitude = Size(Vec);
    if (Magnitude == 0.0f) return { 0, 0, 0 };

    return { Vec.X / Magnitude, Vec.Y / Magnitude, Vec.Z / Magnitude };
}
// ✅ Kreuzprodukt (Cross Product)
SDK::FVector VectorUtils::CrossProduct(const SDK::FVector& A, const SDK::FVector& B) {
    return {
        A.Y * B.Z - A.Z * B.Y,
        A.Z * B.X - A.X * B.Z,
        A.X * B.Y - A.Y * B.X
    };
}



// ✅ Raycast ausführen
Raycaster::RaycastResult Raycaster::PerformRaycast(SDK::UWorld* World, SDK::APlayerController* Controller, float Distance) {
    RaycastResult Result;

    if (!World || !Controller || !Controller->PlayerCameraManager)
        return Result;

    Result.StartPoint = Controller->PlayerCameraManager->GetCameraLocation();    
    SDK::FVector ForwardVector = Controller->PlayerCameraManager->GetActorForwardVector();
    
    Result.EndPoint = {
        Result.StartPoint.X + ForwardVector.X * Distance,
        Result.StartPoint.Y + ForwardVector.Y * Distance,
        Result.StartPoint.Z + ForwardVector.Z * Distance
    };

    if (CustomLineTrace(World, Result.StartPoint, Result.EndPoint, Result.HitResult)) {
        Result.bHit = true;
        Result.EndPoint = Result.HitResult.Location;
    }

    if (RaycastHistory.size() >= MaxStoredRays)
        RaycastHistory.erase(RaycastHistory.begin());
    Result.EndPoint = Result.StartPoint + (ForwardVector * 300.0f);
    RaycastHistory.push_back(Result);
    return Result;
}

// ✅ Eigene LineTrace-Implementierung
bool Raycaster::CustomLineTrace(SDK::UWorld* World, const SDK::FVector& Start, const SDK::FVector& End, SDK::FHitResult& OutHit) {
    const float CollisionThreshold = 10.0f;

    

    for (SDK::AActor* Actor : World->PersistentLevel->Actors) {
        if (!Actor || Actor->bHidden || !Actor->bActorEnableCollision)
            continue;

        SDK::FVector ActorLocation = Actor->K2_GetActorLocation();
        float DistanceToRay = VectorUtils::CalculateDistanceToRay(Start, End, ActorLocation);

        /*if (DistanceToRay <= CollisionThreshold) {
            OutHit.Location = SDK::FVector_NetQuantize(ActorLocation);
            *reinterpret_cast<SDK::AActor**>(&OutHit.Actor) = Actor;
            return true;
        }*/
    }
    return false;
}

// ✅ Ray zeichnen
void Raycaster::DrawDebugRay(ImDrawList* drawlist, SDK::APlayerController* Controller) {
    for (const auto& Result : RaycastHistory) {
        Vector2 ScreenStart, ScreenEnd;
        SDK::FVector CamPos = Controller->PlayerCameraManager->GetCameraLocation();
        SDK::FRotator Rotation = Controller->PlayerCameraManager->GetCameraRotation();
        float fov = Controller->PlayerCameraManager->GetFOVAngle();

        if (UEWorldToScreen(Vector3(Result.StartPoint.X, Result.StartPoint.Y, Result.StartPoint.Z), ScreenStart,
            Vector3(Rotation.Pitch, Rotation.Yaw, Rotation.Roll),
            Vector3(CamPos.X, CamPos.Y, CamPos.Z), fov, 1440, 2560) &&
            UEWorldToScreen(Vector3(Result.EndPoint.X, Result.EndPoint.Y, Result.EndPoint.Z), ScreenEnd,
                Vector3(Rotation.Pitch, Rotation.Yaw, Rotation.Roll),
                Vector3(CamPos.X, CamPos.Y, CamPos.Z), fov, 1440, 2560)) {

            ImU32 color = Result.bHit ? IM_COL32(255, 0, 0, 255) : IM_COL32(0, 255, 0, 255);
            drawlist->AddLine(ImVec2(ScreenStart.x, ScreenStart.y), ImVec2(ScreenEnd.x, ScreenEnd.y), color, 2.0f);
        }
    }
}

// ✅ Raycast History abrufen
const std::vector<Raycaster::RaycastResult>& Raycaster::GetRaycastHistory() {
    return RaycastHistory;
}


