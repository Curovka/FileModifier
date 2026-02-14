@echo off
chcp 65001 >nul
echo ===================================================
echo    Компиляция
echo ===================================================

set QT_PATH=C:\Qt\5.15.2\mingw81_64
set MINGW_PATH=C:\qt\5.15.2\mingw81_64\tools\mingw64\bin

echo.
echo Очистка предыдущей сборки...
if exist release rmdir /s /q release
if exist debug rmdir /s /q debug
if exist Makefile del /q /f Makefile
if exist Makefile.* del /q /f Makefile.*

echo Генерация Makefile...
"%QT_PATH%\bin\qmake.exe" -makefile "CONFIG+=static" FileModifier.pro

echo Компиляция проекта...
mingw32-make -f Makefile.Release

if errorlevel 1 (
    echo.
    echo ===================================================
    echo     Ошибка компиляции
    echo ===================================================
    pause
    exit /b 1
)

echo.
echo Копирование необходимых DLL...
copy "%MINGW_PATH%\libgcc_s_seh-1.dll" release\ 2>nul
copy "%MINGW_PATH%\libstdc++-6.dll" release\ 2>nul
copy "%MINGW_PATH%\libwinpthread-1.dll" release\ 2>nul
copy "%QT_PATH%\bin\libEGL.dll" release\ 2>nul
copy "%QT_PATH%\bin\libGLESv2.dll" release\ 2>nul
copy "%QT_PATH%\bin\Qt5Core.dll" release\ 2>nul
copy "%QT_PATH%\bin\Qt5Widgets.dll" release\ 2>nul
copy "%QT_PATH%\bin\Qt5Gui.dll" release\ 2>nul
copy "%QT_PATH%\bin\Qt5Qml.dll" release\ 2>nul
copy "%QT_PATH%\bin\Qt5Quick.dll" release\ 2>nul
copy "%QT_PATH%\bin\Qt5QuickWidgets.dll" release\ 2>nul
copy "%QT_PATH%\bin\Qt5QmlModels.dll" release\ 2>nul

echo.
echo Определение размера файла...
for %%F in ("release\BankSystem.exe") do set filesize=%%~zF

echo.
echo ===================================================
echo        Сборка успешно завершена!
echo ===================================================
echo Исполняемый файл: release\FileModifier.exe
echo Размер: %filesize% байт (%filesize:~0,-6% MB)
echo.

pause
exit /b 0