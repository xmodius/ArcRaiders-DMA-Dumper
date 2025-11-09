# Arc Raiders DMA Offset Dumper

A DMA-based offset dumper for Arc Raiders that uses MemProcFS to bypass Easy Anti-Cheat.

## Features

✅ **DMA-Based**: Uses your DMA hardware (ScreamerM2) to read memory  
✅ **EAC Bypass**: Completely bypasses Easy Anti-Cheat protection  
✅ **Automatic**: Finds process and module base address automatically  
✅ **Clean Output**: Generates both human-readable and C++ header files  

---

## Requirements

- **DMA Hardware**: ScreamerM2 or compatible FPGA DMA device
- **MemProcFS**: vmm.dll and leechcore.dll (included in your DMA test folder)
- **Visual Studio 2022**: For compilation
- **Arc Raiders**: Game must be running and in a match

---

## Setup Instructions

### 1. Compile the Dumper

Open **Developer Command Prompt for VS 2022** and run:

```cmd
cd path\to\arc_dma_dumper
compile.bat
```

### 2. Copy Required DLLs

Copy these files from your DMA speed test folder to the dumper directory:

```
C:\Users\enfui\Desktop\lones-DMA-speed-test-main\lones-DMA-speed-test-main\
├── vmm.dll
├── leechcore.dll
├── FTD3XX.dll
├── dbghelp.dll
├── symsrv.dll
└── vcruntime140.dll
```

**Quick command:**
```cmd
copy "C:\Users\enfui\Desktop\lones-DMA-speed-test-main\lones-DMA-speed-test-main\*.dll" .
```

### 3. Run the Dumper

1. **Launch Arc Raiders** and **get into a match** (not just menu)
2. Run the dumper:

```cmd
ArcRaiders_DMA_Dumper.exe
```

---

## Output Files

After successful execution, you'll get:

1. **offsets.txt** - Human-readable offset dump with explanations
2. **Offsets_Dumped.h** - C++ header file ready to integrate into your DMA project

---

## How It Works

1. **Initializes DMA** using MemProcFS with FPGA device
2. **Finds PioneerGame.exe** process via DMA process enumeration
3. **Gets module base address** using DMA module mapping
4. **Reads memory** directly through DMA hardware (bypasses EAC)
5. **Scans for offsets** including GWorld, GNames, and structure offsets
6. **Generates output files** for easy integration

---

## Troubleshooting

### "Failed to initialize DMA"
- Make sure your DMA device is connected
- Check that the DMA drivers are installed
- Verify your DMA test works first

### "Arc Raiders is not running"
- Launch the game
- Get into an actual match (not just menu)
- Make sure the process name is PioneerGame.exe

### "Failed to get module map"
- The game might not be fully loaded yet
- Try running the dumper after you're in a match for 10-15 seconds

---

## Next Steps

Once you have the offsets:

1. Integrate `Offsets_Dumped.h` into your main ArcRaiders DMA project
2. Implement GWorld/GNames decryption (Theia obfuscator)
3. Build the full DMA cheat with ESP, aimbot, etc.
4. Test on your alt account

---

## Credits

- **MemProcFS**: Ulf Frisk (https://github.com/ufrisk/MemProcFS)
- **Offset Research**: UnknownCheats community
- **DMA Hardware**: ScreamerM2

---

## Disclaimer

This tool is for educational and research purposes only. Use at your own risk.
