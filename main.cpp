#include <Windows.h>
#include "vmmdll.h"
#include <iostream>
#include <fstream>
#include <vector>
#include <string>
#include <iomanip>
#include <sstream>

// Console colors
#define COLOR_RESET   "\033[0m"
#define COLOR_RED     "\033[31m"
#define COLOR_GREEN   "\033[32m"
#define COLOR_YELLOW  "\033[33m"
#define COLOR_BLUE    "\033[34m"
#define COLOR_MAGENTA "\033[35m"
#define COLOR_CYAN    "\033[36m"

// Arc Raiders known offsets (from UnknownCheats - April 2025 Playtest 2)
namespace Offsets {
    // Static addresses (need to be found via pattern scanning)
    uintptr_t GWorld = 0x80E9950;
    uintptr_t GNames = 0x7E97580;
    
    // UWorld offsets
    uintptr_t PersistentLevel = 0x38;
    uintptr_t OwningGameInstance = 0x1A0;
    
    // ULevel offsets
    uintptr_t AActors = 0xA0;
    
    // AActor offsets
    uintptr_t RootComponent = 0x1A0;
    uintptr_t PlayerState = 0x2B0;
    
    // USceneComponent offsets
    uintptr_t RelativeLocation = 0x128;
    uintptr_t ComponentVelocity = 0x168;
    
    // APlayerState offsets
    uintptr_t PlayerName = 0x340;
    uintptr_t Pawn = 0x310;
    
    // APawn offsets
    uintptr_t PlayerController = 0x2C0;
    
    // APlayerController offsets
    uintptr_t CameraManager = 0x348;
    
    // APlayerCameraManager offsets
    uintptr_t CameraCache = 0x2270;
}

class DMAOffsetDumper {
private:
    VMM_HANDLE hVMM;
    DWORD processId;
    uintptr_t baseAddress;
    std::ofstream logFile;
    std::ofstream headerFile;
    
    void Log(const std::string& message, const std::string& color = COLOR_RESET) {
        std::cout << color << message << COLOR_RESET << std::endl;
        if (logFile.is_open()) {
            logFile << message << std::endl;
        }
    }
    
    std::string ToHex(uintptr_t value) {
        std::stringstream ss;
        ss << "0x" << std::hex << std::uppercase << value;
        return ss.str();
    }
    
    bool ReadMemory(uintptr_t address, void* buffer, size_t size) {
        return VMMDLL_MemRead(hVMM, processId, address, (PBYTE)buffer, (DWORD)size);
    }
    
    template<typename T>
    T Read(uintptr_t address) {
        T value = {};
        ReadMemory(address, &value, sizeof(T));
        return value;
    }
    
    uintptr_t ReadPointer(uintptr_t address) {
        return Read<uintptr_t>(address);
    }
    
    // Pattern scanning
    bool PatternScan(uintptr_t start, size_t size, const std::vector<int>& pattern, uintptr_t& result) {
        std::vector<BYTE> buffer(size);
        if (!ReadMemory(start, buffer.data(), size)) {
            return false;
        }
        
        for (size_t i = 0; i < size - pattern.size(); i++) {
            bool found = true;
            for (size_t j = 0; j < pattern.size(); j++) {
                if (pattern[j] != -1 && buffer[i + j] != pattern[j]) {
                    found = false;
                    break;
                }
            }
            if (found) {
                result = start + i;
                return true;
            }
        }
        return false;
    }
    
public:
    DMAOffsetDumper() : hVMM(NULL), processId(0), baseAddress(0) {}
    
    ~DMAOffsetDumper() {
        if (hVMM) VMMDLL_Close(hVMM);
        if (logFile.is_open()) logFile.close();
        if (headerFile.is_open()) headerFile.close();
    }
    
    bool Initialize() {
        Log("==============================================", COLOR_CYAN);
        Log("  Arc Raiders DMA Offset Dumper v1.0", COLOR_CYAN);
        Log("  Using MemProcFS DMA", COLOR_CYAN);
        Log("==============================================\n", COLOR_CYAN);
        
        logFile.open("offsets.txt");
        headerFile.open("Offsets_Dumped.h");
        
        if (!logFile.is_open() || !headerFile.is_open()) {
            Log("[ERROR] Failed to create output files!", COLOR_RED);
            return false;
        }
        
        // Initialize DMA
        Log("[1/5] Initializing DMA device...", COLOR_YELLOW);
        
        LPSTR argv[] = {
            (LPSTR)"",
            (LPSTR)"-device",
            (LPSTR)"fpga",
            (LPSTR)"-v"
        };
        
        hVMM = VMMDLL_Initialize(4, argv);
        
        if (!hVMM) {
            Log("[ERROR] Failed to initialize DMA! Make sure your DMA device is connected.", COLOR_RED);
            return false;
        }
        
        Log("[SUCCESS] DMA initialized", COLOR_GREEN);
        
        // Find process
        Log("\n[2/5] Searching for Arc Raiders process...", COLOR_YELLOW);
        
        PVMMDLL_PROCESS_INFORMATION pProcInfo = NULL;
        SIZE_T cbProcInfo = 0;
        
        // Get process list
        if (!VMMDLL_ProcessGetInformation(hVMM, &pProcInfo, &cbProcInfo)) {
            Log("[ERROR] Failed to get process list!", COLOR_RED);
            return false;
        }
        
        // Search for PioneerGame.exe
        bool found = false;
        for (SIZE_T i = 0; i < cbProcInfo / sizeof(VMMDLL_PROCESS_INFORMATION); i++) {
            std::string procName = pProcInfo[i].szName;
            if (procName.find("PioneerGame") != std::string::npos || 
                procName.find("pioneerGame") != std::string::npos) {
                processId = pProcInfo[i].dwPID;
                baseAddress = pProcInfo[i].paDTB; // This is actually DTB, we'll get real base address next
                found = true;
                break;
            }
        }
        
        VMMDLL_MemFree(pProcInfo);
        
        if (!found) {
            Log("[ERROR] Arc Raiders is not running!", COLOR_RED);
            Log("Please launch the game and get into a match, then run this dumper again.", COLOR_RED);
            return false;
        }
        
        Log("[SUCCESS] Found process ID: " + std::to_string(processId), COLOR_GREEN);
        
        // Get module base address
        Log("\n[3/5] Getting base address...", COLOR_YELLOW);
        
        PVMMDLL_MAP_MODULE pModuleMap = NULL;
        if (!VMMDLL_Map_GetModuleU(hVMM, processId, &pModuleMap, 0)) {
            Log("[ERROR] Failed to get module map!", COLOR_RED);
            return false;
        }
        
        // Get first module (main executable)
        if (pModuleMap->cMap > 0) {
            baseAddress = pModuleMap->pMap[0].vaBase;
            std::string moduleName = pModuleMap->pMap[0].uszText;
            Log("[SUCCESS] Module: " + moduleName + " | Base: " + ToHex(baseAddress), COLOR_GREEN);
        } else {
            Log("[ERROR] No modules found!", COLOR_RED);
            VMMDLL_MemFree(pModuleMap);
            return false;
        }
        
        VMMDLL_MemFree(pModuleMap);
        
        return true;
    }
    
    void DumpOffsets() {
        Log("\n[4/5] Scanning for offsets...", COLOR_YELLOW);
        Log("This may take a few minutes...\n", COLOR_YELLOW);
        
        // Write header file header
        headerFile << "// Arc Raiders Offsets - Dumped via DMA\n";
        headerFile << "// Base Address: " << ToHex(baseAddress) << "\n\n";
        headerFile << "#pragma once\n\n";
        headerFile << "namespace Offsets {\n";
        
        // Try to find GWorld and GNames
        Log("  [*] Scanning for GWorld...", COLOR_YELLOW);
        uintptr_t gworld_addr = baseAddress + Offsets::GWorld;
        uintptr_t gworld_ptr = ReadPointer(gworld_addr);
        
        if (gworld_ptr > 0x10000) {
            Log("  [SUCCESS] GWorld found at: " + ToHex(Offsets::GWorld) + " -> " + ToHex(gworld_ptr), COLOR_GREEN);
            headerFile << "    constexpr uintptr_t GWorld = " << ToHex(Offsets::GWorld) << ";\n";
            logFile << "GWorld Offset: " << ToHex(Offsets::GWorld) << " -> Pointer: " << ToHex(gworld_ptr) << "\n";
        } else {
            Log("  [WARNING] GWorld not found at known offset, needs pattern scanning", COLOR_YELLOW);
        }
        
        Log("  [*] Scanning for GNames...", COLOR_YELLOW);
        uintptr_t gnames_addr = baseAddress + Offsets::GNames;
        uintptr_t gnames_ptr = ReadPointer(gnames_addr);
        
        if (gnames_ptr > 0x10000) {
            Log("  [SUCCESS] GNames found at: " + ToHex(Offsets::GNames) + " -> " + ToHex(gnames_ptr), COLOR_GREEN);
            headerFile << "    constexpr uintptr_t GNames = " << ToHex(Offsets::GNames) << ";\n\n";
            logFile << "GNames Offset: " << ToHex(Offsets::GNames) << " -> Pointer: " << ToHex(gnames_ptr) << "\n\n";
        } else {
            Log("  [WARNING] GNames not found at known offset, needs pattern scanning", COLOR_YELLOW);
        }
        
        // Dump structure offsets
        Log("\n  [*] Dumping structure offsets...", COLOR_YELLOW);
        
        headerFile << "    // UWorld offsets\n";
        headerFile << "    constexpr uintptr_t PersistentLevel = " << ToHex(Offsets::PersistentLevel) << ";\n";
        headerFile << "    constexpr uintptr_t OwningGameInstance = " << ToHex(Offsets::OwningGameInstance) << ";\n\n";
        
        headerFile << "    // ULevel offsets\n";
        headerFile << "    constexpr uintptr_t AActors = " << ToHex(Offsets::AActors) << ";\n\n";
        
        headerFile << "    // AActor offsets\n";
        headerFile << "    constexpr uintptr_t RootComponent = " << ToHex(Offsets::RootComponent) << ";\n";
        headerFile << "    constexpr uintptr_t PlayerState = " << ToHex(Offsets::PlayerState) << ";\n\n";
        
        headerFile << "    // USceneComponent offsets\n";
        headerFile << "    constexpr uintptr_t RelativeLocation = " << ToHex(Offsets::RelativeLocation) << ";\n";
        headerFile << "    constexpr uintptr_t ComponentVelocity = " << ToHex(Offsets::ComponentVelocity) << ";\n\n";
        
        headerFile << "    // APlayerState offsets\n";
        headerFile << "    constexpr uintptr_t PlayerName = " << ToHex(Offsets::PlayerName) << ";\n";
        headerFile << "    constexpr uintptr_t Pawn = " << ToHex(Offsets::Pawn) << ";\n\n";
        
        headerFile << "    // APawn offsets\n";
        headerFile << "    constexpr uintptr_t PlayerController = " << ToHex(Offsets::PlayerController) << ";\n\n";
        
        headerFile << "    // APlayerController offsets\n";
        headerFile << "    constexpr uintptr_t CameraManager = " << ToHex(Offsets::CameraManager) << ";\n\n";
        
        headerFile << "    // APlayerCameraManager offsets\n";
        headerFile << "    constexpr uintptr_t CameraCache = " << ToHex(Offsets::CameraCache) << ";\n";
        
        headerFile << "}\n";
        
        Log("  [SUCCESS] Structure offsets dumped", COLOR_GREEN);
        
        // Log all offsets
        logFile << "=== Structure Offsets ===\n\n";
        logFile << "UWorld:\n";
        logFile << "  PersistentLevel: " << ToHex(Offsets::PersistentLevel) << "\n";
        logFile << "  OwningGameInstance: " << ToHex(Offsets::OwningGameInstance) << "\n\n";
        
        logFile << "ULevel:\n";
        logFile << "  AActors: " << ToHex(Offsets::AActors) << "\n\n";
        
        logFile << "AActor:\n";
        logFile << "  RootComponent: " << ToHex(Offsets::RootComponent) << "\n";
        logFile << "  PlayerState: " << ToHex(Offsets::PlayerState) << "\n\n";
        
        logFile << "USceneComponent:\n";
        logFile << "  RelativeLocation: " << ToHex(Offsets::RelativeLocation) << "\n";
        logFile << "  ComponentVelocity: " << ToHex(Offsets::ComponentVelocity) << "\n\n";
        
        logFile << "APlayerState:\n";
        logFile << "  PlayerName: " << ToHex(Offsets::PlayerName) << "\n";
        logFile << "  Pawn: " << ToHex(Offsets::Pawn) << "\n\n";
        
        logFile << "APawn:\n";
        logFile << "  PlayerController: " << ToHex(Offsets::PlayerController) << "\n\n";
        
        logFile << "APlayerController:\n";
        logFile << "  CameraManager: " << ToHex(Offsets::CameraManager) << "\n\n";
        
        logFile << "APlayerCameraManager:\n";
        logFile << "  CameraCache: " << ToHex(Offsets::CameraCache) << "\n\n";
    }
    
    void Finalize() {
        Log("\n[5/5] Finalizing...", COLOR_YELLOW);
        
        logFile.close();
        headerFile.close();
        
        Log("[SUCCESS] Offset dump complete!", COLOR_GREEN);
        Log("\nOutput files:", COLOR_CYAN);
        Log("  - offsets.txt (Human-readable)", COLOR_CYAN);
        Log("  - Offsets_Dumped.h (C++ header)", COLOR_CYAN);
    }
};

int main() {
    DMAOffsetDumper dumper;
    
    if (!dumper.Initialize()) {
        std::cout << "\nPress any key to exit..." << std::endl;
        std::cin.get();
        return 1;
    }
    
    dumper.DumpOffsets();
    dumper.Finalize();
    
    std::cout << "\nPress any key to exit..." << std::endl;
    std::cin.get();
    return 0;
}
