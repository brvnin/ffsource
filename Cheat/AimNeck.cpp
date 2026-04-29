#include "AimNeck.h"
#include <chrono>
#include <sstream>
#include <iomanip>
#include <algorithm>
#include <cctype>
#include <thread>
#include <atomic>

// Instância global
EmulatorMemory g_EmulatorMem;

// ==================== FUNÇÕES AUXILIARES SEH (sem objetos C++) ====================

// Leitura segura de memória (SEH) - retorna true se sucesso
static bool SafeMemCopyRead(void* dest, const void* src, size_t size) {
    __try {
        memcpy(dest, src, size);
        return true;
    }
    __except (EXCEPTION_EXECUTE_HANDLER) {
        return false;
    }
}

// Escrita segura de memória (SEH) - retorna true se sucesso
static bool SafeMemCopyWrite(void* dest, const void* src, size_t size) {
    __try {
        memcpy(dest, src, size);
        return true;
    }
    __except (EXCEPTION_EXECUTE_HANDLER) {
        return false;
    }
}

// Estados dos botões
bool g_AimLegitActive = false;
bool g_AimNeckActive = false;

// Estado de processamento
bool g_AimProcessing = false;
std::string g_AimStatusMessage = "";

// Cache de endereços para ativação instantânea
std::vector<uintptr_t> g_CachedLegitAddresses;
std::vector<uintptr_t> g_CachedNeckAddresses;
bool g_CacheReady = false;
bool g_CacheScanning = false;

// Endereços ativos para reaplicação contínua
std::vector<uintptr_t> g_ActiveLegitAddresses;
std::vector<uintptr_t> g_ActiveNeckAddresses;
bool g_AimLegitRunning = false;
bool g_AimNeckRunning = false;

// ==================== IMPLEMENTAÇÃO EmulatorMemory ====================

bool EmulatorMemory::OpenEmulatorProcess() {
    // Verifica se estamos rodando dentro do HD-Player
    DWORD currentPid = GetCurrentProcessId();
    wchar_t currentProcessName[MAX_PATH] = { 0 };
    GetModuleFileNameW(NULL, currentProcessName, MAX_PATH);

    // Verifica se o nome do processo atual é HD-Player
    if (wcsstr(currentProcessName, L"HD-Player") != nullptr) {
        isInternal = true;
        processId = currentPid;
        hProcess = GetCurrentProcess();
        return true;
    }

    isInternal = false;

    // Se já está aberto e o processo ainda existe, reutiliza
    if (hProcess != nullptr && processId != 0) {
        DWORD exitCode = 0;
        if (GetExitCodeProcess(hProcess, &exitCode) && exitCode == STILL_ACTIVE) {
            return true;
        }
        CloseEmulatorProcess();
    }

    // Procura o processo HD-Player
    HANDLE snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (snapshot == INVALID_HANDLE_VALUE) {
        return false;
    }

    PROCESSENTRY32W pe32;
    pe32.dwSize = sizeof(pe32);

    if (Process32FirstW(snapshot, &pe32)) {
        do {
            if (wcscmp(pe32.szExeFile, L"HD-Player.exe") == 0) {
                processId = pe32.th32ProcessID;
                break;
            }
        } while (Process32NextW(snapshot, &pe32));
    }

    CloseHandle(snapshot);

    if (processId == 0) {
        return false;
    }

    // Abre o processo com TODAS as permissões necessárias
    hProcess = OpenProcess(PROCESS_ALL_ACCESS, FALSE, processId);
    if (hProcess == nullptr) {
        hProcess = OpenProcess(PROCESS_VM_READ | PROCESS_VM_WRITE | PROCESS_VM_OPERATION | PROCESS_QUERY_INFORMATION, FALSE, processId);
        if (hProcess == nullptr) {
            processId = 0;
            return false;
        }
    }

    // Verifica se é 64-bit
    BOOL isWow64 = FALSE;
    if (IsWow64Process(hProcess, &isWow64)) {
        is64Bit = !isWow64;
    }

    return true;
}

void EmulatorMemory::CloseEmulatorProcess() {
    if (hProcess != nullptr) {
        CloseHandle(hProcess);
        hProcess = nullptr;
    }
    processId = 0;
}

bool EmulatorMemory::ReadMemory(uintptr_t address, void* buffer, size_t size) {
    if (address == 0 || buffer == nullptr || size == 0) return false;

    // Se estamos rodando dentro do processo, acesso direto à memória
    if (isInternal) {
        return SafeMemCopyRead(buffer, reinterpret_cast<void*>(address), size);
    }

    // Acesso externo via ReadProcessMemory
    if (hProcess == nullptr) return false;
    SIZE_T bytesRead = 0;
    return ReadProcessMemory(hProcess, reinterpret_cast<LPCVOID>(address), buffer, size, &bytesRead) && bytesRead == size;
}

bool EmulatorMemory::WriteMemory(uintptr_t address, const void* buffer, size_t size) {
    if (address == 0 || buffer == nullptr || size == 0) return false;

    // Se estamos rodando dentro do processo, acesso direto à memória
    if (isInternal) {
        DWORD oldProtect = 0;
        // Tenta mudar proteção primeiro
        if (VirtualProtect(reinterpret_cast<LPVOID>(address), size, PAGE_EXECUTE_READWRITE, &oldProtect)) {
            bool success = SafeMemCopyWrite(reinterpret_cast<void*>(address), buffer, size);
            VirtualProtect(reinterpret_cast<LPVOID>(address), size, oldProtect, &oldProtect);
            return success;
        }
        // Tenta sem mudar proteção
        return SafeMemCopyWrite(reinterpret_cast<void*>(address), buffer, size);
    }

    // Acesso externo via WriteProcessMemory (otimizado - fast memory)
    if (hProcess == nullptr) return false;

    SIZE_T bytesWritten = 0;

    // Fast write - tenta escrever diretamente primeiro (mais rápido)
    if (WriteProcessMemory(hProcess, reinterpret_cast<LPVOID>(address), buffer, size, &bytesWritten) && bytesWritten == size) {
        return true;
    }

    // Se falhou, tenta mudar proteção e escrever (mais lento, mas necessário)
    DWORD oldProtect = 0;
    if (VirtualProtectEx(hProcess, reinterpret_cast<LPVOID>(address), size, PAGE_EXECUTE_READWRITE, &oldProtect)) {
        bool success = WriteProcessMemory(hProcess, reinterpret_cast<LPVOID>(address), buffer, size, &bytesWritten) && bytesWritten == size;
        VirtualProtectEx(hProcess, reinterpret_cast<LPVOID>(address), size, oldProtect, &oldProtect);
        return success;
    }

    return false;
}

EmulatorMemory::PatternInfo EmulatorMemory::BuildPattern(const std::string& patternString) {
    PatternInfo info;
    std::istringstream stream(patternString);
    std::string token;

    while (stream >> token) {
        if (token == "??" || token == "?" || token == "!!") {
            // Wildcard - qualquer byte
            info.bytes.push_back(0);
            info.mask.push_back(false);
        }
        else {
            // Byte fixo
            try {
                uint8_t byte = static_cast<uint8_t>(std::stoul(token, nullptr, 16));
                info.bytes.push_back(byte);
                info.mask.push_back(true);
            }
            catch (...) {
                // Token inválido, trata como wildcard
                info.bytes.push_back(0);
                info.mask.push_back(false);
            }
        }
    }

    info.length = info.bytes.size();
    return info;
}

bool EmulatorMemory::MatchesPattern(const uint8_t* buffer, size_t bufferSize, size_t startIndex, const PatternInfo& pattern) {
    if (startIndex + pattern.length > bufferSize) return false;

    for (size_t i = 0; i < pattern.length; i++) {
        if (pattern.mask[i] && buffer[startIndex + i] != pattern.bytes[i]) {
            return false;
        }
    }
    return true;
}

std::vector<uintptr_t> EmulatorMemory::AoBScan(const std::string& pattern, bool readable, bool writable, bool executable) {
    std::vector<uintptr_t> results;

    // Para modo interno, não precisa de hProcess
    if (!isInternal && hProcess == nullptr) return results;

    PatternInfo patternInfo = BuildPattern(pattern);
    if (patternInfo.length == 0) return results;

    // Obtém informações do sistema
    SYSTEM_INFO sysInfo;
    GetSystemInfo(&sysInfo);

    uintptr_t minAddress = reinterpret_cast<uintptr_t>(sysInfo.lpMinimumApplicationAddress);
    uintptr_t maxAddress = reinterpret_cast<uintptr_t>(sysInfo.lpMaximumApplicationAddress);

    uintptr_t currentAddress = minAddress;

    // Primeiro byte fixo do padrão para otimização
    uint8_t firstByte = 0;
    bool hasFirstByte = false;
    if (!patternInfo.bytes.empty() && patternInfo.mask[0]) {
        firstByte = patternInfo.bytes[0];
        hasFirstByte = true;
    }

    // Handle para VirtualQuery - usa processo atual se interno
    HANDLE queryHandle = isInternal ? GetCurrentProcess() : hProcess;

    while (currentAddress < maxAddress) {
        MEMORY_BASIC_INFORMATION memInfo;
        if (VirtualQueryEx(queryHandle, reinterpret_cast<LPCVOID>(currentAddress), &memInfo, sizeof(memInfo)) == 0) {
            break;
        }

        // Verifica se a região é válida para scan
        bool isCommitted = (memInfo.State == MEM_COMMIT);
        bool hasValidSize = (memInfo.RegionSize > 0 && memInfo.RegionSize < 50000000);
        bool isNotGuard = !(memInfo.Protect & PAGE_GUARD);
        bool isNotNoAccess = !(memInfo.Protect & PAGE_NOACCESS);

        bool isReadable = (memInfo.Protect & PAGE_READONLY) || (memInfo.Protect & PAGE_READWRITE) ||
            (memInfo.Protect & PAGE_EXECUTE_READ) || (memInfo.Protect & PAGE_EXECUTE_READWRITE);
        bool isWritable = (memInfo.Protect & PAGE_READWRITE) || (memInfo.Protect & PAGE_EXECUTE_READWRITE);
        bool isExecutable = (memInfo.Protect & PAGE_EXECUTE) || (memInfo.Protect & PAGE_EXECUTE_READ) ||
            (memInfo.Protect & PAGE_EXECUTE_READWRITE);

        bool protectionOk = false;
        if (readable && isReadable) protectionOk = true;
        if (writable && isWritable) protectionOk = true;
        if (executable && isExecutable) protectionOk = true;

        if (isCommitted && hasValidSize && protectionOk && isNotGuard && isNotNoAccess) {
            std::vector<uint8_t> buffer(memInfo.RegionSize);
            size_t bytesRead = 0;
            bool readSuccess = false;

            if (isInternal) {
                // Acesso direto à memória usando função auxiliar SEH
                readSuccess = SafeMemCopyRead(buffer.data(), memInfo.BaseAddress, memInfo.RegionSize);
                if (readSuccess) {
                    bytesRead = memInfo.RegionSize;
                }
            }
            else {
                // Acesso externo via ReadProcessMemory
                SIZE_T br = 0;
                readSuccess = ReadProcessMemory(queryHandle, memInfo.BaseAddress, buffer.data(), memInfo.RegionSize, &br) && br > 0;
                bytesRead = br;
            }

            if (readSuccess && bytesRead > patternInfo.length) {
                // Busca o padrão no buffer com otimização
                size_t maxIndex = bytesRead - patternInfo.length;
                for (size_t i = 0; i <= maxIndex; i++) {
                    // Otimização: verifica primeiro byte antes de checar todo o padrão
                    if (hasFirstByte && buffer[i] != firstByte) {
                        continue;
                    }

                    if (MatchesPattern(buffer.data(), bytesRead, i, patternInfo)) {
                        uintptr_t foundAddress = reinterpret_cast<uintptr_t>(memInfo.BaseAddress) + i;
                        results.push_back(foundAddress);
                    }
                }
            }
        }

        // Avança para a próxima região
        currentAddress = reinterpret_cast<uintptr_t>(memInfo.BaseAddress) + memInfo.RegionSize;
    }

    return results;
}

// ==================== AoB SCAN FAST (PARALELO) ====================

std::vector<uintptr_t> EmulatorMemory::AoBScanFast(const std::string& pattern, bool readable, bool writable, bool executable) {
    std::vector<uintptr_t> results;
    results.reserve(200);

    if (!isInternal && hProcess == nullptr) return results;

    PatternInfo patternInfo = BuildPattern(pattern);
    if (patternInfo.length == 0) return results;

    HANDLE queryHandle = isInternal ? GetCurrentProcess() : hProcess;

    // Primeiro byte para otimização
    uint8_t firstByte = 0;
    bool hasFirstByte = false;
    if (!patternInfo.bytes.empty() && patternInfo.mask[0]) {
        firstByte = patternInfo.bytes[0];
        hasFirstByte = true;
    }

    // Range de memória otimizado mas não muito restritivo
    SYSTEM_INFO sysInfo;
    GetSystemInfo(&sysInfo);
    uintptr_t minAddress = reinterpret_cast<uintptr_t>(sysInfo.lpMinimumApplicationAddress);
    uintptr_t maxAddress = reinterpret_cast<uintptr_t>(sysInfo.lpMaximumApplicationAddress);

    // Otimização: limita apenas para áreas relevantes, mas mantém range amplo
    // Evita memória muito baixa (sistema) mas mantém área de jogos
    if (minAddress < 0x100000) minAddress = 0x100000;
    // Mantém range completo para 64-bit também funcionar

    // Configuração de threads otimizada (reduzido para acelerar)
    const size_t numThreads = std::min<size_t>(std::thread::hardware_concurrency(), 4);
    std::vector<std::thread> threads;
    std::vector<std::vector<uintptr_t>> threadResults(numThreads);
    std::atomic<size_t> totalFound(0);
    std::atomic<bool> limitReached(false);

    // Estrutura de região otimizada
    struct MemRegion {
        uintptr_t base;
        size_t size;
    };

    // Worker otimizado - scan direto
    auto workerFunc = [&](size_t threadId, uintptr_t startAddr, uintptr_t endAddr) {
        std::vector<uintptr_t>& localResults = threadResults[threadId];
        localResults.reserve(12);

        uint8_t* scanBuffer = new uint8_t[1 * 1024 * 1024]; // 1MB por thread (reduzido para acelerar)
        uintptr_t currentAddr = startAddr;

        while (currentAddr < endAddr && !limitReached.load()) {
            MEMORY_BASIC_INFORMATION memInfo;
            if (VirtualQueryEx(queryHandle, reinterpret_cast<LPCVOID>(currentAddr), &memInfo, sizeof(memInfo)) == 0) {
                break;
            }

            // Skip rápido - filtros muito otimizados (pula regiões grandes)
            if (memInfo.State != MEM_COMMIT ||
                memInfo.RegionSize == 0 ||
                memInfo.RegionSize > 5000000 || // Reduzido de 10MB para 5MB
                (memInfo.Protect & (PAGE_GUARD | PAGE_NOACCESS))) {
                currentAddr = reinterpret_cast<uintptr_t>(memInfo.BaseAddress) + memInfo.RegionSize;
                continue;
            }

            bool isReadable = (memInfo.Protect & (PAGE_READONLY | PAGE_READWRITE | PAGE_EXECUTE_READ | PAGE_EXECUTE_READWRITE)) != 0;
            bool isWritable = (memInfo.Protect & (PAGE_READWRITE | PAGE_EXECUTE_READWRITE)) != 0;
            if (!((readable && isReadable) || (writable && isWritable))) {
                currentAddr = reinterpret_cast<uintptr_t>(memInfo.BaseAddress) + memInfo.RegionSize;
                continue;
            }

            // Lê região (máx 1MB - reduzido para acelerar)
            size_t regionSize = static_cast<size_t>(memInfo.RegionSize);
            size_t readSize = std::min<size_t>(regionSize, 1 * 1024 * 1024);
            bool readSuccess = false;

            if (isInternal) {
                readSuccess = SafeMemCopyRead(scanBuffer, memInfo.BaseAddress, readSize);
            }
            else {
                SIZE_T br = 0;
                readSuccess = ReadProcessMemory(queryHandle, memInfo.BaseAddress, scanBuffer, readSize, &br);
                readSize = br;
            }

            if (readSuccess && readSize >= patternInfo.length) {
                uintptr_t baseAddr = reinterpret_cast<uintptr_t>(memInfo.BaseAddress);
                size_t maxIdx = readSize - patternInfo.length;

                // Scan otimizado
                for (size_t i = 0; i <= maxIdx && !limitReached.load(); i++) {
                    if (hasFirstByte && scanBuffer[i] != firstByte) {
                        i += 30; // Skip muito otimizado (aumentado para acelerar muito)
                        continue;
                    }

                    // Match pattern - early exit
                    bool match = true;
                    for (size_t j = 0; j < patternInfo.length; j++) {
                        if (patternInfo.mask[j] && scanBuffer[i + j] != patternInfo.bytes[j]) {
                            match = false;
                            break;
                        }
                    }

                    if (match) {
                        localResults.push_back(baseAddr + i);
                        totalFound.fetch_add(1);
                    }
                }
            }

            currentAddr = reinterpret_cast<uintptr_t>(memInfo.BaseAddress) + memInfo.RegionSize;
        }

        delete[] scanBuffer;
        };

    // Dividir trabalho entre threads
    uintptr_t rangePerThread = (maxAddress - minAddress) / numThreads;
    for (size_t t = 0; t < numThreads; t++) {
        uintptr_t start = minAddress + (rangePerThread * t);
        uintptr_t end = (t == numThreads - 1) ? maxAddress : (minAddress + (rangePerThread * (t + 1)));
        threads.emplace_back(workerFunc, t, start, end);
    }

    // Aguardar threads
    for (auto& t : threads) {
        t.join();
    }

    // Combinar resultados (sem limite - processa todos)
    for (const auto& tr : threadResults) {
        results.insert(results.end(), tr.begin(), tr.end());
    }

    return results;
}

// ==================== FUNÇÕES DE AIM ====================

AimResult ExecuteAimLegit() {
    AimResult result;
    auto startTime = std::chrono::steady_clock::now();

    // Abre o processo do emulador
    if (!g_EmulatorMem.OpenEmulatorProcess()) {
        result.success = false;
        result.message = "HD-Player nao encontrado! Abra o emulador.";
        return result;
    }

    // Executa AoB scan FAST (paralelo - muito mais rápido!)
    std::vector<uintptr_t> addresses = g_EmulatorMem.AoBScanFast(AimPatterns::AIM_LEGIT, true, true, false);
    result.foundCount = static_cast<int>(addresses.size());

    if (addresses.empty()) {
        auto endTime = std::chrono::steady_clock::now();
        result.elapsedSeconds = std::chrono::duration<double>(endTime - startTime).count();
        result.success = false;
        std::ostringstream oss;
        oss << "LEGIT FAILED (0 found, " << std::fixed << std::setprecision(2) << result.elapsedSeconds << "s)";
        result.message = oss.str();
        return result;
    }

    int swapCount = 0;

    // Processa cada endereço encontrado
    for (const auto& currentAddress : addresses) {
        uintptr_t headbytes = currentAddress + AimPatterns::LEGIT_READ_OFFSET;
        uintptr_t chestbytes = currentAddress + AimPatterns::LEGIT_WRITE_OFFSET;

        int32_t readValue = g_EmulatorMem.Read<int32_t>(headbytes);
        int32_t writeValue = g_EmulatorMem.Read<int32_t>(chestbytes);

        // Faz o swap sempre que os valores são válidos (permite ativar múltiplas vezes)
        if ((readValue != 0 || writeValue != 0) &&
            g_EmulatorMem.Write<int32_t>(chestbytes, readValue) &&
            g_EmulatorMem.Write<int32_t>(headbytes, writeValue)) {
            swapCount++;
        }
    }

    auto endTime = std::chrono::steady_clock::now();
    result.elapsedSeconds = std::chrono::duration<double>(endTime - startTime).count();

    result.success = true;

    std::ostringstream oss;
    oss << "AIM LEGIT OK (" << result.foundCount << " found, " << swapCount << " swapped, "
        << std::fixed << std::setprecision(2) << result.elapsedSeconds << "s)";
    result.message = oss.str();

    g_AimLegitActive = true;

    return result;
}

AimResult ExecuteAimNeck() {
    AimResult result;
    auto startTime = std::chrono::steady_clock::now();

    // Abre o processo do emulador
    if (!g_EmulatorMem.OpenEmulatorProcess()) {
        result.success = false;
        result.message = "HD-Player nao encontrado! Abra o emulador.";
        return result;
    }

    // Executa AoB scan FAST (paralelo - muito mais rápido!)
    std::vector<uintptr_t> addresses = g_EmulatorMem.AoBScanFast(AimPatterns::AIM_LEGIT, true, true, false);
    result.foundCount = static_cast<int>(addresses.size());

    if (addresses.empty()) {
        auto endTime = std::chrono::steady_clock::now();
        result.elapsedSeconds = std::chrono::duration<double>(endTime - startTime).count();
        result.success = false;
        std::ostringstream oss;
        oss << "NECK FAILED (0 found, " << std::fixed << std::setprecision(2) << result.elapsedSeconds << "s)";
        result.message = oss.str();
        return result;
    }

    int swapCount = 0;

    // Processa cada endereço encontrado
    for (const auto& currentAddress : addresses) {
        uintptr_t headbytes = currentAddress + AimPatterns::LEGIT_WRITE_OFFSET;
        uintptr_t chestbytes = currentAddress + AimPatterns::LEGIT_READ_OFFSET;

        int32_t readValue = g_EmulatorMem.Read<int32_t>(headbytes);
        int32_t writeValue = g_EmulatorMem.Read<int32_t>(chestbytes);

        // Faz o swap sempre que os valores são válidos (permite ativar múltiplas vezes)
        if ((readValue != 0 || writeValue != 0) &&
            g_EmulatorMem.Write<int32_t>(chestbytes, readValue) &&
            g_EmulatorMem.Write<int32_t>(headbytes, writeValue)) {
            swapCount++;
        }
    }

    auto endTime = std::chrono::steady_clock::now();
    result.elapsedSeconds = std::chrono::duration<double>(endTime - startTime).count();

    result.success = true;

    std::ostringstream oss;
    oss << "AIM NECK OK (" << result.foundCount << " found, " << swapCount << " swapped, "
        << std::fixed << std::setprecision(2) << result.elapsedSeconds << "s)";
    result.message = oss.str();

    g_AimNeckActive = true;

    return result;
}

// ==================== FUNÇÕES ASSÍNCRONAS (RÁPIDAS) ====================

void ExecuteAimLegitAsync() {
    if (g_AimProcessing) return;

    g_AimProcessing = true;

    std::thread([]() {
        auto startTime = std::chrono::steady_clock::now();

        if (!g_EmulatorMem.OpenEmulatorProcess()) {
            g_AimStatusMessage = "HD-Player nao encontrado!";
            g_AimProcessing = false;
            return;
        }

        // Scan completo (como IMMORTAL FREE)
        g_AimStatusMessage = "Escaneando...";

        // Usa scan rápido paralelo
        std::vector<uintptr_t> addresses = g_EmulatorMem.AoBScanFast(AimPatterns::AIM_LEGIT, true, true, false);

        // Se encontrou poucos, faz scan completo também
        if (addresses.size() < 5) {
            g_AimStatusMessage = "Scan amplo...";
            std::vector<uintptr_t> fullScan = g_EmulatorMem.AoBScan(AimPatterns::AIM_LEGIT, true, true, false);

            // Combina resultados (remove duplicatas)
            if (!fullScan.empty()) {
                for (const auto& addr : fullScan) {
                    if (std::find(addresses.begin(), addresses.end(), addr) == addresses.end()) {
                        addresses.push_back(addr);
                    }
                }
            }
        }

        // Verificação do IMMORTAL FREE: só funciona se foundCount <= 47
        int foundCount = static_cast<int>(addresses.size());
        if (foundCount > 47) {
            auto endTime = std::chrono::steady_clock::now();
            int ms = static_cast<int>(std::chrono::duration<double, std::milli>(endTime - startTime).count());
            g_AimStatusMessage = "LEGIT FAILED (" + std::to_string(foundCount) + " found, " + std::to_string(ms) + "ms) - Muitos endereços";
            g_AimProcessing = false;
            return;
        }

        if (addresses.empty()) {
            auto endTime = std::chrono::steady_clock::now();
            int ms = static_cast<int>(std::chrono::duration<double, std::milli>(endTime - startTime).count());
            g_AimStatusMessage = "LEGIT FAILED (0 found, " + std::to_string(ms) + "ms) - Jogo em partida?";
            g_AimProcessing = false;
            return;
        }

        int swapCount = 0;
        // Aplica swap nos endereços encontrados (como IMMORTAL FREE - só se foundCount <= 47)
        for (const auto& currentAddress : addresses) {
            // Offsets do IMMORTAL FREE
            uintptr_t headbytes = currentAddress + AimPatterns::LEGIT_READ_OFFSET;  // 0xD6
            uintptr_t chestbytes = currentAddress + AimPatterns::LEGIT_WRITE_OFFSET; // 0xA6

            // Lê valores originais
            int32_t readValue = g_EmulatorMem.Read<int32_t>(headbytes);
            int32_t writeValue = g_EmulatorMem.Read<int32_t>(chestbytes);

            // Faz o swap (escreve head no chest e chest no head)
            if (g_EmulatorMem.Write<int32_t>(chestbytes, readValue) &&
                g_EmulatorMem.Write<int32_t>(headbytes, writeValue)) {
                swapCount++;
            }
        }

        auto endTime = std::chrono::steady_clock::now();
        int ms = static_cast<int>(std::chrono::duration<double, std::milli>(endTime - startTime).count());

        std::ostringstream oss;
        oss << "LEGIT OK (" << addresses.size() << "f, " << swapCount << "s, " << ms << "ms)";
        g_AimStatusMessage = oss.str();
        g_AimLegitActive = true;
        g_AimProcessing = false;
        }).detach();
}

void ExecuteAimNeckAsync() {
    if (g_AimProcessing) return;

    g_AimProcessing = true;

    std::thread([]() {
        auto startTime = std::chrono::steady_clock::now();

        if (!g_EmulatorMem.OpenEmulatorProcess()) {
            g_AimStatusMessage = "HD-Player nao encontrado!";
            g_AimProcessing = false;
            return;
        }

        // Scan completo sem limite (como IMMORTAL FREE - processa TODOS os endereços)
        g_AimStatusMessage = "Escaneando...";

        // Usa scan rápido paralelo (sem limite de resultados)
        std::vector<uintptr_t> addresses = g_EmulatorMem.AoBScanFast(AimPatterns::AIMBOT_SCAN, true, true, false);

        // Se encontrou poucos, faz scan completo também para garantir que encontra todos
        if (addresses.size() < 10) {
            g_AimStatusMessage = "Scan amplo...";
            std::vector<uintptr_t> fullScan = g_EmulatorMem.AoBScan(AimPatterns::AIMBOT_SCAN, true, true, false);

            // Combina resultados (remove duplicatas)
            if (!fullScan.empty()) {
                for (const auto& addr : fullScan) {
                    if (std::find(addresses.begin(), addresses.end(), addr) == addresses.end()) {
                        addresses.push_back(addr);
                    }
                }
            }
        }

        // NECK processa TODOS os endereços encontrados (como IMMORTAL FREE - foundCount != 0)
        int foundCount = static_cast<int>(addresses.size());
        if (foundCount == 0) {
            auto endTime = std::chrono::steady_clock::now();
            int ms = static_cast<int>(std::chrono::duration<double, std::milli>(endTime - startTime).count());
            g_AimStatusMessage = "NECK FAILED (0 found, " + std::to_string(ms) + "ms) - Jogo em partida?";
            g_AimProcessing = false;
            return;
        }

        int swapCount = 0;
        // Aplica swap nos endereços encontrados (como IMMORTAL FREE - processa TODOS)
        for (const auto& currentAddress : addresses) {
            // Offsets do IMMORTAL FREE
            uintptr_t headbytes = currentAddress + AimPatterns::HEAD_OFFSET;  // 0xA9
            uintptr_t chestbytes = currentAddress + AimPatterns::CHEST_OFFSET; // 0xA5

            // Lê valores originais
            int32_t readValue = g_EmulatorMem.Read<int32_t>(headbytes);
            int32_t writeValue = g_EmulatorMem.Read<int32_t>(chestbytes);

            // Faz o swap (escreve head no chest e chest no head)
            if (g_EmulatorMem.Write<int32_t>(chestbytes, readValue) &&
                g_EmulatorMem.Write<int32_t>(headbytes, writeValue)) {
                swapCount++;
            }
        }

        auto endTime = std::chrono::steady_clock::now();
        int ms = static_cast<int>(std::chrono::duration<double, std::milli>(endTime - startTime).count());

        std::ostringstream oss;
        oss << "NECK OK (" << addresses.size() << "f, " << swapCount << "s, " << ms << "ms)";
        g_AimStatusMessage = oss.str();
        g_AimNeckActive = true;
        g_AimProcessing = false;
        }).detach();
}

// ==================== FUNÇÕES INSTANTÂNEAS (COM CACHE) ====================

// Scan em background para popular cache
void StartBackgroundCacheScan() {
    if (g_CacheScanning) return;

    g_CacheScanning = true;

    std::thread([]() {
        if (!g_EmulatorMem.OpenEmulatorProcess()) {
            g_CacheScanning = false;
            return;
        }

        // Escaneia ambos os padrões
        g_CachedLegitAddresses = g_EmulatorMem.AoBScanFast(AimPatterns::AIM_LEGIT, true, true, false);
        g_CachedNeckAddresses = g_EmulatorMem.AoBScanFast(AimPatterns::AIMBOT_SCAN, true, true, false);

        g_CacheReady = !g_CachedLegitAddresses.empty() || !g_CachedNeckAddresses.empty();
        g_CacheScanning = false;
        }).detach();
}

// Aplica swap nos endereços do cache (INSTANTÂNEO!)
AimResult ExecuteAimLegitInstant() {
    AimResult result;
    auto startTime = std::chrono::steady_clock::now();

    // Se cache não está pronto, faz scan rápido
    if (g_CachedLegitAddresses.empty()) {
        if (!g_EmulatorMem.OpenEmulatorProcess()) {
            result.success = false;
            result.message = "HD-Player nao encontrado!";
            return result;
        }
        g_CachedLegitAddresses = g_EmulatorMem.AoBScanFast(AimPatterns::AIM_LEGIT, true, true, false);
    }

    if (g_CachedLegitAddresses.empty()) {
        result.success = false;
        result.message = "LEGIT FAILED (0 cached)";
        return result;
    }

    // Garante que o processo está aberto antes de fazer o swap
    if (!g_EmulatorMem.OpenEmulatorProcess()) {
        result.success = false;
        result.message = "HD-Player nao encontrado!";
        return result;
    }

    int swapCount = 0;

    for (const auto& addr : g_CachedLegitAddresses) {
        uintptr_t headbytes = addr + AimPatterns::LEGIT_READ_OFFSET;
        uintptr_t chestbytes = addr + AimPatterns::LEGIT_WRITE_OFFSET;

        int32_t readValue = g_EmulatorMem.Read<int32_t>(headbytes);
        int32_t writeValue = g_EmulatorMem.Read<int32_t>(chestbytes);

        // Faz o swap sempre que os valores são válidos (permite ativar múltiplas vezes)
        if ((readValue != 0 || writeValue != 0) &&
            g_EmulatorMem.Write<int32_t>(chestbytes, readValue) &&
            g_EmulatorMem.Write<int32_t>(headbytes, writeValue)) {
            swapCount++;
        }
    }

    auto endTime = std::chrono::steady_clock::now();
    result.elapsedSeconds = std::chrono::duration<double>(endTime - startTime).count();
    result.foundCount = static_cast<int>(g_CachedLegitAddresses.size());
    result.success = true;

    std::ostringstream oss;
    oss << "LEGIT OK (" << swapCount << " swapped, "
        << static_cast<int>(result.elapsedSeconds * 1000) << "ms)";
    result.message = oss.str();

    g_AimLegitActive = true;
    return result;
}

AimResult ExecuteAimNeckInstant() {
    AimResult result;
    auto startTime = std::chrono::steady_clock::now();

    // Se cache não está pronto, faz scan rápido
    if (g_CachedNeckAddresses.empty()) {
        if (!g_EmulatorMem.OpenEmulatorProcess()) {
            result.success = false;
            result.message = "HD-Player nao encontrado!";
            return result;
        }
        g_CachedNeckAddresses = g_EmulatorMem.AoBScanFast(AimPatterns::AIMBOT_SCAN, true, true, false);
    }

    if (g_CachedNeckAddresses.empty()) {
        result.success = false;
        result.message = "NECK FAILED (0 cached)";
        return result;
    }

    // Garante que o processo está aberto antes de fazer o swap
    if (!g_EmulatorMem.OpenEmulatorProcess()) {
        result.success = false;
        result.message = "HD-Player nao encontrado!";
        return result;
    }

    int swapCount = 0;

    for (const auto& addr : g_CachedNeckAddresses) {
        uintptr_t headbytes = addr + AimPatterns::HEAD_OFFSET;
        uintptr_t chestbytes = addr + AimPatterns::CHEST_OFFSET;

        int32_t readValue = g_EmulatorMem.Read<int32_t>(headbytes);
        int32_t writeValue = g_EmulatorMem.Read<int32_t>(chestbytes);

        // Faz o swap sempre que os valores são válidos (permite ativar múltiplas vezes)
        if ((readValue != 0 || writeValue != 0) &&
            g_EmulatorMem.Write<int32_t>(chestbytes, readValue) &&
            g_EmulatorMem.Write<int32_t>(headbytes, writeValue)) {
            swapCount++;
        }
    }

    auto endTime = std::chrono::steady_clock::now();
    result.elapsedSeconds = std::chrono::duration<double>(endTime - startTime).count();
    result.foundCount = static_cast<int>(g_CachedNeckAddresses.size());
    result.success = true;

    std::ostringstream oss;
    oss << "NECK OK (" << swapCount << " swapped, "
        << static_cast<int>(result.elapsedSeconds * 1000) << "ms)";
    result.message = oss.str();

    g_AimNeckActive = true;
    return result;
}
