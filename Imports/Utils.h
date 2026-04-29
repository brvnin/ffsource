#pragma once

#include <windows.h>
#include <shlwapi.h>
#include <cstdint>
#include <string>
#include <vector>
#include <functional>
#include <regex>
#include <sstream>
#include <iostream>
#include <process.h>
#include "../Cfg/strenc.h"
#include "../Cfg/encrypt.hh"
#include "../Cfg/minhook/MinHook.h"

// Declarações extern (definições estão em Utils.cpp)
extern uintptr_t il2cpp;
extern uintptr_t unity;
extern int SWidth;
extern int SHeight;
extern HWND JanelaAlvo;

void GravarRegistroInt(const char* chave, int valor);
void LerRegistroInt(const char* chave, int& valor);
HWND LookupWindowByClassName(const char* toFindClassName);

std::string ObterLocalDeInstalacao(DWORD pid);
void ShellQuiet(const char* cmd);
std::string Shell$$Return(const char* cmd);
void ShellQuiet$$Adb(const char* cmd);
std::string ShellReturn$$Adb(const char* cmd);
void ShellReturn$$HDPlayer(const char* cmd);
std::string DetectPortFromNetstat();

enum class ABIType {
    Unknown,
    ARM32,
    ARM64,
    X86,
    X86_64
};

extern ABIType VMM_ABI;

const char* ABIToString(ABIType abi);
ABIType DetectABIFromAndroid();

bool ConnectEmulator();

uintptr_t ObterEnderecoDaBiblioteca(const char* libName);

struct VMMStruct {
    void* pVM;
    uint64_t GuestCR3;
};
extern VMMStruct VMM;

extern void* (*VMMGetCpuById)(void* pVM, int idCpu);
extern int (*PGMPhysRead)(void* pVM, uintptr_t GCPhys, void* pvBuf, size_t bufSize);
extern int (*PGMPhysWrite)(void* pVM, uintptr_t GCPhys, void* pvBuf, size_t bufSize);
extern uint64_t(*CPUMGetGuestCR3)(void* pVCpu);

extern int (*PGMPhysRead_Orig)(void* pVM, uintptr_t GCPhys, void* pvBuf, size_t bufSize);
int PGMPhysReadHook(void* pVM, uintptr_t GCPhys, void* pvBuf, size_t bufSize);
uintptr_t EnderecoVirtualParaFisico64(uint64_t guestCR3, uint64_t virtualAddr);
uintptr_t EnderecoVirtualParaFisico32(uint64_t guestCR3, uint32_t virtualAddr);
uintptr_t TranslateVirtualToPhysical(uintptr_t va, uintptr_t cr3, uintptr_t& pa);

template<typename T>
T Ler(uintptr_t virtualAddress) {
    T var{};
    void* pVM = VMM.pVM;
    if (pVM != nullptr && VMM.GuestCR3 != 0) {
        uintptr_t physAddr = 0;
        if (TranslateVirtualToPhysical(virtualAddress, VMM.GuestCR3, physAddr) != 0) {
            if (PGMPhysRead(pVM, physAddr, &var, sizeof(T)) == 0) {
                return var;
            }
        }
    }
    return T();
}

// Guest 32-bit: ponteiros têm 4 bytes. Em build 64-bit, Ler<uintptr_t> leria 8 e corromperia.
template<>
inline uintptr_t Ler<uintptr_t>(uintptr_t virtualAddress) {
    return (uintptr_t)Ler<uint32_t>(virtualAddress);
}

template<typename T>
void Escrever(uintptr_t virtualAddress, T value) {
    void* pVM = VMM.pVM;
    if (pVM != nullptr && VMM.GuestCR3 != 0) {
        uintptr_t physAddr = 0;
        if (TranslateVirtualToPhysical(virtualAddress, VMM.GuestCR3, physAddr) != 0) {
            PGMPhysWrite(pVM, physAddr, &value, sizeof(T));
        }
    }
}
// ChromeX
void LoadLibraryAndHook();
