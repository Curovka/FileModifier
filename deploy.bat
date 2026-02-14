@echo off
chcp 65001 >nul
echo ===================================================
echo    Развертывание
echo ===================================================

set QT_PATH=C:\Qt\5.15.2\mingw81_64
set BUILD_PATH=release
set DEPLOY_PATH=FileModifier_Deploy
set EXE_NAME=FileModifier.exe

echo.
echo Создание директории для развертывания...
if exist "%DEPLOY_PATH%" rmdir /s /q "%DEPLOY_PATH%"
mkdir "%DEPLOY_PATH%"

echo Копирование исполняемого файла...
if not exist "%BUILD_PATH%\%EXE_NAME%" (
    echo Ошибка: Исполняемый файл не найден
    pause
    exit /b 1
)
copy "%BUILD_PATH%\%EXE_NAME%" "%DEPLOY_PATH%\" >nul

echo Развертывание Qt библиотек...
"%QT_PATH%\bin\windeployqt.exe" --release --no-compiler-runtime --no-translations "%DEPLOY_PATH%\%EXE_NAME%"

echo Копирование дополнительных DLL компилятора...
copy "%QT_PATH%\bin\libgcc_s_seh-1.dll" "%DEPLOY_PATH%\" >nul
copy "%QT_PATH%\bin\libstdc++-6.dll" "%DEPLOY_PATH%\" >nul
copy "%QT_PATH%\bin\libwinpthread-1.dll" "%DEPLOY_PATH%\" >nul

echo.
echo Проверка наличия файлов...
echo Список файлов в папке %DEPLOY_PATH%:
dir "%DEPLOY_PATH%\*.*"

echo.
echo ===================================================
echo      Развертывание успешно завершено!
echo ===================================================
echo Папка с программой: %DEPLOY_PATH%
echo.

pause
exit /b 0