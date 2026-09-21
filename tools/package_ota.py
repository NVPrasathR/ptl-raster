#!/usr/bin/env python3
"""Construct OTA metadata for Raster firmware images.

This package step intentionally only builds metadata and digest values.
It does not generate or manage private keys. External signing must be done
outside this tool using a secure signing system that is configured at the
board level.
"""

from __future__ import annotations

import argparse
import hashlib
import json
from pathlib import Path
from typing import Any


def file_sha256(path: Path) -> str:
    digest = hashlib.sha256()
    with path.open('rb') as fh:
        for chunk in iter(lambda: fh.read(65536), b''):
            digest.update(chunk)
    return digest.hexdigest()


def build_metadata(image_path: Path, target: str, version: str, signature_path: str | None) -> dict[str, Any]:
    image_bytes = image_path.read_bytes()
    digest_hex = file_sha256(image_path)
    metadata = {
        'schema': 'raster-ota-v1',
        'target': target,
        'version': version,
        'length': len(image_bytes),
        'digest': {
            'algorithm': 'sha256',
            'hex': digest_hex,
        },
        'image': {
            'path': image_path.name,
            'type': image_path.suffix.lower(),
        },
        'signature': {
            'external_signing_required': True,
            'algorithm': 'none',
            'detached_path': signature_path,
            'note': 'Private keys are never generated or managed by this tool.'
        },
        'bootloader': {
            'slot': 'staging',
            'commit_policy': 'pending_health_confirm',
            'never_overwrite_active': True,
            'requires_board_backend': True,
        },
    }
    if signature_path:
        metadata['signature']['algorithm'] = 'external-detached'
        metadata['signature']['status'] = 'provided'
    return metadata


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(description='Package a Raster OTA image and metadata.')
    parser.add_argument('--image', required=True, help='Path to the firmware image to package.')
    parser.add_argument('--target', required=True, help='Board target ID, such as rp2354.')
    parser.add_argument('--version', required=True, help='Firmware semantic version.')
    parser.add_argument('--out', required=True, help='Directory for the generated OTA package.')
    parser.add_argument('--signature', required=True, help='Detached signature file produced externally.')
    return parser.parse_args()


def main() -> int:
    args = parse_args()
    image_path = Path(args.image)
    if not image_path.exists():
        raise FileNotFoundError(f'image not found: {image_path}')
    signature_path = Path(args.signature)
    if not signature_path.is_file() or signature_path.stat().st_size == 0:
        raise ValueError('a non-empty detached signature is required')

    out_dir = Path(args.out)
    out_dir.mkdir(parents=True, exist_ok=True)

    metadata = build_metadata(image_path, args.target, args.version, str(signature_path))
    metadata_path = out_dir / f'{image_path.stem}.ota.json'
    package_path = out_dir / f'{image_path.name}.ota'

    package_path.write_bytes(image_path.read_bytes())
    metadata_path.write_text(json.dumps(metadata, indent=2, sort_keys=True) + '\n', encoding='utf-8')

    print(json.dumps({'package': str(package_path), 'metadata': str(metadata_path), 'digest': metadata['digest']['hex'], 'status': 'metadata-built'}, indent=2))
    return 0


if __name__ == '__main__':
    raise SystemExit(main())
