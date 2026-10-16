@echo off
rem Builds Lethe C++ modules, then opens the editor. Logic lives in Scripts\OpenEditor.ps1 (cmd cannot parse UTF-8 Korean text reliably).
powershell -NoProfile -ExecutionPolicy Bypass -File "%~dp0Scripts\OpenEditor.ps1"
