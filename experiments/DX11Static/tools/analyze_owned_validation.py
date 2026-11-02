"""Summarize the bounded, read-only mode5 trial. Never infer FPS improvement."""
import argparse
import json
import math
from pathlib import Path


def analyze(folder):
    result_path = folder / 'owned-validation.json'
    log_path = folder / 'native-static-trial.jsonl'
    events = [json.loads(line) for line in log_path.read_text(encoding='utf-8-sig').splitlines() if line.strip()] if log_path.exists() else []
    saved = json.loads(result_path.read_text(encoding='utf-8-sig')) if result_path.exists() else {}
    proofs = [e for e in events if e.get('type') == 'dx11_owned_native_validation']
    controls = [e for e in events if e.get('type') == 'dx11_owned_native_sampler_control']
    def pair(records, geometry_only=False):
        pairs = {}
        for proof in records:
            depth = proof.get('max_depth_error', float('inf'))
            if ((proof.get('passed') or geometry_only) and proof.get('sdk_scope_ended') and
                proof.get('eye') in (0, 1) and proof.get('frame', 0) > 0 and
                proof.get('covered_samples', 0) > 100 and
                proof.get('coverage_disagreements') == 0 and
                (geometry_only or proof.get('max_rgb_steps', 256) <= 1) and
                math.isfinite(depth) and 0 <= depth <= 4 / 16777215):
                pairs.setdefault(proof['frame'], set()).add(proof['eye'])
        return any(eyes == {0, 1} for eyes in pairs.values())
    complete = pair(proofs)
    geometry_complete = pair(proofs, geometry_only=True)
    control_complete = pair(controls)
    suppressed = saved.get('native_draws_suppressed', 0) != 0 or any(e.get('native_draw_suppressed') is True for e in events)
    passed = saved.get('result') == 'PASS' and saved.get('native_preserved') is True and saved.get('same_frame_stereo_pair') is True and complete and not suppressed
    control_only = (saved.get('result') == 'SAMPLER_CONTROL_ONLY' and
                    saved.get('native_preserved') is True and
                    saved.get('same_frame_sampler_control_pair') is True and
                    control_complete and geometry_complete and not complete and not suppressed)
    failure = next((e for e in events if e.get('type') == 'dx11_static_trial_failed'), None)
    return {'result': 'PASS' if passed else 'SAMPLER_CONTROL_ONLY' if control_only else 'REFUSED' if saved or failure or suppressed else 'INCOMPLETE', 'same_frame_stereo_pair': complete, 'same_frame_geometry_pair': geometry_complete, 'same_frame_sampler_control_pair': control_complete, 'actual_sampling_color_parity': passed, 'native_preserved': not suppressed, 'comparisons': len(proofs), 'atlas_reads': saved.get('atlas_reads'), 'atlas_content_hits': saved.get('atlas_content_hits'), 'failure': failure, 'game_fps_gain_proven': False, 'full_dx11_renderer': False}


if __name__ == '__main__':
    parser = argparse.ArgumentParser()
    parser.add_argument('folder', type=Path)
    args = parser.parse_args()
    print(json.dumps(analyze(args.folder), indent=2))
