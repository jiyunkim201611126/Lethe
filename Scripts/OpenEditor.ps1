# Lethe 에디터 실행용: 프로젝트 C++ 모듈을 빌드한 뒤 에디터를 엽니다. 루트의 OpenEditor.bat을 더블클릭해 실행합니다.
# Binaries는 git에 없고 .uproject 더블클릭은 소스가 바뀌어도 옛 DLL로 열리므로, 최신 버전을 받은 뒤에는 이 스크립트로 여세요.
# git pull은 하지 않습니다(GitHub Desktop 등에서 직접 관리).

$UEVersion = '5.7'
$ProjectRoot = Split-Path -Parent $PSScriptRoot
$Project = Join-Path $ProjectRoot 'Lethe.uproject'

function Exit-AfterKeyPress([int]$Code)
{
	Read-Host '엔터를 누르면 창이 닫힙니다' | Out-Null
	exit $Code
}

# 엔진 경로: Epic 런처 설치 목록에서 찾고, 없으면 기본 설치 경로를 사용합니다.
$EngineDir = $null
try
{
	$Manifest = Join-Path $env:ProgramData 'Epic\UnrealEngineLauncher\LauncherInstalled.dat'
	$Install = (Get-Content -Raw -ErrorAction Stop $Manifest | ConvertFrom-Json).InstallationList | Where-Object { $_.AppName -eq "UE_$UEVersion" } | Select-Object -First 1
	if ($Install) { $EngineDir = $Install.InstallLocation }
}
catch {}
if (-not $EngineDir) { $EngineDir = Join-Path $env:ProgramFiles "Epic Games\UE_$UEVersion" }

$BuildBat = Join-Path $EngineDir 'Engine\Build\BatchFiles\Build.bat'
$EditorExe = Join-Path $EngineDir 'Engine\Binaries\Win64\UnrealEditor.exe'
if (-not (Test-Path $BuildBat) -or -not (Test-Path $EditorExe))
{
	Write-Host "[오류] Unreal Engine $UEVersion 설치 경로를 찾지 못했습니다: $EngineDir" -ForegroundColor Red
	Write-Host "Epic Games 런처에서 Unreal Engine $UEVersion 이 설치되어 있는지 확인해 주세요."
	Exit-AfterKeyPress 1
}

# 한글 파일명이 git status에 잡히면 UBT가 결과 없이 죽는 문제 우회(core.quotepath=false를 이 빌드에만 주입)
$env:GIT_CONFIG_COUNT = '1'
$env:GIT_CONFIG_KEY_0 = 'core.quotepath'
$env:GIT_CONFIG_VALUE_0 = 'false'

Write-Host '프로젝트 코드를 빌드합니다. 처음에는 몇 분 걸릴 수 있습니다...' -ForegroundColor Cyan
& $BuildBat LetheEditor Win64 Development "-Project=$Project" -WaitMutex
# UBT가 크래시하면 음수 종료 코드가 나올 수 있어 0과 직접 비교합니다.
if ($LASTEXITCODE -ne 0)
{
	Write-Host ''
	Write-Host '[빌드 실패] 언리얼 에디터가 켜져 있다면 모두 닫고 다시 실행해 주세요.' -ForegroundColor Red
	Write-Host '그래도 실패하면 이 창의 내용을 캡처해서 보내 주세요.'
	Exit-AfterKeyPress 1
}

Write-Host '빌드 완료. 에디터를 엽니다...' -ForegroundColor Green
Start-Process -FilePath $EditorExe -ArgumentList "`"$Project`""
