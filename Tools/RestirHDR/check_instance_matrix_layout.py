"""Compile the real InstanceData header and verify its CPU/HLSL translation ABI.

Checks generated DXIL loads, not just source spelling. This is a compiler/ABI
regression check, not a GPU rendering test. Requires the Windows SDK DXC.
"""

import argparse
from pathlib import Path
import re
import shutil
import subprocess
import tempfile


def main():
    root = Path(__file__).resolve().parents[2]
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--include-dir', type=Path, default=root / 'EisenValor-Client/EisenValor')
    parser.add_argument('--dxc', default=shutil.which('dxc') or
                        r'C:\Program Files (x86)\Windows Kits\10\bin\10.0.26100.0\x64\dxc.exe')
    args = parser.parse_args()
    with tempfile.TemporaryDirectory(prefix='restir-matrix-') as temp:
        temp = Path(temp)
        shader = temp / 'probe.hlsl'
        shader.write_text('''#define HLSL
#include "RaytracingCommon.h"
StructuredBuffer<InstanceData> input : register(t0);
RWStructuredBuffer<float4> output : register(u0);
[numthreads(1,1,1)] void main() {
    output[0] = mul(float4(0,0,0,1), input[0].worldMatrix);
    output[1] = mul(float4(0,0,0,1), input[0].worldInverse);
    output[2] = mul(float4(0,0,0,1), input[0].previousWorldMatrix);
}
''')
        assembly = temp / 'probe.asm'
        subprocess.run([args.dxc, '-T', 'cs_6_6', '-E', 'main', '-HV', '2021', '-WX',
                        '-enable-16bit-types', '-I', str(args.include_dir), '-Fc', str(assembly),
                        '-Fo', str(temp / 'probe.dxil'), str(shader)], check=True)
        text = assembly.read_text()
        for field in ('worldMatrix', 'worldInverse', 'previousWorldMatrix'):
            if not re.search(r'row_major\s+float4x4\s+' + field + r'\b', text):
                raise SystemExit(f'FAIL: {field} does not preserve XMFLOAT4X4 storage')
        # CPU row-vector translation is row 4: matrixBase + 3 * 16 bytes.
        loads = {int(offset) for offset in re.findall(
            r'call[^\n]*@dx\.op\.rawBufferLoad\.f32\([^\n]*?i32 0, i32 (\d+),', text)}
        if loads != {48, 112, 176}:
            raise SystemExit(f'FAIL: translation load offsets {loads}, expected 48/112/176')
        print('PASS: world/inverse/previous translation loads match CPU row-major offsets 48/112/176')


if __name__ == '__main__':
    main()
