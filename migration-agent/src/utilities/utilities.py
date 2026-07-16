"""
This defines the utility functions for the compiler driver
"""

import shutil
import pygments
from pygments import highlight
from pygments.lexers import CppLexer
from pygments.formatters import TerminalFormatter
from colorama import init, Back, Fore, Style
import textwrap




def pretty_print_dialog(title: str, message: str, auto_wrap=True) -> str:
    # Get the current terminal size
    columns, _ = shutil.get_terminal_size()
    if auto_wrap:
        message = textwrap.fill(message, width=columns - 6)
    border = "+" * min(
        max(len(message), len(title) + 6), columns
    )  # Make border length based on the message length
    dialog = f"\n{border}\n"
    dialog += f"+ {title.center(len(title))} +\n"  # Title centered
    dialog += f"+ {message.center(len(message))} +\n"  # Message centered
    dialog += f"{border}"
    return dialog


# Initialize colorama (needed for Windows)
init(autoreset=True)


def pretty_print_code(title, code):
    columns, _ = shutil.get_terminal_size()
    border = "+" * columns
    highlighted_code = highlight(code, CppLexer(), TerminalFormatter())
    return (
        f"\n{border}"
        + Back.GREEN
        + Fore.BLACK
        + Style.BRIGHT
        + highlighted_code
        + f"{border}\n"
    )



