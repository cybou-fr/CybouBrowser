# Samples the TCP/UDP endpoints of a launched process tree (no capture driver, no admin rights).
# Usage: powershell -File idle_connections.ps1 -Exe build\Release\cybou.exe -Out result.txt [-Seconds 120] [-IntervalMs 0] [-Url https://example.org/]
# Starts the application with --private (no session restore, off-the-record profile) and stops its process tree afterwards.
# Limits: roughly one sample per second, so connections shorter than that can be missed; UDP shows local endpoints only.
param([int]$Seconds = 90, [string]$Exe, [string]$Out, [string]$Url = '', [int]$IntervalMs = 1500)
$ErrorActionPreference = 'SilentlyContinue'
function Tree([int]$root) {
    $all = Get-CimInstance Win32_Process | Select-Object ProcessId, ParentProcessId, Name
    $ids = New-Object System.Collections.Generic.HashSet[int]
    [void]$ids.Add($root)
    $changed = $true
    while ($changed) {
        $changed = $false
        foreach ($p in $all) { if ($ids.Contains([int]$p.ParentProcessId) -and -not $ids.Contains([int]$p.ProcessId)) { [void]$ids.Add([int]$p.ProcessId); $changed = $true } }
    }
    return $ids
}
$dnsBefore = @((Get-DnsClientCache).Entry | Sort-Object -Unique)
$argList = @('--private'); if ($Url) { $argList += $Url }
$proc = Start-Process -FilePath $Exe -ArgumentList $argList -PassThru
$seen = @{}
$udp = @{}
$names = @{}
$iterations = 0
$start = Get-Date
while (((Get-Date) - $start).TotalSeconds -lt $Seconds) {
    $ids = Tree $proc.Id
    foreach ($c in (Get-NetTCPConnection)) {
        if ($ids.Contains([int]$c.OwningProcess)) {
            $name = (Get-Process -Id $c.OwningProcess).ProcessName
            $key = "$name TCP $($c.RemoteAddress):$($c.RemotePort) [$($c.State)]"
            $seen[$key] = 1 + [int]$seen[$key]
        }
    }
    foreach ($u in (Get-NetUDPEndpoint)) {
        if ($ids.Contains([int]$u.OwningProcess)) {
            $name = (Get-Process -Id $u.OwningProcess).ProcessName
            $key = "$name UDP local $($u.LocalAddress):$($u.LocalPort)"
            $udp[$key] = 1
        }
    }
    $iterations++
    if ($IntervalMs -gt 0) { Start-Sleep -Milliseconds $IntervalMs }
}
$tree = Tree $proc.Id
$image = @(Get-Process -Id ($tree | ForEach-Object { $_ }) | Select-Object -ExpandProperty ProcessName | Sort-Object -Unique)
foreach ($id in $tree) { Stop-Process -Id $id -Force }
Start-Sleep -Seconds 3
$dnsAfter = @((Get-DnsClientCache).Entry | Sort-Object -Unique)
$newNames = $dnsAfter | Where-Object { $dnsBefore -notcontains $_ }
$lines = @()
$lines += "SAMPLES: $iterations over $Seconds s"
$lines += "PROCESSES: " + ($image -join ', ')
$lines += "--- TCP remote endpoints (sample count) ---"
$seen.GetEnumerator() | Sort-Object Name | ForEach-Object { $lines += "$($_.Name) x$($_.Value)" }
$lines += "--- UDP local endpoints ---"
$udp.Keys | Sort-Object | ForEach-Object { $lines += $_ }
$lines += "--- New DNS cache entries ---"
$newNames | ForEach-Object { $lines += $_ }
$lines += "--- survivors ---"
$lines += (Get-Process -Name cybou, QtWebEngineProcess | ForEach-Object { "$($_.ProcessName) $($_.Id)" }) -join '; '
$lines | Set-Content -Path $Out -Encoding utf8
