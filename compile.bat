@echo off
echo ========================================
echo Arc Raiders DMA Dumper - Compile
echo ========================================
echo.

echo [*] Compiling with cl.exe...
echo.

cl.exe /EHsc /std:c++17 /O2 /Fe:ArcRaiders_DMA_Dumper.exe main.cpp Advapi32.lib Shell32.lib

if %ERRORLEVEL% EQU 0 (
    echo.
    echo ========================================
    echo [SUCCESS] Compilation complete!
    echo ========================================
    echo.
    echo Executable: ArcRaiders_DMA_Dumper.exe
    echo.
    echo IMPORTANT: Copy the following files from your DMA test folder:
    echo   - vmm.dll
    echo   - leechcore.dll
    echo   - FTD3XX.dll
    echo   - dbghelp.dll
    echo   - symsrv.dll
    echo   - vcruntime140.dll
    echo.
    echo To run:
    echo 1. Copy the DLLs listed above to this directory
    echo 2. Launch Arc Raiders and get into a match
    echo 3. Run: ArcRaiders_DMA_Dumper.exe
    echo.
) else (
    echo.
    echo ========================================
    echo [ERROR] Compilation failed!
    echo ========================================
    echo.
)

pause
