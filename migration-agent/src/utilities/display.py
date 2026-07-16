"""
This defines uitility functions for display and user interaction
"""

import curses
import difflib
import textwrap
import sys
from pygments import highlight
from pygments.lexers import CppLexer
from pygments.formatters import TerminalFormatter
from global_config import *


def diff_strings(old_code, new_code):
    # Split the code into lines for better comparison
    old_lines = old_code.splitlines()
    new_lines = new_code.splitlines()
    diff = difflib.unified_diff(old_lines, new_lines)
    return list(diff)


def show_code_dialog(code_message, code, reason_message, reason):
    """
    Displays a dialog with the suggested C/C++ code snippet, the reason for the change and asks the user for a Yes/No response
    by typing 'y' for Yes or 'n' for No. The window adjusts its size based on the code length.
    It also supports scrolling if the content exceeds the window height.

    :param code_message: The message to display in the first dialog.
    :param code: The C/C++ code snippet to display in the dialog.
    :param reason_message: The message to display in the second dialog.
    :param reason: The reason for the change
    :return: True or False based on user input.
    """

    def show_code_win(curses, win_width, begin_y, begin_x, code_lines):
        win_height = max(15, len(code_lines) + 6)
        code_win = curses.newwin(win_height, win_width, begin_y, begin_x)
        code_win.box()
        # Create a scrollable window by enabling scrolling
        code_win.scrollok(True)

        # Add the code message and print it with background color
        code_win.attron(curses.color_pair(1))  # Apply background color pair
        code_win.addstr(0, 2, code_message)
        code_win.attroff(curses.color_pair(1))  # Turn off background color

        # Add diff conent to the code window
        for i, line in enumerate(code_lines):
            line = textwrap.fill(line, win_width - 2, placeholder="...")
            y = 2 + i
            x = 1
            if line.startswith("-"):  # Lines removed from the original text
                code_win.addstr(
                    y, x, line, curses.color_pair(3)
                )  # Red for removed lines
            elif line.startswith("+"):  # Lines added in the modified text
                code_win.addstr(
                    y, x, line, curses.color_pair(4)
                )  # Red for removed lines
            else:
                code_win.addstr(y, x, line)  # Normal text for unchanged lines

        code_win.refresh()

        return code_win, win_height

    def show_reason_win(curses, win_width, begin_y, begin_x, reason):
        # Create the second window below the first one
        reason_lines = reason.splitlines()
        win_height = max(5, min(15, len(reason_lines) + 6))
        reason_win = curses.newwin(win_height, win_width, begin_y, begin_x)
        reason_win.box()

        # Add the reasoning message and print it with background color
        reason_win.attron(curses.color_pair(1))  # Apply background color pair
        reason_win.addstr(0, 2, reason_message)
        reason_win.attroff(curses.color_pair(1))  # Turn off background color

        # Print the second message inside the second window
        for i, line in enumerate(reason_lines):
            reason_win.addstr(
                1 + i, 2, line.rstrip()
            )  # Print starting from row 2 to leave space for the box

        reason_win.refresh()

        return reason_win, win_height

    def show_action_win(curses, win_width, begin_y, begin_x, prompt):
        # Create the third window below the reaon window
        action_win_height = 5
        action_win = curses.newwin(action_win_height, win_width, begin_y, begin_x)
        action_win.box()

        # Add the action message and print it with background color
        action_win.attron(curses.color_pair(1))  # Apply background color pair
        action_win.addstr(0, 2, "Action")
        action_win.attroff(curses.color_pair(1))  # Turn off background color

        # Ask if the user wants to accept the change
        action_win.attron(
            curses.color_pair(3) | curses.A_BOLD
        )  # Apply red color and bold to the prompt
        action_win.addstr(2, 2, prompt)
        action_win.attroff(curses.color_pair(3) | curses.A_BOLD)  # Turn off red color
        action_win.addstr(2, 2 + len(prompt) + 1, "_", curses.A_BLINK)
        action_win.refresh()

        while True:
            # Wait for user input
            key = action_win.getch()

            # Handle Yes (y) or No (n) response
            if key == ord("y"):  # 'y' key for Yes
                return True
            elif key == ord("n"):  # 'n' key for No
                return False

    def init_curses():
        # Initialize curses color functionality
        curses.start_color()
        curses.init_pair(
            1, curses.COLOR_WHITE, curses.COLOR_BLUE
        )  # White text on Blue background
        curses.init_pair(
            2, curses.COLOR_YELLOW, curses.COLOR_BLACK
        )  # Yellow text on Black background
        curses.init_pair(
            3, curses.COLOR_RED, curses.COLOR_BLACK
        )  # Red text on Black background for the prompt
        curses.init_pair(
            4, curses.COLOR_GREEN, curses.COLOR_BLACK
        )  # Green for added lines
        return curses

    def show_dialog(stdscr):
        curses = init_curses()
        prompt = "Would you accept the LLM suggested code? Changes will be rolled back if we can't fix it (y/n):"

        # Clear screen
        # stdscr.clear()

        # Get the height and width of the terminal
        h, w = stdscr.getmaxyx()

        if isinstance(code, str):
            code_lines = code.splitlines()
        else:
            code_lines = code

        # Truncate the code lines
        if len(code_lines) > h - 2:
            code_lines = code_lines[0 : h - 2]

        # Set up the window height and width
        max_line_length = max(max(len(line) for line in code_lines), len(prompt))
        win_width = (
            max(50, max_line_length + 10) + 4
        )  # Ensure a minimum width of 50 columns

        # Wrap the code if needed
        if win_width > w:
            win_width = w

        begin_y = 4
        begin_x = (w - win_width) // 2
        _, code_win_height = show_code_win(
            curses, win_width, begin_y=begin_y, begin_x=begin_x, code_lines=code_lines
        )
        begin_y = begin_y + code_win_height
        _, reason_win_height = show_reason_win(
            curses, win_width, begin_y=begin_y, begin_x=begin_x, reason=reason
        )
        begin_y = begin_y + reason_win_height
        return show_action_win(
            curses, win_width, begin_y=begin_y, begin_x=begin_x, prompt=prompt
        )

    return curses.wrapper(show_dialog)


# Show a diaglog to ask if the user want to accept the changes.
def code_dialog(old_code: str, new_code: str, reason: str, cfile: str):
    code_diff = diff_strings(old_code=old_code, new_code=new_code)
    if code_diff:
        return show_code_dialog(
            code_message="Suggested code changes for " + cfile,
            code=code_diff,
            reason_message="Reasons",
            reason=reason,
        )
