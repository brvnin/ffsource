#pragma once

#include <cstdint>
#include "../Imports/Scope.h"

// Sistema de Aimbot Ombro (antigo Type 1)
void ProcessShoulderAimbot(uintptr_t ClosestEnemy, uintptr_t JogadorLocal, bool LoginMemory);

// Funções do sistema de collider
void InitializeCollider();
void ShutdownCollider();
void UpdateColliderTarget(uintptr_t target);
