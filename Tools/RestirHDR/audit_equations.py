"""Scalar counterexamples for the ReSTIR equation audit, not a GPU test.

Run from any directory. Checks algebraic invariants and reports the consequences
of the legacy M policy and current visibility/geometry policies. Does not estimate scene incidence.
"""

import json
import math


def main():
    # Fresh RIS: probability_i = target_i / sum(target); output_i = F_i / probability_i / M.
    terms = (6.0, 12.0, 12.0)
    total = sum(terms)
    for spp in (1, 2, 4):
        candidates = terms * spp
        weight_sum = sum(candidates)
        expectation = sum((f / weight_sum) * f * weight_sum / (f * spp) for f in candidates)
        assert math.isclose(expectation, total)

    # A valid previous domain represents 20 trials. Its selected path fails the
    # forward shift, but the current path can shift backward, with unit targets/J.
    # Current MIS = 1/(1+20); historical contribution = 0.
    source_weight = 1 / 21
    legacy_stored_m = 1
    domain_stored_m = 21

    # Next frame: both shifts succeed, unit targets/J, current estimate = 1.
    # Keep the actual cap of 20 * currentM in both cases.
    def next_output(stored_m):
        history_m = min(stored_m, 20)
        return (1 + history_m * source_weight) / (1 + history_m)

    # Collinear primary=0, emitter=1. Initial ray starts at .01, tMax=.98;
    # reuse ray starts at .02, tMax=(1-.02)-.05=.93. Blocker at .97.
    initial_end = .01 + (1.0 - .02)
    reuse_end = .02 + ((1.0 - .02) - .05)
    blocker = .97
    assert .01 < blocker < initial_end
    assert blocker > reuse_end

    # Source geometry uses RayTCurrent measured from an offset origin.
    # For a normal-incidence ray of length .1 after offset .01, the surface-to-
    # surface edge is .11. The source and destination geometry factors differ.
    source_g = 1 / .1 ** 2
    destination_g = 1 / .11 ** 2
    assert not math.isclose(destination_g / source_g, 1.0)

    results = {
        'scope': 'Analytic code-policy counterexamples; no GPU execution or causal claim about captured streaks.',
        'fresh_additive_mean_1_2_4_spp': total,
        'rejected_history': {
            'current_mis': source_weight,
            'stored_M_legacy_success_only': legacy_stored_m,
            'stored_M_domain_count': domain_stored_m,
            'next_output_legacy_success_only': next_output(legacy_stored_m),
            'next_output_preserving_domain_M': next_output(domain_stored_m),
        },
        'visibility_counterexample': {
            'initial_world_endpoint': initial_end,
            'reuse_world_endpoint': reuse_end,
            'blocker_position': blocker,
            'initial_visible': False,
            'reuse_visible': True,
        },
        'self_geometry_counterexample': {
            'source_G': source_g,
            'destination_G': destination_g,
            'geometry_ratio': destination_g / source_g,
            'note': 'Geometry factor only, not the complete Jacobian; PDFs also require a self-shift test.',
        },
        'suffix_clamp_counterexample': {
            'source_suffix': 100000,
            'stored_suffix': 65504,
            'relative_loss': 1 - 65504 / 100000,
            'note': 'Only activated above FP16 range; not established in the captured scene.',
        },
    }
    print(json.dumps(results, indent=2))


if __name__ == '__main__':
    main()
