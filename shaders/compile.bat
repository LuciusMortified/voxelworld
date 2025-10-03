@echo off
setlocal enabledelayedexpansion

echo ========================================
echo Vulkan Shader Compiler
echo ========================================

:: Проверяем наличие glslc
where glslc >nul 2>nul
if %errorlevel% neq 0 (
    echo ERROR: glslc compiler not found in PATH
    echo Please install Vulkan SDK and add it to PATH
    pause
    exit /b 1
)

set compiled_count=0
set failed_count=0

:: Компилируем все .vert файлы
echo.
echo Compiling vertex shaders...
for %%f in (*.vert) do (
    echo Compiling %%f...
    glslc "%%f" -o "%%~nf.vert.spv"
    if !errorlevel! equ 0 (
        echo   ✓ %%f compiled successfully
        set /a compiled_count+=1
    ) else (
        echo   ✗ Failed to compile %%f
        set /a failed_count+=1
    )
)

:: Компилируем все .frag файлы
echo.
echo Compiling fragment shaders...
for %%f in (*.frag) do (
    echo Compiling %%f...
    glslc "%%f" -o "%%~nf.frag.spv"
    if !errorlevel! equ 0 (
        echo   ✓ %%f compiled successfully
        set /a compiled_count+=1
    ) else (
        echo   ✗ Failed to compile %%f
        set /a failed_count+=1
    )
)

echo.
echo ========================================
echo Compilation Summary:
echo   Successfully compiled: !compiled_count! shaders
echo   Failed: !failed_count! shaders
echo ========================================

if !failed_count! gtr 0 (
    echo.
    echo WARNING: Some shaders failed to compile!
    pause
    exit /b 1
) else (
    echo All shaders compiled successfully!
    pause
    exit /b 0
)
