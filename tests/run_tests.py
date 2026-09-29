#!/usr/bin/env python3
import os
import sys
import subprocess
import argparse
import difflib
import re

# Core language tests to run (paths relative to workspace root)
CORE_TESTS = [
    'tests/basic/basic.np',
    'tests/basic/main.np',
    'tests/features/all_features_test.np',
    'tests/features/test_phases.np',
    'tests/features/test_oop_and_control.np',
    'tests/features/test_checkpoint1_returns_pointers.np',
    'tests/features/test_checkpoint2_strings_assert.np',
    'tests/features/test_checkpoint3_interfaces.np',
    'tests/features/test_checkpoint4_channels.np',
    'tests/features/test_checkpoint5_stdlib_generics.np',
    'tests/hello-world/hello.np',
    'tests/imports/test_import_pkg.np',
    'tests/imports/test_go_style_imports.np',
    'tests/imports/test_go_prefix_alias.np',
    'tests/math/fibonacci.np',
    'tests/math/math.np',
    'tests/stdlib/stdlib_test.np',
    'tests/stdlib/test_stdlib_modules.np',
    'tests/stdlib/test_sys_argv.np',
    'tests/features/test_forward_decl.np'
]

def run_compiler(test_file):
    try:
        # Run local ./np compiler relative to the root directory
        result = subprocess.run(
            ['./np', test_file],
            stdout=subprocess.PIPE,
            stderr=subprocess.PIPE,
            text=True,
            timeout=15
        )
        return result.returncode, result.stdout, result.stderr
    except subprocess.TimeoutExpired:
        return -1, "", "TIMEOUT"
    except Exception as e:
        return -1, "", str(e)

def normalize_output(text):
    # Normalize newlines
    text = text.replace('\r\n', '\n').strip()
    # Mask pointer-like large integers (12 or more digits, optional minus sign)
    # E.g. 106015486695376, 3868996801937078784, -1977849499002537472
    text = re.sub(r'-?\b\d{12,}\b', '[PTR]', text)
    # Mask scientific notation timestamps like 1.78304e+09
    text = re.sub(r'\b1\.\d+e\+09\b', '[TIMESTAMP]', text)
    # Mask standard epoch timestamps like 1719... or 178...
    text = re.sub(r'\b17\d{8,9}\b', '[TIMESTAMP]', text)
    return text

def main():
    parser = argparse.ArgumentParser(description="NP Compiler Core Test Suite")
    parser.add_argument('--generate', action='store_true', help="Generate expected output (.expected) files")
    args = parser.parse_args()

    # Change directory to the workspace root
    script_dir = os.path.dirname(os.path.abspath(__file__))
    root_dir = os.path.dirname(script_dir)
    os.chdir(root_dir)

    print(f"Running NP Compiler Test Suite on {len(CORE_TESTS)} core tests...")
    
    passed_count = 0
    failed_count = 0

    for test_file in CORE_TESTS:
        expected_file = test_file + '.expected'
        
        if args.generate:
            print(f"Generating expected output for {test_file}...", end="")
            code, stdout, stderr = run_compiler(test_file)
            if code != 0:
                print(f" \033[91mFAILED (exit code {code})\033[0m")
                print(f"Stderr: {stderr}")
            else:
                stdout_norm = normalize_output(stdout)
                with open(expected_file, 'w', encoding='utf-8') as f:
                    f.write(stdout_norm)
                print(" \033[92mDONE\033[0m")
            continue

        # Test mode
        if not os.path.exists(expected_file):
            print(f"Skipping {test_file} (no .expected file found)")
            continue

        print(f"Running test {test_file}...", end="")
        code, stdout, stderr = run_compiler(test_file)

        if code != 0:
            print(f" \033[91mFAILED (exit code {code})\033[0m")
            print(f"Stderr: {stderr}")
            failed_count += 1
            continue

        # Load expected output
        with open(expected_file, 'r', encoding='utf-8') as f:
            expected = f.read()

        # Compare normalized outputs
        stdout_norm = normalize_output(stdout)
        expected_norm = normalize_output(expected)

        if stdout_norm == expected_norm:
            print(" \033[92mPASSED\033[0m")
            passed_count += 1
        else:
            print(f" \033[91mFAILED (output mismatch)\033[0m")
            failed_count += 1
            # Show diff
            diff = difflib.unified_diff(
                expected_norm.splitlines(),
                stdout_norm.splitlines(),
                fromfile='Expected',
                tofile='Actual',
                lineterm=''
            )
            print("\n".join(diff))
            print("-" * 40)

    if args.generate:
        print("\nExpected outputs generated successfully.")
        return

    print("\n" + "="*40)
    print(f"Test Summary:")
    print(f"  \033[92mPassed: {passed_count}\033[0m")
    print(f"  \033[91mFailed: {failed_count}\033[0m")
    print("="*40)

    if failed_count > 0:
        sys.exit(1)
    else:
        sys.exit(0)

if __name__ == '__main__':
    main()
