import importlib.util
from pathlib import Path
import tempfile
import unittest

spec = importlib.util.spec_from_file_location('stage_pages', Path(__file__).parents[1] / 'stage_pages.py')
pages = importlib.util.module_from_spec(spec)
spec.loader.exec_module(pages)


class StagePagesTests(unittest.TestCase):
    def test_app_includes_validation_dependencies_and_rejects_their_removal(self):
        with tempfile.TemporaryDirectory() as temporary:
            destination = Path(temporary) / 'site'
            pages.stage(destination)
            self.assertTrue((destination / 'index.html').is_file())
            self.assertTrue((destination / 'validation' / 'compare.js').is_file())
            self.assertTrue((destination / 'validation' / 'cpu-fixtures.json').is_file())
            self.assertFalse(any(path.is_symlink() for path in destination.rglob('*')))
            for name in ['compare.js', 'cpu-fixtures.json']:
                path = destination / 'validation' / name
                contents = path.read_bytes()
                path.unlink()
                with self.assertRaisesRegex(ValueError, name.replace('.', r'\.')):
                    pages.validate_runtime_files(destination)
                path.write_bytes(contents)


if __name__ == '__main__':
    unittest.main()
