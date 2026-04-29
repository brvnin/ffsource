#include "Utils.h"
#include "Offsets.h"
#include <cstring>

uintptr_t il2cpp = 0x0;
uintptr_t unity = 0x0;
int SWidth = 0;
int SHeight = 0;
HWND JanelaAlvo = NULL;
ABIType VMM_ABI = ABIType::Unknown;

VMMStruct VMM = { NULL, 0 };

void* (*VMMGetCpuById)(void* pVM, int idCpu) = nullptr;
int (*PGMPhysRead)(void* pVM, uintptr_t GCPhys, void* pvBuf, size_t bufSize) = nullptr;
int (*PGMPhysWrite)(void* pVM, uintptr_t GCPhys, void* pvBuf, size_t bufSize) = nullptr;
uint64_t(*CPUMGetGuestCR3)(void* pVCpu) = nullptr;
int (*PGMPhysRead_Orig)(void* pVM, uintptr_t GCPhys, void* pvBuf, size_t bufSize) = nullptr;

void GravarRegistroInt(const char* chave, int valor) {
    HKEY hKey;
    if (RegCreateKeyExA(HKEY_CURRENT_USER, AY_OBFUSCATE("Software\\Safety"), 0, NULL, 0, KEY_WRITE, NULL, &hKey, NULL) == ERROR_SUCCESS) {
        RegSetValueExA(hKey, chave, 0, REG_DWORD, (const BYTE*)&valor, sizeof(valor));
        RegCloseKey(hKey);
    }
}

void LerRegistroInt(const char* chave, int& valor) {
    HKEY hKey;
    DWORD tamanho = sizeof(valor);
    if (RegOpenKeyExA(HKEY_CURRENT_USER, AY_OBFUSCATE("Software\\Safety"), 0, KEY_READ, &hKey) == ERROR_SUCCESS) {
        RegQueryValueExA(hKey, chave, NULL, NULL, (BYTE*)&valor, &tamanho);
        RegCloseKey(hKey);
    }
}

HWND LookupWindowByClassName(const char* toFindClassName) {
    HWND result = NULL;
    auto enumCallback = [&](HWND currWnd) {
        char currClassName[260];
        if (RealGetWindowClassA(currWnd, currClassName, sizeof(currClassName)) < 1)
            return true;
        if (strcmp(currClassName, toFindClassName) == 0) {
            result = currWnd;
            return false;
        }
        return true;
    };
    std::function<bool(HWND)> callbackWrapper = enumCallback;
    EnumWindows([](HWND hwnd, LPARAM lParam) -> BOOL {
        auto callback = reinterpret_cast<std::function<bool(HWND)>*>(lParam);
        if ((*callback)(hwnd) == false)
            return FALSE;
        EnumChildWindows(hwnd, [](HWND childHwnd, LPARAM childLParam) -> BOOL {
            auto childCallback = reinterpret_cast<std::function<bool(HWND)>*>(childLParam);
            return (*childCallback)(childHwnd);
        }, lParam);
        return TRUE;
    }, reinterpret_cast<LPARAM>(&callbackWrapper));
    return result;
}

std::string ObterLocalDeInstalacao(DWORD pid) {
    HANDLE hProcess = OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, FALSE, pid);
    if (hProcess != NULL) {
        char buffer[MAX_PATH];
        DWORD dwSize = sizeof(buffer);
        if (QueryFullProcessImageNameA(hProcess, 0, buffer, &dwSize)) {
            PathRemoveFileSpec(buffer);
            CloseHandle(hProcess);
            return buffer;
        }
        CloseHandle(hProcess);
    }
    return "";
}

void ShellQuiet(const char* cmd) {
    STARTUPINFO si;
    PROCESS_INFORMATION pi;
    ZeroMemory(&si, sizeof(si));
    si.cb = sizeof(si);
    ZeroMemory(&pi, sizeof(pi));
    if (CreateProcess(NULL, const_cast<char*>(cmd), NULL, NULL, FALSE, CREATE_NO_WINDOW, NULL, NULL, &si, &pi)) {
        WaitForSingleObject(pi.hProcess, INFINITE);
        CloseHandle(pi.hProcess);
        CloseHandle(pi.hThread);
    }
}

std::string Shell$$Return(const char* cmd) {
    HANDLE hReadPipe, hWritePipe;
    SECURITY_ATTRIBUTES saAttr{ sizeof(SECURITY_ATTRIBUTES), NULL, TRUE };
    if (!CreatePipe(&hReadPipe, &hWritePipe, &saAttr, 0))
        return "";
    STARTUPINFOA si{};
    si.cb = sizeof(si);
    si.hStdError = hWritePipe;
    si.hStdOutput = hWritePipe;
    si.dwFlags |= STARTF_USESTDHANDLES | STARTF_USESHOWWINDOW;
    si.wShowWindow = SW_HIDE;
    PROCESS_INFORMATION pi{};
    std::string command = std::string("cmd.exe /C ") + cmd;
    BOOL success = CreateProcessA(NULL, const_cast<char*>(command.c_str()), NULL, NULL, TRUE, CREATE_NO_WINDOW, NULL, NULL, &si, &pi);
    CloseHandle(hWritePipe);
    std::string result;
    if (success) {
        char buffer[4096];
        DWORD bytesRead;
        while (ReadFile(hReadPipe, buffer, sizeof(buffer), &bytesRead, NULL) && bytesRead > 0)
            result.append(buffer, bytesRead);
        WaitForSingleObject(pi.hProcess, INFINITE);
        CloseHandle(pi.hProcess);
        CloseHandle(pi.hThread);
    }
    CloseHandle(hReadPipe);
    return result;
}

void ShellQuiet$$Adb(const char* cmd) {
    std::string AdbPath = ObterLocalDeInstalacao(_getpid());
    AdbPath += AY_OBFUSCATE("\\HD-Adb ");
    AdbPath += cmd;
    ShellQuiet(AdbPath.c_str());
}

std::string ShellReturn$$Adb(const char* cmd) {
    std::string AdbPath = ObterLocalDeInstalacao(_getpid());
    AdbPath = "\"" + AdbPath + "\\HD-Adb\" ";
    AdbPath += cmd;
    return Shell$$Return(AdbPath.c_str());
}

void ShellReturn$$HDPlayer(const char* cmd) {
    std::string AdbPath = ObterLocalDeInstalacao(_getpid());
    AdbPath += AY_OBFUSCATE("\\HD-Player ");
    AdbPath += cmd;
    ShellQuiet(AdbPath.c_str());
}

std::string DetectPortFromNetstat() {
    DWORD pid = _getpid();
    std::string cmd = "cmd /c netstat -ano | findstr LISTENING";
    std::string output = Shell$$Return(cmd.c_str());
    if (output.empty()) {
        return "";
    }
    std::istringstream stream(output);
    std::string line;
    while (std::getline(stream, line)) {
        line = std::regex_replace(line, std::regex("^\\s+"), "");
        std::regex ws_re("\\s+");
        std::vector<std::string> tokens(
            std::sregex_token_iterator(line.begin(), line.end(), ws_re, -1),
            std::sregex_token_iterator()
        );
        if (tokens.size() < 5) continue;
        std::string localAddr = tokens[1];
        std::string pidStr = tokens[4];
        if (pidStr != std::to_string(pid)) continue;
        return localAddr;
    }
    return "";
}

const char* ABIToString(ABIType abi) {
    switch (abi) {
    case ABIType::ARM32: return "ARM32";
    case ABIType::ARM64: return "ARM64";
    case ABIType::X86: return "X86";
    case ABIType::X86_64: return "X86_64";
    default: return "Unknown";
    }
}

ABIType DetectABIFromAndroid() {
    std::string porta = DetectPortFromNetstat();
    if (porta.empty()) return ABIType::Unknown;
    std::string abiCmd = "-s " + porta + " shell getprop ro.product.cpu.abi";
    std::string abi = ShellReturn$$Adb(abiCmd.c_str());
    abi.erase(abi.find_last_not_of(" \n\r\t") + 1);
    if (abi == "x86_64") {
        return ABIType::X86_64;
    }
    if (abi == "arm64-v8a") {
        return ABIType::ARM64;
    }
    if (abi == "x86") {
        return ABIType::X86;
    }
    if (abi == "armeabi-v7a") {
        return ABIType::ARM32;
    }
    return ABIType::Unknown;
}

bool ConnectEmulator() {
    static bool Conectado = false;
    if (!Conectado) {
        std::string porta = DetectPortFromNetstat();
        if (porta.empty()) {
            return false;
        }
        std::string conn_adb = "connect " + porta;
        ShellQuiet$$Adb(conn_adb.c_str());
        VMM_ABI = DetectABIFromAndroid();
        Conectado = true;
    }
    return Conectado;
}

uintptr_t ObterEnderecoDaBiblioteca(const char* libName) {
    std::string porta = DetectPortFromNetstat();
    if (porta.empty()) {
        return 0;
    }
    std::string pidCmd = "-s " + porta + " shell pidof com.dts.freefireth";
    std::cout << "[ObterEnderecoDaBiblioteca] Buscando PID com comando: " << pidCmd << std::endl;
    std::string pid = ShellReturn$$Adb(pidCmd.c_str());
    pid.erase(pid.find_last_not_of(" \n\r\t") + 1);
    if (pid.empty()) {
        return 0;
    }
    char command[512];
    sprintf_s(command, "-s %s shell /boot/android/android/system/xbin/bstk/su 0 busybox cat /proc/%s/maps", porta.c_str(), pid.c_str());
    std::string output = ShellReturn$$Adb(command);
    if (output.empty()) {
        return 0;
    }
    std::istringstream stream(output);
    std::string line;
    while (std::getline(stream, line)) {
        if (line.find(libName) != std::string::npos) {
            std::string baseAddress = line.substr(0, line.find('-'));
            uintptr_t address = std::stoull(baseAddress, nullptr, 16);
            return address;
        }
    }
    return 0;
}

int PGMPhysReadHook(void* pVM, uintptr_t GCPhys, void* pvBuf, size_t bufSize) {
    VMM.pVM = pVM;
    return PGMPhysRead_Orig(pVM, GCPhys, pvBuf, bufSize);
}

uintptr_t EnderecoVirtualParaFisico64(uint64_t guestCR3, uint64_t virtualAddr) {
    const uint64_t P_BIT = 1ULL << 0;
    const uint64_t PS_BIT = 1ULL << 7;
    uint64_t ChecarCanonical = (virtualAddr >> 47);
    if (ChecarCanonical != 0 && ChecarCanonical != 0x1FFFFULL)
        return 0;
    uint64_t pml4Base = guestCR3 & 0x000FFFFFFFFFF000ULL;
    uint64_t pml4Index = (virtualAddr >> 39) & 0x1FFULL;
    uint64_t pml4Entry = 0;
    if (PGMPhysRead(VMM.pVM, pml4Base + (pml4Index * 8), &pml4Entry, sizeof(uint64_t)) != 0 || !(pml4Entry & P_BIT))
        return 0;
    uint64_t pdptBase = pml4Entry & 0x000FFFFFFFFFF000ULL;
    uint64_t pdptIndex = (virtualAddr >> 30) & 0x1FFULL;
    uint64_t pdptEntry = 0;
    if (PGMPhysRead(VMM.pVM, pdptBase + (pdptIndex * 8), &pdptEntry, sizeof(uint64_t)) != 0 || !(pdptEntry & P_BIT))
        return 0;
    if (pdptEntry & PS_BIT) {
        uint64_t base1G = pdptEntry & 0x000FFFFFC0000000ULL;
        uint64_t offs1G = virtualAddr & 0x3FFFFFFFULL;
        return base1G | offs1G;
    }
    uint64_t pdBase = pdptEntry & 0x000FFFFFFFFFF000ULL;
    uint64_t pdIndex = (virtualAddr >> 21) & 0x1FFULL;
    uint64_t pdEntry = 0;
    if (PGMPhysRead(VMM.pVM, pdBase + (pdIndex * 8), &pdEntry, sizeof(uint64_t)) != 0 || !(pdEntry & P_BIT))
        return 0;
    if (pdEntry & PS_BIT) {
        uint64_t base2M = pdEntry & 0x000FFFFFFFFFE000ULL;
        uint64_t offs2M = virtualAddr & 0x1FFFFFULL;
        return base2M | offs2M;
    }
    uint64_t ptBase = pdEntry & 0x000FFFFFFFFFF000ULL;
    uint64_t ptIndex = (virtualAddr >> 12) & 0x1FFULL;
    uint64_t ptEntry = 0;
    if (PGMPhysRead(VMM.pVM, ptBase + (ptIndex * 8), &ptEntry, sizeof(uint64_t)) != 0 || !(ptEntry & P_BIT))
        return 0;
    return (ptEntry & 0x000FFFFFFFFFF000ULL) | (virtualAddr & 0xFFFULL);
}

uintptr_t EnderecoVirtualParaFisico32(uint64_t guestCR3, uint32_t virtualAddr) {
    uint32_t pdeBase = (uint32_t)(guestCR3 & 0xFFFFF000);
    uint32_t pdeIndex = (virtualAddr >> 22) & 0x3FF;
    uint32_t pdeEntry = 0;
    if (PGMPhysRead(VMM.pVM, pdeBase + (pdeIndex * 4), &pdeEntry, sizeof(uint32_t)) == 0 && (pdeEntry & 0x1)) {
        uint32_t ptBase = pdeEntry & 0xFFFFF000;
        uint32_t ptIndex = (virtualAddr >> 12) & 0x3FF;
        uint32_t ptEntry = 0;
        if (PGMPhysRead(VMM.pVM, ptBase + (ptIndex * 4), &ptEntry, sizeof(uint32_t)) == 0 && (ptEntry & 0x1))
            return (ptEntry & 0xFFFFF000) | (virtualAddr & 0xFFF);
    }
    return 0;
}

uintptr_t TranslateVirtualToPhysical(uintptr_t va, uintptr_t cr3, uintptr_t& pa) {
    switch (VMM_ABI) {
    case ABIType::X86_64:
    case ABIType::ARM64:
        pa = EnderecoVirtualParaFisico64(cr3, va);
        return pa;
    case ABIType::X86:
    case ABIType::ARM32:
        pa = EnderecoVirtualParaFisico32(cr3, static_cast<uint32_t>(va));
        return pa;
    default:
        pa = EnderecoVirtualParaFisico64(cr3, va);
        if (pa == 0)
            pa = EnderecoVirtualParaFisico32(cr3, static_cast<uint32_t>(va & 0xFFFFFFFF));
        return pa;
    }
}

void LoadLibraryAndHook() {
    HMODULE BstkVMM = GetModuleHandleA(AY_OBFUSCATE("BstkVMM.dll"));
    if (BstkVMM != 0) {
        VMMGetCpuById = (void* (*)(void*, int))GetProcAddress(BstkVMM, static_cast<LPCSTR>(AY_OBFUSCATE("VMMGetCpuById")));
        PGMPhysRead = (int (*)(void*, uintptr_t, void*, size_t))GetProcAddress(BstkVMM, static_cast<LPCSTR>(AY_OBFUSCATE("PGMPhysRead")));
        PGMPhysWrite = (int (*)(void*, uintptr_t, void*, size_t))GetProcAddress(BstkVMM, static_cast<LPCSTR>(AY_OBFUSCATE("PGMPhysWrite")));
        CPUMGetGuestCR3 = (uint64_t(*)(void*))GetProcAddress(BstkVMM, static_cast<LPCSTR>(AY_OBFUSCATE("CPUMGetGuestCR3")));
        MH_Initialize();
        MH_CreateHook(PGMPhysRead, PGMPhysReadHook, (LPVOID*)&PGMPhysRead_Orig);
        MH_EnableHook(PGMPhysRead);
        const DWORD timeoutMs = 60000;
        const DWORD sleepPerAttempt = 100;
        DWORD start = GetTickCount();
        while (VMM.GuestCR3 == 0) {
            if ((GetTickCount() - start) >= timeoutMs)
                break;
            if (il2cpp != 0 && VMM.pVM != nullptr) {
                void* cpuAddr = VMMGetCpuById(VMM.pVM, 0);
                if (cpuAddr != nullptr) {
                    uintptr_t GuestCR3 = CPUMGetGuestCR3(cpuAddr);
                    uintptr_t physAddr = 0;
                    if (TranslateVirtualToPhysical(il2cpp, GuestCR3, physAddr) != 0) {
                        int ELFReader = 0;
                        if (PGMPhysRead(VMM.pVM, physAddr, &ELFReader, sizeof(ELFReader)) == 0) {
                            if (ELFReader == (int)string2Offset(AY_OBFUSCATE("0x464C457F"))) {
                                VMM.GuestCR3 = GuestCR3;
                            }
                        }
                    }
                }
            }
            Sleep(sleepPerAttempt);
        }
    }
}
