#include "SilentAim.h"
#include "../Imports/Utils.h"
#include "../Imports/Offsets.h"
#include "../Imports/Process.h"
#include "../Imports/Scope.h"
#include "../Unity/Vector3.h"
#include "../Cfg/strenc.h"
#include <random>
#include <string>

static std::random_device rd;
static std::mt19937 gen(rd());

static Vector3 GetSilentHitboxPosition(uintptr_t entity, int hitboxIndex) {
    if (hitboxIndex == 0) return GetHeadPosition(entity);

    bool Garota = Ler<bool>(entity + Offsets.isfemale);
    uintptr_t off;
    switch (hitboxIndex) {
        case 1:  off = string2Offset(AY_OBFUSCATE("0x14")); break;                           // Chest
        case 2:  off = string2Offset(AY_OBFUSCATE("0x10")); break;                           // Stomach
        case 3:  off = Garota ? string2Offset(AY_OBFUSCATE("0x10")) : string2Offset(AY_OBFUSCATE("0x48")); break; // Hip
        case 4:  off = Garota ? string2Offset(AY_OBFUSCATE("0x1C")) : string2Offset(AY_OBFUSCATE("0x18")); break; // LShoulder
        case 5:  off = Garota ? string2Offset(AY_OBFUSCATE("0x2C")) : string2Offset(AY_OBFUSCATE("0x28")); break; // RShoulder
        case 6:  off = Garota ? string2Offset(AY_OBFUSCATE("0x20")) : string2Offset(AY_OBFUSCATE("0x1C")); break; // LArm
        case 7:  off = Garota ? string2Offset(AY_OBFUSCATE("0x30")) : string2Offset(AY_OBFUSCATE("0x2C")); break; // RArm
        case 8:  off = Garota ? string2Offset(AY_OBFUSCATE("0x24")) : string2Offset(AY_OBFUSCATE("0x20")); break; // LForearm
        case 9:  off = Garota ? string2Offset(AY_OBFUSCATE("0x34")) : string2Offset(AY_OBFUSCATE("0x30")); break; // RForearm
        case 10: off = Garota ? string2Offset(AY_OBFUSCATE("0x28")) : string2Offset(AY_OBFUSCATE("0x24")); break; // LHand
        case 11: off = Garota ? string2Offset(AY_OBFUSCATE("0x38")) : string2Offset(AY_OBFUSCATE("0x34")); break; // RHand
        case 12: off = Garota ? string2Offset(AY_OBFUSCATE("0x40")) : string2Offset(AY_OBFUSCATE("0x3C")); break; // LThigh
        case 13: off = string2Offset(AY_OBFUSCATE("0x4C")); break;                          // RThigh
        case 14: off = string2Offset(AY_OBFUSCATE("0x40")); break;                           // LShin
        case 15: off = string2Offset(AY_OBFUSCATE("0x50")); break;                           // RShin
        default: return GetHeadPosition(entity);
    }
    return ObterOssos(entity, off);
}

static const int SILENT_WRITE_LOOP_COUNT = 28000;

void UpdateSilentScene(uintptr_t localPlayer, uintptr_t targetEntity, int hitboxIndex) {
    if (localPlayer == 0 || targetEntity == 0) return;
    
    extern bool IsSilentActive();
    if (!IsSilentActive()) return;

    if (!Ler<bool>(localPlayer + Offsets.LPEIEILIKGC))
        return;

    Vector3 targetPos = GetSilentHitboxPosition(targetEntity, hitboxIndex);
    if (targetPos.X == 0 && targetPos.Y == 0 && targetPos.Z == 0) return;
    targetPos.Y += 0.05f;
    uintptr_t weaponPtr = Ler<uintptr_t>(localPlayer + Offsets.MADMMIICBNN);
    if (weaponPtr == 0) return;

    uintptr_t directionOffset = weaponPtr + 0x2C;
    uintptr_t startPosOffset = weaponPtr + 0x38;

    std::uniform_real_distribution<float> dis(0.0f, 1.0f);

    for (int i = 0; i < SILENT_WRITE_LOOP_COUNT; i++) {
        Vector3 shootOrigin = Ler<Vector3>(startPosOffset);
        if (shootOrigin.X == 0 && shootOrigin.Y == 0 && shootOrigin.Z == 0) continue;
     
        Vector3 direction;
        direction.X = targetPos.X - shootOrigin.X;
        direction.Y = targetPos.Y - shootOrigin.Y;
        direction.Z = targetPos.Z - shootOrigin.Z;
        if (direction.X == 0 && direction.Y == 0 && direction.Z == 0) continue;

        const float smoothMin = 0.0015f;
        const float smoothMax = 0.0035f;
        float smoothingFactor = (dis(gen) * (smoothMax - smoothMin)) + smoothMin;
        direction.X += (dis(gen) * smoothingFactor) - (smoothingFactor * 0.5f);
        direction.Y += (dis(gen) * smoothingFactor) - (smoothingFactor * 0.5f);
        direction.Z += (dis(gen) * smoothingFactor) - (smoothingFactor * 0.5f);

        Escrever<Vector3>(directionOffset, direction);
    }
}

void InitializeSilentAim() {
    (void)0;
}

void ShutdownSilentAim() {
    (void)0;
}
