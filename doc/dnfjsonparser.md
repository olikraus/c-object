# JSON Structural Analysis with SIMD

This document describes the high-performance structural character detection implemented in `test/dnfjsonparser.c` using SSE4.2 SIMD instructions.

## 1. Goal
The objective is to identify the eight critical structural characters in a JSON file while correctly handling string boundaries and escape sequences.

| Character | Hex Code | Purpose |
| :---: | :---: | :--- |
| `"` | `0x22` | String Delimiter |
| `,` | `0x2C` | Element Separator |
| `[` | `0x5B` | Array Start |
| `]` | `0x5D` | Array End |
| `{` | `0x7B` | Object Start |
| `}` | `0x7D` | Object End |
| `\` | `0x5C` | Escape Character |
| `:` | `0x3A` | Key/Value Separator |

## 2. SIMD Character Mapping
Each identified character is mapped to a specific bit in a metadata byte. This allows for parallel detection of all eight characters using a single bitwise AND.

| Bit Position | Value (Hex) | Character |
| :---: | :---: | :---: |
| Bit 0 | `0x01` | `"` |
| Bit 1 | `0x02` | `,` |
| Bit 2 | `0x04` | `[` |
| Bit 3 | `0x08` | `\` |
| Bit 4 | `0x10` | `]` |
| Bit 5 | `0x20` | `{` |
| Bit 6 | `0x40` | `}` |
| Bit 7 | `0x80` | `:` |

## 3. Nibble Lookup Tables
To perform detection in parallel across 16 bytes, we use the `_mm_shuffle_epi8` (Pshufb) instruction. This requires two lookup tables (High and Low nibble) that return the character's bitmask if the nibble matches.

### High Nibble Table `ht[16]`
Maps the upper 4 bits of a byte to a candidate bitmask.

| Index | Value | Characters |
| :---: | :---: | :--- |
| `0x2` | `0x03` | `"` (0x22), `,` (0x2C) |
| `0x3` | `0x80` | `:` (0x3A) |
| `0x5` | `0x1C` | `[` (0x5B), `\` (0x5C), `]` (0x5D) |
| `0x7` | `0x60` | `{` (0x7B), `}` (0x7D) |
| Others | `0x00` | No structural characters in this range |

### Low Nibble Table `lt[16]`
Maps the lower 4 bits of a byte to a candidate bitmask.

| Index (Hex) | Value | Characters |
| :---: | :---: | :--- |
| `0x2` | `0x01` | `"` (low nibble 0x2) |
| `0xA` | `0x80` | `:` (low nibble 0xA) |
| `0xB` | `0x24` | `[` (0xB), `{` (0xB) |
| `0xC` | `0x0A` | `,` (0xC), `\` (0xC) |
| `0xD` | `0x50` | `]` (0xD), `}` (0xD) |
| Others | `0x00` | No structural characters with these low bits |

## 4. Detection Logic
The detection for a character `c` is calculated as:
`result = ht[c >> 4] & lt[c & 0x0F]`

In SIMD, this is performed on 16 bytes simultaneously:
1. Load 16 bytes into `__m128i`.
2. Extract low nibbles and high nibbles.
3. Perform parallel lookups in `lt` and `ht`.
4. Bitwise `AND` the results.
5. If the resulting vector is not zero, at least one structural character was found.

### SIMD Implementation (C / SSE4.2)

```c
// 1. Load 16 bytes from the input buffer
__m128i data = _mm_loadu_si128((__m128i*)(buffer + pos));

// 2. Extract nibbles
__m128i low_nibbles = _mm_and_si128(data, low_mask); 
__m128i high_nibbles = _mm_and_si128(_mm_srli_epi16(data, 4), low_mask);

// 3. Parallel Lookup
__m128i low_lookup = _mm_shuffle_epi8(lt, low_nibbles);
__m128i high_lookup = _mm_shuffle_epi8(ht, high_nibbles);

// 4. Combine results
__m128i result = _mm_and_si128(low_lookup, high_lookup);

// 5. Detection check
// Using _mm_cmpeq_epi8 to handle bit 7 correctly
uint32_t found_mask = (uint32_t)~_mm_movemask_epi8(_mm_cmpeq_epi8(result, _mm_setzero_si128())) & 0xFFFF;

if (found_mask != 0) {
    // Process individual characters...
}
```

## 5. API and Recursive Descent Parser
The implementation is encapsulated in the `djp_t` structure and its associated `djp_` functions.

### The `djp_t` Structure
```c
typedef struct {
    char *buffer;        /* Raw JSON content. */
    size_t size;         /* Size of raw JSON. */
    uint32_t *pos_array; /* Structural token positions. */
    size_t pos_cnt;      /* Token count. */
    size_t token_idx;    /* Parser state. */
    int verbose;         /* Output control flag. */
} djp_t;
```

### Key API Functions
- `djp_init`: Initializes the structure.
- `djp_read_file`: High-performance file I/O into the buffer.
- `djp_scan`: SIMD-accelerated structural character identification.
- `djp_parse`: Recursive descent structural validation.
- `djp_destroy`: Resource cleanup.

### Parser Logic
- `djp_parse_value`: Dispatches to `djp_parse_object`, `djp_parse_array`, or handles strings/atoms.
- `djp_parse_object`: Validates string keys, colons, and comma-separated values.
- `djp_parse_array`: Validates comma-separated values.
- `djp_has_content_between_tokens`: Efficiently identifies atoms and detects illegal trailing commas.

## 6. Performance Benchmark (10MB JSON)

A comparison between `dnfjsonparser` and Node.js (`JSON.parse`) was conducted using a 10MB DNF JSON file.

| Platform / Tool | Task | Time | Throughput |
| :--- | :--- | :--- | :--- |
| **Node.js (V8)** | Full End-to-End | ~30.8 ms | ~326 MB/s |
| **dnfjsonparser (C)** | End-to-End (Read + Scan + Parse) | **~17.2 ms** | **~581 MB/s** |

### Breakdown of dnfjsonparser
- **File I/O (`fread`)**: ~6.0 ms
- **Structural Scan (SIMD)**: ~7.3 ms
- **Structural Validation (Parse)**: ~3.9 ms

### Analysis
- **Full Cycle Speed**: The modular C implementation is **1.8x faster** than Node.js's internal processing.
- **Modularity**: The transition to the `djp_t` API allows for clean orchestration and multiple parsing passes on the same scanned data.
