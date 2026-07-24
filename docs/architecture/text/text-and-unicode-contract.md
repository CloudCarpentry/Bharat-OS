# Bharat-OS Text and Unicode Contract

## 1. Goals
- **UTF-8 at OS/user/service boundaries**: Standardize text as UTF-8 encoded across all UAPI/service interfaces.
- **Byte-safe kernel**: Keep the kernel core minimal and ASCII-safe, treating strings as length-bounded byte sequences but tolerant of UTF-8.
- **Profile-driven rendering capabilities**: Support varying levels of text rendering from simple ASCII fallbacks on tiny devices to full shaping on desktops.
- **Indian-language readiness**: Ensure foundations are in place for Devanagari, Gujarati, Kannada, Tamil, Telugu, Bengali, Malayalam, Gurmukhi, and Odia.

## 2. Non-goals
- **No full Unicode normalization in kernel**: Normalization policy belongs in services/stacks.
- **No shaping in kernel**: Complex text shaping (ligatures, matra positioning) is prohibited in the kernel.
- **No mandatory large font tables for tiny/RT profile**: Maintain small memory footprint for constrained devices.

## 3. Layer Ownership
- **Kernel**: Minimal mechanism for safe console logging, replacement glyphs, and byte-safe string handling. No policy-heavy text work.
- **Core Library (`core/lib/text`)**: Canonical strict UTF-8 validation, decoding, encoding, and basic cell-width logic.
- **System Services (`services/system/console`, `shell`)**: Richer behavior such as cursor movement by grapheme, command history, and sanitized display.
- **Media Stacks (`stacks/media/text`)**: Font management, glyph caching, and complex text shaping (e.g., HarfBuzz integration).
- **User Space (`user/ui`)**: Final GUI rendering and user-facing text policies.

## 4. UAPI Contracts
### Text Span
```c
typedef enum bh_text_encoding {
    BH_TEXT_ENCODING_ASCII = 0,
    BH_TEXT_ENCODING_UTF8  = 1,
    BH_TEXT_ENCODING_BYTES = 2,
} bh_text_encoding_t;

typedef struct bh_text_span {
    const char *data;
    size_t len;
    bh_text_encoding_t encoding;
} bh_text_span_t;
```

### Input Events
```c
typedef struct bh_text_input_event {
    uint64_t timestamp_ns;
    uint32_t codepoint;
    uint32_t modifiers;
    uint8_t  utf8[4];
    uint8_t  utf8_len;
} bh_text_input_event_t;
```

## 5. UTF-8 Validation Rules
Bharat-OS enforces strict UTF-8 validation. The following are REJECTED:
- Overlong encodings (non-shortest forms).
- Surrogate range (U+D800–U+DFFF).
- Codepoints > U+10FFFF.
- 5-byte and 6-byte historical UTF-8 sequences.
- Truncated sequences or invalid continuation bytes.

## 6. Console Behavior
- **Serial Console**: Transparently passes valid UTF-8 bytes; strips or replaces invalid sequences.
- **Framebuffer Console**: Renders available glyphs; falls back to a replacement glyph (e.g., `□` or `?`) for unknown or invalid sequences.
- **Panic Path**: Must remain extremely robust; degrades to ASCII-safe rendering (escaping non-ASCII if necessary) to ensure debug information is never lost due to encoding errors.

## 7. Profile Matrix
| Profile | Required Support |
| :--- | :--- |
| **Tiny / RT** | ASCII fallback, minimal UTF-8 tolerance. |
| **Server** | UTF-8 logs, CLI, and file names. |
| **Desktop / Mobile** | Full UTF-8, Indian language shaping, rich fonts. |
| **Automotive** | Controlled UTF-8 set for safety messages and UI. |

## 8. Test Requirements
- **Strict Validation**: Exhaustive tests for all rejected UTF-8 forms.
- **Indian Scripts**: Verification of zero-width combining mark handling for target scripts.
- **Sanitization**: Ensure control/escape sequences are properly handled in console paths.
