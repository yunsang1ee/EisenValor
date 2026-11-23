"""Controlled estimator/shift checks for secondary NEE; this is not a GPU scene test."""
import json
import math
import random
from pathlib import Path


def check():
    rng = random.Random(20261011)
    max_mis_error = 0.0
    max_shift_error = 0.0
    for _ in range(2000):
        # Integrate the two BSDF lobes and one light proposal analytically.
        pd, ps, pl = (10 ** rng.uniform(-3, 0) for _ in range(3))
        fd, fs, le = (rng.uniform(0.01, 8) for _ in range(3))
        pb = pd + ps
        wb, wl = pb / (pb + pl), pl / (pb + pl)
        estimate = pd * (fd * le / pd) * wb + ps * (fs * le / ps) * wb
        estimate += pl * ((fd + fs) * le / pl) * wl
        expected = (fd + fs) * le
        max_mis_error = max(max_mis_error, abs(estimate - expected) / expected)
        assert math.isclose(estimate, expected, rel_tol=1e-12)

        # rc->light is fixed, so the NEE proposal has no after-rc BSDF PDF ratio.
        psrc, pdst, gsrc, gdst, bdst, rcdst = (rng.uniform(0.01, 3) for _ in range(6))
        shifted_f = bdst / pdst * rcdst * le * wl / pl
        jacobian = gdst / gsrc * pdst / psrc
        expected_shift = bdst / psrc * rcdst * le * wl / pl * gdst / gsrc
        max_shift_error = max(max_shift_error, abs(shifted_f * jacobian - expected_shift) / expected_shift)
        assert math.isclose(shifted_f * jacobian, expected_shift, rel_tol=1e-12)
        reverse_j = gsrc / gdst * psrc / pdst
        assert math.isclose(jacobian * reverse_j, 1.0, rel_tol=1e-12)

    # Exact expectation of a single WRS result, including a zero BSDF suffix.
    trials = [(0.0, 0.0, 0.0), (0.6, 1.2, 0.3), (2.4, 0.2, 0.9)]
    luminance = lambda c: sum(x * y for x, y in zip(c, (0.2126, 0.7152, 0.0722)))
    expected_rgb = [sum(c[k] for c in trials) for k in range(3)]
    for spp in (1, 2, 4):
        terms = trials * spp
        total = sum(map(luminance, terms))
        expectation = [sum((luminance(c) / total) * c[k] * total / (luminance(c) * spp)
                           for c in terms if luminance(c) > 0) for k in range(3)]
        assert all(math.isclose(a, b, rel_tol=1e-12) for a, b in zip(expectation, expected_rgb))

    # Debug AOV: forward camera throughput must agree with recursive transport.
    # This isolates light-sampled terms at fixed BSDF/MIS weights; it is NOT the
    # difference between real F7 ON/OFF images (which use different hit MIS).
    max_aov_error = 0.0
    for case in range(2000):
        depths = rng.randint(1, 5)
        weights = [[rng.uniform(0.0, 2.0) for _ in range(3)] for _ in range(depths)]
        lights = [[rng.uniform(0.0, 10.0) for _ in range(3)] for _ in range(depths)]
        if case % 4 == 0:
            lights[0] = [0.0] * 3  # rc may be dark while deeper NEE still contributes.
        if case % 4 == 1:
            lights = [[0.0] * 3 for _ in range(depths)]  # OFF / occluded / no lights.
        throughput, aov = [1.0] * 3, [0.0] * 3
        for weight, light in zip(weights, lights):
            throughput = [a * b for a, b in zip(throughput, weight)]
            aov = [a + b * c for a, b, c in zip(aov, throughput, light)]
        reference = [0.0] * 3
        for weight, light in reversed(list(zip(weights, lights))):
            reference = [w * (l + r) for w, l, r in zip(weight, light, reference)]
        max_aov_error = max(max_aov_error, *(abs(a - r) for a, r in zip(aov, reference)))
        assert all(math.isclose(a, r, rel_tol=1e-12, abs_tol=1e-12) for a, r in zip(aov, reference))
        # One nonzero trial plus misses: normalize by all SPP, not successful hits.
        for spp in (1, 2, 4):
            mean = [sum(t[k] for t in [aov] + [[0.0] * 3] * (spp - 1)) / spp for k in range(3)]
            assert all(math.isclose(m, r / spp, rel_tol=1e-12, abs_tol=1e-12)
                       for m, r in zip(mean, reference))

    # The share diagnostic's denominator is the finalized fresh RGB. With a
    # luminance target, its luminance is independent of which color WRS selects.
    max_share_error = 0.0
    for _ in range(2000):
        terms = [tuple(rng.uniform(0.01, 10.0) for _ in range(3)) for _ in range(5)]
        for spp in (1, 2, 4):
            total_luminance = sum(map(luminance, terms))
            nee_luminance = sum(map(luminance, terms[:2])) / spp
            expected_share = sum(map(luminance, terms[:2])) / total_luminance
            for selected in terms:
                resolved = [x * total_luminance / (luminance(selected) * spp) for x in selected]
                share = nee_luminance / luminance(resolved)
                max_share_error = max(max_share_error, abs(share - expected_share))
                assert math.isclose(share, expected_share, rel_tol=1e-12)

    # Source-contract checks accompany the independent scalar model.
    root = Path(__file__).resolve().parents[2]
    shader = root / 'EisenValor-Client/EisenValor/Resource/Shader'
    candidate = (shader / 'RaytracingRestirPT.hlsl').read_text(encoding='utf-8-sig')
    temporal = (shader / 'RestirTemporalReuse.hlsl').read_text(encoding='utf-8-sig')
    assert 'child.color = weight;' in candidate
    assert 'child.rcSourcePdfBefore = pdf;' in candidate
    assert 'candidate.color *= incoming.color;' in candidate
    assert 'candidate.rcSourcePdfAfter = 0.0f;' in candidate
    assert 'payload.color += secondaryLighting;' in candidate
    assert 'RestirHasEmissiveNeeAtDepth(payload.recursionDepth)' in candidate
    assert 'reservoir.sampleCount += completedPrimaryTrials;' in candidate
    assert 'else if (RestirIsRcOutgoingNee(sourceSample))' in temporal
    assert 'reconnectSurface, reconnectView, rcWi, RESTIR_BSDF_LOBE_ALL' in temporal
    return {
        'scope': '2000 controlled MIS/NEE-shift cases and additive primary-trial expectation; no GPU path execution',
        'mis_max_relative_error': max_mis_error,
        'shift_max_relative_error': max_shift_error,
        'primary_trial_expected_rgb_at_1_2_4_spp': expected_rgb,
        'zero_bsdf_suffix_preserves_secondary_nee': True,
        'secondary_nee_aov_2000_path_checks_max_abs_error': max_aov_error,
        'secondary_nee_share_2000_cases_max_abs_error': max_share_error,
        'source_contract_checks': 'PASS',
    }


if __name__ == '__main__':
    print(json.dumps(check(), indent=2))
