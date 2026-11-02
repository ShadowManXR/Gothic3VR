import copy
import json
import tempfile
import unittest
from pathlib import Path
from analyze_owned_validation import analyze


class EvidenceTests(unittest.TestCase):
    def check(self, saved, events):
        # Keep automatically cleaned fixtures under the known workspace.
        with tempfile.TemporaryDirectory(prefix='owned-evidence-', dir=Path.cwd() / 'work') as directory:
            folder = Path(directory)
            (folder / 'owned-validation.json').write_text(json.dumps(saved))
            (folder / 'native-static-trial.jsonl').write_text('\n'.join(map(json.dumps, events)))
            return analyze(folder)

    def setUp(self):
        self.saved = dict(result='PASS', native_preserved=True, same_frame_stereo_pair=True,
                          same_frame_sampler_control_pair=True, native_draws_suppressed=0)
        self.events = [dict(type='dx11_owned_native_validation', passed=True, sdk_scope_ended=True,
                            frame=20, eye=eye, covered_samples=500, coverage_disagreements=0,
                            max_rgb_steps=0, max_depth_error=0) for eye in (0, 1)]

    def test_complete_actual_pair(self):
        self.assertEqual(self.check(self.saved, self.events)['result'], 'PASS')

    def test_control_only_is_never_actual_parity(self):
        controls = copy.deepcopy(self.events)
        for event in controls:
            event['type'] = 'dx11_owned_native_sampler_control'
        for event in self.events:
            event.update(passed=False, max_rgb_steps=44)
        self.saved.update(result='SAMPLER_CONTROL_ONLY', same_frame_stereo_pair=False)
        result = self.check(self.saved, self.events + controls)
        self.assertEqual(result['result'], 'SAMPLER_CONTROL_ONLY')
        self.assertFalse(result['actual_sampling_color_parity'])

    def test_depth_failure_refuses_claimed_pass(self):
        self.events[1]['max_depth_error'] = .01
        self.assertEqual(self.check(self.saved, self.events)['result'], 'REFUSED')

    def test_cross_frame_pair_refused(self):
        self.events[1]['frame'] += 1
        self.assertEqual(self.check(self.saved, self.events)['result'], 'REFUSED')

    def test_native_suppression_refused(self):
        self.saved['native_draws_suppressed'] = 1
        self.assertEqual(self.check(self.saved, self.events)['result'], 'REFUSED')

    def test_no_coverage_refused(self):
        self.events[0]['covered_samples'] = 0
        self.assertEqual(self.check(self.saved, self.events)['result'], 'REFUSED')


if __name__ == '__main__':
    unittest.main()
