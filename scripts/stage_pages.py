#!/usr/bin/env python3
"""Stage only the Three.js app and its referenced icons for GitHub Pages."""
import argparse
from pathlib import Path
import re
import shutil

ROOT = Path(__file__).resolve().parents[1]
SOURCE = ROOT / 'threejs_webgl_des'


def validate_runtime_files(destination):
    """Reject missing literal module imports and URL-based runtime resources."""
    missing = set()
    for path in destination.rglob('*.js'):
        text = path.read_text()
        references = re.findall(
            r'''(?:\bfrom\s*|\bimport\s*\(\s*|\bimport\s+|\bnew\s+URL\s*\(\s*)["'](\.[^"']+)["']''',
            text,
        )
        for reference in references:
            if not (path.parent / reference).is_file():
                missing.add(f'{path.relative_to(destination)}: {reference}')
    if missing:
        raise ValueError('Missing runtime dependencies:\n' + '\n'.join(sorted(missing)))


def stage(destination):
    destination = destination.resolve()
    if destination.exists():
        raise ValueError(f'Staging directory already exists: {destination}')
    destination.mkdir(parents=True)
    for name in ['index.html', 'style.css', 'sw.js', 'manifest.webmanifest', '.nojekyll']:
        shutil.copy2(SOURCE / name, destination / name)
    for name in ['src', 'data', 'vendor']:
        shutil.copytree(SOURCE / name, destination / name, symlinks=False)
    # The app's Validate button uses these files at runtime.
    (destination / 'validation').mkdir()
    for name in ['compare.js', 'cpu-fixtures.json']:
        shutil.copy2(SOURCE / 'validation' / name, destination / 'validation' / name)
    for path in (SOURCE / 'assets').iterdir():
        if path.name != 'icons':
            shutil.copytree(path, destination / 'assets' / path.name, symlinks=False)

    # Includes extensionless talent icon names and filenames in JS, JSON,
    # CSS, HTML, the manifest, and the service worker's offline asset list.
    tokens = set()
    for path in destination.rglob('*'):
        if path.is_file() and path.suffix in {'.js', '.mjs', '.json', '.css', '.html', '.webmanifest'}:
            tokens.update(re.findall(r'[A-Za-z0-9_-]+', path.read_text()))
    icons = destination / 'assets' / 'icons'
    icons.mkdir(parents=True)
    count = 0
    for path in (SOURCE / 'assets' / 'icons').iterdir():
        if path.stem in tokens:
            shutil.copy2(path, icons / path.name, follow_symlinks=True)
            count += 1
    files = [path for path in destination.rglob('*') if path.is_file()]
    size = sum(path.stat().st_size for path in files)
    if size >= 1_000_000_000:
        raise ValueError('Pages output exceeds the 1 GB site limit')
    if any(path.is_symlink() for path in destination.rglob('*')):
        raise ValueError('Pages output must not contain symlinks')
    validate_runtime_files(destination)
    print(f'Staged {len(files)} files, {count} icons, {size / 1_000_000:.1f} MB at {destination}')


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('destination', type=Path)
    stage(parser.parse_args().destination)
