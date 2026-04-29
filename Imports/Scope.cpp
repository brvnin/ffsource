#include "Scope.h"
#include <cstring>

Discord* DiscordRPC = nullptr;

bool isAttached = false;
bool OverlayView = true;
char LicenseKey[256] = "";

KeysBindStruct KeysBind = {
	VK_INSERT, 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0
};

bool AimbotLegit = false;
bool GhostHack = false;
bool Aimlock2x = false;
bool FastSwitch = false;

float FastSwapSpeed = 10.0f;
float SpinbotSpeed = 0.0f;
float NoClipSpeed = 3.14159265f;
float SpeedHackerVelocity = 0.0025f;

bool Bloquearmira = false;
bool NoRecoil = false;

bool ESPNome = false;
bool ESPLinha = false;
bool ESPEsqueleto = false;
bool ESPCaixa = false;
bool ESPDistancia = false;
bool ESPVida = false;

bool WallHack = false;
bool SpeedTimer = false;
bool NoClip = false;
bool UpPlayer = false;
bool DownPlayer = false;
bool DownLocalPlayer = false;
bool TeleportPlayer = false;

bool UnderCamEnabled = false;
bool TelekillToMe = false;

bool AimbotCollider = false;
float AimbotColliderDelay = 0.0f;

bool MagneticKillv3 = false;
bool MagneticKillv2 = false;

int AimbotDistance = 100;
int AimbotFov = 100;
int MagneticPos = 0;
int MagType = 1;

bool ShoulderAimbot = false;
bool fastreload = false;

bool LoginMemory = false;
float LoginFov = 50.0f;
float DistanceMaxLogin = 130.0f;
bool ZeroLoginMort = true;
int DelayLogin = 351;

bool ShowKeybindPanel = true;
bool IgnoreBots = false;
bool IgnoreKnocked = true;

bool SpinBot = false;
bool StreamProof = false;
bool Silent = false;
int SilentHead = 0;
int SilentBody = 0;
bool ShowSilentFov = false;
float SilentFov = 100.0f;
int SilentDistance = 200;
float SilentFovColor[4] = { 1.0f, 1.0f, 1.0f, 1.0f };
bool TeleportMark = false;
bool longparachute = false;
bool fovawm = false;
int Priority = 0;

bool ShutDown = false;
LONG Unloading = 0;

float espTextSize = 12.0f;
float espThickness = 1.0f;
int linePosition = 1;
int healthPosition = 1;
float espHealthBarOffsetX = 0.0f;
float espHealthBarOffsetY = 0.0f;
bool espRequireForeground = false;
int boxStyle = 1;

int Enemies = 0;
bool KeybindsMenu = true;
int KeybindMode = 1;

// Modos de keybind individuais para aimbots
int ShoulderAimbotBindMode = 1;  // 0 = Hold, 1 = Toggle
int AimbotColliderBindMode = 1;  // 0 = Hold, 1 = Toggle
int SilentBindMode = 1;          // 0 = Hold, 1 = Toggle

float colorName[4] = { 255,255,255,255 };
float colorLine[4] = { 255,255,255,255 };
float colorSkeleton[4] = { 255,255,255,255 };
float colorBox[4] = { 255,255,255,255 };
float colorDistance[4] = { 255,255,255,255 };
float colorDying[4] = { 255,0,0,255 };
float AimbotFovColor[4] = { 255,0,0,255 };
float Filledcolor[4] = { 0.f, 0.f, 0.f, 0.1f };
bool ShowAimbotFov = false;
bool RgbMode = false;

float ParticleColour[4] = { 255,255,255,255 };
ImColor Particle = ImColor(0, 255, 200, 255);
int CurrentTab = 0;
int CurrentWindow = 0;
bool PreviewMode = false;

float LogoScale = 0.75f;
