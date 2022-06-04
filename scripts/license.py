import argparse
import re
from pathlib import Path

SOURCE_CODE_FILE_EXTENSIONS = [".h", ".cpp", ".inl"]
IGNORE_FOLDERS = ["build", "objs", "tests", "scripts"]

LICENCE_RE = re.compile(r"^// Compiler for PHP \(aka KPHP\)\n"
                        r"// Copyright \(c\) \d{4} LLC «V Kontakte»\n"
                        r"// Distributed under the GPL v3 License, see LICENSE\.notice\.txt")


def _remove_sub_path(full_path: Path, sub_path: Path) -> Path:
    full_path_str = full_path.as_posix()
    sub_path_str = sub_path.as_posix()

    if not full_path_str.startswith(sub_path_str):
        return Path(full_path_str)

    new_path = full_path_str[len(sub_path_str):]
    if not new_path.startswith("/"):
        return Path(new_path)

    return Path(new_path[1:])


def _get_sources(root_dir: Path):
    path = root_dir.glob("**/*")
    for file in path:
        if file.suffix not in SOURCE_CODE_FILE_EXTENSIONS:
            continue

        path_file_in_project = _remove_sub_path(file, root_dir)
        if path_file_in_project.parts[0] in IGNORE_FOLDERS:
            continue

        yield file


def check_license(root_dir: Path):
    errors = []

    for file in _get_sources(root_dir):
        try:
            file_content = file.read_text()
        except Exception:
            print(f"Error when reading the file '{file}'")
            continue

        license_is_file = LICENCE_RE.search(file_content)
        if license_is_file is not None:
            continue

        errors.append(f"There is no license at the beginning of the '{file}' file")

    return errors


def main(root_dir: Path):
    errors = []
    errors.extend(check_license(root_dir))

    if len(errors) == 0:
        exit(0)

    for error in errors:
        print(error)

    exit(1)


if __name__ == "__main__":
    parser = argparse.ArgumentParser(formatter_class=argparse.ArgumentDefaultsHelpFormatter)
    parser.add_argument("-w", type=str, dest="root_dir", required=True, help="kphp root dir")
    args = parser.parse_args()

    main(Path(args.root_dir))
