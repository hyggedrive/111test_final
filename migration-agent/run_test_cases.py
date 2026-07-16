import os
import sys
import shutil
import tempfile
import subprocess
from pathlib import Path
from src.global_config import *
import colorama


# ANSI escape codes for colors
GREEN = "\033[92m"
RED = "\033[91m"
RESET = "\033[0m"  # Reset color
colorama.init()


def copy_to_tempdir(src):
    """Copy the source directory to a temporary directory and return its path."""
    dest = tempfile.mkdtemp()
    shutil.copytree(src, dest, dirs_exist_ok=True)
    return dest


# def run_build_script(script_path):
#     """Source the build script and capture its environment variables."""
#     command = f"source {script_path}"

#     os.system(command)


def init():
    """Initialize the environment and print build status."""
    print("Building BiSheng AI... \t\t\t", end="", flush=True)
    # run_build_script("setup_compiler_driver.sh")
    print("Done")

    print(f"CC={os.environ.get('CC')}")
    print(f"CXX={os.environ.get('CXX')}")

    os.environ["AUTO_ACCEPT"] = "1"
    os.environ["LLM_SILENT"] = "1"
    # os.environ["LLM_RETRY_TIMES"] = "1"
    os.environ["LLM_DEVELOPMENT"] = "0"


def run_make_command(test_case, command):
    """Run the make command in the given directory and return the result."""
    # 让输出直接打到终端
    result = subprocess.run(
        command,
        shell=True,
        cwd=test_case,
        # stdout=None,  # subprocess.PIPE,
        # stderr=None,  # subprocess.PIPE,
        stdout=subprocess.PIPE,
        stderr=subprocess.PIPE,
        text=True,
    )
    # print(result.returncode)   # 只打印退出码
    return result

def run_one_test_case(test_case, work_dir):
    """Compile and check the result of a single test case."""
    print(f"Compile {test_case.relative_to(work_dir)} ...\t\t\t\t", end="", flush=True)

    # Clean and make
    run_make_command(test_case, "make clean")
    result = run_make_command(test_case, "make")

    if result.returncode == 0:
        print(f" [{GREEN}PASS{RESET}]")
    else:
        print(f" [{RED}FAILED{RESET}]")

    return result.returncode


def collect_test_folders(work_dir):
    """Collect all subfolders (test cases) within the given directory."""
    folder_path = Path(work_dir)
    test_folders = [
        subfolder for subfolder in folder_path.iterdir() if subfolder.is_dir()
    ]
    return test_folders


def print_test_summary(num_test_cases, num_pass, num_failed):
    """Print the summary of the test results."""
    pass_rate = (num_pass / num_test_cases) * 100 if num_test_cases > 0 else 0
    print(
        f"Tested {num_test_cases} test cases, passed {num_pass}, failed {num_failed}, pass rate: {pass_rate:.2f}%"
    )


def run_test(test_sample_path):
    """Run all tests in the provided sample path."""
    work_dir = copy_to_tempdir(test_sample_path)
    print(f"Running tests using {get_model_id()} as the LLM.")

    test_folders = collect_test_folders(work_dir)

    num_test_cases = 0
    num_pass = 0
    num_failed = 0

    for category in test_folders:
        test_category_folders = collect_test_folders(category)
        for folder in test_category_folders:
            ret = run_one_test_case(folder, work_dir)
            num_test_cases += 1
            if ret == 0:
                num_pass += 1
            else:
                num_failed += 1

    print_test_summary(num_test_cases, num_pass, num_failed)

    # Clean up the temporary working directory
    try:
        shutil.rmtree(work_dir)
    except Exception as e:
        print(f"Failed to clean up temp directory: {e}")


if __name__ == "__main__":
    test_sample_path = "test_examples"
    if len(sys.argv) > 1:
        test_sample_path = sys.argv[1]

    init()
    run_test(test_sample_path)
