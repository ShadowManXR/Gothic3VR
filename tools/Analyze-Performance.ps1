[CmdletBinding()]
param([string]$LogPath='', [int]$LastRecords=60, [string]$GameDirectory='D:\SteamLibrary\steamapps\common\Gothic 3')
$ErrorActionPreference='Stop'
if(!$LogPath){
    $directory=Join-Path $GameDirectory 'scripts\Gothic3VRDiagnostics\logs'
    $latest=Get-ChildItem -LiteralPath $directory -Filter 'stereo-game-*.jsonl' -File | Sort-Object LastWriteTime -Descending | Select-Object -First 1
    if(!$latest){throw 'No stereo session log exists yet.'}
    $LogPath=$latest.FullName
}
$records=@(Get-Content -LiteralPath $LogPath -Tail 4000 | ForEach-Object {
    if($_ -match '"type":"vr_frame_performance"'){try{$_ | ConvertFrom-Json}catch{}}
} | Select-Object -Last ([Math]::Max(1,[Math]::Min(600,$LastRecords))))
if(!$records.Count){throw 'No performance windows in this session yet. Use VR for at least a few seconds with version 0.22.13 or later.'}
$result=@(foreach($group in ($records | Group-Object batched_copies)){
    $windows=@($group.Group);$frames=($windows | Measure-Object frames -Sum).Sum
    $intervals=($windows | Measure-Object interval_samples -Sum).Sum
    $intervalSum=0.0;foreach($w in $windows){$intervalSum+=$w.interval_mean_ms*$w.interval_samples}
    $stages=@(foreach($name in $windows[0].cpu_wall_mean_ms.PSObject.Properties.Name){
        $sum=0.0;$maximum=0.0
        foreach($w in $windows){$sum+=$w.cpu_wall_mean_ms.$name*$w.frames;$maximum=[Math]::Max($maximum,$w.cpu_wall_max_ms.$name)}
        [ordered]@{Stage=$name;MeanMs=[Math]::Round($sum/$frames,3);MaxMs=[Math]::Round($maximum,3)}
    })
    [ordered]@{Log=$LogPath;BatchedCopies=$windows[0].batched_copies;Windows=$windows.Count;Frames=$frames
        FrameIntervalMeanMs=if($intervals){[Math]::Round($intervalSum/$intervals,3)}else{0}
        WorstWindowP95Ms=($windows | Measure-Object interval_p95_recent_ms -Maximum).Maximum
        StageCpuWallTimes=$stages
        Note='CPU wall times; xr_wait includes pacing; frame_work overlaps stages; worst window p95 is not a combined percentile.'}
})
ConvertTo-Json -InputObject $result -Depth 6
