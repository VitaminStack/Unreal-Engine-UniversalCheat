#include "OverlayCord.h"
#include "../Helper/Logger.h"
#include <cstring>

namespace OverlayCord
{
	namespace Communication
	{
		bool ConnectToProcess(ConnectedProcessInfo& processInfo)
		{
			std::string mappedFilename = "DiscordOverlay_Framebuffer_Memory_" + std::to_string(processInfo.ProcessId);
			processInfo.File = OpenFileMappingA(FILE_MAP_ALL_ACCESS, false, mappedFilename.c_str());
			if (!processInfo.File || processInfo.File == INVALID_HANDLE_VALUE)
				return false;

			processInfo.MappedAddress = static_cast<Header*>(MapViewOfFile(processInfo.File, FILE_MAP_ALL_ACCESS, 0, 0, 0));
			return processInfo.MappedAddress;
		}

		void DisconnectFromProcess(ConnectedProcessInfo& processInfo)
		{
			UnmapViewOfFile(processInfo.MappedAddress);
			processInfo.MappedAddress = nullptr;

			CloseHandle(processInfo.File);
			processInfo.File = nullptr;
		}

		void SendFrame(ConnectedProcessInfo& processInfo, UINT width, UINT height, void* frame, UINT size)
		{
                if (!processInfo.MappedAddress) {
                                LOG_ERROR("MappedAddress is NULL. Is OverlayCord connected?");
                                return;
                        }

			processInfo.MappedAddress->Width = width;
			processInfo.MappedAddress->Height = height;
			memcpy(processInfo.MappedAddress->Buffer, frame, size);
			processInfo.MappedAddress->FrameCount++;
		}
	}

	namespace Drawing {

		void DrawCharacter(Frame& frame, UINT x, UINT y, char c, Pixel color) {
			// Falls Zeichen nicht in der unterstützten ASCII-Tabelle ist -> Ignorieren
			if (c < 32 || c > 127) return;

			// Prüfen, ob das Zeichen in FontData existiert
			auto it = FontData.find(c);
			if (it == FontData.end()) return; // Falls nicht vorhanden, ignorieren

			const std::array<uint8_t, 8>& charBitmap = it->second;

			// Zeichne die Pixel aus FontData
			for (int row = 0; row < 8; ++row) {
				for (int col = 0; col < 8; ++col) {
					if (charBitmap[row] & (1 << (7 - col))) {
						SetPixel(frame, x + col, y + row, color);
					}
				}
			}
		}

		void Draw_Text(Frame& frame, UINT x, UINT y, const char* text, Pixel color, UINT spacing) {
			if (text == nullptr) return; // Falls der Text null ist, nicht rendern

			size_t length = strlen(text);

			UINT offsetX = 0;
			for (size_t i = 0; i < length; i++) {
				if (text[i] != ' ') { // Leerzeichen überspringen
					DrawCharacter(frame, x + offsetX, y, text[i], color);
				}
				offsetX += spacing; // Zeichenabstand
			}
		}

		Frame CreateFrame(UINT width, UINT height) {
			Frame output;
			output.Width = width;
			output.Height = height;
			output.Size = width * height * 4;
			output.Buffer = malloc(output.Size);
			memset(output.Buffer, 0, output.Size);
			return output;
		}

		void CleanFrame(Frame& frame) {
			memset(frame.Buffer, 0, frame.Size);
		}

		void SetPixel(Frame& frame, UINT x, UINT y, Pixel color) {
			if (x < frame.Width && y < frame.Height) {
				Pixel* buf = static_cast<Pixel*>(frame.Buffer);
				buf[y * frame.Width + x] = color;
			}
		}

		void DrawLine(Frame& frame, UINT x1, UINT y1, UINT x2, UINT y2, Pixel color) {
			int dx = abs(static_cast<int>(x2 - x1)), sx = x1 < x2 ? 1 : -1;
			int dy = -abs(static_cast<int>(y2 - y1)), sy = y1 < y2 ? 1 : -1;
			int err = dx + dy, e2;

			while (true) {
				SetPixel(frame, x1, y1, color);
				if (x1 == x2 && y1 == y2)
					break;
				e2 = 2 * err;
				if (e2 >= dy) { err += dy; x1 += sx; }
				if (e2 <= dx) { err += dx; y1 += sy; }
			}
		}

		void DrawRectangle(Frame& frame, UINT x1, UINT y1, UINT width, UINT height, Pixel color) {
			for (UINT x = x1; x < x1 + width; x++) {
				SetPixel(frame, x, y1, color);
				SetPixel(frame, x, y1 + height - 1, color);
			}

			for (UINT y = y1; y < y1 + height; y++) {
				SetPixel(frame, x1, y, color);
				SetPixel(frame, x1 + width - 1, y, color);
			}
		}

		void DrawCircle(Frame& frame, UINT x0, UINT y0, UINT radius, Pixel color) {
			int x = radius;
			int y = 0;
			int radiusError = 1 - x;

			while (x >= y) {
				SetPixel(frame, x0 + x, y0 + y, color);
				SetPixel(frame, x0 - x, y0 + y, color);
				SetPixel(frame, x0 - x, y0 - y, color);
				SetPixel(frame, x0 + x, y0 - y, color);
				SetPixel(frame, x0 + y, y0 + x, color);
				SetPixel(frame, x0 - y, y0 + x, color);
				SetPixel(frame, x0 - y, y0 - x, color);
				SetPixel(frame, x0 + y, y0 - x, color);

				y++;
				if (radiusError < 0) {
					radiusError += 2 * y + 1;
				}
				else {
					x--;
					radiusError += 2 * (y - x + 1);
				}
			}
		}
	}
}



DWORD GetProcessIDByName(const char* exeName) {
	HANDLE hSnap = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
	if (hSnap == INVALID_HANDLE_VALUE) return 0;

	PROCESSENTRY32 pe;
	pe.dwSize = sizeof(PROCESSENTRY32);

	DWORD processID = 0;
	if (Process32First(hSnap, &pe)) {
		do {
			wchar_t wideExeName[MAX_PATH];
			MultiByteToWideChar(CP_ACP, 0, exeName, -1, wideExeName, MAX_PATH);

			if (_wcsicmp(pe.szExeFile, wideExeName) == 0) {
				processID = pe.th32ProcessID;
				break;
			}
		} while (Process32Next(hSnap, &pe));
	}

	CloseHandle(hSnap);
	return processID;
}
HWND GetHWNDByProcessID(DWORD processID) {
	HWND hwnd = GetTopWindow(nullptr);
	while (hwnd) {
		DWORD windowPID;
		GetWindowThreadProcessId(hwnd, &windowPID);
		if (windowPID == processID) return hwnd;
		hwnd = GetNextWindow(hwnd, GW_HWNDNEXT);
	}
	return nullptr;
}
void StartRenderLoop(OverlayCord::Communication::ConnectedProcessInfo& processInfo) {
        LOG_INFO("Starting render loop...");

	// Erstelle ein 1280x720 Framebuffer für das Overlay
        OverlayCord::Drawing::Frame mainFrame = OverlayCord::Drawing::CreateFrame(2560, 1440);
        if (!mainFrame.Buffer) {
                LOG_ERROR("Failed to allocate frame buffer");
                return;
        }

	
	OverlayCord::Drawing::Pixel textColor = { 0, 0, 255, 255 }; // Weißer Text

	LARGE_INTEGER frequency, t1, t2;
	QueryPerformanceFrequency(&frequency);
	QueryPerformanceCounter(&t1);

	// Loop bis "VK_INSERT" gedrückt wird
	while (!GetAsyncKeyState(VK_END) & 1) {
		QueryPerformanceCounter(&t2);
		double deltaTime = (t2.QuadPart - t1.QuadPart) / static_cast<double>(frequency.QuadPart);
		t1 = t2;

		OverlayCord::Drawing::CleanFrame(mainFrame);

		SDK::UEngine* Engine = SDK::UEngine::GetEngine();
                SDK::UWorld* World = SDK::UWorld::GetWorld();
                if (!World || !World->OwningGameInstance) {
                        LOG_WARN("World or OwningGameInstance invalid");
                        continue;
                }
                auto& LocalPlayers = World->OwningGameInstance->LocalPlayers;
                if (LocalPlayers.Num() <= 0 || !LocalPlayers[0]) {
                        LOG_WARN("LocalPlayer not available");
                        continue;
                }
                SDK::APlayerController* MyController = LocalPlayers[0]->PlayerController;
                if (!MyController) {
                        LOG_WARN("PlayerController invalid");
                        continue;
                }
		SDK::ULevel* Level = World->PersistentLevel;
		SDK::TArray<SDK::AActor*>& Actors = Level->Actors;
                for (SDK::AActor* Actor : Actors)
                {
                        if (!PointerChecks::IsValidPtr(Actor, "AActor") ||
                            !PointerChecks::IsValidPtr(Actor->Class, "ActorClass") ||
                            !Actor->IsA(SDK::EClassCastFlags::Actor) ||
                            !Actor->IsA(SDK::AActor::StaticClass()))
                                continue;
			std::string ActorName = Actor->GetName();
			float fov = MyController->PlayerCameraManager->GetFOVAngle();
			SDK::FVector camPos = MyController->PlayerCameraManager->GetCameraLocation();
			SDK::FRotator rotation = MyController->PlayerCameraManager->GetCameraRotation();
			if (Actor->RootComponent)
			{
				SDK::FVector actorPos = Actor->RootComponent->RelativeLocation;

				if (actorPos.IsZero()) continue;  // ✅ Abbruch bei ungültiger Position
				Vector2 screenPos;
				if (!UEWorldToScreen(Vector3(actorPos.X, actorPos.Y, actorPos.Z), screenPos,
					Vector3(rotation.Pitch, rotation.Yaw, rotation.Roll),
					Vector3(camPos.X, camPos.Y, camPos.Z), fov, 1440, 2560)) {
					continue;
				}
				if (screenPos.x > 0 && screenPos.x < 2560 && screenPos.y > 0 && screenPos.y < 1440)
				{
					float distance = camPos.GetDistanceTo(actorPos) / 100.0f;
					if (distance > 2.f && distance) {
						std::string aName = Actor->Class->GetName();
						OverlayCord::Drawing::Draw_Text(mainFrame, screenPos.x, screenPos.y, aName.c_str(), textColor, 7);
					}
				}
			}
			
		}
		
		

		
			


		
				

		// Frame senden
		OverlayCord::Communication::SendFrame(processInfo, mainFrame.Width, mainFrame.Height, mainFrame.Buffer, mainFrame.Size);
	}

	LOG_INFO("[>] Render loop stopped.\n");
}

#pragma comment(lib, "d3d11.lib")

ImGuiOverlay::ImGuiOverlay(UINT w, UINT h, DWORD processId, bool debug)
	: width(w), height(h), enableDebug(debug) {

	if (!InitDirectX() || !InitImGui() || !CreateRenderTarget()) {
		throw std::runtime_error("Failed to initialize ImGuiOverlay");
	}

	overlayProcess.ProcessId = processId;
	if (!OverlayCord::Communication::ConnectToProcess(overlayProcess)) {
		DebugLog("Error: Failed to connect to OverlayCord process!");
	}
}

ImGuiOverlay::~ImGuiOverlay() {
	OverlayCord::Communication::DisconnectFromProcess(overlayProcess);

	if (renderTargetView) renderTargetView->Release();
	if (renderTargetTexture) renderTargetTexture->Release();
	if (context) context->Release();
	if (device) device->Release();

	ImGui_ImplDX11_Shutdown();
	ImGui::DestroyContext();
}

bool ImGuiOverlay::InitDirectX() {
	HRESULT result = D3D11CreateDevice(nullptr, D3D_DRIVER_TYPE_HARDWARE, nullptr, 0, nullptr, 0,
		D3D11_SDK_VERSION, &device, nullptr, &context);
	if (FAILED(result)) {
		DebugLog("Error: Failed to initialize DirectX 11!");
		return false;
	}
	DebugLog("DirectX 11 initialized successfully.");
	return true;
}

bool ImGuiOverlay::InitImGui() {
	ImGui::CreateContext();
	if (!ImGui_ImplDX11_Init(device, context)) {
		DebugLog("Error: Failed to initialize ImGui!");
		return false;
	}
	DebugLog("ImGui initialized successfully.");
	return true;
}

bool ImGuiOverlay::CreateRenderTarget() {
	D3D11_TEXTURE2D_DESC desc = {};
	desc.Width = width;
	desc.Height = height;
	desc.MipLevels = 1;
	desc.ArraySize = 1;
	desc.Format = DXGI_FORMAT_B8G8R8A8_UNORM;
	desc.SampleDesc.Count = 1;
	desc.Usage = D3D11_USAGE_DEFAULT;
	desc.BindFlags = D3D11_BIND_RENDER_TARGET;

	if (FAILED(device->CreateTexture2D(&desc, nullptr, &renderTargetTexture))) {
		DebugLog("Error: Failed to create Render Target Texture!");
		return false;
	}

	if (FAILED(device->CreateRenderTargetView(renderTargetTexture, nullptr, &renderTargetView))) {
		DebugLog("Error: Failed to create Render Target View!");
		return false;
	}

	DebugLog("Render target created successfully.");
	return true;
}

void ImGuiOverlay::Render() {
	context->OMSetRenderTargets(1, &renderTargetView, nullptr);

	ImGuiIO& io = ImGui::GetIO();
	io.DisplaySize = ImVec2(2560, 1440);  // Setze eine Standardgröße
	ImGui_ImplDX11_NewFrame();
	ImGui::NewFrame();

	ImGui::SetNextWindowSize(ImVec2(300, 200));
	ImGui::Begin("Overlay UI");
	static float slider = 0.5f;
	ImGui::SliderFloat("Value", &slider, 0.0f, 1.0f);
	ImGui::End();

	ImGui::Render();
	ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());
}

void ImGuiOverlay::SendToOverlay() {
	ID3D11Texture2D* stagingTexture = nullptr;
	D3D11_TEXTURE2D_DESC desc;
	renderTargetTexture->GetDesc(&desc);
	desc.Usage = D3D11_USAGE_STAGING;
	desc.CPUAccessFlags = D3D11_CPU_ACCESS_READ;
	desc.BindFlags = 0;

	if (FAILED(device->CreateTexture2D(&desc, nullptr, &stagingTexture))) {
		DebugLog("Error: Failed to create staging texture!");
		return;
	}

	context->CopyResource(stagingTexture, renderTargetTexture);

	D3D11_MAPPED_SUBRESOURCE mappedResource;
	if (SUCCEEDED(context->Map(stagingTexture, 0, D3D11_MAP_READ, 0, &mappedResource))) {
		OverlayCord::Communication::SendFrame(overlayProcess, width, height, mappedResource.pData, mappedResource.RowPitch * height);
		context->Unmap(stagingTexture, 0);
		DebugLog("Frame sent to OverlayCord.");
	}
	else {
		DebugLog("Error: Failed to map staging texture!");
	}

	stagingTexture->Release();
}

void ImGuiOverlay::DebugLog(const std::string& msg) {
	if (enableDebug) {
		std::cout << msg << std::endl;
	}
}