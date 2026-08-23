<#
.SYNOPSIS
    Сравнивает два отчёта test_world_grid и печатает вердикт по порогам.

.EXAMPLE
    pwsh .claude/skills/render-bench/compare.ps1 -Before before/parked.txt -After after/parked.txt
#>
param(
    [Parameter(Mandatory)][string]$Before,
    [Parameter(Mandatory)][string]$After,

    # Стадии дешевле этого не сравниваются: на десятых долях микросекунды
    # процент ничего не значит.
    [double]$Floor = 0.05
)

function Read-Stages([string]$path) {
    $stages = [ordered]@{}
    foreach ($line in Get-Content -LiteralPath $path) {
        if ($line -match '^(?<name>\S[^\s].*?)\s{2,}(?<p50>[\d.]+)\s+(?<p95>[\d.]+)\s+(?<p99>[\d.]+)\s+(?<max>[\d.]+)\s*$') {
            $stages[$matches.name.Trim()] = [pscustomobject]@{
                p50 = [double]$matches.p50
                p95 = [double]$matches.p95
                p99 = [double]$matches.p99
                max = [double]$matches.max
            }
        }
    }
    return $stages
}

# Строки таблицы, которые считают штуки, а не миллисекунды: процент от них
# ничего не значит, но разошедшийся счётчик означает, что сравниваются две
# разные сцены.
$countRows = @('cascades drawn', 'grid_pending', 'grid_staged')

function Read-Summary([string]$path) {
    $text = Get-Content -LiteralPath $path

    $first = {
        param($pattern, $group)
        $hit = $text | Select-String -Pattern $pattern | Select-Object -First 1
        if (-not $hit) { return '' }
        if ($group) { return $hit.Matches.Groups[$group].Value }
        return $hit.Line
    }

    return [pscustomobject]@{
        Present = & $first '^present mode: (.+)$' 1
        Samples = & $first '^samples: (\d+)$' 1
        Ready   = & $first '^scene ready after .+$' $null
        Memory  = & $first '^memory: .+$' $null
    }
}

$b = Read-Stages $Before
$a = Read-Stages $After
$bs = Read-Summary $Before
$as = Read-Summary $After

if ($bs.Present -ne $as.Present) {
    Write-Host "present mode отличается: $($bs.Present) против $($as.Present) -- числа несравнимы" -ForegroundColor Red
}
if ($bs.Samples -ne $as.Samples) {
    Write-Host "разное число кадров: $($bs.Samples) против $($as.Samples)" -ForegroundColor Yellow
}

# Пороги протокола: p50 -- 2 %, p99 -- 5 %, GPU-стадии -- 3 %.
function Get-Threshold([string]$stage, [string]$metric) {
    if ($stage.StartsWith('gpu_')) { return 3.0 }
    if ($metric -eq 'p99') { return 5.0 }
    return 2.0
}

$rows = foreach ($name in $b.Keys) {
    if (-not $a.Contains($name)) { continue }

    $isCount = $countRows -contains $name

    foreach ($metric in 'p50', 'p99') {
        $before = $b[$name].$metric
        $after  = $a[$name].$metric

        if (-not $isCount -and $before -lt $Floor -and $after -lt $Floor) { continue }
        if ($isCount -and $metric -eq 'p99') { continue }

        $delta = if ($before -gt 0) { 100.0 * ($after - $before) / $before } else { [double]::NaN }
        $limit = Get-Threshold $name $metric

        $verdict = if ($isCount) { if ($before -eq $after) { 'count =' } else { 'count differs' } }
                   elseif ([double]::IsNaN($delta)) { 'new' }
                   elseif ([math]::Abs($delta) -lt $limit) { '=' }
                   elseif ($delta -gt 0) { 'WORSE' }
                   else { 'better' }

        [pscustomobject]@{
            stage   = $name
            metric  = $metric
            before  = [math]::Round($before, 3)
            after   = [math]::Round($after, 3)
            'delta%' = if ([double]::IsNaN($delta)) { '' } else { [math]::Round($delta, 1) }
            verdict = $verdict
        }
    }
}

$rows | Format-Table -AutoSize

Write-Host "`nбыло:  $($bs.Ready)"
Write-Host "стало: $($as.Ready)"
Write-Host "было:  $($bs.Memory)"
Write-Host "стало: $($as.Memory)"

$worse = @($rows | Where-Object { $_.verdict -eq 'WORSE' })
if ($worse.Count -gt 0) {
    Write-Host "`nрегрессия сверх порога: $($worse.Count) строк" -ForegroundColor Red
} else {
    Write-Host "`nничего сверх порога не ухудшилось" -ForegroundColor Green
}
