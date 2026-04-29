#ifndef IMGUI_DEFINE_MATH_OPERATORS
#define IMGUI_DEFINE_MATH_OPERATORS
#endif
#include "Imports/includes.h"
#include <algorithm>
#include <cctype>
#include <cmath>
#include <random>
#include <unordered_map>
#include <chrono>
#include <atomic>
#include "Cheat/Aimbot.h"
#include "Cheat/AimNeck.h"
#include "Cheat/SilentAim.h"
#include "Notify.hpp"
#include "discord_avatar.h"
#include "remember_me.h"


#include "auth/json.hpp"
#include "auth/auth.h"


using json = nlohmann::json;

// Instância estática da classe Auth
static Auth* authInstance = nullptr;
static bool isLoggedIn = false;
static std::string expirationInfo = "";
static std::string loggedUser = "User";
static std::string loggedDiscordID = "None";
static int loggedExpiryDays = 0;
static std::atomic<bool> keyInputErrorRequested = false;


DWORD WINAPI ThreadOverlay();

void AbrirConsole() {
    AllocConsole();
    FILE* f;
    freopen_s(&f, "CONOUT$", "w", stdout);
    freopen_s(&f, "CONIN$", "r", stdin);
    SetConsoleTitleA("Console Debug - Safety Softwares");
}

Vector3 GetViewDirection(uintptr_t cameraPtr) {
    return Ler<Vector3>(cameraPtr);
}

inline Vector3 ObterDirecaoMagnetic(UnityMatrix MatrixADDR) {
    Vector3 forward(MatrixADDR._13, MatrixADDR._23, MatrixADDR._33);
    return Vector3::Normalized(forward);
}

float DistanciaLinha(const Vector3& origem, const Vector3& dir, const Vector3& ponto) {
    Vector3 v = ponto - origem;
    float t = Vector3::Dot(v, dir);
    if (t < 0) t = 0;
    Vector3 proj = origem + dir * t;
    return Vector3::Distance(ponto, proj);
}

bool IsBindModeActive(int key) {
    if (key <= 0) {
        return false;
    }

    bool isPressed = (GetAsyncKeyState(key) & 0x8000) != 0;
    if (KeybindMode == 0) {
        return isPressed;
    }

    static std::unordered_map<int, bool> toggleStates;
    static std::unordered_map<int, bool> previousPressed;

    bool& wasPressed = previousPressed[key];
    bool& toggleState = toggleStates[key];

    if (isPressed && !wasPressed) {
        toggleState = !toggleState;
    }

    wasPressed = isPressed;
    return toggleState;
}

// Função para verificar keybind com modo específico (Hold ou Toggle)
bool IsBindModeActiveCustom(int key, int bindMode) {
    if (key <= 0) {
        return false;
    }

    bool isPressed = (GetAsyncKeyState(key) & 0x8000) != 0;
    if (bindMode == 0) { // Hold
        return isPressed;
    }

    // Toggle
    static std::unordered_map<int, bool> toggleStates;
    static std::unordered_map<int, bool> previousPressed;

    bool& wasPressed = previousPressed[key];
    bool& toggleState = toggleStates[key];

    if (isPressed && !wasPressed) {
        toggleState = !toggleState;
    }

    wasPressed = isPressed;
    return toggleState;
}

// Funções para verificar estado dos aimbots
bool IsShoulderAimbotActive() {
    if (!ShoulderAimbot) return false;
    if (KeysBind.ShoulderAimbot == 0) return true;
    return IsBindModeActiveCustom(KeysBind.ShoulderAimbot, ShoulderAimbotBindMode);
}

bool IsAimbotColliderActive() {
    if (!AimbotCollider) return false;
    if (KeysBind.Aimbot == 0) return true;
    return IsBindModeActiveCustom(KeysBind.Aimbot, AimbotColliderBindMode);
}

bool IsSilentActive() {
    if (!Silent) return false;
    if (KeysBind.Silent == 0) return true;
    return IsBindModeActiveCustom(KeysBind.Silent, SilentBindMode);
}

struct PlayerData {
    std::string Name;
    uintptr_t Entity;
    float Distance;
    int Health;
    bool IsDying;
};

struct PlayerCacheEntry {
    PlayerData Data;
    DWORD LastSeen;
};

static std::vector<PlayerData> RenderedPlayers;
static std::unordered_map<uintptr_t, PlayerCacheEntry> PlayersCache;

static DWORD lastCommit = 0;
constexpr DWORD kCommitIntervalMs = 1000;
constexpr DWORD kKeepAliveMs = 1500;

void PushPlayer(const std::string& nameStr, uintptr_t entity, float dist, int hp, bool dying)
{
    if (!entity || hp <= 0) return;

    PlayerData d{ nameStr, entity, dist, hp, dying };
    DWORD now = GetTickCount();
    PlayersCache[entity] = { d, now };
}

void CommitPlayersIfNeeded()
{
    DWORD now = GetTickCount();
    if (now - lastCommit < kCommitIntervalMs)
        return;

    for (auto it = PlayersCache.begin(); it != PlayersCache.end(); )
    {
        if (now - it->second.LastSeen > kKeepAliveMs)
            it = PlayersCache.erase(it);
        else
            ++it;
    }

    RenderedPlayers.clear();
    RenderedPlayers.reserve(PlayersCache.size());
    for (auto& kv : PlayersCache)
        RenderedPlayers.push_back(kv.second.Data);

    std::sort(RenderedPlayers.begin(), RenderedPlayers.end(),
        [](const PlayerData& a, const PlayerData& b) { return a.Distance < b.Distance; });

    lastCommit = now;
}

void ProcessNoRecoil(uintptr_t JogadorLocal) {
    uintptr_t Inventario = Ler<uintptr_t>(JogadorLocal + string2Offset(AY_OBFUSCATE("0x448")));
    if (!Inventario) return;

    uintptr_t Item = Ler<uintptr_t>(Inventario + string2Offset(AY_OBFUSCATE("0x54")));
    if (!Item) return;

    uintptr_t weaponptr = Ler<uintptr_t>(Item + string2Offset(AY_OBFUSCATE("0x58")));
    if (!weaponptr) return;

    float recoil = Ler<float>(weaponptr + string2Offset(AY_OBFUSCATE("0xC")));

    if (NoRecoil) {
        if (recoil != 0.0f) {
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
            Escrever<float>(weaponptr + string2Offset(AY_OBFUSCATE("0xC")), 0.0f);
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
        }
    }
    else {
        if (recoil != 0.0174825) {
            Escrever<float>(weaponptr + string2Offset(AY_OBFUSCATE("0xC")), 0.0174825);
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
        }
    }
}

void UnifiedTeleportSystem(uintptr_t JogadorLocal, Vector3 targetPosition) {
    if (!JogadorLocal) return;
    
    uintptr_t playerCachedTransform = Ler<uintptr_t>(JogadorLocal + Offsets.m_CachedTransform);
    if (playerCachedTransform != 0) {
        Vector3 teleportPos = targetPosition;
        teleportPos.Y += 0.5f;
        Transform$$DefinirPosicao(playerCachedTransform, teleportPos);
    }
}

static int GetTeleportMarkKey() {
    return KeysBind.TeleportMark;
}

// Teleporta para a posição do mark/waypoint do mapa quando a tecla está ativa (toggle)
void UpdateTeleportMarkSystem(uintptr_t JogadorLocal) {
    static bool wasKeyPressed = false;
    static bool toggleState = false;
    static int lastMode = 1;

    if (!JogadorLocal) return;

    int teleportMarkKey = GetTeleportMarkKey();
    if (teleportMarkKey <= 0) {
        toggleState = false;
        TeleportMark = false;
        return;
    }

    if (lastMode != KeybindMode) {
        wasKeyPressed = false;
        toggleState = false;
        if (KeybindMode == 1) {
            TeleportMark = false;
        }
        lastMode = KeybindMode;
    }

    bool isKeyPressed = (GetAsyncKeyState(teleportMarkKey) & 0x8000) != 0;
    if (KeybindMode == 0) {
        TeleportMark = isKeyPressed;
    }
    else {
        if (isKeyPressed && !wasKeyPressed) {
            toggleState = !toggleState;
            TeleportMark = toggleState;
        }
    }
    wasKeyPressed = isKeyPressed;

    if (!TeleportMark) return;

    try {
        uintptr_t GameEngine = Ler<uintptr_t>(Ler<uintptr_t>(Ler<uintptr_t>(il2cpp + Offsets.GameEngine) + Offsets.jsonuni));
        if (GameEngine == 0) return;

        uintptr_t BaseGame = Ler<uintptr_t>(GameEngine + Offsets.BaseGame);
        if (BaseGame == 0) return;

        uintptr_t currentGame = 0;
        uintptr_t _initBase = Ler<uintptr_t>(il2cpp + Offsets.GameEngine);
        if (_initBase != 0 && _initBase != 0xFFFFFFFF) {
            uintptr_t _baseGameFacade = Ler<uintptr_t>(_initBase + Offsets.jsonuni);
            if (_baseGameFacade != 0 && _baseGameFacade != 0xFFFFFFFF) {
                uintptr_t _gameFacade = Ler<uintptr_t>(_baseGameFacade);
                if (_gameFacade != 0 && _gameFacade != 0xFFFFFFFF) {
                    uintptr_t _staticClass = Ler<uintptr_t>(_gameFacade + Offsets.jsonuni);
                    if (_staticClass != 0 && _staticClass != 0xFFFFFFFF) {
                        currentGame = Ler<uintptr_t>(_staticClass);
                    }
                }
            }
        }
        if (currentGame == 0 || currentGame == 0xFFFFFFFF) {
            currentGame = BaseGame;
        }
        if (currentGame == 0 || currentGame == 0xFFFFFFFF) return;

        Vector3 markPos = Vector3::Zero();
        uintptr_t _1 = Ler<uintptr_t>(currentGame + 0x8);
        if (!_1 || _1 == 0xFFFFFFFF) return;
        uintptr_t _2 = Ler<uintptr_t>(_1 + 0x1E4);
        if (!_2 || _2 == 0xFFFFFFFF) return;
        uintptr_t _3 = Ler<uintptr_t>(_2 + 0x50);
        if (!_3 || _3 == 0xFFFFFFFF) return;
        uintptr_t _4 = Ler<uintptr_t>(_3 + 0x88);
        if (!_4 || _4 == 0xFFFFFFFF) return;
        markPos = Ler<Vector3>(_4 + 0x54);

        if (markPos.X == 0 && markPos.Y == 0 && markPos.Z == 0) return;

        markPos.Y += 0.10f;
        UnifiedTeleportSystem(JogadorLocal, markPos);
    }
    catch (const std::exception&) {}
}

void ProcessTeleport(uintptr_t targetPlayer, uintptr_t localPlayer, const std::string& playerName) {
    if (!TeleportPlayer || targetPlayer == 0 || localPlayer == 0) return;

    static bool keyPressedLastFrame = false;
    bool keyCurrentlyPressed = (GetAsyncKeyState(KeysBind.TeleportPlayer) & 0x8000) != 0;

    if (keyCurrentlyPressed && !keyPressedLastFrame) {
        Vector3 enemyPos = GetPlayerPosition(targetPlayer, 0);

        uintptr_t playerCachedTransform = Ler<uintptr_t>(localPlayer + Offsets.m_CachedTransform);
        if (playerCachedTransform != 0) {
            Transform$$DefinirPosicao(playerCachedTransform, enemyPos);

            if (!playerName.empty()) {

                /*NotificationManager::AdicionarNotificacao("Teleported to " + playerName);*/
            }
        }
    }

    keyPressedLastFrame = keyCurrentlyPressed;
}

void ProcessTeleport(uintptr_t targetPlayer, uintptr_t localPlayer) {
    ProcessTeleport(targetPlayer, localPlayer, "");
}
void ProcessRotate360(uintptr_t JogadorLocal) {
    static float accumulatedRotation = 0.0f;
    static bool wasRotating = false;
    static auto lastUpdateTime = std::chrono::steady_clock::now();

    if (!SpinBot) {
        if (wasRotating) {
            wasRotating = false;
        }
        return;
    }

    uintptr_t transform = Ler<uintptr_t>(JogadorLocal + Offsets.m_CachedTransform);
    if (transform != 0) {
        uintptr_t transformObj = Ler<uintptr_t>(transform + string2Offset(AY_OBFUSCATE("0x8")));
        if (transformObj != 0) {
            uintptr_t index = Ler<int>(transformObj + string2Offset(AY_OBFUSCATE("0x24")));
            uintptr_t matrixBase = Ler<uintptr_t>(transformObj + string2Offset(AY_OBFUSCATE("0x20")));
            if (matrixBase != 0) {
                uintptr_t matrixList = Ler<uintptr_t>(matrixBase + string2Offset(AY_OBFUSCATE("0x10")));
                if (matrixList != 0) {
                    Matrix localMatrix = Ler<Matrix>(matrixList + (index * sizeof(Matrix)));

                    Quaternion currentRot = Quaternion(localMatrix.Rotation.X, localMatrix.Rotation.Y, localMatrix.Rotation.Z, localMatrix.Rotation.W);
                    Vector3 euler = Quaternion::ToEuler(currentRot);

                    if (!wasRotating) {
                        accumulatedRotation = euler.Y;
                        wasRotating = true;
                        lastUpdateTime = std::chrono::steady_clock::now();
                    }

                    auto now = std::chrono::steady_clock::now();
                    auto deltaTime = std::chrono::duration<float>(now - lastUpdateTime).count();
                    lastUpdateTime = now;

                    if (deltaTime > 0.1f) deltaTime = 0.016f; 

                    float rotationSpeed = (SpinbotSpeed / 100.0f) * 360.0f;
                    accumulatedRotation += rotationSpeed * deltaTime;

                    accumulatedRotation = fmodf(accumulatedRotation, 360.0f);
                    if (accumulatedRotation < 0.0f) accumulatedRotation += 360.0f;

                    euler.Y = accumulatedRotation;

                    Quaternion newRot = Quaternion::FromEuler(euler.X, euler.Y, euler.Z);
                    localMatrix.Rotation.X = newRot.X;
                    localMatrix.Rotation.Y = newRot.Y;
                    localMatrix.Rotation.Z = newRot.Z;
                    localMatrix.Rotation.W = newRot.W;

                    Escrever<Matrix>(matrixList + (index * sizeof(Matrix)), localMatrix);
                    Escrever<Matrix>(matrixList + (index * sizeof(Matrix)), localMatrix);
                }
            }
        }
    }
}


void ProcessNoclip(uintptr_t JogadorLocal, UnityMatrix matrix) {
    static Vector3 lastNoclipPos = Vector3(0, 0, 0);
    static bool wasNoclipActive = false;
    static int confirmFrames = 0;
    static Vector3 noclipStartPos = Vector3(0, 0, 0);
    static bool noclipStartPosSaved = false;
    const int CONFIRM_FRAME_COUNT = 1200;
    const float MAX_DISTANCE = 30.5f;

    if (NoClip) {
        Vector3 currentPos = GetPlayerPosition(JogadorLocal, 0);
        if (currentPos.X == 0 && currentPos.Y == 0 && currentPos.Z == 0) {
            return;
        }

        if (!noclipStartPosSaved) {
            noclipStartPos = currentPos;
            noclipStartPosSaved = true;
        }

        Vector3 forwardDir = Vector3(matrix._13, matrix._23, matrix._33);
        forwardDir = Vector3::Normalized(forwardDir); // CORRIGIDO

        Vector3 forwardDirHorizontal = Vector3(forwardDir.X, 0.0f, forwardDir.Z);
        if (Vector3::Magnitude(forwardDirHorizontal) > 0.01f) {
            forwardDirHorizontal = Vector3::Normalized(forwardDirHorizontal); // CORRIGIDO
        }
        else {
            forwardDirHorizontal = Vector3(0, 0, 1);
        }

        Vector3 rightDir = Vector3(matrix._11, 0.0f, matrix._31);
        if (Vector3::Magnitude(rightDir) > 0.01f) {
            rightDir = Vector3::Normalized(rightDir); // CORRIGIDO
        }
        else {
            rightDir = Vector3(1, 0, 0);
        }

        bool keyW = (GetAsyncKeyState('W') & 0x8000) != 0;
        bool keyS = (GetAsyncKeyState('S') & 0x8000) != 0;
        bool keyA = (GetAsyncKeyState('A') & 0x8000) != 0;
        bool keyD = (GetAsyncKeyState('D') & 0x8000) != 0;

        Vector3 moveDirection = Vector3(0, 0, 0);

        if (keyW) {
            moveDirection = moveDirection + forwardDirHorizontal;
        }
        if (keyS) {
            moveDirection = moveDirection - forwardDirHorizontal;
        }
        if (keyA) {
            moveDirection = moveDirection - rightDir;
        }
        if (keyD) {
            moveDirection = moveDirection + rightDir;
        }

        moveDirection.Y = 0.0f;

        bool isMoving = keyW || keyS || keyA || keyD;

        if (isMoving && Vector3::Magnitude(moveDirection) > 0.01f) {
            moveDirection = Vector3::Normalized(moveDirection); // CORRIGIDO

            float flySpeed = 0.05f;

            Vector3 newPos = currentPos + (moveDirection * flySpeed);
            newPos.Y = currentPos.Y;

            Vector3 displacement = newPos - noclipStartPos;
            displacement.Y = 0.0f;
            float distanceFromStart = Vector3::Magnitude(displacement);

            if (distanceFromStart > MAX_DISTANCE) {
                Vector3 directionToNewPos = Vector3::Normalized(displacement); // CORRIGIDO
                Vector3 clampedDisplacement = directionToNewPos * MAX_DISTANCE;
                newPos = noclipStartPos + clampedDisplacement;
                newPos.Y = currentPos.Y;
            }

            lastNoclipPos = newPos;
            wasNoclipActive = true;
            confirmFrames = 0;

            uintptr_t HHCBNAPCKHF = Ler<uintptr_t>(JogadorLocal + Offsets.PlayerNetwork_HHCBNAPCKHF);
            if (HHCBNAPCKHF != 0) {
                uint32_t offsetPos1 = string2Offset(AY_OBFUSCATE("0x1C"));
                uint32_t offsetPos2 = string2Offset(AY_OBFUSCATE("0x28"));
                Escrever<Vector3>(HHCBNAPCKHF + offsetPos1, newPos);
                Escrever<Vector3>(HHCBNAPCKHF + offsetPos2, newPos);
            }

            uintptr_t playerCachedTransform = Ler<uintptr_t>(JogadorLocal + Offsets.m_CachedTransform);
            if (playerCachedTransform != 0) {
                Transform$$DefinirPosicao(playerCachedTransform, newPos);
            }
        }
        else {
            if (lastNoclipPos.X != 0 || lastNoclipPos.Y != 0 || lastNoclipPos.Z != 0) {
                uintptr_t HHCBNAPCKHF = Ler<uintptr_t>(JogadorLocal + Offsets.PlayerNetwork_HHCBNAPCKHF);
                if (HHCBNAPCKHF != 0) {
                    uint32_t offsetPos1 = string2Offset(AY_OBFUSCATE("0x1C"));
                    uint32_t offsetPos2 = string2Offset(AY_OBFUSCATE("0x28"));
                    Escrever<Vector3>(HHCBNAPCKHF + offsetPos1, lastNoclipPos);
                    Escrever<Vector3>(HHCBNAPCKHF + offsetPos2, lastNoclipPos);
                }

                uintptr_t playerCachedTransform = Ler<uintptr_t>(JogadorLocal + Offsets.m_CachedTransform);
                if (playerCachedTransform != 0) {
                    Transform$$DefinirPosicao(playerCachedTransform, lastNoclipPos);
                }
            }
        }
    }
    else if (wasNoclipActive && confirmFrames < CONFIRM_FRAME_COUNT) {
        if (lastNoclipPos.X != 0 || lastNoclipPos.Y != 0 || lastNoclipPos.Z != 0) {
            uintptr_t HHCBNAPCKHF = Ler<uintptr_t>(JogadorLocal + Offsets.PlayerNetwork_HHCBNAPCKHF);
            if (HHCBNAPCKHF != 0) {
                uint32_t offsetPos1 = string2Offset(AY_OBFUSCATE("0x1C"));
                uint32_t offsetPos2 = string2Offset(AY_OBFUSCATE("0x28"));
                Escrever<Vector3>(HHCBNAPCKHF + offsetPos1, lastNoclipPos);
                Escrever<Vector3>(HHCBNAPCKHF + offsetPos2, lastNoclipPos);
            }

            uintptr_t playerCachedTransform = Ler<uintptr_t>(JogadorLocal + Offsets.m_CachedTransform);
            if (playerCachedTransform != 0) {
                for (int i = 0; i < 10; i++) {
                    Transform$$DefinirPosicao(playerCachedTransform, lastNoclipPos);
                }
            }

            confirmFrames++;
        }
    }
    else {
        if (wasNoclipActive && confirmFrames >= CONFIRM_FRAME_COUNT) {
            wasNoclipActive = false;
            confirmFrames = 0;
            lastNoclipPos = Vector3(0, 0, 0);
            noclipStartPosSaved = false;
            noclipStartPos = Vector3(0, 0, 0);
        }
    }
}

void ProcessDownEnemy(uintptr_t JogadorLocal) {
    if (!DownPlayer) return;

    uintptr_t ClosestEnemy = 0;
    float ClosestDistance = 9999.9f;
    Vector3 PlayerPos = GetPlayerPosition(JogadorLocal, 0);

    uintptr_t GameEngine = Ler<uintptr_t>(Ler<uintptr_t>(Ler<uintptr_t>(il2cpp + Offsets.GameEngine) + Offsets.jsonuni));
    if (GameEngine == 0) return;

    uintptr_t BaseGame = Ler<uintptr_t>(GameEngine + Offsets.BaseGame);
    if (BaseGame == 0) return;

    uintptr_t Partida = Ler<uintptr_t>(BaseGame + Offsets.Partida);
    if (Partida == 0) return;

    uintptr_t List = Ler<uintptr_t>(Partida + string2Offset(AY_OBFUSCATE("0x11C")));
    if (List == 0) return;

    int entityCount = Ler<int>(List + string2Offset(AY_OBFUSCATE("0xC")));
    if (entityCount <= 0) return;

    uintptr_t ListBase = Ler<uintptr_t>(List + string2Offset(AY_OBFUSCATE("0x8")));
    if (ListBase == 0) return;

    for (int i = 0; i < entityCount; i++) {
        uintptr_t Entity = Ler<uintptr_t>(ListBase + string2Offset(AY_OBFUSCATE("0x10")) + string2Offset(AY_OBFUSCATE("0x4")) * i);
        if (Entity == 0 || Entity == JogadorLocal) continue;

        uintptr_t AvatarManager = Ler<uintptr_t>(Entity + Offsets.AvatarManager);
        if (AvatarManager == 0) continue;

        uintptr_t UmaAvatarSimple = Ler<uintptr_t>(AvatarManager + Offsets.UmaAvatarSimple);
        if (UmaAvatarSimple == 0) continue;

        uintptr_t UmaData = Ler<uintptr_t>(UmaAvatarSimple + Offsets.UMAData);
        if (UmaData == 0) continue;

        if (Ler<bool>(UmaData + Offsets.TeamMate)) continue;

        uintptr_t IPRIDataPool = Ler<uintptr_t>(Entity + Offsets.PRIDataPool);
        if (IPRIDataPool == 0) continue;

        uintptr_t ReplicationDataPoolUnsafe = Ler<uintptr_t>(IPRIDataPool + Offsets.ReplicationDataPoolUnsafe);
        if (ReplicationDataPoolUnsafe == 0) continue;

        uintptr_t ReplicationDataUnsafe = Ler<uintptr_t>(ReplicationDataPoolUnsafe + Offsets.ReplicationDataUnsafe);
        if (ReplicationDataUnsafe == 0) continue;

        short Health = Ler<short>(ReplicationDataUnsafe + Offsets.Health);
        if (Health <= 0) continue;

        bool isDying = false;
        uintptr_t m_IsKnockStatus = Ler<uintptr_t>(Entity + Offsets.PlayerNetwork_HHCBNAPCKHF);
        if (m_IsKnockStatus != 0) {
            int enemyStatus = Ler<int>(m_IsKnockStatus + Offsets.StatePlayer);
            isDying = (enemyStatus == 8);
        }
        if (isDying) continue;

        Vector3 EnemyPos = GetPlayerPosition(Entity, 0);
        if (EnemyPos.X == 0 && EnemyPos.Y == 0 && EnemyPos.Z == 0) continue;

        float distance = Vector3::Distance(PlayerPos, EnemyPos);
        if (distance > AimbotDistance) continue;

        if (distance < ClosestDistance) {
            ClosestDistance = distance;
            ClosestEnemy = Entity;
        }
    }

    const float DOWN_OFFSET = 0.9f;

    if (ClosestEnemy != 0) {
        Vector3 currentPos = GetPlayerPosition(ClosestEnemy, 0);
        if (currentPos.X == 0 && currentPos.Y == 0 && currentPos.Z == 0) {
            return;
        }

        Vector3 downPos = currentPos;
        downPos.Y -= DOWN_OFFSET;

        uintptr_t enemyNetwork = Ler<uintptr_t>(ClosestEnemy + Offsets.PlayerNetwork_HHCBNAPCKHF);
        if (enemyNetwork != 0) {
            uint32_t offsetPos1 = string2Offset(AY_OBFUSCATE("0x1C"));
            uint32_t offsetPos2 = string2Offset(AY_OBFUSCATE("0x28"));
            Escrever<Vector3>(enemyNetwork + offsetPos1, downPos);
            Escrever<Vector3>(enemyNetwork + offsetPos2, downPos);
        }

        uintptr_t enemyCachedTransform = Ler<uintptr_t>(ClosestEnemy + Offsets.m_CachedTransform);
        if (enemyCachedTransform != 0) {
            Transform$$DefinirPosicao(enemyCachedTransform, downPos);
        }
    }
}
void ProcessDownLocalPlayer(uintptr_t JogadorLocal) {
    if (!DownLocalPlayer || JogadorLocal == 0) return;

    const float DOWN_OFFSET = 0.9f;

    {
        Vector3 currentPos = GetPlayerPosition(JogadorLocal, 0);
        if (currentPos.X == 0 && currentPos.Y == 0 && currentPos.Z == 0) {
            return;
        }

        Vector3 downPos = currentPos;
        downPos.Y -= DOWN_OFFSET;

        uintptr_t HHCBNAPCKHF = Ler<uintptr_t>(JogadorLocal + Offsets.PlayerNetwork_HHCBNAPCKHF);
        if (HHCBNAPCKHF != 0) {
            uint32_t offsetPos1 = string2Offset(AY_OBFUSCATE("0x1C"));
            uint32_t offsetPos2 = string2Offset(AY_OBFUSCATE("0x28"));
            Escrever<Vector3>(HHCBNAPCKHF + offsetPos1, downPos);
            Escrever<Vector3>(HHCBNAPCKHF + offsetPos2, downPos);
        }

        uintptr_t playerCachedTransform = Ler<uintptr_t>(JogadorLocal + Offsets.m_CachedTransform);
        if (playerCachedTransform != 0) {
            Transform$$DefinirPosicao(playerCachedTransform, downPos);
        }
    }
}

void ProcessUpEnemy(uintptr_t JogadorLocal) {
    if (!UpPlayer) return;

    uintptr_t ClosestEnemy = 0;
    float ClosestDistance = 9999.9f;
    Vector3 PlayerPos = GetPlayerPosition(JogadorLocal, 0);

    uintptr_t GameEngine = Ler<uintptr_t>(Ler<uintptr_t>(Ler<uintptr_t>(il2cpp + Offsets.GameEngine) + Offsets.jsonuni));
    if (GameEngine == 0) return;

    uintptr_t BaseGame = Ler<uintptr_t>(GameEngine + Offsets.BaseGame);
    if (BaseGame == 0) return;

    uintptr_t Partida = Ler<uintptr_t>(BaseGame + Offsets.Partida);
    if (Partida == 0) return;

    uintptr_t List = Ler<uintptr_t>(Partida + string2Offset(AY_OBFUSCATE("0x11C")));
    if (List == 0) return;

    int entityCount = Ler<int>(List + string2Offset(AY_OBFUSCATE("0xC")));
    if (entityCount <= 0) return;

    uintptr_t ListBase = Ler<uintptr_t>(List + string2Offset(AY_OBFUSCATE("0x8")));
    if (ListBase == 0) return;

    for (int i = 0; i < entityCount; i++) {
        uintptr_t Entity = Ler<uintptr_t>(ListBase + string2Offset(AY_OBFUSCATE("0x10")) + string2Offset(AY_OBFUSCATE("0x4")) * i);
        if (Entity == 0 || Entity == JogadorLocal) continue;

        uintptr_t AvatarManager = Ler<uintptr_t>(Entity + Offsets.AvatarManager);
        if (AvatarManager == 0) continue;

        uintptr_t UmaAvatarSimple = Ler<uintptr_t>(AvatarManager + Offsets.UmaAvatarSimple);
        if (UmaAvatarSimple == 0) continue;

        uintptr_t UmaData = Ler<uintptr_t>(UmaAvatarSimple + Offsets.UMAData);
        if (UmaData == 0) continue;

        if (Ler<bool>(UmaData + Offsets.TeamMate)) continue;

        uintptr_t IPRIDataPool = Ler<uintptr_t>(Entity + Offsets.PRIDataPool);
        if (IPRIDataPool == 0) continue;

        uintptr_t ReplicationDataPoolUnsafe = Ler<uintptr_t>(IPRIDataPool + Offsets.ReplicationDataPoolUnsafe);
        if (ReplicationDataPoolUnsafe == 0) continue;

        uintptr_t ReplicationDataUnsafe = Ler<uintptr_t>(ReplicationDataPoolUnsafe + Offsets.ReplicationDataUnsafe);
        if (ReplicationDataUnsafe == 0) continue;

        short Health = Ler<short>(ReplicationDataUnsafe + Offsets.Health);
        if (Health <= 0) continue;

        bool isDying = false;
        uintptr_t m_IsKnockStatus = Ler<uintptr_t>(Entity + Offsets.PlayerNetwork_HHCBNAPCKHF);
        if (m_IsKnockStatus != 0) {
            int enemyStatus = Ler<int>(m_IsKnockStatus + Offsets.StatePlayer);
            isDying = (enemyStatus == 8);
        }
        if (isDying) continue;

        Vector3 EnemyPos = GetPlayerPosition(Entity, 0);
        if (EnemyPos.X == 0 && EnemyPos.Y == 0 && EnemyPos.Z == 0) continue;

        float distance = Vector3::Distance(PlayerPos, EnemyPos);
        if (distance > AimbotDistance) continue;

        if (distance < ClosestDistance) {
            ClosestDistance = distance;
            ClosestEnemy = Entity;
        }
    }

    const float UP_OFFSET = 0.0600f;

    if (ClosestEnemy != 0) {
        Vector3 currentPos = GetPlayerPosition(ClosestEnemy, 0);
        if (currentPos.X == 0 && currentPos.Y == 0 && currentPos.Z == 0) {
            return;
        }

        Vector3 upPos = currentPos;
        upPos.Y += UP_OFFSET;

        uintptr_t enemyNetwork = Ler<uintptr_t>(ClosestEnemy + Offsets.PlayerNetwork_HHCBNAPCKHF);
        if (enemyNetwork != 0) {
            uint32_t offsetPos1 = string2Offset(AY_OBFUSCATE("0x1C"));
            uint32_t offsetPos2 = string2Offset(AY_OBFUSCATE("0x28"));
            Escrever<Vector3>(enemyNetwork + offsetPos1, upPos);
            Escrever<Vector3>(enemyNetwork + offsetPos2, upPos);
        }

        uintptr_t enemyCachedTransform = Ler<uintptr_t>(ClosestEnemy + Offsets.m_CachedTransform);
        if (enemyCachedTransform != 0) {
            Transform$$DefinirPosicao(enemyCachedTransform, upPos);
        }
    }
}

void ProcessMagneticEntity(uintptr_t JogadorLocal, UnityMatrix matrix, Vector3 MinhaPosicao_Camera) {
    uintptr_t ClosestEnemy = 0;
    Vector3 ClosestEnemyPos(0, 0, 0);
    Vector3 forward = ObterDirecaoMagnetic(matrix);
    Vector3 PlayerPos = GetPlayerPosition(JogadorLocal, 0);

    uintptr_t GameEngine = Ler<uintptr_t>(Ler<uintptr_t>(Ler<uintptr_t>(il2cpp + Offsets.GameEngine) + Offsets.jsonuni));
    if (GameEngine == 0) return;

    uintptr_t BaseGame = Ler<uintptr_t>(GameEngine + Offsets.BaseGame);
    if (BaseGame == 0) return;

    uintptr_t Partida = Ler<uintptr_t>(BaseGame + Offsets.Partida);
    if (Partida == 0) return;

    uintptr_t List = Ler<uintptr_t>(Partida + string2Offset(AY_OBFUSCATE("0x11C")));
    if (List == 0) return;

    int entityCount = Ler<int>(List + string2Offset(AY_OBFUSCATE("0xC")));
    if (entityCount <= 0) return;

    uintptr_t ListBase = Ler<uintptr_t>(List + string2Offset(AY_OBFUSCATE("0x8")));
    if (ListBase == 0) return;

    float DistanciaMinimaDaMira = 9999.9f;

    for (int i = 0; i < entityCount; i++) {
        uintptr_t Entity = Ler<uintptr_t>(ListBase + string2Offset(AY_OBFUSCATE("0x10")) + string2Offset(AY_OBFUSCATE("0x4")) * i);
        if (Entity == 0 || Entity == JogadorLocal) continue;

        uintptr_t AvatarManager = Ler<uintptr_t>(Entity + Offsets.AvatarManager);
        if (AvatarManager == 0) continue;

        uintptr_t UmaAvatarSimple = Ler<uintptr_t>(AvatarManager + Offsets.UmaAvatarSimple);
        if (UmaAvatarSimple == 0) continue;

        uintptr_t UmaData = Ler<uintptr_t>(UmaAvatarSimple + Offsets.UMAData);
        if (UmaData == 0) continue;

        if (Ler<bool>(UmaData + Offsets.TeamMate)) continue;

        uintptr_t IPRIDataPool = Ler<uintptr_t>(Entity + Offsets.PRIDataPool);
        if (IPRIDataPool == 0) continue;

        uintptr_t ReplicationDataPoolUnsafe = Ler<uintptr_t>(IPRIDataPool + Offsets.ReplicationDataPoolUnsafe);
        if (ReplicationDataPoolUnsafe == 0) continue;

        uintptr_t ReplicationDataUnsafe = Ler<uintptr_t>(ReplicationDataPoolUnsafe + Offsets.ReplicationDataUnsafe);
        if (ReplicationDataUnsafe == 0) continue;

        short Health = Ler<short>(ReplicationDataUnsafe + Offsets.Health);
        if (Health <= 0) continue;

        bool isDying = false;
        uintptr_t m_IsKnockStatus = Ler<uintptr_t>(Entity + Offsets.PlayerNetwork_HHCBNAPCKHF);
        if (m_IsKnockStatus != 0) {
            int enemyStatus = Ler<int>(m_IsKnockStatus + Offsets.StatePlayer);
            isDying = (enemyStatus == 8);
        }
        if (isDying) continue;

        Vector3 EnemyPos = GetPlayerPosition(Entity, 0);
        if (EnemyPos.X == 0 && EnemyPos.Y == 0 && EnemyPos.Z == 0) continue;

        float distance = Vector3::Distance(PlayerPos, EnemyPos);
        if (distance > AimbotDistance) continue;

        Vector3 toEnemy = EnemyPos - MinhaPosicao_Camera;
        float dotProduct = Vector3::Dot(toEnemy, forward);
        Vector3 closestPointOnRay = MinhaPosicao_Camera + (forward * dotProduct);
        float distanceToCrosshair = Vector3::Distance(EnemyPos, closestPointOnRay);

        if (distanceToCrosshair < DistanciaMinimaDaMira) {
            DistanciaMinimaDaMira = distanceToCrosshair;
            ClosestEnemy = Entity;
            ClosestEnemyPos = EnemyPos;
        }

    }

    if (MagneticKillv3 && AimbotDistance >= 10)
    {
        static bool travado = false;
        static uintptr_t inimigoTravado = 0;
        static Vector3 dirTravada(0, 0, 0);
        static float distTravada = 0.0f;
        static auto ultimoReset = std::chrono::steady_clock::now();

        if (MagneticKillv3 && IsBindModeActive(KeysBind.Magnetic))
        {
            if (ClosestEnemy != 0)
            {
                if (!travado)
                {
                    travado = true;
                    inimigoTravado = ClosestEnemy;

                    Vector3 minhaPos = GetPlayerPosition(JogadorLocal, 0);
                    Vector3 posInimigo = GetPlayerPosition(inimigoTravado, 0);

                    Vector3 direcao = ObterDirecaoMagnetic(matrix);
                    dirTravada = NormalizeVector(direcao);
                    distTravada = Vector3::Distance(minhaPos, posInimigo);
                }

                if (inimigoTravado == ClosestEnemy && inimigoTravado != 0)
                {
                    Vector3 minhaPos = GetPlayerPosition(JogadorLocal, 0);
                    Vector3 posInimigo = GetPlayerPosition(inimigoTravado, 0);

                    uintptr_t HHCBNAPCKHF = Ler<uintptr_t>(inimigoTravado + Offsets.PlayerNetwork_HHCBNAPCKHF);
                    if (HHCBNAPCKHF == 0) {
                        travado = false;
                        inimigoTravado = 0;
                        return;
                    }

                    float distVisao = DistanciaLinha(minhaPos, dirTravada, posInimigo);
                    if (distVisao <= 8.0f)
                    {
                        Vector3 posAlvo = minhaPos + (dirTravada * distTravada);
                        Vector3 posFinal(posAlvo.X, posAlvo.Y, posAlvo.Z);

                        float dist = Vector3::Distance(posInimigo, posFinal);
                        if (dist > 8.0f)
                        {
                            Vector3 diff = posFinal - posInimigo;
                            Vector3 dir = NormalizeVector(diff);
                            posFinal = posInimigo + dir * 8.0f;
                        }

                        Escrever<Vector3>(HHCBNAPCKHF + string2Offset(AY_OBFUSCATE("0x1C")), posFinal);
                        Escrever<Vector3>(HHCBNAPCKHF + string2Offset(AY_OBFUSCATE("0x28")), posFinal);

                        ultimoReset = std::chrono::steady_clock::now();
                    }
                }
            }
            else
            {
                travado = false;
                inimigoTravado = 0;
            }
        }
        else
        {
            auto agora = std::chrono::steady_clock::now();
            auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(agora - ultimoReset).count();
            if (ms >= 300)
            {
                travado = false;
                inimigoTravado = 0;
            }
        }
    }
}

uintptr_t JogadorLocal = 0;

void DesenharESP(int width, int height) {

    static bool lastStreamMode = false;
    if (StreamProof != lastStreamMode && enumWindow != nullptr) {
        UINT affinity = StreamProof ? WDA_EXCLUDEFROMCAPTURE : WDA_NONE;
        SetWindowDisplayAffinity(enumWindow, affinity);
        lastStreamMode = StreamProof;
    }
    Enemies = 0;
    uintptr_t ClosestEnemyForTeleport = 0;
    SWidth = width;
    SHeight = height;

    if (!isLoggedIn) {
        return;
    }
    if (!isAttached) {
        return;
    }
    if (VMM.GuestCR3 == 0) {
        return;
    }

    uintptr_t ClosestEnemy = 0;
    float DistanciaMaxima = 9999.0f;
    uintptr_t ClosestSilentEnemy = 0;
    float DistanciaMaximaSilent = 9999.0f;

    // Guest é 32-bit: ler ponteiros com 4 bytes (Ler<uint32_t>) para não juntar dois ponteiros em um
    uintptr_t gameEngineOffset = Offsets.GameEngine != 0 ? Offsets.GameEngine : string2Offset(AY_OBFUSCATE("0xA544F18"));
    uintptr_t GameEngine = (uintptr_t)Ler<uint32_t>((uintptr_t)Ler<uint32_t>((uintptr_t)Ler<uint32_t>(il2cpp + gameEngineOffset) + Offsets.jsonuni));
    if (GameEngine == 0) {
        GameEngine = (uintptr_t)Ler<uint32_t>((uintptr_t)Ler<uint32_t>(il2cpp + gameEngineOffset) + Offsets.jsonuni);
    }
    if (GameEngine == 0) {
        return;
    }

    uintptr_t BaseGame = (uintptr_t)Ler<uint32_t>(GameEngine + Offsets.BaseGame);
    uintptr_t currentGame = 0;
    uintptr_t _initBase = (uintptr_t)Ler<uint32_t>(il2cpp + gameEngineOffset);
    if (_initBase != 0 && _initBase != 0xFFFFFFFF) {
        uintptr_t _baseGameFacade = (uintptr_t)Ler<uint32_t>(_initBase + Offsets.jsonuni);
        if (_baseGameFacade != 0 && _baseGameFacade != 0xFFFFFFFF) {
            uintptr_t _gameFacade = (uintptr_t)Ler<uint32_t>(_baseGameFacade);
            if (_gameFacade != 0 && _gameFacade != 0xFFFFFFFF) {
                uintptr_t _staticClass = (uintptr_t)Ler<uint32_t>(_gameFacade + Offsets.jsonuni);
                if (_staticClass != 0 && _staticClass != 0xFFFFFFFF) {
                    currentGame = (uintptr_t)Ler<uint32_t>(_staticClass);
                }
            }
        }
    }
    if (currentGame == 0 || currentGame == 0xFFFFFFFF)
        currentGame = BaseGame;
    if (currentGame == 0 || currentGame == 0xFFFFFFFF) {
        return;
    }
    BaseGame = currentGame;

    uintptr_t Partida = (uintptr_t)Ler<uint32_t>(BaseGame + Offsets.Partida);
    if (Partida == 0) {
        return;
    }

    int MatchState = Ler<int>(Partida + string2Offset(AY_OBFUSCATE("0x74")));
    if (MatchState != 1) {
        return;
    }

        JogadorLocal = (uintptr_t)Ler<uint32_t>(Partida + Offsets.JogadorLocal);
        
        // No Reload Logic
        if (fastreload && JogadorLocal != 0) {
            uintptr_t localplayerAttr = (uintptr_t)Ler<uint32_t>(JogadorLocal + 0x45C);
            if (localplayerAttr != 0 && localplayerAttr < 0xFFFFFFFF) {
                Escrever<bool>(localplayerAttr + 0x91, true);
            }
        }

        uintptr_t Arma = (uintptr_t)Ler<uint32_t>((uintptr_t)Ler<uint32_t>(JogadorLocal + Offsets.Inventario) + Offsets.Inventario_Item);
        uintptr_t JogadorAtributo = (uintptr_t)Ler<uint32_t>(JogadorLocal + Offsets.PlayerAttributes);
        uintptr_t TempoJogo = (uintptr_t)Ler<uint32_t>(JogadorLocal + Offsets.GameTimer);


        // Bloquear Mira (igual meu: Ptr_gameVar, m_GameVarDef, offset 0x5A8)
        uintptr_t Ptr_gameVar = (uintptr_t)Ler<uint32_t>(il2cpp + Offsets.GameVarClass.GameVarPtr);
        uintptr_t m_GameVarDef = (uintptr_t)Ler<uint32_t>(Ptr_gameVar + 0x5c);
        if (Ptr_gameVar != 0 && m_GameVarDef != 0) {
            if (Bloquearmira) {
                Escrever<bool>(m_GameVarDef + 0x5A8, false);
            }
            else {
                Escrever<bool>(m_GameVarDef + 0x5A8, true);
            }
        }

        // Long Parachute + Under Camera
        uintptr_t baseUnder = Ler<uintptr_t>(il2cpp + string2Offset(AY_OBFUSCATE("0xA547A98")));
        if (baseUnder != 0) {
            baseUnder = Ler<uintptr_t>(baseUnder + 0x5C);
            if (baseUnder != 0) {
                float valueToWrite = 1.0f;
                if (UnderCamEnabled) {
                    valueToWrite = 2.5f;
                }
                if (longparachute) {
                    valueToWrite = 19.0f;
                }
                Escrever<float>(baseUnder + 0x184, valueToWrite);
            }
        }

        // Scope Sniper (FOV AWM)
        static bool fovAtivo = false;
        static uintptr_t savedWeaponAddr = 0;
        static int savedFovVal = 0;

        if (fovawm) {
            uintptr_t ActiveUISightingWeapon = Ler<uintptr_t>(JogadorLocal + string2Offset(AY_OBFUSCATE("0x39C")));
            if (ActiveUISightingWeapon != 0) {
                uintptr_t WeaponData = Ler<uintptr_t>(ActiveUISightingWeapon + 0x64);
                if (WeaponData != 0) {
                    if (!fovAtivo) {
                        savedWeaponAddr = WeaponData;
                        savedFovVal = Ler<int>(WeaponData + 0xB0);
                        Escrever<int>(WeaponData + 0xB0, 0);
                        fovAtivo = true;
                    }
                }
            }
        }
        else {
            if (fovAtivo) {
                try {
                    if (savedWeaponAddr != 0) {
                        Escrever<int>(savedWeaponAddr + 0xB0, savedFovVal);
                    }
                }
                catch (const std::exception&) {}
                fovAtivo = false;
                savedWeaponAddr = 0;
            }
        }

        // aimlock2x
        /*if (Arma != 0) {
            uintptr_t PFMPPELJECF = (uintptr_t)Ler<uint32_t>(Arma + 0x380);
            if (PFMPPELJECF != 0) {
                uintptr_t JCPOIGJPKHP = (uintptr_t)Ler<uint32_t>(PFMPPELJECF + 0x84);
                if (JCPOIGJPKHP != 0) {
                    Escrever<float>(JCPOIGJPKHP + 0x4C, 1.90277084e17f);
                }
            }
        }*/


        // Fast Swap
        static std::vector<std::pair<uintptr_t, float>> s_FastSwapOriginals;
        static bool s_FastSwapWasActive = false;

        if (JogadorLocal != 0) {
            bool isFastSwapActive = FastSwitch;

            if (isFastSwapActive) {
                if (!s_FastSwapWasActive) {
                    float originalSwapSpeed = Ler<float>(JogadorLocal + Offsets.m_SwapWeaponTime);

                    bool exists = false;
                    for (auto& pair : s_FastSwapOriginals) {
                        if (pair.first == JogadorLocal) {
                            exists = true;
                            break;
                        }
                    }

                    if (!exists) {
                        s_FastSwapOriginals.emplace_back(JogadorLocal, originalSwapSpeed);
                    }
                }

                Escrever<float>(JogadorLocal + Offsets.m_SwapWeaponTime, FastSwapSpeed);
            }
            else {
                if (s_FastSwapWasActive) {
                    for (const auto& pair : s_FastSwapOriginals) {
                        if (pair.first == JogadorLocal && pair.first != 0) {
                            Escrever<float>(pair.first + Offsets.m_SwapWeaponTime, pair.second);
                            break;
                        }
                    }

                    for (size_t i = 0; i < s_FastSwapOriginals.size(); i++) {
                        if (s_FastSwapOriginals[i].first == JogadorLocal) {
                            s_FastSwapOriginals.erase(s_FastSwapOriginals.begin() + i);
                            break;
                        }
                    }
                }
            }

            s_FastSwapWasActive = isFastSwapActive;
        }

        // Camera direita + Down (igual meu: baseUnder, il2cpp+0xA547A98, offset 0x184)
        // Já processado acima na seção Long Parachute

        Vector3 MinhaPosicao = Transform$$ObterPosicao((uintptr_t)Ler<uint32_t>(JogadorLocal + Offsets.MainTransform));
        Vector3 MinhaPosicao_Camera = Transform$$ObterPosicao((uintptr_t)Ler<uint32_t>(JogadorLocal + Offsets.MainCamTran));
        Vector3 MinhaPosicao_Cabeca = GetHeadPosition(JogadorLocal);
        
        uintptr_t CameraControllerManager = (uintptr_t)Ler<uint32_t>(BaseGame + 0x74);
        if (CameraControllerManager == 0) {
            return;
        }

        uintptr_t Camera = (uintptr_t)Ler<uint32_t>(CameraControllerManager + string2Offset(AY_OBFUSCATE("0xC")));
        if (Camera == 0) {
            return;
        }

        uintptr_t IntPtrCam = (uintptr_t)Ler<uint32_t>(Camera + string2Offset(AY_OBFUSCATE("0x8")));
        if (IntPtrCam == 0) {
            return;
        }

        UnityMatrix matrix = Ler<UnityMatrix>(IntPtrCam + Offsets.Matrix);


        bool LJHKFOOOPBF = Ler<bool>(JogadorLocal + string2Offset(AY_OBFUSCATE("0x4C0")));
        if (GhostHack != LJHKFOOOPBF) {
            Escrever<bool>(JogadorLocal + string2Offset(AY_OBFUSCATE("0x4C0")), GhostHack);
        }

        // WallHack (igual meu: comentado)
        uintptr_t valueRead = Ler<uint32_t>(0x0E0977FC);
        uintptr_t value = WallHack ? 0xC1200000 : 0x3F800000;
        if (valueRead != value) {
            Escrever<uint32_t>(0x0E0977FC, value);
        }

        static bool MyPosSaved = false;
        if (GhostHack) {
            bool Garota = Ler<bool>(JogadorLocal + Offsets.isfemale);

            uintptr_t TransformHead;
            uintptr_t TransformChest;
            uintptr_t TransformStomach;
            uintptr_t TransformHip;
            uintptr_t TransformLeftShoulder;
            uintptr_t TransformLeftArm;
            uintptr_t TransformLeftForearm;
            uintptr_t TransformLeftHand;
            uintptr_t TransformRightShoulder;
            uintptr_t TransformRightArm;
            uintptr_t TransformRightForearm;
            uintptr_t TransformRightHand;
            uintptr_t TransformLeftThigh;
            uintptr_t TransformLeftShin;
            uintptr_t TransformLeftFoot;
            uintptr_t TransformRightFoot;
            uintptr_t TransformRightThigh;
            uintptr_t TransformRightShin;

            if (Garota) {
                TransformHead = string2Offset(AY_OBFUSCATE("0x3C")); //Cabeça
                TransformChest = string2Offset(AY_OBFUSCATE("0x18")); //Peito
                TransformStomach = string2Offset(AY_OBFUSCATE("0x14")); // Barriga
                TransformHip = string2Offset(AY_OBFUSCATE("0x10"));// Quadil

                TransformLeftShoulder = string2Offset(AY_OBFUSCATE("0x1C")); //OmbroEsquerdo
                TransformLeftArm = string2Offset(AY_OBFUSCATE("0x20")); //BraçoEsquerdo
                TransformLeftForearm = string2Offset(AY_OBFUSCATE("0x24")); //AntBraçoEsquerdo
                TransformLeftHand = string2Offset(AY_OBFUSCATE("0x28")); //MãoEsquerdo

                TransformRightShoulder = string2Offset(AY_OBFUSCATE("0x2C")); //OmbroDireito
                TransformRightArm = string2Offset(AY_OBFUSCATE("0x30")); //BraçoEsquerdo
                TransformRightForearm = string2Offset(AY_OBFUSCATE("0x34")); //AntBraçoDireito
                TransformRightHand = string2Offset(AY_OBFUSCATE("0x38")); //MãoDireito

                TransformLeftThigh = string2Offset(AY_OBFUSCATE("0x40")); //CoxaEsquerda
                TransformLeftShin = string2Offset(AY_OBFUSCATE("0x44")); //CanelaEsquerda
                TransformLeftFoot = string2Offset(AY_OBFUSCATE("0x48")); //PéEsquerdo

                TransformRightThigh = string2Offset(AY_OBFUSCATE("0x4C")); //CoxaDireito
                TransformRightShin = string2Offset(AY_OBFUSCATE("0x50")); //CanelaDireito
                TransformRightFoot = string2Offset(AY_OBFUSCATE("0x54")); //PéDireito
            }
            else {
                TransformHead = string2Offset(AY_OBFUSCATE("0x38")); //Cabeça
                TransformChest = string2Offset(AY_OBFUSCATE("0x14")); //Peito
                TransformStomach = string2Offset(AY_OBFUSCATE("0x10")); //Barriga
                TransformHip = string2Offset(AY_OBFUSCATE("0x48")); //Quadril

                TransformLeftShoulder = string2Offset(AY_OBFUSCATE("0x18")); //OmbroEsquerdo
                TransformLeftArm = string2Offset(AY_OBFUSCATE("0x1C")); //BraçoEsquerdo
                TransformLeftForearm = string2Offset(AY_OBFUSCATE("0x20")); //AntBraçoEsquerdo
                TransformLeftHand = string2Offset(AY_OBFUSCATE("0x24")); //MãoEsquerdo

                TransformRightShoulder = string2Offset(AY_OBFUSCATE("0x28")); //OmbroDireito
                TransformRightArm = string2Offset(AY_OBFUSCATE("0x2C")); //BraçoDireito
                TransformRightForearm = string2Offset(AY_OBFUSCATE("0x30")); //AntBraçoDireito
                TransformRightHand = string2Offset(AY_OBFUSCATE("0x34")); //MãoDireito

                TransformLeftThigh = string2Offset(AY_OBFUSCATE("0x3C")); //CoxaEsquerda
                TransformLeftShin = string2Offset(AY_OBFUSCATE("0x40")); //CanelaEsquerda
                TransformLeftFoot = string2Offset(AY_OBFUSCATE("0x44")); //PéEsquerdo

                TransformRightThigh = string2Offset(AY_OBFUSCATE("0x4C"));//CoxaDireito
                TransformRightShin = string2Offset(AY_OBFUSCATE("0x50"));//CanelaDireito
                TransformRightFoot = string2Offset(AY_OBFUSCATE("0x54"));//PéDireito
            }

            static Vector3 EnemyPositionHeadBoxVEC3 = Vector3(0, 0, 0);
            static Vector3 LocationChestVEC3 = Vector3(0, 0, 0);
            static Vector3 LocationStomachVEC3 = Vector3(0, 0, 0);

            static Vector3 LocationHipVEC3 = Vector3(0, 0, 0);
            static Vector3 LocationLeftShoulderVEC3 = Vector3(0, 0, 0);
            static Vector3 LocationLeftArmVEC3 = Vector3(0, 0, 0);
            static Vector3 LocationLeftForearmVEC3 = Vector3(0, 0, 0);
            static Vector3 LocationLeftHandVEC3 = Vector3(0, 0, 0);
            static Vector3 LocationRightShoulderVEC3 = Vector3(0, 0, 0);
            static Vector3 LocationRightArmVEC3 = Vector3(0, 0, 0);
            static Vector3 LocationRightForearmVEC3 = Vector3(0, 0, 0);
            static Vector3 LocationRightHandVEC3 = Vector3(0, 0, 0);
            static Vector3 LocationLeftThighVEC3 = Vector3(0, 0, 0);
            static Vector3 LocationLeftShinVEC3 = Vector3(0, 0, 0);
            static Vector3 LocationLeftFootVEC3 = Vector3(0, 0, 0);
            static Vector3 LocationRightThighVEC3 = Vector3(0, 0, 0);
            static Vector3 LocationRightShinVEC3 = Vector3(0, 0, 0);
            static Vector3 LocationRightFootVEC3 = Vector3(0, 0, 0);

            if (!MyPosSaved) {
                MyPosSaved = true;
                EnemyPositionHeadBoxVEC3 = ObterOssos(JogadorLocal, TransformHead);
                LocationChestVEC3 = ObterOssos(JogadorLocal, TransformChest);
                LocationStomachVEC3 = ObterOssos(JogadorLocal, TransformStomach);

                LocationHipVEC3 = ObterOssos(JogadorLocal, TransformHip);
                LocationLeftShoulderVEC3 = ObterOssos(JogadorLocal, TransformLeftShoulder);
                LocationLeftArmVEC3 = ObterOssos(JogadorLocal, TransformLeftArm);
                LocationLeftForearmVEC3 = ObterOssos(JogadorLocal, TransformLeftForearm);

                LocationLeftHandVEC3 = ObterOssos(JogadorLocal, TransformLeftHand);
                LocationRightShoulderVEC3 = ObterOssos(JogadorLocal, TransformRightShoulder);
                LocationRightArmVEC3 = ObterOssos(JogadorLocal, TransformRightArm);

                LocationRightForearmVEC3 = ObterOssos(JogadorLocal, TransformRightForearm);
                LocationRightHandVEC3 = ObterOssos(JogadorLocal, TransformRightHand);
                LocationLeftThighVEC3 = ObterOssos(JogadorLocal, TransformLeftThigh);
                LocationLeftShinVEC3 = ObterOssos(JogadorLocal, TransformLeftShin);
                LocationLeftFootVEC3 = ObterOssos(JogadorLocal, TransformLeftFoot);
                LocationRightThighVEC3 = ObterOssos(JogadorLocal, TransformRightThigh);
                LocationRightShinVEC3 = ObterOssos(JogadorLocal, TransformRightThigh);
                LocationRightFootVEC3 = ObterOssos(JogadorLocal, TransformRightFoot);
            }

            Vector3 EnemyPositionHeadBox = World2Screen(matrix, EnemyPositionHeadBoxVEC3);
            Vector3 LocationChest = World2Screen(matrix, LocationChestVEC3);
            Vector3 LocationStomach = World2Screen(matrix, LocationStomachVEC3);
            Vector3 LocationHip = World2Screen(matrix, LocationHipVEC3);
            Vector3 LocationLeftShoulder = World2Screen(matrix, LocationLeftShoulderVEC3);
            Vector3 LocationLeftArm = World2Screen(matrix, LocationLeftArmVEC3);
            Vector3 LocationLeftForearm = World2Screen(matrix, LocationLeftForearmVEC3);

            Vector3 LocationLeftHand = World2Screen(matrix, LocationLeftHandVEC3);
            Vector3 LocationRightShoulder = World2Screen(matrix, LocationRightShoulderVEC3);
            Vector3 LocationRightArm = World2Screen(matrix, LocationRightArmVEC3);
            Vector3 LocationRightForearm = World2Screen(matrix, LocationRightForearmVEC3);
            Vector3 LocationRightHand = World2Screen(matrix, LocationRightHandVEC3);
            Vector3 LocationLeftThigh = World2Screen(matrix, LocationLeftThighVEC3);
            Vector3 LocationLeftShin = World2Screen(matrix, LocationLeftShinVEC3);
            Vector3 LocationLeftFoot = World2Screen(matrix, LocationLeftFootVEC3);
            Vector3 LocationRightThigh = World2Screen(matrix, LocationRightThighVEC3);
            Vector3 LocationRightShin = World2Screen(matrix, LocationRightShinVEC3);
            Vector3 LocationRightFoot = World2Screen(matrix, LocationRightFootVEC3);


            if (EnemyPositionHeadBox.Z > 0) {
                DrawLine(EnemyPositionHeadBox.X, EnemyPositionHeadBox.Y, LocationChest.X, LocationChest.Y, 2.0f, 255, 255, 255);
                DrawLine(LocationChest.X, LocationChest.Y, LocationStomach.X, LocationStomach.Y, 2.0f, 255, 255, 255);
                DrawLine(LocationStomach.X, LocationStomach.Y, LocationHip.X, LocationHip.Y, 2.0f, 255, 255, 255);

                DrawLine(LocationChest.X, LocationChest.Y, LocationLeftShoulder.X, LocationLeftShoulder.Y, 2.0f, 255, 255, 255);
                DrawLine(LocationLeftShoulder.X, LocationLeftShoulder.Y, LocationLeftArm.X, LocationLeftArm.Y, 2.0f, 255, 255, 255);
                DrawLine(LocationLeftArm.X, LocationLeftArm.Y, LocationLeftForearm.X, LocationLeftForearm.Y, 2.0f, 255, 255, 255);
                DrawLine(LocationLeftForearm.X, LocationLeftForearm.Y, LocationLeftHand.X, LocationLeftHand.Y, 2.0f, 255, 255, 255);

                DrawLine(LocationChest.X, LocationChest.Y, LocationRightShoulder.X, LocationRightShoulder.Y, 2.0f, 255, 255, 255);
                DrawLine(LocationRightShoulder.X, LocationRightShoulder.Y, LocationRightArm.X, LocationRightArm.Y, 2.0f, 255, 255, 255);
                DrawLine(LocationRightArm.X, LocationRightArm.Y, LocationRightForearm.X, LocationRightForearm.Y, 2.0f, 255, 255, 255);
                DrawLine(LocationRightForearm.X, LocationRightForearm.Y, LocationRightHand.X, LocationRightHand.Y, 2.0f, 255, 255, 255);

                DrawLine(LocationHip.X, LocationHip.Y, LocationLeftThigh.X, LocationLeftThigh.Y, 2.0f, 255, 255, 255);
                DrawLine(LocationLeftThigh.X, LocationLeftThigh.Y, LocationLeftShin.X, LocationLeftShin.Y, 2.0f, 255, 255, 255);
                DrawLine(LocationLeftShin.X, LocationLeftShin.Y, LocationLeftFoot.X, LocationLeftFoot.Y, 2.0f, 255, 255, 255);

                DrawLine(LocationHip.X, LocationHip.Y, LocationRightThigh.X, LocationRightThigh.Y, 2.0f, 255, 255, 255);
                DrawLine(LocationRightThigh.X, LocationRightThigh.Y, LocationRightShin.X, LocationRightShin.Y, 2.0f, 255, 255, 255);
                DrawLine(LocationRightShin.X, LocationRightShin.Y, LocationRightFoot.X, LocationRightFoot.Y, 2.0f, 255, 255, 255);
            }
        }
        else {
            MyPosSaved = false;
        }
        
        uintptr_t List = (uintptr_t)Ler<uint32_t>(Partida + string2Offset(AY_OBFUSCATE("0x11C")));

        for (int i = 0; i < Ler<int>(List + string2Offset(AY_OBFUSCATE("0xC"))); i++) {

            uintptr_t Entity = (uintptr_t)Ler<uint32_t>((uintptr_t)Ler<uint32_t>(List + string2Offset(AY_OBFUSCATE("0x8"))) + string2Offset(AY_OBFUSCATE("0x10")) + string2Offset(AY_OBFUSCATE("0x4")) * i);
            if (Entity == 0) continue;

            int indexPlayer = TypeDefIndex(Entity);
            if (indexPlayer != PlayerNetwork && indexPlayer != PlayerUGCCommon && indexPlayer != Player_TrainingHumanTarget_Stand && indexPlayer != Player_TrainingHumanTarget) continue;

            uintptr_t AvatarManager = (uintptr_t)Ler<uint32_t>(Entity + Offsets.AvatarManager);
            if (AvatarManager == 0) continue;

            uintptr_t UmaAvatarSimple = (uintptr_t)Ler<uint32_t>(AvatarManager + Offsets.UmaAvatarSimple);
            if (UmaAvatarSimple == 0) continue;

            uintptr_t UmaData = (uintptr_t)Ler<uint32_t>(UmaAvatarSimple + Offsets.UMAData);
            if (UmaData == 0) continue;

            int EntityProperties = Ler<int>(Entity + string2Offset(AY_OBFUSCATE("0xBD8"))); // igual meu: m_TransformType
            if (EntityProperties == 0) {
                bool IsVisible = Ler<bool>(UmaAvatarSimple + Offsets.EstaVisivel);
                if (!IsVisible) continue;
            }

            uintptr_t IPRIDataPool = (uintptr_t)Ler<uint32_t>(Entity + Offsets.PRIDataPool);
            if (IPRIDataPool == 0) continue;

            uintptr_t ReplicationDataPoolUnsafe = (uintptr_t)Ler<uint32_t>(IPRIDataPool + Offsets.ReplicationDataPoolUnsafe);
            if (ReplicationDataPoolUnsafe == 0) continue;

            uintptr_t ReplicationDataUnsafe = (uintptr_t)Ler<uint32_t>(ReplicationDataPoolUnsafe + Offsets.ReplicationDataUnsafe);
            if (ReplicationDataUnsafe == 0) continue;

            short Health = Ler<short>(ReplicationDataUnsafe + Offsets.Health);
            if (Health <= 0) continue;

            bool isDying = false;
            uintptr_t HHCBNAPCKHF = (uintptr_t)Ler<uint32_t>(Entity + Offsets.PlayerNetwork_HHCBNAPCKHF);
            if (HHCBNAPCKHF != 0) {
                int enemyStatus = Ler<int>(HHCBNAPCKHF + Offsets.StatePlayer);
                isDying = (enemyStatus == 8);
            }

            static Vector3 PlayerPos;

            uintptr_t localPlayer = Entity;
            bool isLocalPlayer = Ler<bool>(UmaData + Offsets.isLocalPlayer);

            if (isLocalPlayer == 1) {
                PlayerPos = GetPlayerPosition(localPlayer, 0);
            }
            else {
                uintptr_t FNCMBMMKLLI = (uintptr_t)Ler<uint32_t>(Partida + string2Offset(AY_OBFUSCATE("0x64")));
                if (FNCMBMMKLLI != 0) {
                    localPlayer = (uintptr_t)Ler<uint32_t>(FNCMBMMKLLI + string2Offset(AY_OBFUSCATE("0x28")));
                    if (Entity == localPlayer) {
                        PlayerPos = GetPlayerPosition(localPlayer, 0);
                    }
                }
            }

            if (isLocalPlayer == 1) continue;

            bool isTeam = Ler<bool>(UmaData + Offsets.TeamMate);
            if (isTeam) continue;

            Vector3 EnemyPos = GetPlayerPosition(Entity, 0);
            float Distance = Vector3::Distance(EnemyPos, PlayerPos);

            bool Garota = Ler<bool>(Entity + Offsets.isfemale); // CDOBMFNCJHD

            uintptr_t TransformHead = Garota ? string2Offset(AY_OBFUSCATE("0x3C")) : string2Offset(AY_OBFUSCATE("0x38"));
            uintptr_t TransformChest = string2Offset(AY_OBFUSCATE("0x14"));
            uintptr_t TransformStomach = string2Offset(AY_OBFUSCATE("0x10"));
            uintptr_t TransformHip = Garota ? string2Offset(AY_OBFUSCATE("0x10")) : string2Offset(AY_OBFUSCATE("0x48"));

            uintptr_t TransformLeftShoulder = Garota ? string2Offset(AY_OBFUSCATE("0x1C")) : string2Offset(AY_OBFUSCATE("0x18"));
            uintptr_t TransformLeftArm = Garota ? string2Offset(AY_OBFUSCATE("0x20")) : string2Offset(AY_OBFUSCATE("0x1C"));
            uintptr_t TransformLeftForearm = Garota ? string2Offset(AY_OBFUSCATE("0x24")) : string2Offset(AY_OBFUSCATE("0x20"));
            uintptr_t TransformLeftHand = Garota ? string2Offset(AY_OBFUSCATE("0x28")) : string2Offset(AY_OBFUSCATE("0x24"));

            uintptr_t TransformRightShoulder = Garota ? string2Offset(AY_OBFUSCATE("0x2C")) : string2Offset(AY_OBFUSCATE("0x28"));
            uintptr_t TransformRightArm = Garota ? string2Offset(AY_OBFUSCATE("0x30")) : string2Offset(AY_OBFUSCATE("0x2C"));
            uintptr_t TransformRightForearm = Garota ? string2Offset(AY_OBFUSCATE("0x34")) : string2Offset(AY_OBFUSCATE("0x30"));
            uintptr_t TransformRightHand = Garota ? string2Offset(AY_OBFUSCATE("0x38")) : string2Offset(AY_OBFUSCATE("0x34"));

            uintptr_t TransformLeftThigh = Garota ? string2Offset(AY_OBFUSCATE("0x40")) : string2Offset(AY_OBFUSCATE("0x3C"));
            uintptr_t TransformLeftShin = string2Offset(AY_OBFUSCATE("0x40"));
            uintptr_t TransformLeftFoot = Garota ? string2Offset(AY_OBFUSCATE("0x48")) : string2Offset(AY_OBFUSCATE("0x44"));

            uintptr_t TransformRightThigh = string2Offset(AY_OBFUSCATE("0x4C"));
            uintptr_t TransformRightShin = string2Offset(AY_OBFUSCATE("0x50"));
            uintptr_t TransformRightFoot = string2Offset(AY_OBFUSCATE("0x54"));

            uintptr_t CapsuleCollider = (uintptr_t)Ler<uint32_t>((uintptr_t)Ler<uint32_t>(Entity + string2Offset(AY_OBFUSCATE("0x6F0"))) + string2Offset(AY_OBFUSCATE("0x8"))); // ListTransform igual meu

            auto CapsuleCollider$$Head = World2Screen(matrix, ObterOssos(Entity, TransformHead));
            auto CapsuleCollider$$Chest = World2Screen(matrix, ObterOssos(Entity, TransformChest));
            auto CapsuleCollider$$Stomach = World2Screen(matrix, ObterOssos(Entity, TransformStomach));
            auto CapsuleCollider$$Hip = World2Screen(matrix, ObterOssos(Entity, TransformHip));

            auto CapsuleCollider$$LeftShoulder = World2Screen(matrix, ObterOssos(Entity, TransformLeftShoulder));
            auto CapsuleCollider$$LeftArm = World2Screen(matrix, ObterOssos(Entity, TransformLeftArm));
            auto CapsuleCollider$$LeftForearm = World2Screen(matrix, ObterOssos(Entity, TransformLeftForearm));
            auto CapsuleCollider$$LeftHand = World2Screen(matrix, ObterOssos(Entity, TransformLeftHand));

            auto CapsuleCollider$$RightShoulder = World2Screen(matrix, ObterOssos(Entity, TransformRightShoulder));
            auto CapsuleCollider$$RightArm = World2Screen(matrix, ObterOssos(Entity, TransformRightArm));
            auto CapsuleCollider$$RightForearm = World2Screen(matrix, ObterOssos(Entity, TransformRightForearm));
            auto CapsuleCollider$$RightHand = World2Screen(matrix, ObterOssos(Entity, TransformRightHand));

            auto CapsuleCollider$$LeftThigh = World2Screen(matrix, ObterOssos(Entity, TransformLeftThigh));
            auto CapsuleCollider$$LeftShin = World2Screen(matrix, ObterOssos(Entity, TransformLeftShin));
            auto CapsuleCollider$$LeftFoot = World2Screen(matrix, ObterOssos(Entity, TransformLeftFoot));

            auto CapsuleCollider$$RightThigh = World2Screen(matrix, ObterOssos(Entity, TransformRightThigh));
            auto CapsuleCollider$$RightShin = World2Screen(matrix, ObterOssos(Entity, TransformRightShin));
            auto CapsuleCollider$$RightFoot = World2Screen(matrix, ObterOssos(Entity, TransformRightFoot));

            if (!IsBoneValid(CapsuleCollider$$Head) || !IsBoneValid(CapsuleCollider$$Chest) || !IsBoneValid(CapsuleCollider$$Stomach) || !IsBoneValid(CapsuleCollider$$Hip) || !IsBoneValid(CapsuleCollider$$LeftShoulder) || !IsBoneValid(CapsuleCollider$$LeftArm) ||
                !IsBoneValid(CapsuleCollider$$LeftForearm) || !IsBoneValid(CapsuleCollider$$LeftHand) || !IsBoneValid(CapsuleCollider$$RightShoulder) || !IsBoneValid(CapsuleCollider$$RightArm) || !IsBoneValid(CapsuleCollider$$RightForearm) || !IsBoneValid(CapsuleCollider$$RightHand) ||
                !IsBoneValid(CapsuleCollider$$LeftThigh) || !IsBoneValid(CapsuleCollider$$LeftShin) || !IsBoneValid(CapsuleCollider$$LeftFoot) || !IsBoneValid(CapsuleCollider$$RightThigh) || !IsBoneValid(CapsuleCollider$$RightShin) || !IsBoneValid(CapsuleCollider$$RightFoot)) {
                continue;
            }

            Vector3 EnemyHeadPos = GetHeadPosition(Entity);

            Vector3 WorldEnemyPos = World2Screen(matrix, EnemyPos + (Vector3::Down() * 0.1f));
            Vector3 WorldEnemyHeadPos = World2Screen(matrix, EnemyHeadPos + (Vector3::Up() * 0.20f));

            Vector3 PosInScr = World2Screen(matrix, EnemyPos + (Vector3::Down() * 0.1) + Vector3::Zero());
            Vector3 PosHeadInScr = World2Screen(matrix, EnemyHeadPos + (Vector3::Up() * 0.20) + Vector3::Zero());

            if (!IsBoneValid(PosInScr) || !IsBoneValid(PosHeadInScr) || !IsBoneValid(WorldEnemyPos) || !IsBoneValid(WorldEnemyHeadPos)) {
                continue;
            }

            float Y = (SWidth / 4.0) / Distance;
            PosHeadInScr.Y = PosHeadInScr.Y - Y + (SWidth / 4.0) / Distance;
            Vector2 v2Middle = Vector2((float)(SWidth / 2), (float)(SHeight / 2));
            Vector2 v2Loc = Vector2(PosHeadInScr.X, PosHeadInScr.Y);

            float DistanciaNaTela = Vector2::Distance(v2Middle, v2Loc);

            if (TelekillToMe) {
                if (!isDying) {
                    if (Distance <= 8.0f) {
                        if (Distance <= DistanciaMaxima) {
                            DistanciaMaxima = Distance;
                            ClosestEnemy = Entity;
                        }
                    }
                    else {
                        if (isInsideFov((int)PosHeadInScr.X, (int)PosHeadInScr.Y)) {
                            if (DistanciaNaTela <= DistanciaMaxima) {
                                DistanciaMaxima = DistanciaNaTela;
                                ClosestEnemy = Entity;
                            }
                        }
                    }
                }
            }
            else {
                if (isInsideFov((int)PosHeadInScr.X, (int)PosHeadInScr.Y)) {
                    if (!isDying) {
                        if (DistanciaNaTela <= DistanciaMaxima) {
                            DistanciaMaxima = DistanciaNaTela;
                            ClosestEnemy = Entity;
                        }
                    }
                }
            }

            // Silent Target Logic
            if (!isDying) {
                if (DistanciaNaTela <= SilentFov) { // FOV Check
                    if (Distance <= (float)SilentDistance) { // World Distance limit
                        if (DistanciaNaTela <= DistanciaMaximaSilent) {
                            DistanciaMaximaSilent = DistanciaNaTela;
                            ClosestSilentEnemy = Entity;
                        }
                    }
                }
            }

            if (WorldEnemyPos.Z < 0 || WorldEnemyHeadPos.Z < 0) continue;

            float Height = (WorldEnemyPos.Y) - WorldEnemyHeadPos.Y;
            float Width = Height * 0.55f;

            Rect PlayerRect(WorldEnemyHeadPos.X - (Width / 2), WorldEnemyHeadPos.Y, Width, Height);

            wchar_t buffer[30];
            int distanceMeters = static_cast<int>(Distance + 0.5f);
            swprintf(buffer, 30, AY_OBFUSCATE(L"%dm"), distanceMeters);

            // Offsets para evitar sobreposição: quando health bar Top, nome sobe; quando Bottom, distância desce
            const float healthBarTopHeight = 7.0f;   // 5px barra + 2 gap
            const float healthBarBottomHeight = 9.0f; // 5px barra + 2 gap + 2
            float nameOffsetY = -1.0f;
            float distanceOffsetY = 1.0f;
            if (ESPVida) {
                if (healthPosition == 2) nameOffsetY = -(healthBarTopHeight + 2.0f);  // Top: nome logo acima da barra (não tão pra cima)
                else if (healthPosition == 3) distanceOffsetY = healthBarBottomHeight;  // Bottom: distância embaixo da barra
            }

            std::string nameStr = "BOT";
            auto BaseProfileInfo = (uintptr_t)Ler<uint32_t>(Entity + string2Offset(AY_OBFUSCATE("0x15F4")));
            if (BaseProfileInfo != 0) {
                auto PlayerName = (uintptr_t)Ler<uint32_t>(BaseProfileInfo + string2Offset(AY_OBFUSCATE("0x18")));
                if (PlayerName != 0) {
                    auto getNameField = PlayerName + string2Offset(AY_OBFUSCATE("0xC"));
                    int getNameCount = Ler<int>(PlayerName + string2Offset(AY_OBFUSCATE("0x8")));
                    nameStr = ObterStr(getNameField, getNameCount);
                }
            }

            if (ESPNome) {
                const char* name = AY_OBFUSCATE("BOT");
                auto BaseProfileInfo = (uintptr_t)Ler<uint32_t>(Entity + string2Offset(AY_OBFUSCATE("0x15F4")));
                if (BaseProfileInfo != 0) {
                    auto PlayerName = (uintptr_t)Ler<uint32_t>(BaseProfileInfo + string2Offset(AY_OBFUSCATE("0x18")));
                    if (PlayerName != 0) {
                        auto getNameField = PlayerName + string2Offset(AY_OBFUSCATE("0xC"));
                        int getNameCount = Ler<int>(PlayerName + string2Offset(AY_OBFUSCATE("0x8")));
                        nameStr = ObterStr(getNameField, getNameCount);
                        name = nameStr.c_str();
                    }
                }
                ImVec4 color = isDying ? ImVec4(colorDying[0], colorDying[1], colorDying[2], colorDying[3]) : ImVec4(colorName[0], colorName[1], colorName[2], colorName[3]);
                Vector2 namePos(WorldEnemyHeadPos.X, WorldEnemyHeadPos.Y + nameOffsetY);
                DrawName(name, espTextSize, namePos, color.x, color.y, color.z);
            }

            PushPlayer(nameStr, Entity, Distance, (int)Health, isDying);
            static int EnemiesDisplay = 0;
            static auto lastSeenTime = std::chrono::steady_clock::now();

            auto now = std::chrono::steady_clock::now();
            constexpr auto holdTime = std::chrono::milliseconds(1200);

            if (Enemies > EnemiesDisplay)
            {
                EnemiesDisplay = Enemies;
                lastSeenTime = now;
            }
            else if (now - lastSeenTime > holdTime)
            {
                EnemiesDisplay = Enemies;
            }

            int shownEnemies = Enemies;

            if (ESPLinha) {
                ImVec4 color = isDying ? ImVec4(colorDying[0], colorDying[1], colorDying[2], colorDying[3]) : ImVec4(colorLine[0], colorLine[1], colorLine[2], colorLine[3]);
                // Linha termina acima do nome quando em cima (não atravessa o texto)
                float lineTopY = WorldEnemyHeadPos.Y + nameOffsetY - (ESPNome ? espTextSize + 2.0f : 0);
                // Quando barra de vida (Bottom) + distância: linha termina embaixo da distância
                float lineBottomY = PlayerRect.y + PlayerRect.height + distanceOffsetY;
                if (ESPDistancia) lineBottomY += espTextSize + 4.0f;  // Abaixo do texto de distância
                if (linePosition == 1)
                    DrawLine(width / 2, height, WorldEnemyHeadPos.X, lineBottomY, espThickness, color.x, color.y, color.z, 1.0f);
                if (linePosition == 0)
                    DrawLine(width / 2, 0, WorldEnemyHeadPos.X, lineTopY, espThickness, color.x, color.y, color.z, 1.0f);
            }

            if (ESPCaixa) {
                ImVec4 color = isDying ? ImVec4(colorDying[0], colorDying[1], colorDying[2], colorDying[3]) : ImVec4(colorBox[0], colorBox[1], colorBox[2], colorBox[3]);
                if (boxStyle == 0) {
                    DrawBox(PlayerRect, espThickness, color.x, color.y, color.z);
                }
                else {
                    DrawCorneredBox(PlayerRect, espThickness, 10, color.x, color.y, color.z);
                }
            }

            if (ESPEsqueleto) {
                ImVec4 color = isDying ? ImVec4(colorDying[0]/255.0f, colorDying[1]/255.0f, colorDying[2]/255.0f, colorDying[3]/255.0f) : ImVec4(colorSkeleton[0]/255.0f, colorSkeleton[1]/255.0f, colorSkeleton[2]/255.0f, colorSkeleton[3]/255.0f);

                DrawLine(CapsuleCollider$$Head.X, CapsuleCollider$$Head.Y, CapsuleCollider$$Chest.X, CapsuleCollider$$Chest.Y, espThickness, color.x, color.y, color.z);
                DrawLine(CapsuleCollider$$Chest.X, CapsuleCollider$$Chest.Y, CapsuleCollider$$Stomach.X, CapsuleCollider$$Stomach.Y, espThickness, color.x, color.y, color.z);
                DrawLine(CapsuleCollider$$Stomach.X, CapsuleCollider$$Stomach.Y, CapsuleCollider$$Hip.X, CapsuleCollider$$Hip.Y, espThickness, color.x, color.y, color.z);

                DrawLine(CapsuleCollider$$Chest.X, CapsuleCollider$$Chest.Y, CapsuleCollider$$LeftShoulder.X, CapsuleCollider$$LeftShoulder.Y, espThickness, color.x, color.y, color.z);
                DrawLine(CapsuleCollider$$LeftShoulder.X, CapsuleCollider$$LeftShoulder.Y, CapsuleCollider$$LeftArm.X, CapsuleCollider$$LeftArm.Y, espThickness, color.x, color.y, color.z);
                DrawLine(CapsuleCollider$$LeftArm.X, CapsuleCollider$$LeftArm.Y, CapsuleCollider$$LeftForearm.X, CapsuleCollider$$LeftForearm.Y, espThickness, color.x, color.y, color.z);
                DrawLine(CapsuleCollider$$LeftForearm.X, CapsuleCollider$$LeftForearm.Y, CapsuleCollider$$LeftHand.X, CapsuleCollider$$LeftHand.Y, espThickness, color.x, color.y, color.z);

                DrawLine(CapsuleCollider$$Chest.X, CapsuleCollider$$Chest.Y, CapsuleCollider$$RightShoulder.X, CapsuleCollider$$RightShoulder.Y, espThickness, color.x, color.y, color.z);
                DrawLine(CapsuleCollider$$RightShoulder.X, CapsuleCollider$$RightShoulder.Y, CapsuleCollider$$RightArm.X, CapsuleCollider$$RightArm.Y, espThickness, color.x, color.y, color.z);
                DrawLine(CapsuleCollider$$RightArm.X, CapsuleCollider$$RightArm.Y, CapsuleCollider$$RightForearm.X, CapsuleCollider$$RightForearm.Y, espThickness, color.x, color.y, color.z);
                DrawLine(CapsuleCollider$$RightForearm.X, CapsuleCollider$$RightForearm.Y, CapsuleCollider$$RightHand.X, CapsuleCollider$$RightHand.Y, espThickness, color.x, color.y, color.z);

                DrawLine(CapsuleCollider$$Hip.X, CapsuleCollider$$Hip.Y, CapsuleCollider$$LeftThigh.X, CapsuleCollider$$LeftThigh.Y, espThickness, color.x, color.y, color.z);
                DrawLine(CapsuleCollider$$LeftThigh.X, CapsuleCollider$$LeftThigh.Y, CapsuleCollider$$LeftShin.X, CapsuleCollider$$LeftShin.Y, espThickness, color.x, color.y, color.z);
                DrawLine(CapsuleCollider$$LeftShin.X, CapsuleCollider$$LeftShin.Y, CapsuleCollider$$LeftFoot.X, CapsuleCollider$$LeftFoot.Y, espThickness, color.x, color.y, color.z);

                DrawLine(CapsuleCollider$$Hip.X, CapsuleCollider$$Hip.Y, CapsuleCollider$$RightThigh.X, CapsuleCollider$$RightThigh.Y, espThickness, color.x, color.y, color.z);
                DrawLine(CapsuleCollider$$RightThigh.X, CapsuleCollider$$RightThigh.Y, CapsuleCollider$$RightShin.X, CapsuleCollider$$RightShin.Y, espThickness, color.x, color.y, color.z);
                DrawLine(CapsuleCollider$$RightShin.X, CapsuleCollider$$RightShin.Y, CapsuleCollider$$RightFoot.X, CapsuleCollider$$RightFoot.Y, espThickness, color.x, color.y, color.z);
            }

            if (ESPDistancia) {
                ImVec4 color = isDying ? ImVec4(colorDying[0], colorDying[1], colorDying[2], colorDying[3]) : ImVec4(colorDistance[0], colorDistance[1], colorDistance[2], colorDistance[3]);
                Vector2 distancePos(PlayerRect.x + (PlayerRect.width / 2), PlayerRect.y + PlayerRect.height + distanceOffsetY);
                DrawDistance(buffer, espTextSize, distancePos, color.x, color.y, color.z);
            }
            
            if (ESPVida) {
                DrawRoundHealthBarWithPosition(PlayerRect, 200, Health, healthPosition, espHealthBarOffsetY, espHealthBarOffsetX);
            }

        }

        if (ShowAimbotFov) {
            ImColor outlineColor = RgbMode ? GetRGBColor(1.5f) : ImColor(AimbotFovColor[0], AimbotFovColor[1], AimbotFovColor[2], AimbotFovColor[3]);

            ImColor fillColor = ImColor(Filledcolor[0], Filledcolor[1], Filledcolor[2], Filledcolor[3]);

            DrawFov(target, AimbotFov, espThickness, outlineColor.Value.x, outlineColor.Value.y, outlineColor.Value.z, outlineColor.Value.w, true, fillColor.Value.x, fillColor.Value.y, fillColor.Value.z, fillColor.Value.w); 
        }

        if (ShowSilentFov) {
            ImColor outlineSilent = ImColor(SilentFovColor[0], SilentFovColor[1], SilentFovColor[2], SilentFovColor[3]);
            DrawFov(target, SilentFov, espThickness, outlineSilent.Value.x, outlineSilent.Value.y, outlineSilent.Value.z, outlineSilent.Value.w, false, 0, 0, 0, 0); 
        }

        CommitPlayersIfNeeded();

        if (ClosestEnemy != 0) {
            if (AimbotCollider) {
                UpdateColliderTarget(ClosestEnemy);
            }
            else {
                UpdateColliderTarget(0);
            }
            
            ProcessShoulderAimbot(ClosestEnemy, JogadorLocal, true); // Processar Aimbot Ombro (sempre chamar para permitir limpeza quando desativado)
        }
        else {
            UpdateColliderTarget(0);
            ProcessShoulderAimbot(0, JogadorLocal, true); // Limpar quando não há inimigo
        }
        
        // Silent Aim
        if (JogadorLocal != 0) {
            uintptr_t silentTarget = IsSilentActive() ? ClosestSilentEnemy : 0;
            int silentHitboxIndex = 0;
            if (silentTarget != 0) {
                int total = SilentHead + SilentBody;
                if (total > 0) {
                    static std::random_device rdHitbox;
                    static std::mt19937 genHitbox(rdHitbox());
                    std::uniform_int_distribution<int> disHitbox(0, total - 1);
                    int r = disHitbox(genHitbox);
                    if (r < SilentHead)
                        silentHitboxIndex = 0; // Head
                    else {
                        std::uniform_int_distribution<int> disBody(1, 15);
                        silentHitboxIndex = disBody(genHitbox); // Body (hitboxes 1-15)
                    }
                }
            }
            UpdateSilentScene(JogadorLocal, silentTarget, silentHitboxIndex);
        }
        else {
            UpdateSilentScene(0, 0, 0);
        }
        
        // Tp To Enemy
        ProcessTeleport(ClosestEnemyForTeleport, JogadorLocal);
        // Tp Mark
        UpdateTeleportMarkSystem(JogadorLocal);
        // No Clip
        ProcessNoclip(JogadorLocal, matrix);
        // Up Enemy
        ProcessUpEnemy(JogadorLocal);
        // Down Enemy
        ProcessDownEnemy(JogadorLocal);
        // Down Local Player
        ProcessDownLocalPlayer(JogadorLocal);
        // Spin Bot
        ProcessRotate360(JogadorLocal);
        // No Recoil
        ProcessNoRecoil(JogadorLocal);
        // Magnetic Kill v2 (igual à referência)
        if (MagneticKillv2 && ClosestEnemy != 0 && JogadorLocal != 0) {
            bool LPEIEILIKGC = Ler<bool>(JogadorLocal + string2Offset(AY_OBFUSCATE("0x4E0"))); // private Boolean <IsPrepareAttack>k__BackingField
            bool JEPFNELBGID = Ler<bool>(JogadorLocal + string2Offset(AY_OBFUSCATE("0x180"))); // protected Boolean m_IsCurFrameFowardLockToAimRot
            Vector3 minhaPos = GetPlayerPosition(JogadorLocal, 0);
            Vector3 posInimigo = GetPlayerPosition(ClosestEnemy, 0);
            float Distancia = Vector3::Distance(minhaPos, posInimigo);
            
            if (Distancia >= 2 && (LPEIEILIKGC || JEPFNELBGID)) {
                Vector3 EnemyHeadPos = GetHeadPosition(ClosestEnemy);
                Vector3 EnemyChestPos = GetChestPosition(ClosestEnemy);
                Vector3 dirMagnetic = Vector3::Normalized(ObterDirecaoMagnetic(matrix));
                
                Vector3 posAlvo;
                
                int effectiveMagType = MagType;
                if (MagType == 2) {
                    // Sequential: primeiros tiros na cabeça, restante no peito
                    static int shotCount = 0;
                    const int headShots = 3;
                    
                    if (shotCount < headShots) {
                        effectiveMagType = 0; // Head
                        shotCount++;
                    } else {
                        effectiveMagType = 1; // Chest
                        shotCount++;
                        if (shotCount >= headShots * 2) {
                            shotCount = 0;
                        }
                    }
                }
                
                if (effectiveMagType == 0) {
                    // Magnetic na Cabeça
                    Vector3 toHead = EnemyHeadPos - MinhaPosicao_Camera;
                    float projLength = Vector3::Dot(toHead, dirMagnetic);
                    Vector3 projectedHead = MinhaPosicao_Camera + dirMagnetic * projLength;
                    Vector3 rootToHead = EnemyHeadPos - posInimigo;
                    posAlvo = projectedHead - rootToHead;
                }
                else if (effectiveMagType == 1) {
                    Vector3 toChest = EnemyChestPos - MinhaPosicao_Camera;
                    float projLength = Vector3::Dot(toChest, dirMagnetic);
                    Vector3 projectedChest = MinhaPosicao_Camera + dirMagnetic * projLength;
                    Vector3 rootToChest = EnemyChestPos - posInimigo;
                    posAlvo = projectedChest - rootToChest;
                }
                else {
                    posAlvo = minhaPos + dirMagnetic * Distancia;
                }
                
                // Limite de movimento anti-detection (8 unidades)
                float distMovimento = Vector3::Distance(posInimigo, posAlvo);
                if (distMovimento > 8.0f) {
                    Vector3 dir = Vector3::Normalized(posAlvo - posInimigo);
                    posAlvo = posInimigo + dir * 8.0f;
                }
                
                // Escreve nas duas posições que o jogo usa para "magic bullet"
                uint32_t HHCBNAPCKHF = Ler<uint32_t>(ClosestEnemy + Offsets.PlayerNetwork_HHCBNAPCKHF);
                if (HHCBNAPCKHF != 0) {
                    Escrever<Vector3>(HHCBNAPCKHF + string2Offset(AY_OBFUSCATE("0x1C")), posAlvo);
                    Escrever<Vector3>(HHCBNAPCKHF + string2Offset(AY_OBFUSCATE("0x28")), posAlvo);
                }
            }
        }
}

DWORD WINAPI ThreadOverlay() {
    DirectOverlaySetOption(D2DOV_FONT_ARIAL | D2DOV_VSYNC | D2DOV_REQUIRE_FOREGROUND);
    
    HWND hwnd = FindWindow(NULL, "Romeira Project");
    if (hwnd != NULL) {
        DirectOverlaySetAlternateForegroundWindow(hwnd);
    }
    
    DirectOverlaySetRequireForeground(espRequireForeground);
    DirectOverlaySetup(DesenharESP, JanelaAlvo);
    return 0;
}

void runRenderTick() {
    eventPoll();
    ImGui::GetStyle().Colors[ImGuiCol_CheckMark] = ImVec4(0.4f, 0.6f, 1.0f, 1.0f);
    ImGui::GetIO().MouseDrawCursor = OverlayView;
    if (Twidht != 0 && Theight != 0) {
        CleanupRenderTarget();
        g_pSwapChain->ResizeBuffers(0, Twidht, Theight, DXGI_FORMAT_UNKNOWN, 0);
        Theight = Twidht = 0;
        CreateRenderTarget();
    }

    WndRECT TargetWindowRect;
    GetClientRect(hTargetWindow, &TargetWindowRect);
    MapWindowPoints(hTargetWindow, nullptr, reinterpret_cast<LPPOINT>(&TargetWindowRect), 2);
    MoveWindow(hwnd, TargetWindowRect.left, TargetWindowRect.top, TargetWindowRect.Width(), TargetWindowRect.Height(), false);

    static bool lastStreamMode = false;
    if (StreamProof != lastStreamMode && hwnd != nullptr) {
        UINT affinity = StreamProof ? WDA_EXCLUDEFROMCAPTURE : WDA_NONE;
        SetWindowDisplayAffinity(hwnd, affinity);
        lastStreamMode = StreamProof;
    }

    ImGui_ImplDX11_NewFrame();
    ImGui_ImplWin32_NewFrame();
    ImGui::NewFrame();

    if (KeybindsMenu && CurrentTab != 0) {
        static ImVec2 panelPos = ImVec2(100, 100); 
        static bool isDragging = false;
        static ImVec2 dragOffset = ImVec2(0, 0);

        float panelWidth = 180.0f;
        float headerHeight = 30.0f;
        float cornerRadius = 5.0f;
        float margin = 10.0f;

        ImU32 headerColor = IM_COL32(40, 40, 40, 230);
        ImU32 bodyColor = IM_COL32(25, 25, 25, 230);
        ImU32 borderColor = IM_COL32(60, 60, 60, 150);
        ImU32 titleColor = IM_COL32(255, 255, 255, 255);
        ImU32 lineColor = IM_COL32(255, 255, 255, 255);        
        ImU32 nameColor = IM_COL32(255, 255, 255, 180);
        ImU32 nameActiveColor = IM_COL32(80, 220, 120, 255);
        ImU32 keyColor = IM_COL32(255, 255, 255, 255);         

        struct BindDisplayItem {
            std::string name;
            int key;
            bool active;
        };

        std::vector<BindDisplayItem> bindList;
        auto addBind = [&](const char* name, int key, bool active) {
            if (key != 0) {
                bindList.push_back({ name, key, active });
            }
        };

        addBind("Aimbot", KeysBind.Aimbot, IsAimbotColliderActive());
        addBind("Shoulder Aimbot", KeysBind.ShoulderAimbot, IsShoulderAimbotActive());
        addBind("Silent Aim", KeysBind.Silent, IsSilentActive());
        addBind("Tp Mark", KeysBind.TeleportMark, TeleportMark);
        addBind("Pull Enemy", KeysBind.Magnetic, MagneticKillv3 && IsBindModeActive(KeysBind.Magnetic));
        addBind("Ghost Hack", KeysBind.GhostHack, GhostHack);
        addBind("Wall Hack", KeysBind.WallHack, WallHack);
        addBind("Under Cam", KeysBind.UnderCamEnabled, UnderCamEnabled);
        addBind("No Clip", KeysBind.NoClip, NoClip);
        addBind("Speed Hack", KeysBind.SpeedTimer, SpeedTimer);
        addBind("Up Enemy", KeysBind.UpPlayerEnabled, UpPlayer);
        addBind("Down Enemy", KeysBind.DownPlayerEnabled, DownPlayer);
        addBind("Down Local Player", KeysBind.DownLocalEnabled, DownLocalPlayer);
        addBind("Tp Enemy", KeysBind.TeleportPlayer, TeleportPlayer);

        float itemHeight = 20.0f;
        float bindsHeight = bindList.size() * itemHeight;
        float enemyHeight = bindList.empty() ? 0 : 30.0f;
        float panelHeight = headerHeight + (bindList.empty() ? 20.0f : bindsHeight + enemyHeight + 20.0f);

        ImVec2 mousePos = ImGui::GetIO().MousePos;
        bool isMouseOverHeader = mousePos.x >= panelPos.x && mousePos.x <= panelPos.x + panelWidth &&
            mousePos.y >= panelPos.y && mousePos.y <= panelPos.y + headerHeight;
        if (isMouseOverHeader && ImGui::IsMouseClicked(ImGuiMouseButton_Left)) {
            isDragging = true;
            dragOffset = ImVec2(mousePos.x - panelPos.x, mousePos.y - panelPos.y);
        }
        if (isDragging) {
            if (ImGui::IsMouseDown(ImGuiMouseButton_Left)) {
                panelPos.x = mousePos.x - dragOffset.x;
                panelPos.y = mousePos.y - dragOffset.y;
            }
            else {
                isDragging = false;
            }
        }

        ImGui::GetForegroundDrawList()->AddRectFilled(panelPos, ImVec2(panelPos.x + panelWidth, panelPos.y + panelHeight), bodyColor, cornerRadius);
        ImGui::GetForegroundDrawList()->AddRectFilled(panelPos, ImVec2(panelPos.x + panelWidth, panelPos.y + headerHeight), headerColor, cornerRadius, ImDrawFlags_RoundCornersTop);
        ImGui::GetForegroundDrawList()->AddRect(panelPos, ImVec2(panelPos.x + panelWidth, panelPos.y + panelHeight), borderColor, cornerRadius, 0, 1.5f);

        ImGui::PushFont(LexendRegular);
        ImVec2 iconSize = ImGui::CalcTextSize(ICON_FA_KEYBOARD);
        ImVec2 iconPos = ImVec2(panelPos.x + margin, panelPos.y + (headerHeight - iconSize.y) / 2);
        
        // Render ícone com FontAwesome mas mantendo contexto Lexend se necessário para outras medidas
        ImGui::PushFont(FontAwesomeSolid);
        ImGui::GetForegroundDrawList()->AddText(iconPos, titleColor, ICON_FA_KEYBOARD);
        ImGui::PopFont();
        
        ImGui::GetForegroundDrawList()->AddText(ImVec2(iconPos.x + iconSize.x + 5.0f, panelPos.y + (headerHeight - ImGui::GetFontSize()) / 2), titleColor, "Painel de Binds");
        ImGui::PopFont();

        float lineY = panelPos.y + headerHeight - 1.0f;
        ImGui::GetForegroundDrawList()->AddLine(ImVec2(panelPos.x, lineY), ImVec2(panelPos.x + panelWidth, lineY), lineColor, 2.0f);

        if (!bindList.empty()) {
            ImGui::PushFont(LexendRegular);
            float currentY = panelPos.y + headerHeight + 10.0f;

            for (const auto& bind : bindList) {
                std::string name = bind.name;
                std::string keyName = GetKeyName(bind.key);

                ImU32 currentNameColor = bind.active ? nameActiveColor : nameColor;
                ImGui::GetForegroundDrawList()->AddText(ImVec2(panelPos.x + margin, currentY), currentNameColor, name.c_str());

                ImVec2 keySize = ImGui::CalcTextSize(keyName.c_str());
                ImVec2 keyPos = ImVec2(panelPos.x + panelWidth - margin - keySize.x, currentY);
                ImGui::GetForegroundDrawList()->AddText(keyPos, keyColor, keyName.c_str());

                currentY += itemHeight;
            }

            float separatorY = currentY + 8.0f;
            ImGui::GetForegroundDrawList()->AddLine(ImVec2(panelPos.x, separatorY), ImVec2(panelPos.x + panelWidth, separatorY), lineColor, 2.0f);

            static int EnemiesDisplay = 0;
            static auto lastSeenTime = std::chrono::steady_clock::now();

            auto now = std::chrono::steady_clock::now();
            constexpr auto holdTime = std::chrono::milliseconds(1200);

            if (Enemies > EnemiesDisplay)
            {
                EnemiesDisplay = Enemies;
                lastSeenTime = now;
            }
            else if (now - lastSeenTime > holdTime)
            {
                EnemiesDisplay = Enemies;
            }

            int shownEnemies = Enemies;

            std::string label = ".gg/";
            std::string value = "romeiramenu";

            ImVec2 labelSize = ImGui::CalcTextSize(label.c_str());
            ImVec2 valueSize = ImGui::CalcTextSize(value.c_str());

            float totalWidth = labelSize.x + valueSize.x;
            ImVec2 basePos = ImVec2(panelPos.x + (panelWidth - totalWidth) / 2, currentY + 15.0f);

            ImU32 labelColor = IM_COL32(180, 180, 180, 255);
            ImU32 valueColor = IM_COL32(255, 255, 255, 255);

            ImDrawList* draw = ImGui::GetForegroundDrawList();
            ImGui::PushFont(FWork::Fonts::LexendRegular);
            draw->AddText(basePos, labelColor, label.c_str());
            draw->AddText(ImVec2(basePos.x + labelSize.x, basePos.y), valueColor, value.c_str());

            ImGui::PopFont();
        }
    }

    if (OverlayView) {

        NotifyManager::Render();
        const ImVec2 loginWindowSize(600.0f, 380.0f);
        const ImVec2 menuWindowSize(860.0f, 520.0f);
        static ImVec2 animatedWindowSize = loginWindowSize;
        static ImVec2 previousAnimatedWindowSize = loginWindowSize;
        static bool smoothLoginTransition = false;
        static float loginToMenuBlend = 0.0f;
        static float transitionTimer = 0.0f;
        const float transitionDuration = 0.8f; // Mais lento para ser mais smooth

        if (smoothLoginTransition)
        {
            transitionTimer += ImGui::GetIO().DeltaTime;
            float t = ImClamp(transitionTimer / transitionDuration, 0.0f, 1.0f);
            
            // EaseOutQuint para um movimento muito suave no final
            float ease = 1.0f - powf(1.0f - t, 5.0f);
            
            animatedWindowSize.x = ImLerp(loginWindowSize.x, menuWindowSize.x, ease);
            animatedWindowSize.y = ImLerp(loginWindowSize.y, menuWindowSize.y, ease);
            loginToMenuBlend = ease;

            if (t >= 1.0f)
            {
                smoothLoginTransition = false;
                loginToMenuBlend = 1.0f;
                CurrentWindow = 1;
                transitionTimer = 0.0f;
            }
        }
        else
        {
            if (CurrentWindow == 1)
            {
                animatedWindowSize = menuWindowSize;
                loginToMenuBlend = 1.0f;
            }
            else
            {
                animatedWindowSize = loginWindowSize;
                loginToMenuBlend = 0.0f;
            }
            transitionTimer = 0.0f;
        }

        const ImVec2 displaySize = ImGui::GetIO().DisplaySize;
        static bool sharedWindowPosInitialized = false;
        static ImVec2 sharedWindowPos(0.0f, 0.0f);
        static bool isMainMenuDragging = false;
        static ImVec2 mainMenuDragOffset(0.0f, 0.0f);
        if (!sharedWindowPosInitialized)
        {
            sharedWindowPos = ImVec2(
                (displaySize.x - animatedWindowSize.x) * 0.5f,
                (displaySize.y - animatedWindowSize.y) * 0.5f
            );
            sharedWindowPosInitialized = true;
        }

        const ImVec2 sizeDelta(
            animatedWindowSize.x - previousAnimatedWindowSize.x,
            animatedWindowSize.y - previousAnimatedWindowSize.y
        );
        if (sizeDelta.x != 0.0f || sizeDelta.y != 0.0f)
        {
            sharedWindowPos.x -= sizeDelta.x * 0.5f;
            sharedWindowPos.y -= sizeDelta.y * 0.5f;
        }
        previousAnimatedWindowSize = animatedWindowSize;

        sharedWindowPos.x = ImClamp(sharedWindowPos.x, 0.0f, ImMax(0.0f, displaySize.x - animatedWindowSize.x));
        sharedWindowPos.y = ImClamp(sharedWindowPos.y, 0.0f, ImMax(0.0f, displaySize.y - animatedWindowSize.y));

        // Controle de visibilidade e opacidade mais suave
        bool showLoginPanel = false;
        bool showMenuPanel = false;
        float loginPanelAlpha = 0.0f;
        float menuPanelAlpha = 0.0f;

        if (smoothLoginTransition)
        {
            // Durante a transição, mostramos ambos com crossfade
            showLoginPanel = true;
            showMenuPanel = true;
            
            // Login desaparece mais rápido
            float t_login = ImClamp(loginToMenuBlend * 2.0f, 0.0f, 1.0f);
            loginPanelAlpha = 1.0f - t_login;
            
            // Menu aparece depois
            float t_menu = ImClamp((loginToMenuBlend - 0.3f) * 1.42f, 0.0f, 1.0f);
            menuPanelAlpha = t_menu;
        }
        else
        {
            if (CurrentWindow == 0)
            {
                showLoginPanel = true;
                loginPanelAlpha = 1.0f;
            }
            else
            {
                showMenuPanel = true;
                menuPanelAlpha = 1.0f;
            }
        }

        static float AnimaTab = 0.0f;
        static int LastCurrentTab = 0;

        if (LastCurrentTab != CurrentTab)
        {
            AnimaTab = (LastCurrentTab > CurrentTab) ? -460.f : 460.f;
            LastCurrentTab = CurrentTab;
        }

        AnimaTab = ImLerp(AnimaTab, 0.f, 6.f * ImGui::GetIO().DeltaTime);

       
        static int LastCurrentSub = 0;
        static int CurrentSub = 0;
        static float Anima = 0.f;

        if (LastCurrentSub != CurrentSub)
        {
            Anima = (LastCurrentSub > CurrentSub) ? -460.f : 460.f;
            LastCurrentSub = CurrentSub;
        }

        Anima = ImLerp(Anima, 0.f, 6.f * ImGui::GetIO().DeltaTime);

        bool previewMode = PreviewMode;
        bool previewExitClicked = false;
        static double lastPreviewNotifyTime = -10.0;

        if (showLoginPanel) {
            ImGui::PushStyleVar(ImGuiStyleVar_Alpha, ImGui::GetStyle().Alpha * loginPanelAlpha);
            ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0x20 / 255.0f, 0x20 / 255.0f, 0x20 / 255.0f, ImGui::GetStyle().Alpha));
            ImGui::SetNextWindowPos(sharedWindowPos, ImGuiCond_Always);
            ImGui::SetNextWindowSize(animatedWindowSize, ImGuiCond_Always);
            ImGui::Begin("Romeira Project", nullptr, ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoScrollWithMouse);
            {
                sharedWindowPos = ImGui::GetWindowPos();
                const ImVec2 windowPos = ImGui::GetWindowPos();
                const ImVec2 windowSize = ImGui::GetWindowSize();
                const ImVec2 mousePos = ImGui::GetIO().MousePos;
                const ImVec2 dragMin = windowPos;
                const ImVec2 dragMax = windowPos + ImVec2(windowSize.x, 36.0f);
                const bool hoveringDragArea = ImGui::IsMouseHoveringRect(dragMin, dragMax, false);

                if (!isMainMenuDragging && hoveringDragArea && ImGui::IsMouseClicked(ImGuiMouseButton_Left))
                {
                    isMainMenuDragging = true;
                    mainMenuDragOffset = ImVec2(mousePos.x - sharedWindowPos.x, mousePos.y - sharedWindowPos.y);
                }

                if (isMainMenuDragging)
                {
                    if (ImGui::IsMouseDown(ImGuiMouseButton_Left))
                    {
                        sharedWindowPos = ImVec2(mousePos.x - mainMenuDragOffset.x, mousePos.y - mainMenuDragOffset.y);
                    }
                    else
                    {
                        isMainMenuDragging = false;
                    }
                }

                ImDrawList* DrawList = ImGui::GetWindowDrawList();
                ImVec2 Pos = ImGui::GetWindowPos();
                ImVec2 Size = ImGui::GetWindowSize();

                const ImVec2 topPanelPos = Pos + ImVec2(11.0f, 11.0f);
                const ImVec2 topPanelSize = ImVec2(380.0f, 53.0f);
                DrawList->AddRectFilled(topPanelPos, topPanelPos + topPanelSize, ImColor(0x27, 0x27, 0x27, (int)(ImGui::GetStyle().Alpha * 255)), 10.0f);
                {
                    // Logo inside top panel
                    ImVec2 logoTopLeft = topPanelPos + ImVec2(17.0f, 14.0f);
                    ImVec2 logoBottomRight = logoTopLeft + ImVec2(25.0f, 25.0f);
                    DrawList->AddImage(Logo, logoTopLeft, logoBottomRight, ImVec2(0, 0), ImVec2(1, 1), ImColor(255, 255, 255, (int)(ImGui::GetStyle().Alpha * 255)));

                    ImFont* textFont = Lexend ? Lexend : (LexendRegular ? LexendRegular : ImGui::GetFont());
                    float fs = textFont->FontSize;
                    const char* t1 = "Bem vindo ao Sharp Internal";
                    const char* t2 = "Suporte em: discord.gg/romeiramenu";
                    ImVec2 s1 = textFont->CalcTextSizeA(fs, FLT_MAX, 0.0f, t1);
                    ImVec2 s2 = textFont->CalcTextSizeA(fs, FLT_MAX, 0.0f, t2);
                    float spacing = 2.0f;
                    float totalH = s1.y + spacing + s2.y;
                    float startY = topPanelPos.y + (topPanelSize.y - totalH) * 0.5f;
                    float x = topPanelPos.x + 62.0f;
                    ImU32 col_primary = IM_COL32(255, 255, 255, (int)(ImGui::GetStyle().Alpha * 255));
                    ImU32 col_secondary = IM_COL32(0x59, 0x59, 0x59, (int)(ImGui::GetStyle().Alpha * 255));
                    DrawList->AddText(textFont, fs, ImVec2(x, startY), col_primary, t1);
                    DrawList->AddText(textFont, fs, ImVec2(x, startY + s1.y + spacing), col_secondary, t2);
                }

                const ImVec2 mainPanelPos = Pos + ImVec2(11.0f, 73.0f);
                const ImVec2 mainPanelSize = ImVec2(578.0f, 278.0f);
                DrawList->AddRectFilled(mainPanelPos, mainPanelPos + mainPanelSize, ImColor(0x27, 0x27, 0x27, (int)(ImGui::GetStyle().Alpha * 255)), 10.0f);

                static char keyBuffer[255] = "";

                std::string Application = "Free Fire Romeira";  // ID do aplicativo
                std::string OwnerID = "gulano-3peui7i3";  // Banco de dados do aplicativo

                const float inputWidth = 285.0f;
                const float buttonWidth = 285.0f;
                const float rowHeight = 35.0f;
                const float spacingY = 12.0f;
                const float totalHeight = rowHeight + spacingY + rowHeight;
                const float centerX = mainPanelPos.x + (mainPanelSize.x - inputWidth) * 0.5f;
                const float centerY = mainPanelPos.y + (mainPanelSize.y - totalHeight) * 0.5f;

                ImGui::SetCursorScreenPos(ImVec2(centerX, centerY));
                if (keyInputErrorRequested.exchange(false))
                {
                    ImGui::InputTextSetError(ImGui::GetID(ICON_FA_KEY), 4.0f);
                }
                ImGui::InputTextEx3(ICON_FA_KEY, "Licensa de Key", keyBuffer, IM_ARRAYSIZE(keyBuffer), ImVec2(inputWidth, rowHeight), ImGuiInputTextFlags_None);

                static bool postLoginStarted = false;
                const ImVec2 loginButtonPos = Pos + ImVec2(167.0f, 212.0f);
                const ImVec2 loginButtonSize = ImVec2(158.0f, 36.0f);
                const ImVec2 previewButtonPos = Pos + ImVec2(329.0f, 212.0f);
                const ImVec2 previewButtonSize = ImVec2(104.0f, 36.0f);


                
                ImGui::SetCursorScreenPos(loginButtonPos);
                if (ImGui::CustomButton("Continue", loginButtonSize, IM_COL32(0x20, 0x20, 0x20, 255), IM_COL32(0x18, 0x18, 0x18, 255), IM_COL32(0x59, 0x59, 0x59, 255), 10.0f))
                {
                    // --- INICIO DO BYPASS ---
                    isLoggedIn = true;
                    PreviewMode = false;
                    postLoginStarted = true;
                    CurrentTab = 2;
                    smoothLoginTransition = true;
                    loggedUser = "ADMIN BYPASS";
                    expirationInfo = "9999 Days";

                    // Iniciando as funções do cheat sem verificar a key
                    std::thread([]() {
                        auto GameEngine = AY_OBFUSCATE("0xB15688C");
                        Offsets.GameEngine = string2Offset(GameEngine);
                        }).detach();

                    std::thread(LoadLibraryAndHook).detach();
                    std::thread(ThreadOverlay).detach();
                    std::thread(NetworkInit).detach();

                    // Se o DiscordRPC causar erro ao compilar, você pode comentar a linha abaixo
                    std::thread([]() { if (DiscordRPC) DiscordRPC->Initialize(); }).detach();
                    
                    NotifyManager::Send("Bypass efetuado! Divirta-se.");
                    // --- FIM DO BYPASS ---
                }

                ImGui::SetCursorScreenPos(previewButtonPos);
                if (ImGui::CustomButton("Preview", previewButtonSize, IM_COL32(0xC9, 0xC9, 0xC9, 255), IM_COL32(0xB0, 0xB0, 0xB0, 255), IM_COL32(0x38, 0x38, 0x38, 255), 10.0f))
                {
                    isLoggedIn = true;
                    PreviewMode = true;
                    loggedUser = "Preview User";
                    expirationInfo = "Preview Mode";
                    CurrentTab = 2;
                    smoothLoginTransition = true;
                    NotifyManager::Send("Modo Preview Ativado!");
                }
            }
            ImGui::End();
            ImGui::PopStyleColor();
            ImGui::PopStyleVar();
        }
        if (showMenuPanel) {
            ImGui::PushStyleVar(ImGuiStyleVar_Alpha, ImGui::GetStyle().Alpha * menuPanelAlpha);
            ImGui::SetNextWindowPos(sharedWindowPos, ImGuiCond_Always);
            ImGui::SetNextWindowSize(animatedWindowSize, ImGuiCond_Always);
            ImGui::Begin("", nullptr, ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse | ImGuiWindowFlags_NoTitleBar);
            {
                sharedWindowPos = ImGui::GetWindowPos();
                const ImVec2 windowPos = ImGui::GetWindowPos();
                const ImVec2 windowSize = ImGui::GetWindowSize();
                const ImVec2 mousePos = ImGui::GetIO().MousePos;
                const ImVec2 dragMin = windowPos;
                const ImVec2 dragMax = windowPos + ImVec2(windowSize.x, 36.0f);
                const bool hoveringDragArea = ImGui::IsMouseHoveringRect(dragMin, dragMax, false);
                //pw
                if (!isMainMenuDragging && hoveringDragArea && ImGui::IsMouseClicked(ImGuiMouseButton_Left))
                {
                    isMainMenuDragging = true;
                    mainMenuDragOffset = ImVec2(mousePos.x - sharedWindowPos.x, mousePos.y - sharedWindowPos.y);
                }

                if (isMainMenuDragging)
                {
                    if (ImGui::IsMouseDown(ImGuiMouseButton_Left))
                    {
                        sharedWindowPos = ImVec2(mousePos.x - mainMenuDragOffset.x, mousePos.y - mainMenuDragOffset.y);
                    }
                    else
                    {
                        isMainMenuDragging = false;
                    }
                }

                ImDrawList* DrawList = ImGui::GetWindowDrawList();
                ImVec2 Pos = ImGui::GetWindowPos();
                ImVec2 Size = ImGui::GetWindowSize();

                const ImVec2 panelPos = Pos + ImVec2(80.0f, 9.0f);
                const ImVec2 panelSize = ImVec2(170.0f, 55.0f);
                const ImVec2 panelEnd = panelPos + panelSize;
                DrawList->AddRectFilled(panelPos, panelEnd, ImColor(0x27, 0x27, 0x27, (int)(ImGui::GetStyle().Alpha * 255)), 15.0f);

                ImVec2 logoTopLeft = panelPos + ImVec2(17.0f, 15.0f);
                ImVec2 logoBottomRight = logoTopLeft + ImVec2(25.0f, 25.0f);
                DrawList->AddImage(Logo, logoTopLeft, logoBottomRight, ImVec2(0, 0), ImVec2(1, 1), ImColor(255, 255, 255, (int)(ImGui::GetStyle().Alpha * 255)));

                ImFont* panelFont = Lexend ? Lexend : ImGui::GetFont();
                DrawList->AddText(panelFont, panelFont->FontSize, panelPos + ImVec2(52.0f, 10.5f), IM_COL32(255, 255, 255, (int)(ImGui::GetStyle().Alpha * 255)), "Sharp Internal");
                DrawList->AddText(panelFont, panelFont->FontSize, panelPos + ImVec2(52.0f, 27.0f), IM_COL32(0x59, 0x59, 0x59, (int)(ImGui::GetStyle().Alpha * 255)), "Cheat Free Fire");

                const float subFontSize = panelFont->FontSize;
                ImVec2 subTextPos = Pos + ImVec2(270.0f, 26.0f);
                if (previewMode)
                {
                    const char* previewText = "Modo Stream Mode - Compre para utilizar";
                    DrawList->AddText(panelFont, subFontSize, subTextPos, IM_COL32(0xC9, 0xC9, 0xC9, (int)(ImGui::GetStyle().Alpha * 255)), previewText);
                }
                else
                {
                    const char* subPrefix = "Sua key acaba em: ";
                    DrawList->AddText(panelFont, subFontSize, subTextPos, IM_COL32(0xC9, 0xC9, 0xC9, (int)(ImGui::GetStyle().Alpha * 255)), subPrefix);
                    ImVec2 subPrefixSize = panelFont->CalcTextSizeA(subFontSize, FLT_MAX, 0.0f, subPrefix);
                    DrawList->AddText(panelFont, subFontSize, ImVec2(subTextPos.x + subPrefixSize.x, subTextPos.y), IM_COL32(255, 255, 255, (int)(ImGui::GetStyle().Alpha * 255)), expirationInfo.c_str());
                }

                if (previewMode)
                {
                    const ImVec2 exitButtonPos = Pos + ImVec2(11.0f, 460.0f);
                    const ImVec2 exitButtonSize = ImVec2(50.0f, 50.0f);
                    ImGui::SetCursorScreenPos(exitButtonPos);
                    ImGui::InvisibleButton("PreviewExit", exitButtonSize);
                    bool exitHovered = ImGui::IsItemHovered();
                    previewExitClicked = ImGui::IsItemClicked();
                    int exitAlpha = (int)(ImGui::GetStyle().Alpha * 255);
                    ImU32 exitColor = exitHovered ? IM_COL32(0x30, 0x30, 0x30, exitAlpha) : IM_COL32(0x27, 0x27, 0x27, exitAlpha);
                    DrawList->AddRectFilled(exitButtonPos, exitButtonPos + exitButtonSize, exitColor, 15.0f);
                    ImGui::PushFont(FontAwesomeSolid);
                    ImVec2 exitIconSize = ImGui::CalcTextSize(ICON_FA_RIGHT_FROM_BRACKET);
                    ImVec2 exitIconPos = ImVec2(exitButtonPos.x + (exitButtonSize.x - exitIconSize.x) * 0.5f, exitButtonPos.y + (exitButtonSize.y - exitIconSize.y) * 0.5f);
                    DrawList->AddText(FontAwesomeSolid, FontAwesomeSolid->FontSize, exitIconPos, IM_COL32(0xC9, 0xC9, 0xC9, exitAlpha), ICON_FA_RIGHT_FROM_BRACKET);
                    ImGui::PopFont();
                    if (previewExitClicked)
                    {
                        PreviewMode = false;
                        CurrentWindow = 0;
                        smoothLoginTransition = false;
                    }
                }

                if (previewMode && ImGui::IsWindowHovered(ImGuiHoveredFlags_RootAndChildWindows) && ImGui::IsMouseClicked(ImGuiMouseButton_Left) && !previewExitClicked)
                {
                    float contentStartX = Pos.x + (Size.x / 3.5f) + 10.0f;
                    float contentStartY = Pos.y + 75.0f;
                    bool mouseInContent = ImGui::GetIO().MousePos.x > contentStartX && ImGui::GetIO().MousePos.y > contentStartY;

                    if (mouseInContent)
                    {
                        double now = ImGui::GetTime();
                        if (now - lastPreviewNotifyTime > 0.6)
                        {
                            NotifyManager::Send("Você está em modo de preview.", 2.5f);
                            lastPreviewNotifyTime = now;
                        }
                    }
                }

                static char SearchOptionsBuf[128] = "";
                ImVec2 searchSize = ImVec2(220.0f, 55.0f);
                float rightMargin = 15.0f;
                ImVec2 searchPos = ImVec2(Pos.x + Size.x - rightMargin - searchSize.x, Pos.y + 9.0f);
                ImVec2 searchEnd = searchPos + searchSize;
                DrawList->AddRectFilled(searchPos, searchEnd, ImColor(0x25, 0x25, 0x25, (int)(ImGui::GetStyle().Alpha * 255)), 15.0f);
                float squareW = 42.0f, squareH = 42.0f;
                ImVec2 squarePos = searchPos + ImVec2(8.0f, (searchSize.y - squareH) * 0.5f);
                ImVec2 squareEnd = squarePos + ImVec2(squareW, squareH);
                DrawList->AddRectFilled(squarePos, squareEnd, ImColor(0x2C, 0x2C, 0x2C, (int)(ImGui::GetStyle().Alpha * 255)), 10.0f);
                ImGui::PushFont(FontAwesomeSolid);
                ImVec2 iconSize = ImGui::CalcTextSize("\xef\x80\x82");
                ImVec2 iconPos = ImVec2(squarePos.x + (squareW - iconSize.x) * 0.5f, squarePos.y + (squareH - iconSize.y) * 0.5f);
                DrawList->AddText(FontAwesomeSolid, FontAwesomeSolid->FontSize * 1.10f, iconPos, IM_COL32(180, 180, 180, (int)(ImGui::GetStyle().Alpha * 255)), "\xef\x80\x82");
                ImGui::PopFont();
                float textFontSize = (LexendRegular ? LexendRegular->FontSize : ImGui::GetFont()->FontSize);
                float textStartX = searchPos.x + 8.0f + squareW + 8.0f;
                ImVec2 prevCursorLocal = ImGui::GetCursorPos();
                float textBoxH = 42.0f;
                ImGui::SetCursorPos(ImVec2(textStartX - Pos.x, (searchPos.y + (searchSize.y - textBoxH) * 0.5f) - Pos.y));
                ImGui::PushFont(LexendRegular ? LexendRegular : ImGui::GetFont());
                ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(0, 0));
                ImGui::PushStyleVar(ImGuiStyleVar_FrameBorderSize, 0.0f);
                ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 0.0f);
                ImGui::PushStyleColor(ImGuiCol_FrameBg, IM_COL32(0x25, 0x25, 0x25, (int)(ImGui::GetStyle().Alpha * 255)));
                ImGui::PushStyleColor(ImGuiCol_FrameBgHovered, IM_COL32(0x25, 0x25, 0x25, (int)(ImGui::GetStyle().Alpha * 255)));
                ImGui::PushStyleColor(ImGuiCol_FrameBgActive, IM_COL32(0x25, 0x25, 0x25, (int)(ImGui::GetStyle().Alpha * 255)));
                ImGui::PushStyleColor(ImGuiCol_Text, IM_COL32(255, 255, 255, (int)(ImGui::GetStyle().Alpha * 255)));
                /*ImGui::InputTextEx3("##SearchOptions", "Search Option", SearchOptionsBuf, IM_ARRAYSIZE(SearchOptionsBuf), ImVec2(searchEnd.x - 8.0f - textStartX, textBoxH), ImGuiInputTextFlags_None);*/
                ImGui::PopStyleColor(4);
                ImGui::PopStyleVar(3);
                ImGui::PopFont();
                ImGui::SetCursorPos(prevCursorLocal);

                // Custom panel 165x437 at position 74x73 with radius 15 and color #272727
                const ImVec2 customPanelPos = Pos + ImVec2(80.0f, 73.0f);
                const ImVec2 customPanelSize = ImVec2(170.0f, 437.0f);
                const ImVec2 customPanelEnd = customPanelPos + customPanelSize;
                DrawList->AddRectFilled(customPanelPos, customPanelEnd, ImColor(0x27, 0x27, 0x27, (int)(ImGui::GetStyle().Alpha * 255)), 15.0f);
                
                // Text "Options" at position 12x10 relative to panel
                ImFont* optionsFont = Lexend ? Lexend : ImGui::GetFont();
                DrawList->AddText(optionsFont, optionsFont->FontSize, customPanelPos + ImVec2(12.0f, 10.0f), IM_COL32(0x59, 0x59, 0x59, (int)(ImGui::GetStyle().Alpha * 255)), "Options");
                
                // Variável static para CurrentSub (sub-tabs de Exploits)
                static int CurrentSub = 0;
                static int LastCurrentSub = 0;
                static int CombatSub = 0;
                
                 
                ImGui::BeginChild("LeftChild", ImVec2(Size.x / 3.5f, Size.y));
                {
                    ImGui::SetCursorPos(ImVec2(15.0f, 13.0f));
                    ImGui::BeginGroup();
                    {
                        ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(ImGui::GetStyle().ItemSpacing.x, 8.0f));
                        ImGui::Tab("Combat", ICON_FA_CROSSHAIRS, 2, &CurrentTab);
                        ImGui::Tab("Exploits", ICON_FA_MICROCHIP, 4, &CurrentTab);
                        ImGui::Tab("World", ICON_FA_GLOBE, 3, &CurrentTab);
                        ImGui::Tab("Misc", ICON_FA_LAYER_GROUP, 6, &CurrentTab);
                        ImGui::Tab("Configs", ICON_FA_CLOUD, 7, &CurrentTab);
                        ImGui::PopStyleVar();

                    }
                    ImGui::EndGroup();
                }
                ImGui::EndChild();
                
                static float AnimaTab = 0.0f;
                static float Anima = 0.0f; 

                ImGui::SetCursorPos(ImVec2(Size.x / 3.5f + 10.0f, AnimaTab)); // +10px para a direita
                ImGui::BeginChild("MainChild", ImVec2(Size.x - (Size.x / 3.5f) - 10.0f, Size.y), ImGuiChildFlags_None, ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse);
                {
                    ImGui::SetCursorPos(ImVec2(ImGui::GetContentRegionMax().x - 235, 18));
                    if (CurrentTab == 2)
                    {
                        static float CombatAnima = 0.f;
                        static int LastCombatSub = 0;

                        if (LastCombatSub > CombatSub)
                        {
                            CombatAnima = -460.f;
                            LastCombatSub = CombatSub;
                        }
                        else if (LastCombatSub < CombatSub)
                        {
                            CombatAnima = 460.f;
                            LastCombatSub = CombatSub;
                        }

                        CombatAnima = ImLerp(CombatAnima, 0.f, 0.1f);

                        ImGui::SetCursorPos(ImVec2(CombatAnima, 65));

                        ImGui::BeginChild("Aimbot");
                        if (previewMode) { 
                            ImGui::PushStyleVar(ImGuiStyleVar_DisabledAlpha, 1.0f); 
                            ImGui::BeginDisabled(); 
                        }
                        {
                            if (CombatSub == 0)
                            {
                                ImGui::SetCursorPos(ImVec2(5, 8));
                                ImGui::BeginGroup();
                                {
                                    if (ImGui::BeginGroupBox("Aimbot", "Ativação de Aimbots", ICON_FA_USER, ImVec2(ImGui::GetWindowSize().x / 2 - 25, 437)))
                                        {
                                            if (ImGui::Checkbox("Aimbot Cabeça", &AimbotCollider)) {
                                                NotifyManager::Send(std::string("Aimbot Cabeça ") + (AimbotCollider ? "ativado" : "desativado"));
                                            }
                                            if (ImGui::Checkbox("Aimbot Ombro", &ShoulderAimbot)) {
                                                NotifyManager::Send(std::string("Aimbot Ombro") + (ShoulderAimbot ? "ativado" : "desativado"));
                                            }
                                            if (ImGui::Checkbox("Aimbot Magnetico", &MagneticKillv2)) {
                                                NotifyManager::Send(std::string("Aimbot Magnetico ") + (MagneticKillv2 ? "ativado" : "desativado"));
                                            }
                                            if (ImGui::Checkbox("Aimbot Preciso", &Aimlock2x)) {
                                                NotifyManager::Send(std::string("Aimbot Preciso ") + (Aimlock2x ? "ativado" : "desativado"));
                                            }
                                            if (ImGui::Checkbox("Aimbot Sniper", &fovawm)) {
                                                NotifyManager::Send(std::string("Aimbot Sniper ") + (fovawm ? "ativado" : "desativado"));
                                            }
                                            if (ImGui::Checkbox("Sem recarga", &fastreload)) {
                                                NotifyManager::Send(std::string("Sem recarga ") + (fastreload ? "ativado" : "desativado"));
                                            }
                                       }
                                            
                                    ImGui::EndGroupBox();
                                }
                                ImGui::EndGroup();

                                ImGui::SetCursorPos(ImVec2(292, 8));
                                ImGui::BeginGroup();
                                {
                                    if (ImGui::BeginGroupBox("Ajustes", "Ajustar Aimbot", ICON_FA_USER, ImVec2(ImGui::GetWindowSize().x / 2 - 3, 437)))
                                    {
                                        if (ShoulderAimbot) {
                                            ImGui::KeyBind("KeyBind", &KeysBind.ShoulderAimbot);
                                            ImGui::Combo("Ombro - Mode", &ShoulderAimbotBindMode, "Segurar\0Unico\0");
                                        }
                                        if (MagneticKillv2) {
                                            ImGui::KeyBind("KeyBind", &KeysBind.MagneticKillv2);
                                            static const char* magTypeItems[] = { "Head", "Chest", "Random" };
                                            ImGui::Combo("Magnetico - Type", &MagType, magTypeItems, IM_ARRAYSIZE(magTypeItems));
                                        }
                                        if (AimbotCollider) {
                                            ImGui::KeyBind("KeyBind", &KeysBind.Aimbot);
                                            ImGui::Combo("Delay Cabeça - Mode", &AimbotColliderBindMode, "Segurar\0Unico\0");
                                            ImGui::SliderFloat("Delay ( ms )", &AimbotColliderDelay, 0.0f, 500.0f, "%.1f ms", ImGuiSliderFlags_None);
                                        }
                                        ImGui::Checkbox("Ignorar Caidos", &IgnoreKnocked);
                                        ImGui::Checkbox("Ignorar Bots", &IgnoreBots);
                                    }
                                    ImGui::EndGroupBox();
                                }
                                ImGui::EndGroup();
                            }
                            else if (CombatSub == 1)
                            {
                                ImGui::SetCursorPos(ImVec2(5, 8));
                                ImGui::BeginGroup();
                                {
                                    if (ImGui::BeginGroupBox("Silent - Aimbot", "Silent Ativação", ICON_FA_BULLSEYE, ImVec2(ImGui::GetWindowSize().x / 2 - 25, 437)))
                                    {
                                        if (ImGui::Checkbox("Silent Aim", &Silent)) {
                                            NotifyManager::Send(std::string("Silent Aim ") + (Silent ? "ativado" : "desativado"));
                                        }
                                    }
                                    ImGui::EndGroupBox();
                                }
                                ImGui::EndGroup();

                                ImGui::SetCursorPos(ImVec2(292, 8));
                                ImGui::BeginGroup();
                                {
                                    if (ImGui::BeginGroupBox("Silent - Ajustar", "Silent configuração", ICON_FA_BULLSEYE, ImVec2(ImGui::GetWindowSize().x / 2 - 3, 437)))
                                    {
                                        if (Silent) {
                                            ImGui::KeyBind("KeyBind", &KeysBind.Silent);
                                            ImGui::Combo("Silent - Modo", &SilentBindMode, "Hold\0Unico\0");
                                            ImGui::SliderInt("Cabeça", &SilentHead, 0, 6, "%d");
                                            ImGui::SliderInt("Peito", &SilentBody, 0, 6, "%d");
                                            ImGui::SliderFloat("Fov", &SilentFov, 10.0f, 400.0f, "%.0f");
                                            ImGui::SliderInt("Max Distancia", &SilentDistance, 0, 500, "%d");
                                            ImGui::Checkbox("Desenhar Fov", &ShowSilentFov);
                                            ImGui::ColorEdit4("Cor do Fov", SilentFovColor, ImGuiColorEditFlags_NoDragDrop | ImGuiColorEditFlags_AlphaBar | ImGuiColorEditFlags_NoTooltip | ImGuiColorEditFlags_NoInputs);
                                        }
                                    }
                                    ImGui::EndGroupBox();
                                }
                                ImGui::EndGroup();
                            }
                        }
                        if (previewMode) { 
                            ImGui::EndDisabled(); 
                            ImGui::PopStyleVar(); 
                        }
                        ImGui::EndChild();
                    }
                    else if (CurrentTab == 3)
                    {
                        ImGui::SetCursorPos(ImVec2(Anima, 65));

                        ImGui::BeginChild("World");
                        if (previewMode) { 
                            ImGui::PushStyleVar(ImGuiStyleVar_DisabledAlpha, 1.0f); 
                            ImGui::BeginDisabled(); 
                        }
                        {
                            ImGui::SetCursorPos(ImVec2(5, 8));
                            ImGui::BeginGroup();
                            {
                                if (ImGui::BeginGroupBox("Visual", "Configurar o ESP", ICON_FA_EYE, ImVec2(ImGui::GetWindowSize().x / 2 - 25, 437)))
                                {
                                    if (ImGui::Checkbox("Esp Linhas", &ESPLinha)) {
                                        NotifyManager::Send(std::string("Linhas ") + (ESPLinha ? "ativado" : "desativado"));
                                    }
                                    if (ImGui::Checkbox("Esp Esqueleto", &ESPEsqueleto)) {
                                        NotifyManager::Send(std::string("Skeleton ") + (ESPEsqueleto ? "ativado" : "desativado"));
                                    }
                                    if (ImGui::Checkbox("Esp Box", &ESPCaixa)) {
                                        NotifyManager::Send(std::string("Box ") + (ESPCaixa ? "ativado" : "desativado"));
                                    }
                                    if (ImGui::Checkbox("Esp Vida", &ESPVida)) {
                                        NotifyManager::Send(std::string("Vida ") + (ESPVida ? "ativado" : "desativado"));
                                    }
                                    if (ImGui::Checkbox("Esp Name", &ESPNome)) {
                                        NotifyManager::Send(std::string("Nome ") + (ESPNome ? "ativado" : "desativado"));
                                    }
                                    if (ImGui::Checkbox("Esp Distancia", &ESPDistancia)) {
                                        NotifyManager::Send(std::string("Distancia ") + (ESPDistancia ? "ativado" : "desativado"));
                                    }
                                    
                                }
                                ImGui::EndGroupBox();
                            }
                            ImGui::EndGroup(); 

                            ImGui::SetCursorPos(ImVec2(292, 8));
                            if (ImGui::BeginGroupBox("Ajusts", "ESP settings", ICON_FA_USER, ImVec2(ImGui::GetWindowSize().x / 2 - 3, 437)))
                            {
                                ImGui::Combo("Posição da Linha", &linePosition, "Cima\0Baixo\0");
                                ImGui::Combo("Posição da Vida", &healthPosition, "Esquerda\0Direita\0Cima\0Baixo");
                                ImGui::Combo("Box Style", &boxStyle, "Full\0Cornered\0");
                                ImGui::SliderFloat("Tamanho dos Texto", &espTextSize, 10.0f, 20.0f);
                                ImGui::SliderFloat("Desenhar Thickness", &espThickness, 0.5f, 2.0f);
                                ImGui::ColorEdit4("Cor do Nome", colorName, ImGuiColorEditFlags_NoDragDrop | ImGuiColorEditFlags_AlphaBar | ImGuiColorEditFlags_NoTooltip | ImGuiColorEditFlags_NoInputs);
                                ImGui::ColorEdit4("Cor da Distancia", colorDistance, ImGuiColorEditFlags_NoDragDrop | ImGuiColorEditFlags_AlphaBar | ImGuiColorEditFlags_NoTooltip | ImGuiColorEditFlags_NoInputs);
                                ImGui::ColorEdit4("Cor do Esqueleto", colorSkeleton, ImGuiColorEditFlags_NoDragDrop | ImGuiColorEditFlags_AlphaBar | ImGuiColorEditFlags_NoTooltip | ImGuiColorEditFlags_NoInputs);
                                ImGui::ColorEdit4("Cor da Box", colorBox, ImGuiColorEditFlags_NoDragDrop | ImGuiColorEditFlags_AlphaBar | ImGuiColorEditFlags_NoTooltip | ImGuiColorEditFlags_NoInputs);
                                ImGui::ColorEdit4("Cor da Linha", colorLine, ImGuiColorEditFlags_NoDragDrop | ImGuiColorEditFlags_AlphaBar | ImGuiColorEditFlags_NoTooltip | ImGuiColorEditFlags_NoInputs);
                            }
                            ImGui::EndGroupBox();
                        }
                        if (previewMode) { 
                            ImGui::EndDisabled(); 
                            ImGui::PopStyleVar(); 
                        }
                        ImGui::EndChild();
                    }
                    else if (CurrentTab == 4) // Exploits
                    {
                        static float Anima = 0.f;

                        if (LastCurrentSub > CurrentSub)
                        {
                            Anima = -460.f;
                            LastCurrentSub = CurrentSub;
                        }
                        else if (LastCurrentSub < CurrentSub)
                        {
                            Anima = 460.f;
                            LastCurrentSub = CurrentSub;
                        }

                        Anima = ImLerp(Anima, 0.f, 0.1f);


                        ImGui::SetCursorPos(ImVec2(Anima, 65));

                        ImGui::BeginChild("Exploits");
                        if (previewMode) { 
                            ImGui::PushStyleVar(ImGuiStyleVar_DisabledAlpha, 1.0f); 
                            ImGui::BeginDisabled(); 
                        }
                        {
                            if (CurrentSub == 0)
                            {
                                ImGui::SetCursorPos(ImVec2(5, 8));
                                ImGui::BeginGroup();
                                {
                                    if (ImGui::BeginGroupBox("Exploits", "Opção para ajudar", ICON_FA_BOMB, ImVec2(ImGui::GetWindowSize().x / 2 - 25, 437)))
                                    {
                                        if (ImGui::Checkbox("Bugar Pixel", &Bloquearmira)) {
                                            NotifyManager::Send(std::string("Bugar Pixel ") + (Bloquearmira ? "ativado" : "desativado"));
                                        }
                                        if (ImGui::Checkbox("Sem Recoil", &NoRecoil)) {
                                            NotifyManager::Send(std::string("Sem Recoil ") + (NoRecoil ? "ativado" : "desativado"));
                                        }
                                        if (ImGui::Checkbox("Trocar Armar (Insta)", &FastSwitch)) {
                                            NotifyManager::Send(std::string("Trocar Armar ") + (FastSwitch ? "ativado" : "desativado"));
                                        }
                                        if (FastSwitch) {
                                            ImGui::SliderFloat("Fast Swap Speed", &FastSwapSpeed, 0.0f, 100.0f, "%.0f", ImGuiSliderFlags_None);
                                        }
                                        if (ImGui::Checkbox("Spin-bot", &SpinBot)) {
                                            NotifyManager::Send(std::string("Spin-bot ") + (SpinBot ? "ativado" : "desativado"));
                                        }
                                        if (SpinBot) {
                                            ImGui::SliderFloat("Spin-bot Speed", &SpinbotSpeed, 0.0f, 20.0f, "%.0f", ImGuiSliderFlags_None);
                                        }
                                        if (ImGui::Checkbox("Aumentar tempo", &SpeedTimer)) {
                                            NotifyManager::Send(std::string("Aumentar tempo ") + (SpeedTimer ? "ativado" : "desativado"));
                                        }
                                        if (SpeedTimer) {
                                            ImGui::SliderFloat("Velocidade (Risk)", &SpeedHackerVelocity, 0.0f, 10.0f, "%.0f", ImGuiSliderFlags_None);
                                        }
                                        if (ImGui::Checkbox("Teleportar para mim", &TelekillToMe)) {
                                            NotifyManager::Send(std::string("Teleportar para mim ") + (TelekillToMe ? "ativado" : "desativado"));
                                        }
                                    }
                                    ImGui::EndGroupBox();

                                    ImGui::SetCursorPos(ImVec2(292, 8));
                                   
                                    ImGui::BeginGroup(); {

                                        if (ImGui::BeginGroupBox("KeyBinds", "KeyBinds configurar", ICON_FA_KEYBOARD, ImVec2(ImGui::GetWindowSize().x / 2 - 3, 437)))
                                        {
                                            ImGui::KeyBind("No Clip", &KeysBind.NoClip);
                                            ImGui::KeyBind("Wall Hack N32", &KeysBind.WallHack);
                                            ImGui::KeyBind("Ghost Hack", &KeysBind.GhostHack);
                                            ImGui::KeyBind("Camera Direita", &KeysBind.UnderCamEnabled);
                                            ImGui::KeyBind("Subir Inimigo", &KeysBind.UpPlayerEnabled);
                                            ImGui::KeyBind("Descer Inimigo", &KeysBind.DownPlayerEnabled);
                                            ImGui::KeyBind("Descer Local", &KeysBind.DownLocalEnabled);
                                            ImGui::KeyBind("Teleporta até Player", &KeysBind.TeleportPlayer);
                                            ImGui::KeyBind("Tp Marcado", &KeysBind.TeleportMark);
                                            ImGui::KeyBind("Longo Parachute", &KeysBind.longparachute);
                                        }
                                        ImGui::EndGroupBox();
                                    }
                                    ImGui::EndGroup();
                                }
                                ImGui::EndGroup();
                            }
                            else if (CurrentSub == 1)
                            {
                                static int SelectedIndex = -1;
                                const ImVec2 startPos = ImGui::GetCursorPos();


                                ImGui::SetCursorPos(ImVec2(22, 8));
                                ImGui::BeginGroup();
                                {
                                    ImVec2 avail = ImGui::GetContentRegionAvail();
                                    ImVec2 childSize = ImVec2(avail.x - 24.0f, avail.y - 70.0f);

                                    if (childSize.x < 1) childSize.x = 1;
                                    if (childSize.y < 1) childSize.y = 1;

                                    if (ImGui::BeginGroupBox("Teleport", "Players Online", ICON_FA_KEYBOARD, childSize))
                                    {
                                        ImVec2 innerAvail = ImGui::GetContentRegionAvail();

                                        if (RenderedPlayers.empty())
                                        {
                                            const char* msg = "Esperando Jogadores...";
                                            ImVec2 textSize = ImGui::CalcTextSize(msg);

                                            ImVec2 cur = ImGui::GetCursorPos();
                                            ImGui::SetCursorPos(ImVec2(cur.x + (innerAvail.x - textSize.x) * 0.5f, cur.y + (innerAvail.y - textSize.y) * 0.5f));
                                            ImGui::TextDisabled("%s", msg);
                                        }
                                        else
                                        {
                                            ImGui::BeginChild("##player_scroll", ImVec2(0, 0), false, ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse);
                                            {
                                                for (int i = 0; i < (int)RenderedPlayers.size(); i++)
                                                {
                                                    bool isSelected = (SelectedIndex == i);

                                                    char buf[256];
                                                    if (RenderedPlayers[i].IsDying)
                                                        snprintf(buf, sizeof(buf), "%s [Knocked] - %.1fm", RenderedPlayers[i].Name.c_str(), RenderedPlayers[i].Distance);
                                                    else
                                                        snprintf(buf, sizeof(buf), "%s [%dHP] - %.1fm", RenderedPlayers[i].Name.c_str(), RenderedPlayers[i].Health, RenderedPlayers[i].Distance);

                                                    if (ImGui::ListSelectableEx(buf, isSelected))
                                                        SelectedIndex = i;
                                                }
                                            }
                                            ImGui::EndChild();
                                        }
                                    }
                                    ImGui::EndGroupBox();
                                    

                                    ImGui::SetCursorPosY(ImGui::GetCursorPosY() + 20);
                                    bool hasSelectedPlayer = (SelectedIndex != -1 && SelectedIndex < (int)RenderedPlayers.size());
                                    if (!hasSelectedPlayer)
                                    {
                                        ImGui::BeginDisabled();
                                    }
                                    if (ImGui::Button("Teleporta até Player", ImVec2(avail.x - 24.0f, 35)))
                                    {
                                        if (hasSelectedPlayer)
                                        {
                                            uintptr_t targetPlayer = RenderedPlayers[SelectedIndex].Entity;
                                            ProcessTeleport(targetPlayer, JogadorLocal, RenderedPlayers[SelectedIndex].Name);
                                        }
                                    }

                                    if (!hasSelectedPlayer)
                                    {
                                        ImGui::EndDisabled();

                                        if (ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenDisabled))
                                        {
                                            ImGui::BeginTooltip();
                                            ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(10, 6));
                                            ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 3.0f);
                                            ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 1.0f);
                                            ImGui::PushStyleColor(ImGuiCol_Border, IM_COL32(35, 35, 35, 255));
                                            ImGui::PushStyleColor(ImGuiCol_PopupBg, IM_COL32(18, 18, 20, 245));
                                            ImGui::TextColored(ImVec4(1, 0, 0, 1), "Selecione o player primeiro!");
                                            ImGui::PopStyleColor(2);
                                            ImGui::PopStyleVar(3);
                                            ImGui::EndTooltip();
                                        }
                                    }

                                    if (hasSelectedPlayer)
                                    {
                                        ImGui::SetCursorPosY(ImGui::GetCursorPosY() + 2);
                                        ImGui::TextColored(ImVec4(0, 1, 0, 1), "Selecionado: %s", RenderedPlayers[SelectedIndex].Name.c_str());
                                    }
                                }
                                ImGui::EndGroup();
                            }
                            
                        }
                        if (previewMode) { 
                            ImGui::EndDisabled(); 
                            ImGui::PopStyleVar(); 
                        }
                        ImGui::EndChild();
                    }
                    else if (CurrentTab == 6)
                    {
                        static int CurrentSub = 0;
                        static float Anima = 0.f;

                        ImGui::SetCursorPos(ImVec2(Anima, 65));
                        ImGui::BeginChild("Misc");
                        if (previewMode) { 
                            ImGui::PushStyleVar(ImGuiStyleVar_DisabledAlpha, 1.0f); 
                            ImGui::BeginDisabled(); 
                        }
                        {
                            ImGui::SetCursorPos(ImVec2(5, 8));
                            ImGui::BeginGroup();
                            {
                                if (ImGui::BeginGroupBox("Settings", "Settings", ICON_FA_BOMB, ImVec2(ImGui::GetWindowSize().x / 2 - 25, 437)))
                                {
                                    if (ImGui::Checkbox("Stream Mode", &StreamProof)) {
                                        NotifyManager::Send(std::string("Stream Proof ") + (StreamProof ? "ativado" : "desativado"));
                                    }
                                    if (ImGui::Checkbox("Tirar Esp no Painel", &espRequireForeground)) {
                                        DirectOverlaySetRequireForeground(espRequireForeground);
                                        NotifyManager::Send(std::string("Smart Overlay ") + (espRequireForeground ? "ativado" : "desativado"));
                                    }
                                    
                                    static const char* priority_options = "Alto\0Normal\0Baixo\0";
                                    static int current_priority = static_cast<int>(Priority);
                                    if (ImGui::Combo("Processo Prioridade", &current_priority, priority_options, 3))
                                    {
                                        Priority = current_priority;

                                        DWORD priorityClass = NORMAL_PRIORITY_CLASS;
                                        switch (current_priority)
                                        {
                                        case 0: priorityClass = HIGH_PRIORITY_CLASS; break;
                                        case 1: priorityClass = NORMAL_PRIORITY_CLASS; break;
                                        case 2: priorityClass = IDLE_PRIORITY_CLASS; break;
                                        }

                                        HANDLE hProcess = GetCurrentProcess();
                                        if (!SetPriorityClass(hProcess, priorityClass))
                                        {
                                            char errMsg[128];
                                            DWORD error = GetLastError();
                                            snprintf(errMsg, sizeof(errMsg), "Failure to prioritize (error %lu)", error);
                                        }
                                    }

                                    ImGui::KeyBind("Bind Abrir/Fechar:", &KeysBind.Menu);
                                    if (KeysBind.Menu == VK_LBUTTON) {
                                        KeysBind.Menu = 0;
                                    }
                                    if (ImGui::Checkbox("Mostrar Lista de Bind", &KeybindsMenu)) {
                                        NotifyManager::Send(std::string("Keybind Panel ") + (KeybindsMenu ? "visível" : "oculto"));
                                    }

                                    const float settingsButtonWidth = 210.0f;
                                    const float settingsButtonStartX = ImGui::GetCursorPosX();
                                    const float settingsButtonAvailX = ImGui::GetContentRegionAvail().x;
                                    ImGui::SetCursorPosX(settingsButtonStartX + (settingsButtonAvailX - settingsButtonWidth) * 0.5f);
                                    if (ImGui::Button("Desligar Presença Rica", ImVec2(settingsButtonWidth, 0.0f))) {
                                        if (DiscordRPC) DiscordRPC->Shutdown();
                                    }
                                }
                                ImGui::EndGroupBox();

                            }
                            ImGui::EndGroup();

                            ImGui::SetCursorPos(ImVec2(292, 8));

                            ImGui::BeginGroup(); {
                                if (ImGui::BeginGroupBox("Bypass SS", "Bypass Neles", ICON_FA_KEYBOARD, ImVec2(ImGui::GetWindowSize().x / 2 - 3, 437)))
                                {
                                    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.784f, 0.0f, 0.0f, 1.0f));

                                    ImGui::PushFont(LexendRegular);
                                    ImGui::Text("Leia Por Favor:");
                                    ImGui::PopFont();
                                    ImGui::PopStyleColor();
                                    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(200.0f / 255.0f, 200.0f / 255.0f, 200.0f / 255.0f, 1.0f));
                                    ImGui::PushFont(LexendRegular);
                                    ImGui::Text("Você clicando o botao\ndesinjetar\ntodas função e emulador\nirao sumir\nai você inejeta denovo!");
                                    ImGui::PopFont();
                                    ImGui::PopStyleColor();

                                    const float unloadButtonWidth = 210.0f;
                                    const float unloadButtonStartX = ImGui::GetCursorPosX();
                                    const float unloadButtonAvailX = ImGui::GetContentRegionAvail().x;
                                    ImGui::SetCursorPosX(unloadButtonStartX + (unloadButtonAvailX - unloadButtonWidth) * 0.5f);
                                    if (ImGui::Button("Desinjeta (Fecha Emu)", ImVec2(unloadButtonWidth, 0.0f)))
                                    {
                                        exit(0);
                                    }
                                    if (ImGui::IsItemHovered(ImGuiHoveredFlags_DelayShort))
                                    {
                                        ImGui::BeginTooltip();
                                        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(10, 6));
                                        ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 3.0f);
                                        ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 1.0f);
                                        ImGui::PushStyleColor(ImGuiCol_Border, IM_COL32(35, 35, 35, 255));
                                        ImGui::PushStyleColor(ImGuiCol_PopupBg, IM_COL32(18, 18, 20, 245));
                                        ImGui::TextColored(ImVec4(1, 0, 0, 1), "Fecha jogo e emulador");
                                        ImGui::PopStyleColor(2);
                                        ImGui::PopStyleVar(3);
                                        ImGui::EndTooltip();
                                    }
                                }
                                ImGui::EndGroupBox();
                            }
                            ImGui::EndGroup();
                        }
                        if (previewMode) { 
                            ImGui::EndDisabled(); 
                            ImGui::PopStyleVar(); 
                        }
                        ImGui::EndChild();
                    }
                    else if (CurrentTab == 7)
                    {
                        static float Anima = 0.f;

                        ImGui::SetCursorPos(ImVec2(Anima, 65));
                        ImGui::BeginChild("Configs");
                        if (previewMode) { 
                            ImGui::PushStyleVar(ImGuiStyleVar_DisabledAlpha, 1.0f); 
                            ImGui::BeginDisabled(); 
                        }
                        {
                            ImGui::SetCursorPos(ImVec2(5, 8));
                            ImGui::BeginGroup();
                            {
                                if (ImGui::BeginGroupBox("Informação", "Detalhe de seu regitro", ICON_FA_KEYBOARD, ImVec2(585, 437)))
                                {
                                    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 1.0f, 1.0f, 1.0f));
                                    ImGui::PushFont(LexendRegular);
                                    ImGui::Text("User:");
                                    ImGui::PopFont();
                                    ImGui::PopStyleColor();

                                    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(200.0f / 255.0f, 200.0f / 255.0f, 200.0f / 255.0f, 1.0f));
                                    ImGui::PushFont(LexendRegular);
                                    ImGui::Text("Username: %s", loggedUser.c_str());
                                    char shortId[16]{};
                                    if (loggedDiscordID.length() > 6)
                                        snprintf(shortId, sizeof(shortId), "%.6s...", loggedDiscordID.c_str());
                                    else
                                        snprintf(shortId, sizeof(shortId), "%s", loggedDiscordID.c_str());
                                    ImGui::Text("Discord ID: %s", shortId);
                                    ImGui::Text("Expira em: %s", expirationInfo.c_str());
                                    ImGui::Text("Desenvolvedor: Romeira");
                                    ImGui::PopFont();
                                    ImGui::PopStyleColor();
                                }
                                ImGui::EndCustomChild();
                            }
                            ImGui::EndGroup();
                            
                        }
                        if (previewMode) { 
                            ImGui::EndDisabled(); 
                            ImGui::PopStyleVar(); 
                        }
                        ImGui::EndChild();
                    }
                }
                ImGui::EndChild();

                ImVec2 tabHeaderScreenPos = Pos + ImVec2(79.0f, 73.0f);
                ImGui::SetCursorScreenPos(tabHeaderScreenPos);
                ImVec2 tabHeaderSize = ImVec2(165.0f, 100.0f);
                ImGui::BeginChild("CustomTabPanel", tabHeaderSize, false, ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoBackground);
                {
                    static int dummySub = 0;
                    
                    if (CurrentTab == 2)
                    {
                        std::vector<const char*> combatTabs = { ICON_FA_CROSSHAIRS "Aimbot", ICON_FA_BULLSEYE "Silent" };
                        ImGui::TabHeader("CombatHeader", &CombatSub, combatTabs, 0);
                    }
                    else if (CurrentTab == 4)
                    {
                        std::vector<const char*> exploitTabs = {ICON_FA_MICROCHIP "Exploits", ICON_FA_LOCATION_ARROW "Teleport"};
                        ImGui::TabHeader("ExploitHeader", &CurrentSub, exploitTabs, 0);
                    }
                    else if (CurrentTab == 3)
                    {
                        std::vector<const char*> worldTabs = {ICON_FA_GLOBE "World"};
                        ImGui::TabHeader("WorldHeader", &dummySub, worldTabs, 0);
                    }
                    else if (CurrentTab == 6)
                    {
                        std::vector<const char*> miscTabs = {ICON_FA_LAYER_GROUP "Misc"};
                        ImGui::TabHeader("MiscHeader", &dummySub, miscTabs, 0);
                    }
                    else if (CurrentTab == 7) 
                    {
                        std::vector<const char*> configTabs = { ICON_FA_CLOUD "Configs" };
                        ImGui::TabHeader("ConfigHeader", &dummySub, configTabs, 0);
                    }
                }
                ImGui::EndChild();

                DrawList->AddRect(Pos, Pos + Size, ImGui::GetColorU32(ImGuiCol_Border), ImGui::GetStyle().WindowRounding);
            }
            ImGui::End();
            ImGui::PopStyleVar();
        }
    }

    SaveKeyBinds();
    SaveSettings();
    NotifyManager::Render();
    ImGui::EndFrame();
    ImGui::Render();
    g_pd3dDeviceContext->OMSetRenderTargets(1, &g_mainRenderTargetView, nullptr);
    g_pd3dDeviceContext->ClearRenderTargetView(g_mainRenderTargetView, new FLOAT[4]{ 0.0f, 0.0f, 0.0f, 0.0f });
    ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());

    g_pSwapChain->Present(1, 0);
}

void CleanupAvatar() {
    if (g_AvatarTexture) {
        DiscordAvatar::FreeTexture(g_AvatarTexture);
        g_AvatarTexture = nullptr;
    }
}

void InitIdow() {
    //AbrirConsole();

    JanelaAlvo = LookupWindowByClassName(AY_OBFUSCATE("BlueStacksApp"));
    if (!JanelaAlvo) {
        return;
    }

    LoadKeyBinds();
    LoadSettings();
    setupWindow();

    InitializeCollider();
    InitializeSilentAim();

    if (!g_pSwapChain) {
        return;
    }

    SetWindowLong(hwnd, GWL_EXSTYLE, WS_EX_TOPMOST | WS_EX_TOOLWINDOW | WS_EX_TRANSPARENT);
    SetForegroundWindow(hwnd);
    SetWindowPos(hwnd, HWND_TOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_FRAMECHANGED);

    while (true) {
        handleKeyPresses();
        runRenderTick();
    }

    ImGui_ImplDX11_Shutdown();
    ImGui_ImplWin32_Shutdown();
    ImGui::DestroyContext();

    ShutdownCollider();
    ShutdownSilentAim();

    static bool CaptureBypassOn = false;
    if (StreamProof != CaptureBypassOn)
    {
        CaptureBypassOn = StreamProof;
        SetWindowDisplayAffinity(hwnd, CaptureBypassOn ? WDA_EXCLUDEFROMCAPTURE : WDA_NONE);
    }

    CleanupDeviceD3D();
    CleanupAvatar();
    ::DestroyWindow(hwnd);
    ::UnregisterClass(wc.lpszClassName, wc.hInstance);
    return;
}

BOOL WINAPI DllMain(HINSTANCE hinstDLL, DWORD fdwReason, LPVOID lpReserved) {
    switch (fdwReason)
    {
    case DLL_PROCESS_ATTACH:
        DisableThreadLibraryCalls(hinstDLL);
        CreateThread(nullptr, NULL, (LPTHREAD_START_ROUTINE)InitIdow, nullptr, NULL, nullptr);
        break;
    case DLL_THREAD_ATTACH:
    case DLL_THREAD_DETACH:
    case DLL_PROCESS_DETACH:
        break;
    }
    return TRUE;
}
