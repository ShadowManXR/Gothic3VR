# 0.23.78 - Bow string contact at the fingertips

The previous visual constraint used the open bind-pose palm centre. In the
native bow finger pose this left the visible contact near the wrist. The right
string hand now anchors the point between the current thumb and index tips to
the evaluated arrow/string nock.

Identify the native thumb/index terminal bones while binding the rig. Estimate
each distal endpoint from its native segment axis and length; pose those
endpoints with the current native finger matrices and express their midpoint
in the current hand frame. The wrist target compensates for this measured
finger contact under the calibrated controller orientation. All finger bones
receive the same rigid hand delta, preserving the native curled pose.

Resolve this before the one-per-frame frozen stereo palette. The arrow, bow,
string geometry, physical controller samples, draw progression and calibrated
hand orientation remain unchanged. Release/cancel/tracking gates from 0.23.77
remain in force. Missing/invalid finger landmarks decline the constraint rather
than substitute a wrist contact. Quick heal remains double left trigger.

Regression checks rotate the hand/controller and curl the posed fingers over
180 frames, verify both transformed tips straddle the nock in both frozen eye
palettes, keep the wrist away from the arrow, and retain draw cancellation and
raw physical samples. Headset validation is required for the final visible
contact. Only the plugin changes; the launcher remains 0.23.77.
