<#
.SYNOPSIS
    Прогоняет test_world_grid по списку сцен и складывает отчёты в каталог.

.DESCRIPTION
    Берёт на себя три вещи, без которых замер молча не состоится: опции с
    "=", рабочий каталог рядом с экзешником (шейдеры ищутся от него) и окно
    на переднем плане (свёрнутое не презентует, и кадры не идут вовсе).

.EXAMPLE
    pwsh .claude/skills/render-bench/run.ps1 -OutDir bench/before
    pwsh .claude/skills/render-bench/run.ps1 -OutDir bench/after -Scenes parked,torches -Frames 600
#>
param(
    [string[]]$Scenes = @('parked', 'spin', 'advance', 'flythrough'),
    [Parameter(Mandatory)][string]$OutDir,
    [int]$Frames = 2000,
    [int]$Warmup = 200,

    # Прочие опции приложения как есть, например '--bench-dynamic=256'.
    [string[]]$Extra = @(),

    [string]$ExeDir = 'build/release/apps/test_world_grid',

    # Первый прогон после сборки даёт мусор -- прогревочный запуск, отчёт
    # которого выбрасывается. Ставить 0 только если сборка уже прогрета.
    [int]$Warmups = 1,

    [int]$TimeoutSeconds = 900
)

$ErrorActionPreference = 'Stop'

$exeDirFull = (Resolve-Path $ExeDir).Path
$exe = Join-Path $exeDirFull 'test_world_grid.exe'
if (-not (Test-Path $exe)) { throw "не найден $exe -- собери таргет test_world_grid" }

New-Item -ItemType Directory -Force -Path $OutDir | Out-Null
$outFull = (Resolve-Path $OutDir).Path

$shell = New-Object -ComObject WScript.Shell

function Invoke-Scene([string]$scene, [string]$report) {
    $args = @("--bench=$scene", "--bench-frames=$Frames", "--bench-warmup=$Warmup")
    if ($report) { $args += "--bench-out=$report" }
    $args += $Extra

    $proc = Start-Process -FilePath $exe -WorkingDirectory $exeDirFull -ArgumentList $args -PassThru
    $deadline = (Get-Date).AddSeconds($TimeoutSeconds)

    while (-not $proc.HasExited -and (Get-Date) -lt $deadline) {
        $null = $shell.AppActivate($proc.Id)
        Start-Sleep -Seconds 5
    }

    if (-not $proc.HasExited) {
        $proc | Stop-Process -Force
        throw "$scene не завершилась за $TimeoutSeconds с. Первый подозреваемый -- опция без '=': тогда бенч не включается и приложение работает как обычное окно."
    }

    return $proc.ExitCode
}

foreach ($scene in $Scenes) {
    for ($i = 0; $i -lt $Warmups; $i++) {
        Write-Host "$scene -- прогрев $($i + 1)/$Warmups (отчёт выбрасывается)"
        $null = Invoke-Scene $scene ''
    }

    $report = Join-Path $outFull "$scene.txt"
    Write-Host "$scene -- замер в $report"

    $code = Invoke-Scene $scene $report
    if ($code -ne 0) { Write-Host "  код возврата $code" -ForegroundColor Yellow }

    $line = Get-Content -LiteralPath $report | Select-String -Pattern '^fps at median frame:'
    if ($line) { Write-Host "  $($line.Line)" }
}

Write-Host "`nотчёты в $outFull"
Write-Host "сравнить: pwsh .claude/skills/render-bench/compare.ps1 -Before <до>/<сцена>.txt -After $OutDir/<сцена>.txt"
