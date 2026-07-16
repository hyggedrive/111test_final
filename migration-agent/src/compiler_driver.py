"""This defines the compiler driver routines"""

import sys
import os
import subprocess
import re
import shutil
import logging
import tempfile
from pathlib import Path
import signal
import traceback
from inference import *
from global_config import *
from utilities.utilities import *
from utilities.display import *
from prompt_engineering import *
from utilities.filemanager import *


class CompilerDriver:
    def __init__(self, CC="clang", CXX="clang++"):
        self.argv = sys.argv
        self.CC = CC
        self.CXX = CXX
        self.compiler = self.CXX
        self.attempts = 0
        self.max_attempts = get_llm_retry_times()
        self.user_defined_compiler_choice = False
        self.temp_dir = None
        self.compiler_outputs = []
        self.compiler_choice = os.getenv("NATIVE_COMPILER") or os.getenv(
            "COMPILER_CHOICE"
        )
        self.initialize_compiler()
        self.temp_dir = tempfile.mkdtemp()

    def initialize_compiler(self):
        """Initialize the compiler based on environment variables or command-line arguments."""
        if self.compiler_choice:
            self.compiler = self.compiler_choice
            self.user_defined_compiler_choice = True
        else:
            self.set_compiler_from_args()

        if not self.check_compiler(self.compiler):
            sys.exit(-1)

        logging.info(f"Using {self.compiler} as the native compiler.")

    def set_compiler_from_args(self):
        """Set the compiler based on file extensions in the arguments."""
        cfiles = self.get_compile_target(self.argv[1:])
        if cfiles:
            for f in cfiles:
                self.compiler = self.CC if f.endswith(".c") else self.CXX
                break
        else:
            self.compiler = self.CXX

    def check_compiler(self, compiler: str):
        """Check if the compiler is accessible."""
        try:
            subprocess.run(
                [compiler, "--version"],
                check=True,
                stdout=subprocess.PIPE,
                stderr=subprocess.PIPE,
            )
            return True
        except Exception as e:
            logging.error(f"Failed to verify {compiler}. Is it in $PATH?")
            print(e)
            return False

    def get_compile_target(self, command):
        """Retrieve the compile target from the command."""
        return [e for e in command if re.match(r".*\.(c|cpp)$", e)]


class CompilerErrorHandler:
    def __init__(self, max_attempts, compiler, temp_dir):
        self.max_attempts = max_attempts
        self.compiler = compiler
        self.attempts = 0
        self.temp_dir = temp_dir
        self.compiler_outputs = []
        self.return_code = None
        self.source_paths = {}
        self.backup_path = None
        self.new_folders = []

    def compile_and_repair(self, compile_targets_options):
        logging.info(f"max_attempts: {self.max_attempts}")
        """Attempt to compile and repair errors."""
        if self.compiler not in compile_targets_options:
            command = [self.compiler] + compile_targets_options
        else:
            command = compile_targets_options
        
        compiler_output, success = self.compile(command)
        if success:
            return

        self.compiler_outputs.append(compiler_output)
        first_error = compiler_output
        prompt_template = None

        while self.attempts < self.max_attempts:
            self.attempts += 1
            logging.info(f"Attempt {self.attempts} to repair the error.")
            command, repair_prompt_template = self.repair(
                command, compiler_output, prompt_template
            )
            if command:
                compiler_output, success = self.compile(command)
                self.compiler_outputs.append(compiler_output)
                if success:
                    logging.info("Successfully fixed the error.")
                    self.return_code = 0  # Reset return code to 0 on success
                    break

                pe = PromptEngine(repair_prompt_template)
                prompt_template = pe.update_template(
                    previous_compile_log=first_error,
                    current_compile_log=compiler_output,
                )
            else:
                break
        else:
            logging.info(f"Exceeded max attempts ({self.max_attempts}).")
            if self.backup_path:
                FileManager.restore_files(
                    source_paths=self.source_paths, backup_path=self.backup_path
                )
            # self.return_code = 1

    def compile(self, command):
        """Call the compiler and handle errors."""
        try:
            result = subprocess.run(command, capture_output=True, text=True, check=True)
            # print("compiler output:", result.stdout)
            return result.stdout, True
        except Exception as e:
            # print("compiler error:", e.stderr, file=sys.stderr)
            if not self.return_code:
                self.return_code = e.returncode
            return e.stderr, False

    def repair(self, command, error, prompt_template=None):
        """Repair compilation errors using suggestions from LLM."""
        compile_files = [e for e in command if e.endswith((".c", ".cpp"))]
        if not compile_files:
            logging.error("No valid source files for compilation.")
            return None, None

        target_code = {file: Path(file).read_text() for file in compile_files}
        code = self.check_relevant_files(error, target_code)

        # Repair using LLM
        lr = LLMRepair()
        llm_suggestions = lr.query_llm_for_fix(
            code, " ".join(command), error, prompt_template, get_model_id()
        )

        if not llm_suggestions:
            logging.debug("Failed to get LLM response.")
            return command, lr.get_repair_prompt_template()

        return (
            self.apply_llm_suggestions(command, llm_suggestions),
            lr.get_repair_prompt_template(),
        )

    def apply_llm_suggestions(self, command, llm_suggestions):
        """Apply LLM suggestions for compilation options and code changes."""
        if llm_suggestions.get("compiler_options"):
            command = self.repair_via_compiler_options(llm_suggestions)

        reason = llm_suggestions.get("reasoning", "NA")
        if llm_suggestions.get("code"):
            for target, code_content in llm_suggestions["code"].items():
                self.repair_via_code_alternation(target, code_content, reason=reason)

        return command

    def repair_via_compiler_options(self, llm_suggestions):
        """Apply LLM's suggested compiler options."""
        compiler_options = llm_suggestions["compiler_options"].split(" ")
        return (
            [self.compiler] + compiler_options
            if self.compiler not in compiler_options
            else compiler_options
        )

    def repair_via_code_alternation(self, target_file, repaired_code, reason):
        """Apply code changes suggested by LLM."""

        def backup(file_name):
            # Set up the backup path
            if not self.backup_path:
                self.backup_path = self.temp_dir
            backup_file_path = os.path.join(self.backup_path, file_name)

            if os.path.exists(target_file) and not os.path.exists(backup_file_path):
                self.source_paths[file_name] = target_file
                FileManager.backup_file(source=target_file, dest=backup_file_path)
            else:
                logging.debug(f"A backup of {target_file} already exists.")

        file_name = Path(target_file).name
        if file_name == "":
            logging.debug(f"Failed to extract the file name from {target_file}")

        source_code = (
            Path(target_file).read_text() if os.path.exists(target_file) else ""
        )

        try:
            if not is_auto_accept_code_change():
                # Reasons may be presented as a list
                if isinstance(reason, list):
                    reason = " ".join(reason)

                ret = code_dialog(
                    old_code=source_code,
                    new_code=repaired_code,
                    reason=reason,
                    cfile=target_file,
                )
            else:
                ret = True  # Auto-accept code changes

            if ret:
                backup(file_name)
                if not is_auto_accept_code_change():
                    logging.info("User accepted the suggested code changes.")

                # Just in case LLM suggest a new file
                self.new_folders = (
                    self.new_folders + FileManager.create_folders_for_path(target_file)
                )
                if (
                    not os.path.exists(target_file)
                    and target_file not in self.new_folders
                ):
                    self.new_folders.append(target_file)

                logging.debug(f"Applying LLM-suggested code changes to {target_file}")

                with open(target_file, "w") as file:
                    file.write(repaired_code)
                return True
            else:
                logging.info("User rejected the LLM suggested code changes.")
                self.clean_up()
                sys.exit(self.return_code)
        except Exception as e:
            logging.error(f"Failed to repair {target_file} via code alternation: {e}")
            traceback.print_exc()
            self.clean_up()
            return False

    def apply_code_changes(self, target_file, repaired_code):
        """Write repaired code back to the file."""
        logging.debug(f"Applying LLM-suggested code changes to {target_file}")
        Path(target_file).write_text(repaired_code)

    def check_relevant_files(self, compile_error, code):
        """Check for relevant files based on the compile error."""
        error_lines = compile_error.splitlines()
        for error in error_lines:
            if ":" in error:
                file_name = error.split(":", 1)[0]
                if file_name.endswith((".cpp", ".c", ".h")) and file_name not in code:
                    if os.path.exists(file_name):
                        code[file_name] = Path(file_name).read_text()
                        logging.debug(f"Also passing {file_name} to the compiler")
        return code

    def clean_up(self):
        """Clean up temporary files and directories."""
        if self.temp_dir:
            if is_development_mode() and self.backup_path:
                logging.debug("Restoring buggy file in development mode.")
                FileManager.restore_files(
                    source_paths=self.source_paths, backup_path=self.backup_path
                )

            if not is_development_mode():
                try:
                    os.remove(LOG_FILE)
                except:
                    pass

            # Remove temporary folders
            try:
                shutil.rmtree(self.temp_dir)
                for folder in self.new_folders:
                    shutil.rmtree(folder)
            except:
                pass


def main():
    compiler_driver = CompilerDriver()
    error_handler = CompilerErrorHandler(
        compiler_driver.max_attempts, compiler_driver.compiler, compiler_driver.temp_dir
    )
    signal.signal(
        signal.SIGINT, lambda sig, frame: exit_gracefully(error_handler=error_handler)
    )

    try:
        error_handler.compile_and_repair(sys.argv[1:])
    except Exception as e:
        logging.error(f"Error occurred: {e}")
    finally:
        error_handler.clean_up()
        # logging.info("return code: %s", error_handler.return_code)
        if error_handler.return_code:
             sys.exit(error_handler.return_code)
        else:
            return 0


def exit_gracefully(error_handler):
    logging.debug("\nExiting gracefully.")
    error_handler.clean_up()
    if error_handler.return_code:
        sys.exit(error_handler.return_code)
    else:
        sys.exit(0)


if __name__ == "__main__":
    main()
