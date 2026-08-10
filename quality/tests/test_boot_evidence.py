# SPDX-License-Identifier: MIT
import importlib.util,pathlib,unittest
P=pathlib.Path(__file__).parents[2]; spec=importlib.util.spec_from_file_location('e',P/'tools/testing/check_boot_evidence.py');e=importlib.util.module_from_spec(spec);spec.loader.exec_module(e)
class EvidenceTests(unittest.TestCase):
 def check(self,name,code,result):
  report,got=e.parse(P/'quality/fixtures/diag'/name,'x86_64','desktop_headless');self.assertEqual((got,report['result']),(code,result))
 def test_success(self):self.check('successful_x86_64_boot.log',0,'succeeded')
 def test_failure(self):self.check('userspace_entry_failure.log',1,'failed')
 def test_missing(self):self.check('missing_stage.log',3,'incomplete')
 def test_duplicate(self):self.check('duplicate_stage.log',2,'malformed')
 def test_malformed(self):self.check('malformed_event.log',2,'malformed')
if __name__=='__main__':unittest.main()
