"""
Prompt engineering utilities
"""

import re


class PromptEngine:
    def __init__(self, prompt_template):
        self.prompt_template = prompt_template
        self.redefined_identifiers = []
        self.missing_files = []

    def _extract_identifier(self, text):
        """Retrieve the identifier from the compilation message."""
        match = re.search(r"'(.*?)'", text)
        return match.group(1) if match else None

    def _check_and_get_identifier(self, pattern, error_message):
        """Check if a pattern exists in the error message and extract the identifier."""
        if pattern in error_message:
            return self._extract_identifier(error_message)
        return None

    def _check_for_redefinition(self, error_message):
        """Check if the error message indicates a redefinition."""
        identifier = self._check_and_get_identifier("redefinition of", error_message)
        if identifier:
            self.redefined_identifiers.append(identifier)

    def _check_for_missing_file(self, error_message):
        """Check if the error message indicates a missing file."""
        identifier = self._check_and_get_identifier("file not found", error_message)
        if identifier:
            self.missing_files.append(identifier)

    def _check_new_compilation_error(self, previous_log, current_error):
        """Check if a new error is introduced by the LLM."""
        match = re.search(r"error:\s*(.*)", current_error, re.IGNORECASE)
        if match:
            error_message = match.group(1)
            if error_message not in previous_log:
                self._check_for_redefinition(error_message)
                self._check_for_missing_file(error_message)

    def update_template(self, previous_compile_log, current_compile_log):
        """Update the prompt template based on the current compilation log."""
        logs = current_compile_log.splitlines()
        for line in logs:
            self._check_new_compilation_error(previous_compile_log, line)

        prompt_suffix = ""
        if self.redefined_identifiers:
            redefine_str = ", ".join(self.redefined_identifiers)
            prompt_suffix += f" Don't provide definitions for the following data structures or classes as they have already been defined elsewhere: ```{redefine_str}```"
        if self.missing_files:
            missing_files_str = ", ".join(self.missing_files)
            prompt_suffix += f" Don't refer to the following files, as they don't exist: ```{missing_files_str}```"

        return self.prompt_template + prompt_suffix
