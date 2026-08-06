# SPDX-License-Identifier: MIT
import json,pathlib,unittest
from jsonschema import Draft202012Validator
P=pathlib.Path(__file__).parents[2]
class SchemaTests(unittest.TestCase):
 def test_schemas(self):
  for p in (P/'contracts/evidence').glob('*.json'): Draft202012Validator.check_schema(json.loads(p.read_text()))
 def test_invalid_version(self):
  s=json.loads((P/'contracts/evidence/bharat_health_snapshot.schema.json').read_text()); self.assertFalse(Draft202012Validator(s).is_valid({'schema_version':2,'state':'HEALTHY','source_id':1,'timestamp_ns':2,'uptime_ns':1,'dropped_events':0}))
if __name__=='__main__':unittest.main()
