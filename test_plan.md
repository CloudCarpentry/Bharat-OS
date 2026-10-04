1. Add new tests for `bh_utf8_encode` in `quality/tests/core/text/test_utf8.c`. The test is already present but maybe the file I am reading from the user's prompt means it's missing in some other file, or that it doesn't cover all cases? Wait, the prompt says:

"Missing tests for bh_utf8_encode"
"Current Code:
```c
size_t bh_utf8_encode(uint32_t cp, char out[4]) {
...
```
Rationale: Very simple pure function translating a code point into a UTF-8 character buffer. Missing coverage for surrogates, normal encoding, and boundary conditions can be easily added."

Wait, `test_utf8_encode` IS in the `quality/tests/core/text/test_utf8.c` file. Let me look closely.
