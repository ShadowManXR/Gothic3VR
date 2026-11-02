"""Analyze omission screening; never label an incomplete image as an FPS gain."""
import argparse
import csv
import json
import math
import statistics
from pathlib import Path

PHASES = ('native_A1', 'omit_static_color_B1', 'native_A2', 'omit_static_color_B2', 'native_A3')

def quantile(values, q):
    ordered = sorted(values)
    return ordered[min(len(ordered) - 1, math.ceil(q * len(ordered)) - 1)]

def analyze(directory):
    directory = Path(directory)
    meta = json.loads((directory / 'screening.json').read_text(encoding='utf-8-sig'))
    report = {'kind': 'screening_only', 'replacement_renderer': False,
              'image_equivalent': False, 'performance_gain_proven': False,
              'strict_upper_bound': False, 'scope': 'static mesh opaque color callbacks',
              'work_time_definition': 'after xrWaitFrame through xrEndFrame; excludes simulation outside this interval',
              'warnings': [
                  'Omitted color passes can also omit depth writes; native Z/shadow functions are forwarded, but equivalent depth/occlusion is not proven.',
                  'Visibility and operation/material preparation remain native. Nested scope timings overlap; do not add them.',
                  'A positive result is headroom for a bounded replacement prototype, not a demonstrated renderer speedup.',
              ]}
    if meta.get('kind') != 'static_color_omission_screening':
        raise ValueError('Unexpected screening type')
    if not meta.get('complete'):
        return {**report, 'status': 'INCOMPLETE', 'reason': meta.get('reason', 'missing')}
    with (directory / 'frames.csv').open(newline='', encoding='utf-8-sig') as stream:
        raw = list(csv.DictReader(stream))
    grouped = {phase: [] for phase in PHASES}
    last_frame = -1
    for entry in raw:
        phase = entry['phase']
        row = {key: float(value) for key, value in entry.items() if key != 'phase'}
        if any(not math.isfinite(value) for value in row.values()):
            raise ValueError('Nonfinite timing or pose')
        if row['frame'] <= last_frame or any(row[key] < 0 for key in ('work_ms', 'interval_ms', 'phase_age_ms')):
            raise ValueError('Invalid frame order or timing')
        last_frame = row['frame']
        if phase not in grouped or row['phase_age_ms'] < 1000:
            continue
        if row['eyes'] != 3 or row['interval_ms'] <= 0:
            continue
        if phase.startswith('omit_'):
            if any(row[f'{hand}_omitted'] <= 0 or row[f'{hand}_omitted'] != row[f'{hand}_targets'] for hand in ('left', 'right')):
                return {**report, 'status': 'INSUFFICIENT_COVERAGE', 'reason': 'Omission was not observed in both eyes'}
        elif row['left_omitted'] or row['right_omitted']:
            raise ValueError('Baseline omitted native work')
        grouped[phase].append(row)
    if any(len(rows) < 20 for rows in grouped.values()):
        return {**report, 'status': 'INSUFFICIENT_SAMPLES', 'phase_counts': {p: len(r) for p, r in grouped.items()}}
    summaries = {}
    for phase, rows in grouped.items():
        median = lambda key: statistics.median(row[key] for row in rows)
        summaries[phase] = {'frames': len(rows), 'work_median_ms': median('work_ms'),
                            'work_p95_ms': quantile([r['work_ms'] for r in rows], .95),
                            'interval_median_ms': median('interval_ms'),
                            'interval_p95_ms': quantile([r['interval_ms'] for r in rows], .95),
                            'target_calls_median': median('left_targets') + median('right_targets'),
                            'scope_inclusive_median_ms': {key[:-3]: median(key) for key in rows[0] if key.endswith('_ms') and key not in ('work_ms', 'interval_ms', 'phase_age_ms', 'xr_wait_ms')}}
    a = [summaries[p] for p in ('native_A1', 'native_A2', 'native_A3')]
    baseline_work = statistics.median(r['work_median_ms'] for r in a)
    baseline_interval = statistics.median(r['interval_median_ms'] for r in a)
    if baseline_work <= 0 or baseline_interval <= 0:
        raise ValueError('Zero baseline timing')
    drift = (max(r['work_median_ms'] for r in a) - min(r['work_median_ms'] for r in a)) / baseline_work
    savings = []
    for before, candidate, after in ((PHASES[0], PHASES[1], PHASES[2]), (PHASES[2], PHASES[3], PHASES[4])):
        adjacent = (summaries[before]['work_median_ms'] + summaries[after]['work_median_ms']) / 2
        savings.append(1 - summaries[candidate]['work_median_ms'] / adjacent)
    valid_rows = [row for rows in grouped.values() for row in rows]
    native_extent = math.sqrt(sum((max(r[f'native_{axis}_m'] for r in valid_rows) - min(r[f'native_{axis}_m'] for r in valid_rows))**2 for axis in 'xyz'))
    head_extent = math.sqrt(sum((max(r[f'head_{axis}_m'] for r in valid_rows) - min(r[f'head_{axis}_m'] for r in valid_rows))**2 for axis in 'xyz'))
    quats = [[r[f'head_q{axis}'] for axis in 'wxyz'] for r in valid_rows]
    def normalize(q):
        length = math.sqrt(sum(x*x for x in q))
        if length < .9 or length > 1.1:
            raise ValueError('Invalid headset quaternion')
        return [x/length for x in q]
    first = normalize(quats[0])
    head_angle = max(math.degrees(2*math.acos(min(1., abs(sum(x*y for x, y in zip(first, normalize(q))))))) for q in quats)
    native_angle = max(max(r[f'native_r{axis}'] for r in valid_rows) - min(r[f'native_r{axis}'] for r in valid_rows) for axis in 'xyz')
    controlled = drift <= .10 and native_extent <= .25 and head_extent <= .25 and head_angle <= 8 and native_angle <= math.radians(8)
    status = 'UNCONTROLLED_COMPARISON' if not controlled else ('HEADROOM_FOR_BOUNDED_STATIC_PROTOTYPE' if min(savings) >= .20 else 'LOW_STATIC_COLOR_HEADROOM')
    return {**report, 'status': status, 'phase_results': summaries,
            'baseline_work_median_ms': baseline_work, 'baseline_interval_median_ms': baseline_interval,
            'omission_work_savings_percent': [100*s for s in savings],
            'baseline_drift_percent': 100*drift, 'native_position_extent_m': native_extent,
            'head_position_extent_m': head_extent, 'head_rotation_extent_degrees': head_angle,
            'native_rotation_extent_radians': native_angle,
            'meaning': 'Screening concerns this static color path only; it cannot reject or validate all possible custom renderer architectures.'}

def main():
    parser = argparse.ArgumentParser()
    parser.add_argument('directory', type=Path)
    args = parser.parse_args()
    try:
        result = analyze(args.directory)
    except (OSError, ValueError, KeyError, csv.Error) as error:
        result = {'status': 'INVALID_OR_MISSING_DATA', 'reason': str(error), 'performance_gain_proven': False}
    (args.directory / 'analysis.json').write_text(json.dumps(result, indent=2), encoding='utf-8')
    print(result['status'])

if __name__ == '__main__':
    main()
