#pragma once
#pragma once
#include <Windows.h>
#include <TlHelp32.h>
#include <vector>
#include <string>
#include <cstdint>
#include <unordered_map>

// Classe para manipulação de memória do emulador HD-Player
class EmulatorMemory {
public:
    // Process handle
    HANDLE hProcess = nullptr;
    DWORD processId = 0;
    bool is64Bit = false;
    bool isInternal = false; // Se estamos rodando dentro do processo

    // Abre o processo HD-Player
    bool OpenEmulatorProcess();

    // Fecha o handle do processo
    void CloseEmulatorProcess();

    // Lê memória do processo
    bool ReadMemory(uintptr_t address, void* buffer, size_t size);

    // Escreve memória no processo
    bool WriteMemory(uintptr_t address, const void* buffer, size_t size);

    // Template para leitura tipada
    template<typename T>
    T Read(uintptr_t address) {
        T value{};
        ReadMemory(address, &value, sizeof(T));
        return value;
    }

    // Template para escrita tipada
    template<typename T>
    bool Write(uintptr_t address, T value) {
        return WriteMemory(address, &value, sizeof(T));
    }

    // AoB Scan - busca padrão na memória
    std::vector<uintptr_t> AoBScan(const std::string& pattern, bool readable = true, bool writable = true, bool executable = false);

    // AoB Scan FAST - versão paralela com múltiplas threads (MUITO MAIS RÁPIDO!)
    std::vector<uintptr_t> AoBScanFast(const std::string& pattern, bool readable = true, bool writable = true, bool executable = false);

private:
    // Estrutura para padrão de busca
    struct PatternInfo {
        std::vector<uint8_t> bytes;
        std::vector<bool> mask; // true = byte fixo, false = wildcard
        size_t length = 0;
    };

    // Converte string de padrão para PatternInfo
    PatternInfo BuildPattern(const std::string& patternString);

    // Verifica se o buffer corresponde ao padrão
    bool MatchesPattern(const uint8_t* buffer, size_t bufferSize, size_t startIndex, const PatternInfo& pattern);
};

// Instância global
extern EmulatorMemory g_EmulatorMem;

// ==================== FUNÇÕES DE AIM ====================

// Padrões de busca (AoB patterns)
namespace AimPatterns {
    // Padrão principal de Aimbot
    const std::string AIMBOT_SCAN = "FF FF 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 FF FF FF FF FF FF FF FF 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 ?? ?? ?? ?? ?? ?? ?? ?? ?? ?? ?? ?? ?? ?? ?? ?? ?? ?? ?? ?? ?? ?? ?? ?? ?? ?? ?? ?? ?? ?? ?? ?? ?? ?? ?? ?? ?? ?? ?? ?? ?? ?? ?? ?? ?? ?? ?? ?? ?? ?? ?? 00 00 00 00 00 00 00 00 00 00 00 00 A5 43";

    // Padrão para Aim Legit
    const std::string AIM_LEGIT = "FF FF FF 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 FF FF FF FF FF FF FF FF 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 ?? ?? ?? ?? ?? ?? ?? ?? ?? ?? ?? ?? ?? ?? ?? ?? ?? ?? ?? ?? ?? ?? ?? ?? ?? ?? ?? ?? ?? ?? ?? ?? ?? ?? ?? ?? ?? ?? ?? ?? ?? ?? ?? ?? ?? ?? ?? ?? ?? ?? ?? 00 00 00 00 00 00 00 00 00 00 00 00 A5 43";

    // Offsets para Aim Legit
    constexpr int64_t LEGIT_READ_OFFSET = 0xE7;
    constexpr int64_t LEGIT_WRITE_OFFSET = 0XA7;

    // Offsets para Aim Neck
    constexpr int64_t HEAD_OFFSET = 0xA9;
    constexpr int64_t CHEST_OFFSET = 0xA5;
}

// Resultado da operação de Aim
struct AimResult {
    bool success = false;
    int foundCount = 0;
    double elapsedSeconds = 0.0;
    std::string message;
};

// ==================== FUNÇÕES PRINCIPAIS ====================

// Executa Aim Legit - troca valores de leitura/escrita para aim suave
AimResult ExecuteAimLegit();

// Executa Aim Neck - troca valores de head/chest para mirar no pescoço
AimResult ExecuteAimNeck();

// Estado dos botões
extern bool g_AimLegitActive;
extern bool g_AimNeckActive;

// Estado de processamento (para não bloquear UI)
extern bool g_AimProcessing;
extern std::string g_AimStatusMessage;

// Cache de endereços para reaplicação contínua
extern std::vector<uintptr_t> g_ActiveLegitAddresses;
extern std::vector<uintptr_t> g_ActiveNeckAddresses;
extern bool g_AimLegitRunning;
extern bool g_AimNeckRunning;

// Cache de endereços para ativação instantânea
extern std::vector<uintptr_t> g_CachedLegitAddresses;
extern std::vector<uintptr_t> g_CachedNeckAddresses;
extern bool g_CacheReady;
extern bool g_CacheScanning;

// Funções assíncronas (não bloqueiam a UI)
void ExecuteAimLegitAsync();
void ExecuteAimNeckAsync();

// Funções para reaplicação contínua (chamar periodicamente)
void ApplyLegitSwap(const std::vector<uintptr_t>& addresses);
void ApplyNeckSwap(const std::vector<uintptr_t>& addresses);
void UpdateAimFeatures(); // Chama periodicamente para reaplicar quando checkboxes estão ativos

// Inicia scan em background para cache (chamar quando jogo iniciar)
void StartBackgroundCacheScan();

// Funções instantâneas (usam cache)
AimResult ExecuteAimLegitInstant();
AimResult ExecuteAimNeckInstant();
