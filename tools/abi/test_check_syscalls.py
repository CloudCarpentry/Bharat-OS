import sys
import os
import unittest
from unittest.mock import patch

# Add the directory containing the test file to sys.path
sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))

import check_syscalls

class TestValidateDuplicates(unittest.TestCase):
    @patch('common.report_error')
    def test_empty_entries(self, mock_report):
        entries = []
        self.assertTrue(check_syscalls.validate_duplicates(entries))
        mock_report.assert_not_called()

    @patch('common.report_error')
    def test_no_duplicates(self, mock_report):
        entries = [("SYS_read", 1), ("SYS_write", 2)]
        self.assertTrue(check_syscalls.validate_duplicates(entries))
        mock_report.assert_not_called()

    @patch('common.report_error')
    def test_duplicate_number(self, mock_report):
        entries = [("SYS_read", 1), ("SYS_write", 1)]
        self.assertFalse(check_syscalls.validate_duplicates(entries))
        mock_report.assert_called_with("Duplicate syscall number 1 used by SYS_read and SYS_write.")

    @patch('common.report_error')
    def test_duplicate_name(self, mock_report):
        entries = [("SYS_read", 1), ("SYS_read", 2)]
        self.assertFalse(check_syscalls.validate_duplicates(entries))
        mock_report.assert_called_with("Duplicate syscall name SYS_read used for numbers 1 and 2.")

    @patch('common.report_error')
    def test_duplicate_name_and_number(self, mock_report):
        entries = [("SYS_read", 1), ("SYS_read", 1)]
        self.assertFalse(check_syscalls.validate_duplicates(entries))
        mock_report.assert_any_call("Duplicate syscall number 1 used by SYS_read and SYS_read.")
        mock_report.assert_any_call("Duplicate syscall name SYS_read used for numbers 1 and 1.")

    @patch('common.report_error')
    def test_multiple_duplicates(self, mock_report):
        entries = [("SYS_read", 1), ("SYS_write", 1), ("SYS_open", 1)]
        self.assertFalse(check_syscalls.validate_duplicates(entries))
        mock_report.assert_any_call("Duplicate syscall number 1 used by SYS_read and SYS_write.")
        mock_report.assert_any_call("Duplicate syscall number 1 used by SYS_write and SYS_open.")

class TestCheckSyscalls(unittest.TestCase):
    @patch('check_syscalls.get_all_entries')
    @patch('common.report_error')
    def test_valid_syscalls(self, mock_report, mock_get_entries):
        baseline = {"1": "SYS_read", "2": "SYS_write"}
        current = {"1": "SYS_read", "2": "SYS_write", "3": "SYS_open"}
        mock_get_entries.return_value = [("SYS_read", 1), ("SYS_write", 2), ("SYS_open", 3)]

        self.assertTrue(check_syscalls.check_syscalls(baseline, current))
        mock_report.assert_not_called()

    @patch('check_syscalls.get_all_entries')
    @patch('common.report_error')
    def test_removed_syscall(self, mock_report, mock_get_entries):
        baseline = {"1": "SYS_read", "2": "SYS_write"}
        current = {"1": "SYS_read"}
        mock_get_entries.return_value = [("SYS_read", 1)]

        self.assertFalse(check_syscalls.check_syscalls(baseline, current))
        mock_report.assert_called_with("Syscall SYS_write (2) was removed. Syscall deletions are forbidden.")

    @patch('check_syscalls.get_all_entries')
    @patch('common.report_error')
    def test_renamed_syscall(self, mock_report, mock_get_entries):
        baseline = {"1": "SYS_read"}
        current = {"1": "SYS_write"}
        mock_get_entries.return_value = [("SYS_write", 1)]

        self.assertFalse(check_syscalls.check_syscalls(baseline, current))
        mock_report.assert_called_with("Syscall 1 was changed from SYS_read to SYS_write. Renumbering/Renaming is forbidden.")

    @patch('check_syscalls.get_all_entries')
    @patch('common.report_error')
    def test_check_syscalls_with_duplicates(self, mock_report, mock_get_entries):
        baseline = {"1": "SYS_read", "2": "SYS_write"}
        current = {"1": "SYS_read", "2": "SYS_write", "3": "SYS_open", "4": "SYS_close"}
        # Return duplicate number
        mock_get_entries.return_value = [("SYS_read", 1), ("SYS_write", 2), ("SYS_open", 3), ("SYS_close", 3)]

        self.assertFalse(check_syscalls.check_syscalls(baseline, current))
        mock_report.assert_called_with("Duplicate syscall number 3 used by SYS_open and SYS_close.")

    @patch('check_syscalls.get_all_entries')
    @patch('common.report_error')
    def test_check_syscalls_with_duplicate_names(self, mock_report, mock_get_entries):
        baseline = {"1": "SYS_read", "2": "SYS_write"}
        current = {"1": "SYS_read", "2": "SYS_write", "3": "SYS_open", "4": "SYS_open"}
        # Return duplicate name
        mock_get_entries.return_value = [("SYS_read", 1), ("SYS_write", 2), ("SYS_open", 3), ("SYS_open", 4)]

        self.assertFalse(check_syscalls.check_syscalls(baseline, current))
        mock_report.assert_called_with("Duplicate syscall name SYS_open used for numbers 3 and 4.")

    @patch('check_syscalls.get_all_entries')
    @patch('common.report_error')
    def test_check_syscalls_with_duplicate_names_and_numbers(self, mock_report, mock_get_entries):
        baseline = {"1": "SYS_read", "2": "SYS_write"}
        current = {"1": "SYS_read", "2": "SYS_write", "3": "SYS_open", "4": "SYS_close"}
        # Return duplicate name and duplicate number
        mock_get_entries.return_value = [("SYS_read", 1), ("SYS_write", 2), ("SYS_open", 3), ("SYS_close", 3), ("SYS_write", 4)]

        self.assertFalse(check_syscalls.check_syscalls(baseline, current))
        mock_report.assert_any_call("Duplicate syscall number 3 used by SYS_open and SYS_close.")
        mock_report.assert_any_call("Duplicate syscall name SYS_write used for numbers 2 and 4.")

    @patch('check_syscalls.get_all_entries')
    @patch('common.report_error')
    def test_check_syscalls_unsorted(self, mock_report, mock_get_entries):
        baseline = {"1": "SYS_read", "2": "SYS_write"}
        current = {"1": "SYS_read", "2": "SYS_write", "3": "SYS_open"}
        # Unsorted order of entries
        mock_get_entries.return_value = [("SYS_write", 2), ("SYS_read", 1), ("SYS_open", 3)]

        self.assertTrue(check_syscalls.check_syscalls(baseline, current))
        mock_report.assert_not_called()

if __name__ == '__main__':
    unittest.main()
