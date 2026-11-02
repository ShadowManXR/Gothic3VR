[CmdletBinding()]
param([string]$LogPath='', [int]$LastRecords=60, [string]$GameDirectory='D:\SteamLibrary\steamapps\common\Gothic 3')
$ErrorActionPreference='Stop'
if(!$LogPath){
    $directory=Join-Path $GameDirectory 'scripts\Gothic3VRDiagnostics\logs'
    $latest=Get-ChildItem -LiteralPath $directory -Filter 'stereo-game-*.jsonl' -File | Sort-Object LastWriteTime -Descending | Select-Object -First 1
    if(!$latest){throw 'No stereo session log exists yet.'}
    $LogPath=$latest.FullName
}
$limit=[Math]::Max(1,[Math]::Min(600,$LastRecords))
$records=@(Get-Content -LiteralPath $LogPath -Tail 8000 | ForEach-Object {
    if($_ -match '"type":"vr_(update|gpu)_performance"'){try{$_ | ConvertFrom-Json}catch{}}
})
if(!$records.Count){throw 'No CPU update/GPU windows yet. Run VR with 0.22.20 or later for a few seconds.'}
$result=@(foreach($type in @('vr_update_performance','vr_gpu_performance')){
    $selected=@($records | Where-Object type -EQ $type | Select-Object -Last $limit)
    foreach($group in ($selected | Group-Object context)){
        $windows=@($group.Group)
        if($type -eq 'vr_update_performance'){
            $count=($windows | Measure-Object updates -Sum).Sum
            if(!$count){continue}
            $stages=@(foreach($name in $windows[0].cpu_wall_mean_per_update_ms.PSObject.Properties.Name){
                $sum=0.0;$maximum=0.0;$calls=0
                foreach($w in $windows){$sum+=$w.cpu_wall_mean_per_update_ms.$name*$w.updates;$maximum=[Math]::Max($maximum,$w.cpu_wall_max_per_update_ms.$name);$calls+=$w.calls.$name}
                [ordered]@{Stage=$name;MeanMsPerUpdate=[Math]::Round($sum/$count,4);MaxMsPerUpdate=$maximum;Calls=$calls}
            })
            [ordered]@{Type=$type;Log=$LogPath;Context=[int]$group.Name;Windows=$windows.Count;Updates=$count;Stages=$stages
                Note='CPU wall times. Input includes finisher; stages overlap. The application callback (also labelled native_update_inclusive in0.22.20) is not the full native simulation. Use Analyze-NativePerformance.ps1 for session/focus phases in0.22.21+.'}
        }else{
            $count=($windows | Measure-Object render_samples -Sum).Sum;$left=0.0;$right=0.0
            foreach($w in $windows){$left+=$w.left_gpu_mean_ms*$w.render_samples;$right+=$w.right_gpu_mean_ms*$w.render_samples}
            [ordered]@{Type=$type;Log=$LogPath;Context=[int]$group.Name;Windows=$windows.Count;Samples=$count
                LeftMeanMs=if($count){[Math]::Round($left/$count,4)}else{$null}
                RightMeanMs=if($count){[Math]::Round($right/$count,4)}else{$null}
                LeftMaxMs=($windows | Measure-Object left_gpu_max_ms -Maximum).Maximum
                RightMaxMs=($windows | Measure-Object right_gpu_max_ms -Maximum).Maximum
                Discarded=($windows | Measure-Object discarded -Sum).Sum;SkippedBusy=($windows | Measure-Object skipped_busy -Sum).Sum
                Note='Sparse GPU timestamps around native eye rendering. Excludes bridge copies/UI/runtime. Context:0 outside world,1 world,2 menu; samples are not total frame time.'}
        }
    }
})
ConvertTo-Json -InputObject $result -Depth 7
