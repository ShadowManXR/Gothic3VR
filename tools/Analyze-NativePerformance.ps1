[CmdletBinding()]
param([string]$LogPath='', [string]$GameDirectory='D:\SteamLibrary\steamapps\common\Gothic 3')
$ErrorActionPreference='Stop'
if(!$LogPath){
    $latest=Get-ChildItem -LiteralPath (Join-Path $GameDirectory 'scripts\Gothic3VRDiagnostics\logs') -Filter 'stereo-game-*.jsonl' -File | Sort-Object LastWriteTime -Descending | Select-Object -First 1
    if(!$latest){throw 'No VR session log exists yet.'};$LogPath=$latest.FullName
}
$rows=@(Get-Content -LiteralPath $LogPath -Tail 12000 | ForEach-Object {
    if($_ -match '"type":"vr_(native_phase_performance|cpu_spike|eye_draw_profile|profile_lines_dropped|scene_stall)"'){try{$_ | ConvertFrom-Json}catch{}}
})
$phases=@(foreach($group in ($rows | Where-Object type -EQ 'vr_native_phase_performance' | Group-Object context)){
    $windows=@($group.Group)
    foreach($name in @($windows | ForEach-Object {$_.calls.PSObject.Properties.Name} | Sort-Object -Unique)){
        $count=0;$sum=0.0;$maximum=0.0
        foreach($w in $windows){$count+=$w.calls.$name;$sum+=$w.cpu_wall_mean_per_call_ms.$name*$w.calls.$name;$maximum=[Math]::Max($maximum,$w.cpu_wall_max_per_call_ms.$name)}
        [ordered]@{Context=[int]$group.Name;Phase=$name;Calls=$count;Sampling=if($name.StartsWith('sampled_')){'One of 32 world stereo frames; temporal captures excluded'}else{'All eligible calls'};MeanMsPerCall=if($count){[Math]::Round($sum/$count,4)}else{$null};MaxMsPerCall=$maximum}
    }
})
$spikes=@($rows | Where-Object type -EQ 'vr_cpu_spike' | Sort-Object wall_ms -Descending | Select-Object -First 20 stage,context,eye,wall_ms,xr_frame_serial,qpc_started,qpc_frequency,thread)
$draws=@($rows | Where-Object type -EQ 'vr_eye_draw_profile')
$drawSpikes=@($draws | Where-Object is_spike -EQ $true | Sort-Object total_cpu_ms -Descending | Select-Object -First 20)
$drawSamples=@($draws | Where-Object is_spike -NE $true | Select-Object -Last 12)
$sceneStalls=@($rows | Where-Object type -EQ 'vr_scene_stall' | Sort-Object wall_ms -Descending | Select-Object -First 20)
$simulation=@(foreach($group in ($rows | Where-Object { $_.type -eq 'vr_native_phase_performance' -and $_.context -eq 1 } | Group-Object context)){
    $windows=@($group.Group);$entityUpdates=0
    foreach($w in $windows){$entityUpdates+=$w.calls.engine_entities_update_inclusive}
    if($entityUpdates){foreach($phase in @('engine_modules_update_inclusive','engine_entities_update_inclusive','character_animation_inclusive','physics_simulate_inclusive','animation_physics_inclusive','ai_script_routine_inclusive','navigation_update_inclusive')){
        $total=0.0;$calls=0;foreach($w in $windows){$calls+=$w.calls.$phase;$total+=$w.calls.$phase*$w.cpu_wall_mean_per_call_ms.$phase}
        [ordered]@{Phase=$phase;Calls=$calls;EntityUpdateCalls=$entityUpdates;MeanMsPerEntityUpdate=[Math]::Round($total/$entityUpdates,4)}
    }}
})
$dropped=($rows | Where-Object type -EQ 'vr_profile_lines_dropped' | Measure-Object count -Sum).Sum
[ordered]@{Log=$LogPath;NativePhases=$phases;GameplaySimulation=$simulation;LargestSceneStalls=$sceneStalls;LargestCpuSpikes=$spikes;LargestEyeDrawSpikes=$drawSpikes;RecentEyeDrawSamples=$drawSamples;DeferredProfilerLinesDropped=if($dropped){$dropped}else{0};Note='Inclusive application-thread CPU wall times, not GPU times. sampled_* phases only time one of 32 world stereo frames and exclude temporal capture: their counts/totals are not comparable to all-frame phases or the entity-update denominator. Simulation phases include work outside VR eyes. Per-entity-update costs overlap and must not be added; the update count is not headset FPS. Scene stall thread CPU is approximate; process I/O includes other threads/buffered reads and may omit mapped-file paging, so it is not proof of blocking disk I/O. Session Process is not the entire engine loop. Spike threshold20ms, per-stage rate limit250ms; this is not an exhaustive hitch count. Eye draw records are sparse. Outside draw API time also includes other driver calls.'} | ConvertTo-Json -Depth 7
