import unittest
import copy
from unittest.mock import patch
import sys
import os

# Ensure tools can be imported
sys.path.insert(0, os.path.dirname(os.path.dirname(os.path.abspath(__file__))))

from tools.abi.check_idl_compat import check_idl_compat

class TestIdlCompat(unittest.TestCase):
    def setUp(self):
        self.baseline = {
            "svc": {
                "id": 1,
                "rpcs": [{"name": "A", "req": "ReqA", "resp": "RespA"}],
                "enums": {
                    "E1": [{"name": "V1", "value": 1}]
                },
                "messages": {
                    "M1": [{"name": "F1", "type": "u32"}]
                }
            }
        }
        self.current = copy.deepcopy(self.baseline)

    @patch('tools.abi.common.report_error')
    def test_append_only_passes(self, mock_report):
        self.current["svc"]["rpcs"].append({"name": "B", "req": "ReqB", "resp": "RespB"})
        self.current["svc"]["enums"]["E1"].append({"name": "V2", "value": 2})
        self.current["svc"]["messages"]["M1"].append({"name": "F2", "type": "u32"})

        self.assertTrue(check_idl_compat(self.baseline, self.current))
        mock_report.assert_not_called()

    @patch('tools.abi.common.report_error')
    def test_remove_rpc_fails(self, mock_report):
        self.current["svc"]["rpcs"] = []
        self.assertFalse(check_idl_compat(self.baseline, self.current))
        mock_report.assert_called_with("Service svc has fewer RPCs. RPCs can only be appended.")

    @patch('tools.abi.common.report_error')
    def test_remove_service_fails(self, mock_report):
        self.current = {}
        self.assertFalse(check_idl_compat(self.baseline, self.current))
        mock_report.assert_called_with("Service svc was removed. IDL deletions are forbidden.")

    @patch('tools.abi.common.report_error')
    def test_change_id_fails(self, mock_report):
        self.current["svc"]["id"] = 2
        self.assertFalse(check_idl_compat(self.baseline, self.current))
        mock_report.assert_called_with("Service svc ID changed from 1 to 2.")

    @patch('tools.abi.common.report_error')
    def test_reorder_rpc_fails(self, mock_report):
        self.baseline["svc"]["rpcs"].append({"name": "B", "req": "ReqB", "resp": "RespB"})
        self.current = copy.deepcopy(self.baseline)

        # Swap RPCs
        self.current["svc"]["rpcs"] = [self.baseline["svc"]["rpcs"][1], self.baseline["svc"]["rpcs"][0]]
        self.assertFalse(check_idl_compat(self.baseline, self.current))
        mock_report.assert_any_call("Service svc RPC 0 changed from A to B. Reordering/renaming is forbidden.")

    @patch('tools.abi.common.report_error')
    def test_change_rpc_req_fails(self, mock_report):
        self.current["svc"]["rpcs"][0]["req"] = "ReqA2"
        self.assertFalse(check_idl_compat(self.baseline, self.current))
        mock_report.assert_called_with("Service svc RPC A req changed from ReqA to ReqA2.")

    @patch('tools.abi.common.report_error')
    def test_change_enum_value_fails(self, mock_report):
        self.current["svc"]["enums"]["E1"][0]["value"] = 2
        self.assertFalse(check_idl_compat(self.baseline, self.current))
        mock_report.assert_called_with("Enum value V1 changed from 1 to 2 in E1.")

    @patch('tools.abi.common.report_error')
    def test_remove_enum_fails(self, mock_report):
        del self.current["svc"]["enums"]["E1"]
        self.assertFalse(check_idl_compat(self.baseline, self.current))
        mock_report.assert_called_with("Enum E1 in service svc was removed.")

    @patch('tools.abi.common.report_error')
    def test_remove_message_fails(self, mock_report):
        del self.current["svc"]["messages"]["M1"]
        self.assertFalse(check_idl_compat(self.baseline, self.current))
        mock_report.assert_called_with("Message/Struct M1 in service svc was removed.")

    @patch('tools.abi.common.report_error')
    def test_remove_message_field_fails(self, mock_report):
        self.current["svc"]["messages"]["M1"] = []
        self.assertFalse(check_idl_compat(self.baseline, self.current))
        mock_report.assert_called_with("Message M1 has fewer fields. Fields can only be appended.")

    @patch('tools.abi.common.report_error')
    def test_change_message_field_type_fails(self, mock_report):
        self.current["svc"]["messages"]["M1"][0]["type"] = "u64"
        self.assertFalse(check_idl_compat(self.baseline, self.current))
        mock_report.assert_called_with("Message M1 field F1 type changed from u32 to u64.")

if __name__ == "__main__":
    unittest.main()
