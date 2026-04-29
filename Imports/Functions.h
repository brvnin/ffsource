#pragma once

int TypeDefIndex(uintptr_t address) {
	uintptr_t inputPtr = Ler<uintptr_t>(address);
	if (inputPtr == 0) return false;
	int TypeDefIndex = Ler<int>(inputPtr + string2Offset(AY_OBFUSCATE("0x10")));
	return TypeDefIndex;
}

int PlayerNetwork = 8886; //  internal class PlayerNetwork : Player, IPlayerNetworkSyncModule
int PlayerUGCCommon = 8948; // internal class PlayerUGCCommon : PlayerNetwork, IBridgingEntity, IUGCPlayer
int Player_TrainingHumanTarget_Stand = 8808; // internal class Player_TrainingHumanTarget_Stand : Player_TrainingHumanTarget
int Player_TrainingHumanTarget = 8809; // internal class Player_TrainingHumanTarget : Player
