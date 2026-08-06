#!/usr/bin/env python3
# SPDX-License-Identifier: MIT
import argparse,json,sys
p=argparse.ArgumentParser(description='Summarize Bharat diagnostic JSON Lines without loading the input into memory.');p.add_argument('jsonl');a=p.parse_args(); counts={}; malformed=0
with open(a.jsonl,encoding='utf-8') as f:
 for line in f:
  try:e=json.loads(line); k=str(e.get('event_type','unknown')); counts[k]=counts.get(k,0)+1
  except (ValueError,TypeError):malformed+=1
json.dump({'event_types':counts,'malformed':malformed},sys.stdout,sort_keys=True);print();sys.exit(2 if malformed else 0)
