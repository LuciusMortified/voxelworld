<#
.SYNOPSIS
    Прогоняет testbed по списку прогонов и складывает отчёты в каталог.

.DESCRIPTION
    Берёт на себя три вещи, без которых замер молча не состоится: ключ
    "--bench", рабочий каталог рядом с экзешником (шейдеры ищутся от него) и
    окно на переднем плане (свёрнутое не презентует, и кадры не идут вовсе).

.EXAMPLE
    pwsh .claude/skills/render-bench/run.ps1 -OutDir bench/before
    pwsh .claude/skills/render-bench/run.ps1 -OutDir bench/after -Runs terrain:parked,standing-lights -Frames 600
#>
param(
    # Прогон -- это сцена и путь камеры через двоеточие. Имя без двоеточия
    # означает путь, который сцена назначила себе сама.
    [string[]]$Runs = @('terrain:parked', 'terrain:spin', 'terrain:walk', 'terrain:orbit'),

    [Parameter(Mandatory)][string]$OutDir,
    [int]$Frames = 2000,
    [int]$Warmup = 200,

    # Прочие опции приложения как есть, например '--moving-lights=256'.
    [string[]]$Extra = @(),

    [string]$ExeDir = 'build/release/apps/testbed',

    # Первый прогон после сборки даёт мусор -- прогревочный запуск, отчёт
    # которого выбрасывается. Ставить 0 только если сборка уже прогрета.
    [int]$Warmups = 1,

    [int]$TimeoutSeconds = 900
)

$ErrorActionPreference = 'Stop'

$exeDirFull = (Resolve-Path $ExeDir).Path
$exe = Join-Path $exeDirFull 'testbed.exe'
if (-not (Test-Path $exe)) { throw "не найден $exe -- собери таргет testbed" }

New-Item -ItemType Directory -Force -Path $OutDir | Out-Null
$outFull = (Resolve-Path $OutDir).Path

$shell = New-Object -ComObject WScript.Shell

function Invoke-Run([string]$scene, [string]$camera, [string]$report) {
    $argv = @("--scene=$scene", '--bench', "--bench-frames=$Frames", "--bench-warmup=$Warmup")
    if ($camera) { $argv += "--camera=$camera" }
    if ($report) {
        $argv += "--bench-out=$report"
        # Тот же прогон деревом, рядом с таблицами: человеку txt, разбору json.
        $argv += "--bench-json=$([System.IO.Path]::ChangeExtension($report, 'json'))"
    }
    $argv += $Extra

    $proc = Start-Process -FilePath $exe -WorkingDirectory $exeDirFull -ArgumentList $argv -PassThru
    $deadline = (Get-Date).AddSeconds($TimeoutSeconds)

    while (-not $proc.HasExited -and (Get-Date) -lt $deadline) {
        $null = $shell.AppActivate($proc.Id)
        Start-Sleep -Seconds 5
    }

    if (-not $proc.HasExited) {
        $proc | Stop-Process -Force
        throw "$scene не завершилась за $TimeoutSeconds с. Первые подозреваемые -- потерянный '--bench' и опция без '=': в обоих случаях замер не включается и приложение работает как обычное окно."
    }

    return $proc.ExitCode
}

foreach ($run in $Runs) {
    $parts  = $run.Split(':')
    $scene  = $parts[0]
    $camera = if ($parts.Count -gt 1) { $parts[1] } else { '' }
    $label  = if ($camera) { "$scene-$camera" } else { $scene }

    for ($i = 0; $i -lt $Warmups; $i++) {
        Write-Host "$label -- прогрев $($i + 1)/$Warmups (отчёт выбрасывается)"
        $null = Invoke-Run $scene $camera ''
    }

    $report = Join-Path $outFull "$label.txt"
    Write-Host "$label -- замер в $report"

    $code = Invoke-Run $scene $camera $report
    if ($code -ne 0) { Write-Host "  код возврата $code" -ForegroundColor Yellow }

    $line = Get-Content -LiteralPath $report | Select-String -Pattern '^fps at median frame:'
    if ($line) { Write-Host "  $($line.Line)" }
}

Write-Host "`nотчёты в $outFull"
Write-Host "сравнить: pwsh .claude/skills/render-bench/compare.ps1 -Before <до>/<прогон>.txt -After $OutDir/<прогон>.txt"
