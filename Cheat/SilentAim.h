#pragma once

#include <cstdint>

// Silent Aim: lógica testarsi (modo + hitbox) + meu smoothing + loop de escritas
void InitializeSilentAim();
void ShutdownSilentAim();
// hitboxIndex: 0=Head, 1=Chest, 2=Stomach, 3=Hip, 4=LShoulder, 5=RShoulder, 6=LArm, 7=RArm, 8=LForearm, 9=RForearm, 10=LHand, 11=RHand, 12=LThigh, 13=RThigh, 14=LShin, 15=RShin
void UpdateSilentScene(uintptr_t localPlayer, uintptr_t targetEntity, int hitboxIndex);
