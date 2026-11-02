[CmdletBinding()]
param(
    [Parameter(Mandatory=$true)][string]$LogPath,
    [string]$OutputDirectory
)
$ErrorActionPreference = 'Stop'
$resolvedLog = (Resolve-Path -LiteralPath $LogPath).Path
if (!$OutputDirectory) { $OutputDirectory = Split-Path $resolvedLog -Parent }
New-Item -ItemType Directory -Path $OutputDirectory -Force | Out-Null
$events = New-Object 'System.Collections.Generic.List[object]'
$invalidLines = 0
foreach ($line in [System.IO.File]::ReadLines($resolvedLog)) {
    if ([string]::IsNullOrWhiteSpace($line)) { continue }
    try { $events.Add(($line | ConvertFrom-Json)) }
    catch { $invalidLines++ }
}
if (!$events.Count) { throw 'The file has no valid diagnostic JSON records.' }
$starts = @($events | Where-Object { $_.type -eq 'start' })
$hooks = @($events | Where-Object { $_.type -eq 'hook' })
$messages = @($events | Where-Object { $_.type -eq 'message' -or $_.type -eq 'log_limit' })
$validations = @($events | Where-Object { $_.type -eq 'hook_validation' })
$initialized = @($messages | Where-Object { $_.message -eq 'four_application_virtual_observers_installed' }).Count -gt 0
$cameras = @($events | Where-Object { $_.type -eq 'camera' })
$devices = @($events | Where-Object { $_.type -eq 'device' })
$controlEvents = @($events | Where-Object { $_.type -eq 'head_control' })
$appliedSamples = @($controlEvents | Where-Object { $_.action -eq 'applied' })
$restoredSamples = @($controlEvents | Where-Object { $_.action -eq 'restored' })
$changedControlSamples = 0
foreach ($sample in $appliedSamples) {
    $difference = 0.0
    foreach ($row in 0..2) { foreach ($column in 0..2) {
        $index = $row*4+$column
        $difference = [Math]::Max($difference,[Math]::Abs([double]$sample.native_view[$index]-[double]$sample.readback_view[$index]))
    } }
    if ($difference -gt 0.0001) { $changedControlSamples++ }
}
$appliedFrames = 0; $restoredFrames = 0
if ($controlEvents.Count) {
    $appliedFrames = ($controlEvents | Measure-Object applied_frames -Maximum).Maximum
    $restoredFrames = ($controlEvents | Measure-Object restored_frames -Maximum).Maximum
}
$controlError = @($appliedSamples + $restoredSamples) | Measure-Object view_rotation_max_error -Maximum
$movementEvents = @($controlEvents | Where-Object { $_.PSObject.Properties['movement_calls'] })
$movementDirections = @($controlEvents | Where-Object { $_.action -eq 'movement_direction' -and $_.wished_direction_world })
$movementCalls = 0; $movementRestores = 0
if ($movementEvents.Count) {
    $movementCalls = ($movementEvents | Measure-Object movement_calls -Maximum).Maximum
    $movementRestores = ($movementEvents | Measure-Object movement_restores -Maximum).Maximum
}
$movingDirections = @($movementDirections | Where-Object {
    $d=$_.wished_direction_world; ([double]$d[0]*$d[0]+[double]$d[1]*$d[1]+[double]$d[2]*$d[2]) -gt 0.000001
})
$movementY = @($movingDirections | ForEach-Object { [Math]::Abs([double]$_.wished_direction_world[1]) }) | Measure-Object -Maximum
$movementMatrixSamples = @($controlEvents | Where-Object { $_.action -eq 'movement_applied' -or $_.action -eq 'movement_restored' })
$movementMatrixError = $movementMatrixSamples | Measure-Object view_rotation_max_error -Maximum
$actionEvents = @($controlEvents | Where-Object { $_.PSObject.Properties['action_calls'] })
$actionDirections = @($controlEvents | Where-Object { $_.action -eq 'action_direction' })
$actionCalls = 0; $actionRestores = 0
if ($actionEvents.Count) {
    $actionCalls = ($actionEvents | Measure-Object action_calls -Maximum).Maximum
    $actionRestores = ($actionEvents | Measure-Object action_restores -Maximum).Maximum
}
$velocityEvents = @($events | Where-Object { $_.type -eq 'locomotion_velocity' })
$velocityCalls=0; $velocitySpeedError=0.0; $velocityVerticalError=0.0; $velocityHeadingError=0.0
if($velocityEvents.Count) {
    $velocityCalls=($velocityEvents | Measure-Object redirected_calls -Maximum).Maximum
    foreach($sample in $velocityEvents) {
        $v=$sample.native_velocity_world; $r=$sample.requested_velocity_world; $d=$sample.direction_world
        $nativeSpeed=[Math]::Sqrt([double]$v[0]*$v[0]+[double]$v[2]*$v[2])
        $requestedSpeed=[Math]::Sqrt([double]$r[0]*$r[0]+[double]$r[2]*$r[2])
        $velocitySpeedError=[Math]::Max($velocitySpeedError,[Math]::Abs($requestedSpeed-$nativeSpeed))
        $velocityVerticalError=[Math]::Max($velocityVerticalError,[Math]::Abs([double]$r[1]-[double]$v[1]))
        $angle=[Math]::Atan2([double]$d[0]*$r[2]-[double]$d[2]*$r[0],[double]$d[0]*$r[0]+[double]$d[2]*$r[2])
        $velocityHeadingError=[Math]::Max($velocityHeadingError,[Math]::Abs($angle)*180/[Math]::PI)
    }
}
$vrInfos = @($events | Where-Object { $_.type -eq 'vr_info' })
$vrPoses = @($events | Where-Object { $_.type -eq 'vr_pose' })
$validVrPoses = @($vrPoses | Where-Object { $_.pose_valid -eq $true })
$correlatedVrPoses = @($validVrPoses | Where-Object { $_.source -eq 'OnPreRender.exit' })
$vrAge = $correlatedVrPoses | Measure-Object age_ms -Average -Maximum
$motionRange = @()
$uniquePoses = @($validVrPoses | ForEach-Object { (@($_.position) + @($_.quaternion)) -join ',' } | Select-Object -Unique).Count
$rotationChange = 0.0
$identityOnly = $validVrPoses.Count -gt 0
if ($validVrPoses.Count) {
    foreach ($axis in 0..2) {
        $extent = $validVrPoses | ForEach-Object { [double]$_.position[$axis] } | Measure-Object -Minimum -Maximum
        $motionRange += [Math]::Round($extent.Maximum - $extent.Minimum,6)
    }
    $reference = $validVrPoses[0].quaternion
    foreach ($pose in $validVrPoses) {
        $dot = 0.0; $normA = 0.0; $normB = 0.0
        foreach ($component in 0..3) {
            $a = [double]$reference[$component]; $b = [double]$pose.quaternion[$component]
            $dot += $a*$b; $normA += $a*$a; $normB += $b*$b
        }
        $angle = 0.0
        if ($normA -gt 0 -and $normB -gt 0) {
            $angle = 2 * [Math]::Acos([Math]::Min(1.0,[Math]::Abs($dot)/[Math]::Sqrt($normA*$normB))) * 180 / [Math]::PI
        }
        $rotationChange = [Math]::Max($rotationChange,$angle)
        if (@($pose.position | Where-Object { [Math]::Abs([double]$_) -gt 0.000001 }).Count -or
            [Math]::Abs([Math]::Abs([double]$pose.quaternion[0])-1) -gt 0.000001 -or
            @($pose.quaternion[1..3] | Where-Object { [Math]::Abs([double]$_) -gt 0.000001 }).Count) { $identityOnly = $false }
    }
}
$headMotionObserved = $rotationChange -gt 0.1 -or @($motionRange | Where-Object { $_ -gt 0.001 }).Count -gt 0
$summaries = @($events | Where-Object { $_.type -eq 'summary' })
$phases = @($events | Where-Object { $_.type -eq 'phase' })
$stacks = @{}
$durations = @{}
$unmatchedExits = 0
foreach ($phase in $phases) {
    $key = '{0}/{1}' -f $phase.thread_id, $phase.name
    if (!$stacks.ContainsKey($key)) { $stacks[$key] = New-Object 'System.Collections.Generic.Stack[object]' }
    if ($phase.edge -eq 'enter') { $stacks[$key].Push($phase) }
    elseif ($phase.edge -eq 'exit') {
        if (!$stacks[$key].Count) { $unmatchedExits++; continue }
        $entry = $stacks[$key].Pop()
        $elapsed = [double]$phase.time_ms - [double]$entry.time_ms
        if ($elapsed -ge 0) {
            if (!$durations.ContainsKey($phase.name)) { $durations[$phase.name] = New-Object 'System.Collections.Generic.List[double]' }
            $durations[$phase.name].Add($elapsed)
        }
    }
}
$metrics = @()
foreach ($name in @('OnProcess','OnPreRender','OnRenderDraw','OnPostRender')) {
    $values = @()
    if ($durations.ContainsKey($name)) { $values = @($durations[$name] | Sort-Object) }
    if ($values.Count) {
        $measurement = $values | Measure-Object -Average -Maximum
        $metrics += [PSCustomObject]@{
            callback=$name;paired_calls=$values.Count;
            average_ms=[Math]::Round($measurement.Average,3);
            p95_ms=[Math]::Round($values[[Math]::Ceiling(0.95*$values.Count)-1],3);
            maximum_ms=[Math]::Round($measurement.Maximum,3)
        }
    }
}
$unmatchedEntries = 0
foreach ($stack in $stacks.Values) { $unmatchedEntries += $stack.Count }
$sequences = @()
foreach ($group in @($phases | Group-Object process_id | Select-Object -First 12)) {
    $sequences += [PSCustomObject]@{
        process_id=$group.Name;
        observed_pre_render_entries=@($group.Group | Where-Object { $_.name -eq 'OnPreRender' -and $_.edge -eq 'enter' }).Count;
        sequence=(@($group.Group | ForEach-Object { '{0}:{1}@T{2}' -f $_.name,$_.edge,$_.thread_id }) -join ' -> ')
    }
}
$dropped = 0
if ($summaries.Count) { $dropped = ($summaries | Measure-Object dropped_records -Maximum).Maximum }
$summary = [ordered]@{
    source=$resolvedLog;records=$events.Count;invalid_lines=$invalidLines;
    installed_hook_records=$hooks.Count;camera_samples=$cameras.Count;device_samples=$devices.Count;
    initialization_complete=$initialized;hook_validations=$validations;
    dropped_records=$dropped;unmatched_phase_entries=$unmatchedEntries;unmatched_phase_exits=$unmatchedExits;
    start=($starts | Select-Object -First 1);callback_timings=$metrics;
    callback_threads=@($phases | Select-Object -ExpandProperty thread_id -Unique);
    sample_sequences=$sequences;messages=$messages;
    last_camera=($cameras | Select-Object -Last 1);last_device=($devices | Select-Object -Last 1);
    direct_locomotion=[ordered]@{
        redirected_calls=$velocityCalls;samples=$velocityEvents.Count;
        maximum_horizontal_speed_error=$velocitySpeedError;maximum_vertical_velocity_error=$velocityVerticalError;
        maximum_requested_direction_error_degrees=$velocityHeadingError;last_event=($velocityEvents | Select-Object -Last 1)
    };
    head_control=[ordered]@{
        records=$controlEvents.Count;applied_frames=$appliedFrames;restored_frames=$restoredFrames;
        applied_samples=$appliedSamples.Count;restored_samples=$restoredSamples.Count;
        changed_view_samples=$changedControlSamples;maximum_view_rotation_error=$controlError.Maximum;
        movement_calls=$movementCalls;movement_restores=$movementRestores;
        action_calls=$actionCalls;action_restores=$actionRestores;action_direction_samples=$actionDirections.Count;
        sampled_session_keys=@($actionDirections | Group-Object session_key | Select-Object Name,Count);
        last_action_direction=($actionDirections | Select-Object -Last 1);
        movement_direction_samples=$movementDirections.Count;nonzero_movement_samples=$movingDirections.Count;
        maximum_absolute_wished_vertical=$movementY.Maximum;
        maximum_movement_view_rotation_error=$movementMatrixError.Maximum;
        last_movement_direction=($movementDirections | Select-Object -Last 1);
        states=@($controlEvents | Where-Object { $_.action -notin @('applied','restored','movement_applied','movement_restored','movement_direction','action_applied','action_restored','action_direction') } | Select-Object time_ms,action,enabled,centered,applied_frames,restored_frames);
        last_event=($controlEvents | Select-Object -Last 1)
    };
    tracking=[ordered]@{
        headset_connections=$vrInfos.Count;pose_samples=$vrPoses.Count;valid_pose_samples=$validVrPoses.Count;
        valid_pre_render_samples=$correlatedVrPoses.Count;
        unique_poses=$uniquePoses;head_motion_observed=$headMotionObserved;identity_pose_only=$identityOnly;
        maximum_rotation_change_degrees=[Math]::Round($rotationChange,6);
        pre_render_sample_age_average_ms=$vrAge.Average;pre_render_sample_age_maximum_ms=$vrAge.Maximum;
        position_range_metres=$motionRange;last_headset=($vrInfos | Select-Object -Last 1);
        last_valid_pose=($validVrPoses | Select-Object -Last 1);last_pose=($vrPoses | Select-Object -Last 1)
    };
    last_counters=($summaries | Select-Object -Last 1)
}
$stem = [System.IO.Path]::GetFileNameWithoutExtension($resolvedLog)
$jsonPath = Join-Path $OutputDirectory ($stem + '-summary.json')
$markdownPath = Join-Path $OutputDirectory ($stem + '-summary.md')
$summary | ConvertTo-Json -Depth 12 | Set-Content -LiteralPath $jsonPath -Encoding UTF8
$report = New-Object 'System.Collections.Generic.List[string]'
$report.Add('# Gothic 3 VR diagnostic summary')
$report.Add('')
$report.Add('Source: `' + $resolvedLog + '`')
$report.Add('')
$report.Add(('Records: {0}; hooks: {1}; cameras: {2}; devices: {3}.' -f $events.Count,$hooks.Count,$cameras.Count,$devices.Count))
if (!$initialized) { $report.Add('This log does not confirm complete hook initialization. Check the messages and validation results below.') }
$report.Add(('Reported dropped records: {0}; invalid lines: {1}; unmatched entries/exits: {2}/{3}.' -f $dropped,$invalidLines,$unmatchedEntries,$unmatchedExits))
$report.Add('')
$report.Add('| Callback | Paired calls | Average ms | P95 ms | Maximum ms |')
$report.Add('|---|---:|---:|---:|---:|')
foreach ($metric in $metrics) {
    $report.Add(('| {0} | {1} | {2} | {3} | {4} |' -f $metric.callback,$metric.paired_calls,$metric.average_ms,$metric.p95_ms,$metric.maximum_ms))
}
$report.Add('')
$report.Add('Timings were observed with instrumentation enabled. Callbacks may be nested; do not add their timings or treat OnPreRender as evidence of a complete frame or stereo eye.')
$report.Add('')
$report.Add('## Captured sequences')
$report.Add('')
foreach ($sequence in $sequences) {
    $report.Add(('- Update {0}, OnPreRender entries {1}: `{2}`' -f $sequence.process_id,$sequence.observed_pre_render_entries,$sequence.sequence))
}
$report.Add('')
$report.Add('## Callback validation')
$report.Add('')
$report.Add('| Callback | Slot | Result | Original module | Target module |')
$report.Add('|---|---:|---|---|---|')
foreach ($validation in $validations) {
    $report.Add(('| {0} | {1} | {2} | {3} | {4} |' -f $validation.name,$validation.slot,$validation.result,$validation.original_module,$validation.target_module))
}
$report.Add('')
$report.Add('## VR tracking')
$report.Add('')
$report.Add(('Connections with headset metadata: {0}; tracking samples: {1}; valid: {2}; valid at OnPreRender.exit: {3}.' -f $vrInfos.Count,$vrPoses.Count,$validVrPoses.Count,$correlatedVrPoses.Count))
if ($vrInfos.Count) {
    $headset = $vrInfos[-1]
    $report.Add(('Reported headset: {0}; manufacturer: {1}; tracking system: {2}.' -f $headset.model,$headset.manufacturer,$headset.tracking_system))
}
if ($correlatedVrPoses.Count) {
    $report.Add(('Sample age at OnPreRender.exit: average {0:N3} ms; maximum {1:N3} ms. This does not measure total headset latency.' -f $vrAge.Average,$vrAge.Maximum))
}
if ($motionRange.Count) { $report.Add(('X/Y/Z position range in meters: {0}. Game coordinates have not yet been aligned with headset coordinates.' -f ($motionRange -join ', '))) }
if ($validVrPoses.Count -and !$headMotionObserved) { $report.Add('Headset motion is not confirmed: samples marked valid do not vary by more than 1 mm or 0.1 degrees. The pose_valid flag alone does not prove motion tracking.') }
if ($identityOnly) { $report.Add('All valid samples are identity poses: zero position and neutral orientation. Investigate the source of these values before controlling the camera.') }
if (!$validVrPoses.Count) { $report.Add('This log does not confirm headset tracking. Check the VDXR messages and Quest 3 connection through Virtual Desktop.') }
$report.Add('')
$report.Add('## Camera control')
$report.Add('')
$report.Add(('Applied/restored frames: {0}/{1}; applied/restored snapshots: {2}/{3}; snapshots with a view differing from the native camera: {4}.' -f $appliedFrames,$restoredFrames,$appliedSamples.Count,$restoredSamples.Count,$changedControlSamples))
$report.Add(('VR-directed movement calls/restores: {0}/{1}; desired direction samples: {2}, nonzero: {3}. Check the desired direction against visible movement in the game.' -f $movementCalls,$movementRestores,$movementDirections.Count,$movingDirections.Count))
$report.Add(('VR-directed movement actions/restores: {0}/{1}; samples after OnAction: {2}. Session key: 1 forward, 2 backward, 3 left, 4 right.' -f $actionCalls,$actionRestores,$actionDirections.Count))
$report.Add(('Velocities aligned directly to VR input: {0}; samples: {1}; maximum horizontal speed error: {2}; maximum vertical component change: {3}; maximum requested heading error: {4} degrees. These values describe physics requests before collisions.' -f $velocityCalls,$velocityEvents.Count,$velocitySpeedError,$velocityVerticalError,$velocityHeadingError))
if ($controlEvents.Count) {
    $report.Add(('Maximum view rotation error in the internal check: {0}. The visual result and axis directions need user confirmation in the game.' -f $controlError.Maximum))
    foreach ($state in $summary.head_control.states) { $report.Add(('- {0:N3} ms: {1}' -f $state.time_ms,$state.action)) }
}
$report.Add('')
$report.Add('## Messages')
$report.Add('')
foreach ($message in $messages) { $report.Add('- ' + $message.message) }
$report | Set-Content -LiteralPath $markdownPath -Encoding UTF8
[PSCustomObject]@{Markdown=$markdownPath;JSON=$jsonPath;Records=$events.Count;Hooks=$hooks.Count;Cameras=$cameras.Count;Devices=$devices.Count}
