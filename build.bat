@echo off
echo.
echo === コンパイル中 ===
gcc -finput-charset=utf-8 -fexec-charset=utf-8 src\main.c -o app.exe -Wall
if %ERRORLEVEL% EQU 0 (
    echo.
    echo [成功] app.exe が生成されました
    echo 実行するには以下を入力してください:
    echo   .\app.exe
    echo.
) else (
    echo.
    echo [エラー] コンパイルに失敗しました
    echo 上のエラーメッセージを確認してください
    echo.
)
