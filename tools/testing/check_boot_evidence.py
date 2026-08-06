#!/usr/bin/env python3
# SPDX-License-Identifier: MIT
"""Convert bounded Bharat serial-log evidence into deterministic boot evidence."""
import argparse, json, re, sys
STAGES=['BOOT_KERNEL_ENTRY','BOOT_PLATFORM_READY','BOOT_MEMORY_READY','BOOT_SCHEDULER_READY','BOOT_INIT_MODULE_FOUND','BOOT_INIT_ELF_VALIDATED','BOOT_INIT_SEGMENTS_MAPPED','BOOT_INIT_STACK_READY','BOOT_INIT_THREAD_CREATED','BOOT_INIT_THREAD_ENQUEUED','BOOT_USER_ENTRY_PREPARED','BOOT_USER_ENTRY_ATTEMPTED','BOOT_USERSPACE_REACHED','BOOT_SERVICE_HANDOFF','BOOT_COMPLETE']
RESULTS={'STARTED','SUCCEEDED','FAILED','SKIPPED'}
PAT=re.compile(r'^\[BH-DIAG\] BOOT_STAGE stage=([A-Z0-9_]+) status=([A-Z]+)(?: timestamp_ns=([0-9]+))?$')
FAIL_PAT=[(re.compile(r'\bmodule not found\b',re.I),'MODULE_NOT_FOUND'),(re.compile(r'\bELF invalid\b',re.I),'ELF_INVALID'),(re.compile(r'\bpage fault\b',re.I),'PAGE_FAULT'),(re.compile(r'\bgeneral protection fault\b',re.I),'GENERAL_PROTECTION_FAULT'),(re.compile(r'\billegal instruction\b',re.I),'ILLEGAL_INSTRUCTION'),(re.compile(r'\bpanic\b',re.I),'PANIC'),(re.compile(r'\bboot timeout\b',re.I),'BOOT_TIMEOUT')]
MAX_LINE=4096; MAX_EVIDENCE=128

def parse(path,arch,profile):
 states={s:'NOT_OBSERVED' for s in STAGES}; seen={}; raw=[]; warnings=[]; malformed=False; order=-1; failure='NONE'; first_failed=None
 with open(path,'r',encoding='utf-8',errors='replace') as f:
  for no,line in enumerate(f,1):
   if no>1_000_000: warnings.append('input truncated at 1000000 lines'); break
   line=line.rstrip('\r\n')[:MAX_LINE]; m=PAT.fullmatch(line)
   if m:
    stage,status,ts=m.groups()
    if stage not in states: warnings.append(f'line {no}: unknown future stage {stage}'); continue
    if status not in RESULTS: malformed=True; warnings.append(f'line {no}: invalid stage status'); continue
    idx=STAGES.index(stage)
    if stage in seen: malformed=True; warnings.append(f'line {no}: duplicate marker {stage}')
    if idx<order: malformed=True; warnings.append(f'line {no}: out-of-order marker {stage}')
    order=max(order,idx); seen[stage]=no; states[stage]=status
    if status=='FAILED' and first_failed is None: first_failed=stage
    if len(raw)<MAX_EVIDENCE: raw.append({'line':no,'text':line})
   elif line.startswith('[BH-DIAG] BOOT_STAGE'):
    malformed=True; warnings.append(f'line {no}: malformed structured marker')
   for pattern,category in FAIL_PAT:
    if pattern.search(line) and failure=='NONE': failure=category; raw.append({'line':no,'text':line[:MAX_LINE]})
 if first_failed and failure=='NONE': failure='UNKNOWN'
 complete=states['BOOT_COMPLETE']=='SUCCEEDED'
 missing=not complete and first_failed is None and failure=='NONE'
 result='succeeded' if complete and not malformed else ('malformed' if malformed else 'failed' if first_failed or failure!='NONE' else 'incomplete')
 last=next((s for s in reversed(STAGES) if states[s]=='SUCCEEDED'),None)
 return {'schema_version':1,'architecture':arch,'profile':profile,'result':result,'last_successful_stage':last,'first_failed_stage':first_failed,'failure_category':failure if not missing else 'EVIDENCE_INCOMPLETE','failure_evidence':'positive' if failure!='NONE' or first_failed else 'missing','raw_evidence_lines':raw[:MAX_EVIDENCE],'stages':[{'stage':s,'result':states[s]} for s in STAGES],'warnings':warnings}, (2 if malformed else 0 if complete else 1 if first_failed or failure!='NONE' else 3)
def main():
 p=argparse.ArgumentParser(description=__doc__,epilog='Exit codes: 0 satisfied, 1 failure, 2 malformed, 3 missing marker, 4 unsupported configuration, 5 internal failure.')
 p.add_argument('--log',required=True); p.add_argument('--arch',required=True,choices=['x86_64','aarch64','riscv64','arm32','riscv32']); p.add_argument('--profile',required=True); p.add_argument('--json-out')
 a=p.parse_args()
 try: report,code=parse(a.log,a.arch,a.profile); encoded=json.dumps(report,indent=2,sort_keys=True)+'\n'; print(f"boot evidence: {report['result']}; last={report['last_successful_stage']}; failure={report['failure_category']}",file=sys.stderr)
 except (OSError,UnicodeError) as e: print(f'boot evidence error: {e}',file=sys.stderr); return 5
 if a.json_out:
  try:
   with open(a.json_out,'w',encoding='utf-8') as f:f.write(encoded)
  except OSError as e: print(f'boot evidence output error: {e}',file=sys.stderr); return 5
 else: sys.stdout.write(encoded)
 return code
if __name__=='__main__':sys.exit(main())
