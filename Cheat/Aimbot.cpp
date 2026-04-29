#include "Aimbot.h"
#include "../Imports/Utils.h"
#include "../Imports/Offsets.h"
#include "../Imports/Scope.h"
#include <windows.h>
#include <algorithm>
#include <random>
#include <vector>
#include <unordered_map>
#include <chrono>
#include <thread>
#include <atomic>
#include <ctime>

// ====================================================================
// SISTEMA DE AIMBOT OMBRO (ANTIGO TYPE 1)
// ====================================================================
void ProcessShoulderAimbot(uintptr_t ClosestEnemy, uintptr_t JogadorLocal, bool LoginMemory) {
    // Variáveis estáticas para manter estado entre chamadas
    static std::vector<uintptr_t> AppliedEntities;     // TODAS as entidades que já foram modificadas
    static std::unordered_map<uintptr_t, int> OriginalValues;  // valor original do TELA de cada uma
    static bool WasActive = false;

    // Se desativou o aimbot, restaurar tudo
    if (!ShoulderAimbot && WasActive) {
        if (!AppliedEntities.empty()) {
            for (uintptr_t addr : AppliedEntities) {
                if (addr == 0) continue;
                int original = OriginalValues[addr];
                try {
                    Escrever<int>(addr + TELA, original);
                }
                catch (...) {}
            }
            AppliedEntities.clear();
            OriginalValues.clear();
        }
        WasActive = false;
        return;
    }

    if (!ShoulderAimbot) return;

    WasActive = true;

    if (ClosestEnemy == 0 || JogadorLocal == 0) return;

    uintptr_t entity = ClosestEnemy;

    // Verificar se o jogador está atirando
    bool LPEIEILIKGC = Ler<bool>(JogadorLocal + string2Offset(AY_OBFUSCATE("0x4E0")));

    // ====================================================================
    // 2. APLICA AIMBOT OMBRO — E GUARDA A ENTIDADE NA LISTA (LoginKey: 0 = usuário escolhe, senão usa a tecla)
    // ====================================================================
    
    // Verificar se o keybind está ativo usando a função externa
    extern bool IsShoulderAimbotActive();
    if (IsShoulderAimbotActive()) {
        short health = -1;
        try {
            uintptr_t rep = Ler<uintptr_t>(Ler<uintptr_t>(Ler<uintptr_t>(entity + Offsets.PRIDataPool) + Offsets.ReplicationDataPoolUnsafe) + Offsets.ReplicationDataUnsafe);
            health = Ler<short>(rep + Offsets.Health);
        }
        catch (...) {}

        if (health > 0) {
            // Se ainda não aplicamos nessa entidade
            if (std::find(AppliedEntities.begin(), AppliedEntities.end(), entity) == AppliedEntities.end()) {
                try {
                    int monitor = Ler<int>(entity + MONITOR);
                    int atual = Ler<int>(entity + TELA);

                    if (monitor != atual) {
                        AppliedEntities.push_back(entity);
                        OriginalValues[entity] = atual;

                        Escrever<int>(entity + TELA, monitor);
                    }
                }
                catch (...) {}
            }
        }
    }

    // Estado é controlado pela variável IsActive
}

static std::atomic<uintptr_t> colliderTarget{ 0 };
static std::atomic<bool> colliderRunning{ false };
static std::thread colliderThread;

static std::random_device rd;
static std::mt19937 gen(rd());

namespace Stealth {
    bool ReadWithDelay(uintptr_t address, int* value) {
        std::uniform_int_distribution<> delayDist(1, 5);
        std::this_thread::sleep_for(std::chrono::milliseconds(delayDist(gen)));

        try {
            *value = Ler<int>(address);
            return true;
        }
        catch (...) {
            return false;
        }
    }

    void WriteWithDelay(uintptr_t addr, int value) {
        std::uniform_int_distribution<> delayDist(1, 8);
        std::this_thread::sleep_for(std::chrono::milliseconds(delayDist(gen)));

        try {
            Escrever<int>(addr, value);
        }
        catch (...) {}
    }

    void HumanDelay(int baseMs, int variationMs) {
        std::uniform_int_distribution<> delayDist(0, variationMs);
        int delay = baseMs + delayDist(gen);
        std::this_thread::sleep_for(std::chrono::milliseconds(delay));
    }

    bool IsValidAddress(uintptr_t addr) {
        if (addr == 0 || addr == 0xFFFFFFFF) return false;
        try {
            int testValue = Ler<int>(addr);
            return true;
        }
        catch (...) {
            return false;
        }
    }
}

bool IsAimbotKeyPressed() {
    extern bool IsAimbotColliderActive();
    return IsAimbotColliderActive();
}

void ColliderWorker() {
    static std::chrono::steady_clock::time_point lastCheck;
    static uintptr_t lastTarget = 0;
    static int targetSwitchCooldown = 0;
    static bool wasKeyPressedLastFrame = false;

    while (colliderRunning) {
        auto now = std::chrono::steady_clock::now();
        bool keyPressed = IsAimbotKeyPressed();

        if (!keyPressed) {
            if (lastTarget != 0) {
                uintptr_t lockedAimAddr = lastTarget + LOGIN_OFF_WRITE;
                if (Stealth::IsValidAddress(lockedAimAddr)) {
                    Stealth::WriteWithDelay(lockedAimAddr, 0);
                }
                lastTarget = 0;
            }
            colliderTarget.store(0);
            wasKeyPressedLastFrame = false;

            std::this_thread::sleep_for(std::chrono::milliseconds(100));
            continue;
        }

        if (!wasKeyPressedLastFrame && keyPressed) {
            Stealth::HumanDelay(50, 25);
        }
        wasKeyPressedLastFrame = keyPressed;

        int interval = AimbotCollider ? 30 : 50;
        if (std::chrono::duration_cast<std::chrono::milliseconds>(now - lastCheck).count() < interval) {
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
            continue;
        }
        lastCheck = now;

        if (!AimbotCollider) {
            if (lastTarget != 0) {
                uintptr_t lockedAimAddr = lastTarget + LOGIN_OFF_WRITE;
                if (Stealth::IsValidAddress(lockedAimAddr)) {
                    Stealth::WriteWithDelay(lockedAimAddr, 0);
                }
                lastTarget = 0;
            }
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
            continue;
        }

        uintptr_t target = colliderTarget.load();

        if (target == 0) {
            if (lastTarget != 0) {
                uintptr_t lockedAimAddr = lastTarget + LOGIN_OFF_WRITE;
                if (Stealth::IsValidAddress(lockedAimAddr)) {
                    Stealth::WriteWithDelay(lockedAimAddr, 0);
                }
                lastTarget = 0;
            }
            std::this_thread::sleep_for(std::chrono::milliseconds(50));
            continue;
        }

        bool isNewTarget = (lastTarget == 0 || lastTarget != target);

        if (isNewTarget && targetSwitchCooldown > 0) {
            targetSwitchCooldown -= interval;
            if (targetSwitchCooldown < 0) targetSwitchCooldown = 0;
            Stealth::HumanDelay(10, 10);
            continue;
        }

        uintptr_t headColliderAddr = target + LOGIN_OFF_READ;
        uintptr_t lockedAimAddr = target + LOGIN_OFF_WRITE;

        if (!Stealth::IsValidAddress(headColliderAddr) || !Stealth::IsValidAddress(lockedAimAddr)) {
            if (lastTarget != 0) {
                lastTarget = 0;
            }
            Stealth::HumanDelay(10, 10);
            continue;
        }

        if (AimbotColliderDelay > 0.0f) {
            std::this_thread::sleep_for(std::chrono::milliseconds(static_cast<int>(AimbotColliderDelay)));
        }

        int collider = 0;
        if (!Stealth::ReadWithDelay(headColliderAddr, &collider) || collider == 0) {
            Stealth::HumanDelay(10, 5);
            continue;
        }

        int current = 0;
        if (Stealth::ReadWithDelay(lockedAimAddr, &current)) {
            if (current != collider) {
                Stealth::WriteWithDelay(lockedAimAddr, collider);

                if (isNewTarget) {
                    targetSwitchCooldown = 500;
                    lastTarget = target;
                }
                Stealth::HumanDelay(15, 10);
            }
            else {
                Stealth::HumanDelay(5, 5);
            }
        }

        std::uniform_int_distribution<> antiDetectDist(0, 1000);
        if (antiDetectDist(gen) > 990) {
            Stealth::WriteWithDelay(lockedAimAddr, 0);
            Stealth::HumanDelay(20, 10);
        }
    }
}

void InitializeCollider() {
    if (!colliderRunning) {
        colliderRunning = true;
        colliderThread = std::thread(ColliderWorker);
    }
}

void ShutdownCollider() {
    if (colliderRunning) {
        colliderRunning = false;
        if (colliderThread.joinable()) {
            colliderThread.join();
        }
    }
}

void UpdateColliderTarget(uintptr_t target) {
    colliderTarget.store(target);
}