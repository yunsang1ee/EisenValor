"""Compare pre-RR linear HDR captures; a 512-frame mean is not ground truth."""

import argparse
import json
from pathlib import Path

import numpy as np


def read_pfm(path):
    with path.open('rb') as stream:
        if stream.readline().strip() != b'PF':
            raise ValueError(f'{path}: expected RGB PFM')
        width, height = map(int, stream.readline().split())
        scale = float(stream.readline())
        pixels = np.frombuffer(stream.read(), dtype='<f4' if scale < 0 else '>f4')
    if pixels.size != width * height * 3 or not np.isfinite(pixels).all():
        raise ValueError(f'{path}: invalid pixel data')
    return pixels.reshape(height, width, 3)[::-1].astype(np.float64) * abs(scale)


def read_metadata(path):
    return dict(line.split('=', 1) for line in path.read_text().splitlines() if '=' in line)


def load_run(directory, roi):
    metadata = read_metadata(directory / 'capture.txt')
    images = {}
    for frames in (1, 32, 128, 512):
        stats = read_metadata(directory / f'mean_{frames}.txt')
        if int(stats['nonfinite_channels']):
            raise ValueError(f'{directory}: nonfinite HDR values; comparison invalid')
        image = read_pfm(directory / f'mean_{frames}.pfm')
        if roi:
            x, y, width, height = roi
            if min(x, y) < 0 or min(width, height) <= 0 or x + width > image.shape[1] or y + height > image.shape[0]:
                raise ValueError('ROI lies outside capture')
            image = image[y:y + height, x:x + width]
        images[frames] = image
    return metadata, images


def metrics(images):
    result = {}
    for frames, rgb in images.items():
        luminance = rgb @ np.array([.2126, .7152, .0722])
        result[frames] = {
            'mean_luminance': float(luminance.mean()),
            'p99_luminance': float(np.percentile(luminance, 99)),
            'p999_luminance': float(np.percentile(luminance, 99.9)),
            'max_luminance': float(luminance.max()),
            'rms_difference_from_own_512_mean': float(np.sqrt(np.mean((rgb - images[512]) ** 2))),
        }
    return result


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('candidate', type=Path)
    parser.add_argument('temporal', type=Path)
    parser.add_argument('--roi', type=int, nargs=4, metavar=('X', 'Y', 'WIDTH', 'HEIGHT'),
                        help='Top-left-origin static wall/floor region, excluding animated objects')
    args = parser.parse_args()
    try:
        a_meta, a = load_run(args.candidate, args.roi)
        b_meta, b = load_run(args.temporal, args.roi)
        if a_meta['source'] != 'CANDIDATE_RAW' or b_meta['source'] != 'FINAL_RAW':
            raise ValueError('Expected CANDIDATE_RAW then FINAL_RAW capture directories')
        for key in ('width', 'height', 'spp', 'format'):
            if a_meta[key] != b_meta[key]:
                raise ValueError(f'Capture conditions differ: {key}')
        for key in ('camera', 'direction', 'fov', 'aspect'):
            left = np.array([float(v) for v in a_meta[key].split(',')])
            right = np.array([float(v) for v in b_meta[key].split(',')])
            if left.shape != right.shape or not np.allclose(left, right, rtol=0, atol=1e-6):
                raise ValueError(f'Capture conditions differ: {key}')
        result = {
            'conditions': a_meta,
            'roi': args.roi,
            'candidate': metrics(a),
            'temporal': metrics(b),
            'cross_mode_rgb_rms_at_512': float(np.sqrt(np.mean((a[512] - b[512]) ** 2))),
            'limitations': [
                '512-frame estimates are correlated with their earlier checkpoints, not ground truth.',
                'Compare static geometry; animation, jitter, and scene changes can explain differences.',
                'These captures bypass RR. They cannot alone prove an RR or DLSS defect.',
                'Input is FP16 linear HDR; readback and CPU processing invalidate performance comparisons.',
            ],
        }
        print(json.dumps(result, indent=2))
    except (ValueError, OSError, KeyError) as error:
        parser.error(str(error))


if __name__ == '__main__':
    main()
