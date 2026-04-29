extern bool SpinBot;
#pragma once
#include <windows.h>
#include <vector>
#include <string>
#include <cstdint>
#include "../Cfg/imgui/imgui.h"
#include "../Cfg/Discord/Discord.h"
#include "../Cfg/strenc.h"

extern Discord* DiscordRPC;



struct KeysBindStruct {
    int Menu;
    int GhostHack;
    int WallHack;
    int UnderCamEnabled;
    int AimbotCollider;
    int Aimbot;
    int NoClip;
    int SpeedTimer;
    int UpPlayerEnabled;
    int DownPlayerEnabled;
    int DownLocalEnabled;
    int TeleportPlayer;
    int Magnetic;
    int MagneticKillv2;
    int TeleportMark;
    int longparachute;
    int ShoulderAimbot;
    int Silent;
};
extern KeysBindStruct KeysBind;

extern bool AimbotLegit;
extern bool GhostHack;
extern bool Aimlock2x;
extern bool FastSwitch;
extern float FastSwapSpeed;

extern float SpinbotSpeed;
extern float NoClipSpeed;
extern float SpeedHackerVelocity;

extern bool Bloquearmira;
extern bool NoRecoil;

extern bool ESPNome;
extern bool ESPLinha;
extern bool ESPEsqueleto;
extern bool ESPCaixa;
extern bool ESPDistancia;
extern bool ESPVida;

extern bool WallHack;
extern bool SpeedTimer;
extern bool NoClip;
extern bool UpPlayer;
extern bool DownPlayer;
extern bool DownLocalPlayer;
extern bool TeleportPlayer;

extern bool UnderCamEnabled;
extern bool TelekillToMe;

extern bool AimbotCollider;
extern float AimbotColliderDelay;

extern bool MagneticKillv3;
extern bool MagneticKillv2;

extern int AimbotDistance;
extern int AimbotFov;
extern int MagneticPos;
extern int MagType;

extern bool ShoulderAimbot;
extern bool fastreload;

extern bool LoginMemory;
extern float LoginFov;
extern float DistanceMaxLogin;
extern bool ZeroLoginMort;
extern int DelayLogin;

extern float LogoScale;

extern bool ShowKeybindPanel;
extern bool ShowAimbotFov;
extern bool IgnoreBots;
extern bool IgnoreKnocked;

extern bool SpinBot;
extern bool StreamProof;
extern bool Silent;
extern int SilentHead;
extern int SilentBody;
extern bool ShowSilentFov;
extern float SilentFov;
extern int SilentDistance;
extern float SilentFovColor[4];
extern bool TeleportMark;
extern bool longparachute;
extern bool fovawm;
extern bool TeleportMark;
extern bool longparachute;
extern bool fovawm;

extern int Priority;

extern bool isAttached;
extern bool OverlayView;
extern char LicenseKey[256];
extern bool PreviewMode;

extern bool ShutDown;
extern LONG Unloading;

extern float espTextSize;
extern float espThickness;
extern int linePosition;
extern int healthPosition;
extern int boxStyle;
extern float espHealthBarOffsetX;
extern float espHealthBarOffsetY;
extern bool espRequireForeground;

extern float colorName[4];
extern float colorLine[4];
extern float colorSkeleton[4];
extern float colorBox[4];
extern float colorDistance[4];
extern float colorDying[4];
extern float AimbotFovColor[4];
extern float Filledcolor[4];
extern bool RgbMode;

extern int Enemies;
extern bool KeybindsMenu;
extern int KeybindMode;

// Modos de keybind individuais para aimbots
extern int ShoulderAimbotBindMode;
extern int AimbotColliderBindMode;
extern int SilentBindMode;

extern float ParticleColour[4];
extern ImColor Particle;
extern int CurrentTab;
extern int CurrentWindow;

constexpr uintptr_t LOGIN_OFF_READ = 0x444;
constexpr uintptr_t LOGIN_OFF_WRITE = 0x50;
constexpr uintptr_t MONITOR = 0x430;
constexpr uintptr_t TELA = 0x3FC;
