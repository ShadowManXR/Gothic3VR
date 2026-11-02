"""Read the explicitly armed native movement trace; never attach to the game."""
import argparse
import collections
import json
import math


def horizontal_dot(a, b):
    length = math.hypot(a[0], a[2]) * math.hypot(b[0], b[2])
    return None if length < 0.001 else (a[0] * b[0] + a[2] * b[2]) / length


def subtract(a, b):
    return [x - y for x, y in zip(a, b)]


def analyze(path):
    frames = collections.defaultdict(dict)
    with open(path, encoding="utf-8-sig") as stream:
        for line in stream:
            try:
                event = json.loads(line)
            except json.JSONDecodeError:
                continue  # The writer may still have an incomplete final line.
            if event.get("type") == "native_movement_trace":
                frames[event["process_id"]][event["stage"]] = event
    summary = collections.Counter()
    examples = []
    for update, stages in frames.items():
        before = stages.get("before_native_input")
        after = stages.get("after_native_input")
        pre = stages.get("before_native_physics")
        post = stages.get("after_native_physics")
        summary["sampled_updates"] += 1
        if not before or not after:
            summary["incomplete_input_pair"] += 1
            continue
        if before["mask"] not in (5, 9) or not before["sprint_selected"]:
            continue
        summary["diagonal_sprint_updates"] += 1
        expected = [bool(before["mask"] & bit) for bit in (1, 4, 8, 2)]
        actual = [timer != 4294967295 for timer in before["timers_FLRB"]]
        mismatch = actual != expected
        summary["timer_mask_mismatches"] += mismatch
        wish = before["wish_world"]
        dots = {
            "fresh_goal_vs_wish": horizontal_dot(after["fresh_goal_delta"], wish),
            "velocity_before_vs_wish": horizontal_dot(pre["velocity_world"], wish) if pre else None,
            "velocity_after_vs_wish": horizontal_dot(post["velocity_world"], wish) if post else None,
            "physics_displacement_vs_wish": horizontal_dot(subtract(post["position_world"], pre["position_world"]), wish) if pre and post else None,
        }
        for name, dot in dots.items():
            summary[("measurable_" if dot is not None else "unavailable_") + name] += 1
        reversed_fields = [name for name, dot in dots.items() if dot is not None and dot < -0.25]
        for name in reversed_fields:
            summary["reversed_" + name] += 1
        if (reversed_fields or mismatch) and len(examples) < 12:
            examples.append({"update": update, "time_ms": before["time_ms"], "mask": before["mask"],
                             "stick_xz": before["stick_xz"], "wish_mode": after["wish_mode"],
                             "native_mode": after["native_mode"], "head_scope": before["head_scope"],
                             "reference": before["reference"], "looking_back": before["looking_back"],
                             "timer_mismatch": mismatch, "dots": dots})
    # Native physics may only author velocity; fixed-step world transfer can
    # happen later. Zero within-call displacement is unavailable evidence,
    # not successful motion. Correlate stable endpoints across selected updates.
    before_events = sorted((stages["before_native_input"] for stages in frames.values()
                            if "before_native_input" in stages), key=lambda event: event["time_ms"])
    interval_examples = []
    def camera_direction(event):
        view, mask = event.get("camera_view"), event["mask"]
        if not view:
            return None
        forward = int(bool(mask & 1)) - int(bool(mask & 2))
        side = int(bool(mask & 8)) - int(bool(mask & 4))
        return [view[2] * forward + view[0] * side, 0, view[10] * forward + view[8] * side]
    for before, following in zip(before_events, before_events[1:]):
        if before["mask"] not in (5, 9) or following["mask"] != before["mask"]:
            continue
        if not (before["sprint_selected"] and following["sprint_selected"]):
            continue
        elapsed = following["time_ms"] - before["time_ms"]
        expected, next_expected = camera_direction(before), camera_direction(following)
        if not 0 < elapsed < 400 or expected is None or next_expected is None:
            continue
        agreement = horizontal_dot(expected, next_expected)
        delta = subtract(following["position_world"], before["position_world"])
        if agreement is None or agreement < .966 or math.hypot(delta[0], delta[2]) < 1:
            continue
        summary["stable_endpoint_displacement_intervals"] += 1
        agreement = horizontal_dot(delta, expected)
        if agreement is not None and agreement < -.25:
            summary["reversed_stable_endpoint_displacement_intervals"] += 1
            if len(interval_examples) < 12:
                interval_examples.append({"time_ms": before["time_ms"], "elapsed_ms": elapsed, "dot": agreement})
    return {"summary": dict(summary), "examples": examples, "interval_examples": interval_examples,
            "interpretation": "Fresh request, consumed sensor result, native velocity and actual displacement are distinct. Negative dot identifies disagreement, not its cause. Within-call zero displacement is unavailable evidence: fixed-step world transfer can occur later. Intersample displacement only verifies matching input endpoints, not continuous input; collisions and recovery require review."}


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("log")
    print(json.dumps(analyze(parser.parse_args().log), indent=2))
