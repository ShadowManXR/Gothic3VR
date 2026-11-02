import csv
import json
import unittest
import uuid
from pathlib import Path
from analyze_renderer_screening import analyze, PHASES

class ScreeningTests(unittest.TestCase):
    def setUp(self):
        self.root = Path(__file__).resolve().parents[4] / 'renderer-feasibility/analyzer-tests' / uuid.uuid4().hex
        self.root.mkdir(parents=True)
        self.meta = {'kind': 'static_color_omission_screening', 'complete': True}
        self.rows = []
        frame = 0
        for phase in PHASES:
            omitted = phase.startswith('omit_')
            for i in range(30):
                frame += 1
                row = {'frame': frame, 'phase': phase, 'phase_age_ms': 1000+i*20,
                       'work_ms': 12 if omitted else 20, 'interval_ms': 16 if omitted else 25,
                       'xr_wait_ms': 0, 'eyes': 3, 'left_targets': 100, 'right_targets': 100,
                       'left_omitted': 100 if omitted else 0, 'right_omitted': 100 if omitted else 0,
                       'mesh_render_ms': 4 if omitted else 10, 'mesh_render_calls': 200}
                for prefix in ('native_', 'head_'):
                    for axis in 'xyz':
                        row[f'{prefix}{axis}_m'] = 0
                for axis in 'xyz':
                    row[f'native_r{axis}'] = 0
                for axis in 'wxyz':
                    row[f'head_q{axis}'] = int(axis == 'w')
                self.rows.append(row)
    def run_data(self):
        (self.root/'screening.json').write_text(json.dumps(self.meta))
        with (self.root/'frames.csv').open('w', newline='') as f:
            writer = csv.DictWriter(f, fieldnames=self.rows[0]);writer.writeheader();writer.writerows(self.rows)
        return analyze(self.root)
    def test_headroom_is_not_gain(self):
        result = self.run_data()
        self.assertEqual(result['status'], 'HEADROOM_FOR_BOUNDED_STATIC_PROTOTYPE')
        self.assertFalse(result['performance_gain_proven'])
        self.assertFalse(result['image_equivalent'])
        self.assertAlmostEqual(result['omission_work_savings_percent'][0], 40)
    def test_low_headroom(self):
        for r in self.rows:
            if r['phase'].startswith('omit_'): r['work_ms'] = 19
        self.assertEqual(self.run_data()['status'], 'LOW_STATIC_COLOR_HEADROOM')
    def test_baseline_drift(self):
        for r in self.rows:
            if r['phase'] == 'native_A3': r['work_ms'] = 30
        self.assertEqual(self.run_data()['status'], 'UNCONTROLLED_COMPARISON')
    def test_movement(self):
        self.rows[-1]['native_x_m'] = 1
        self.assertEqual(self.run_data()['status'], 'UNCONTROLLED_COMPARISON')
    def test_missing_eye_coverage(self):
        next(r for r in self.rows if r['phase'].startswith('omit_'))['right_omitted'] = 0
        self.assertEqual(self.run_data()['status'], 'INSUFFICIENT_COVERAGE')
    def test_incomplete(self):
        self.meta['complete'] = False
        self.assertEqual(self.run_data()['status'], 'INCOMPLETE')
    def test_nan(self):
        self.rows[0]['work_ms'] = float('nan')
        with self.assertRaises(ValueError): self.run_data()
    def test_contaminated_baseline(self):
        self.rows[0]['left_omitted'] = 1
        with self.assertRaises(ValueError): self.run_data()

if __name__ == '__main__':
    unittest.main()
