import os
import argparse
import shutil
from pathlib import Path
from impl.rules_generator import RulesGenerator


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument('--auto', required=True, help='path to auto directory')
    parser.add_argument('--schema', required=True, help='path to vertex schema file')
    parser.add_argument('--rules', required=True, help='path to rewrite rules file')
    args = parser.parse_args()
    name = Path(args.rules).stem
    result = RulesGenerator(args.rules, args.schema).generate_rules()
    output_dir = Path(os.path.join(args.auto, 'compiler', 'rewrite-rules'))
    if output_dir.exists():
        shutil.rmtree(str(output_dir))
    output_dir.mkdir()
    with open(os.path.join(output_dir, name + ".h"), 'w') as f:
        f.write(result.h_src)
    with open(os.path.join(output_dir, name + ".cpp"), 'w') as f:
        f.write(result.cpp_src)


if __name__ == "__main__":
    main()
