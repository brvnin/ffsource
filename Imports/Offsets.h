#ifndef OFFSETS_H
#define OFFSETS_H

#include <cstdint>
#include "../Cfg/strenc.h"
#include "../Cfg/encrypt.hh"

struct GameVarDef {
	uintptr_t GameVarPtr = string2Offset(AY_OBFUSCATE("0xA545028")); // Gamevardef
};

struct {
	// --- [ GameVar ] --- //
	GameVarDef GameVarClass;

	// --- [ Access Class ] --- //
	uintptr_t GameEngine = string2Offset(AY_OBFUSCATE("0"));

	// --- [ Other ] --- // 
	uintptr_t jsonuni = string2Offset(AY_OBFUSCATE("0x5c")); // ok

	// --- [ Player ] --- //
	uintptr_t EmVeiculo = string2Offset(AY_OBFUSCATE("0x2D4")); // public Boolean m_GetInVehicle
	uintptr_t m_CachedTransform = string2Offset(AY_OBFUSCATE("0x38")); // Atualizado: era 0x34 agora é 0x38
	uintptr_t MainTransform = string2Offset(AY_OBFUSCATE("0x1FC")); // public Transform MainCameraTransform
	uintptr_t PRIDataPool = string2Offset(AY_OBFUSCATE("0x44"));
	uintptr_t MainCamTran = string2Offset(AY_OBFUSCATE("0x1FC")); // public Transform MainCameraTransform
	uintptr_t FollowCam = string2Offset(AY_OBFUSCATE("0x3F0")); // protected FollowCamera m_FollowCamera
	uintptr_t HeadTransform = string2Offset(AY_OBFUSCATE("0x3F8"));
	uintptr_t HipTransform = string2Offset(AY_OBFUSCATE("0x3FC"));   // HipNode
	uintptr_t Inventario = string2Offset(AY_OBFUSCATE("0x448")); // protected InventoryManager m_InventoryManager
	uintptr_t AvatarManager = string2Offset(AY_OBFUSCATE("0x460")); // protected AvatarManager m_AvatarManager
	uintptr_t isFiring = string2Offset(AY_OBFUSCATE("0x4C0"));
	uintptr_t PlayerNetwork_HHCBNAPCKHF = string2Offset(AY_OBFUSCATE("0x15E8")); // public ShadowState m_ShadowState
	uintptr_t StatePlayer = string2Offset(AY_OBFUSCATE("0x78")); // private Boolean m_IsExposed
	uintptr_t Inventario_Item = string2Offset(AY_OBFUSCATE("0x54")); // private Item m_itemOnHand
	uintptr_t FireComponent = string2Offset(AY_OBFUSCATE("0x58"));
	uintptr_t tangentTheta = string2Offset(AY_OBFUSCATE("0xC"));
	uintptr_t isfemale = string2Offset(AY_OBFUSCATE("0x771"));  // private Boolean <IsFemale>k__BackingField
	uintptr_t m_SwapWeaponTime = string2Offset(AY_OBFUSCATE("0x4BC"));

	// --- [ Aimlock/Target Snap ] --- //
	uintptr_t AimlockLocal = string2Offset(AY_OBFUSCATE("0x448"));
	uintptr_t m_IM = string2Offset(AY_OBFUSCATE("0x54"));
	uintptr_t Aimlock = string2Offset(AY_OBFUSCATE("0x46C"));
	uintptr_t LPEIEILIKGC = string2Offset(AY_OBFUSCATE("0x4E0"));
	uintptr_t MADMMIICBNN = string2Offset(AY_OBFUSCATE("0x8F0"));
	
	// --- [ HP ] --- //
	uintptr_t ReplicationDataPoolUnsafe = string2Offset(AY_OBFUSCATE("0x8")); // ok
	uintptr_t ReplicationDataUnsafe = string2Offset(AY_OBFUSCATE("0x10")); // ok
	uintptr_t Health = string2Offset(AY_OBFUSCATE("0xC")); // Atualizado de 0x10 para 0xC (ReplicationData Value)

	// --- [ UMAData ] --- //
	uintptr_t TeamMate = string2Offset(AY_OBFUSCATE("0x51")); // ok

	// --- [ Avatar Manager ] --- //
	uintptr_t UmaAvatarSimple = string2Offset(AY_OBFUSCATE("0x94")); // ok

	// --- [ Uma Avatar Simple ] ---- //
	uintptr_t UMAData = string2Offset(AY_OBFUSCATE("0x10")); // ok
	uintptr_t EstaVisivel = string2Offset(AY_OBFUSCATE("0x7C"));  // ok

	// --- [ GameEngine ] --- //
	uintptr_t BaseGame = string2Offset(AY_OBFUSCATE("0x10")); // Atualizado de 0xC para 0x10

	// --- [ BaseGame ] --- // 
	uintptr_t Partida = string2Offset(AY_OBFUSCATE("0x50")); // ok

	// --- [ Partida ] --- //
	uintptr_t JogadorLocal = string2Offset(AY_OBFUSCATE("0x7C")); // ok
	uintptr_t PlayerAttributes = string2Offset(AY_OBFUSCATE("0x618")); // teste
	uintptr_t GameTimer = string2Offset(AY_OBFUSCATE("0x2C")); // teste
	uintptr_t FixedDeltaTime = string2Offset(AY_OBFUSCATE("0x18")); // teste
	uintptr_t LocalSpectator = string2Offset(AY_OBFUSCATE("0x68"));
	uintptr_t Observer = string2Offset(AY_OBFUSCATE("0x9C"));
	uintptr_t TargetPlayer = string2Offset(AY_OBFUSCATE("0x28"));

	// --- [ UmaData - isLocalPlayer ] --- //
	uintptr_t isLocalPlayer = string2Offset(AY_OBFUSCATE("0x50"));

	// --- [ FollowCam ] ---- //
	uintptr_t Camera = string2Offset(AY_OBFUSCATE("0x14")); 

	// --- [ Camera ] ---- //
	uintptr_t Matrix = string2Offset(AY_OBFUSCATE("0xBC")); // ok

} Offsets;

#endif
