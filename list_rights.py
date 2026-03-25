import re

content = open("kernel/include/capability.h").read()
# match CAP_RIGHT_.* = (1U << X)
matches = re.findall(r'CAP_RIGHT_[A-Z_]+\s*=\s*\((1ULL?|1U) << (\d+)\)', content)

# Check which numbers from 0 to 32 are not used
used = set([int(m[1]) for m in matches])
missing = [i for i in range(33) if i not in used]

print("Used bits:", sorted(list(used)))
print("Free bits:", missing)

# Now count total rights
rights = re.findall(r'(CAP_RIGHT_[A-Z_]+)\s*=\s*\(.* << (\d+)\)', content)
print("Total rights:", len(rights))
for r, bit in rights:
    print(r, bit)
