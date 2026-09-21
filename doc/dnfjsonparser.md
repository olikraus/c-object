# JSON Structural Analysis with SIMD

This document describes the high-performance structural character detection implemented in `test/dnfjsonparser.c` using SSSE3 SIMD instructions.

## 1. Goal
The objective is to identify the seven critical structural characters in a JSON file while correctly handling string boundaries and escape sequences.

| Character | Hex Code | Purpose |
| :---: | :---: | :--- |
| `"` | `0x22` | String Delimiter |
| `,` | `0x2C` | Element Separator |
| `[` | `0x5B` | Array Start |
| `]` | `0x5D` | Array End |
| `{` | `0x7B` | Object Start |
| `}` | `0x7D` | Object End |
| `\` | `0x5C` | Escape Character |

## 2. SIMD Character Mapping
Each identified character is mapped to a specific bit in a metadata byte. This allows for parallel detection of all seven characters using a single bitwise AND.

| Bit Position | Value (Hex) | Character |
| :---: | :---: | :---: |
| Bit 0 | `0x01` | `"` |
| Bit 1 | `0x02` | `,` |
| Bit 2 | `0x04` | `[` |
| Bit 3 | `0x08` | `\` |
| Bit 4 | `0x10` | `]` |
| Bit 5 | `0x20` | `{` |
| Bit 6 | `0x40` | `}` |

## 3. Nibble Lookup Tables
To perform detection in parallel across 16 bytes, we use the `_mm_shuffle_epi8` (Pshufb) instruction. This requires two lookup tables (High and Low nibble) that return the character's bitmask if the nibble matches.

### High Nibble Table `ht[16]`
Maps the upper 4 bits of a byte to a candidate bitmask.

| Index | Value | Characters |
| :---: | :---: | :--- |
| `0x2` | `0x03` | `"` (0x22), `,` (0x2C) |
| `0x5` | `0x1C` | `[` (0x5B), `\` (0x5C), `]` (0x5D) |
| `0x7` | `0x60` | `{` (0x7B), `}` (0x7D) |
| Others | `0x00` | No structural characters in this range |

### Low Nibble Table `lt[16]`
Maps the lower 4 bits of a byte to a candidate bitmask.

| Index | Value | Characters |
| :---: | :---: | :--- |
| `0x2` | `0x01` | `"` (low nibble 0x2) |
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

### SIMD Implementation (C / SSSE3)

```c
// 1. Load 16 bytes from the input buffer
__m128i data = _mm_loadu_si128((__m128i*)(buffer + pos));

// 2. Extract nibbles
// mask is 0x0F for every byte
__m128i low_nibbles = _mm_and_si128(data, low_mask); 
// Shift right by 4 bits and mask to get high nibbles
__m128i high_nibbles = _mm_and_si128(_mm_srli_epi16(data, 4), low_mask);

// 3. Parallel Lookup
// _mm_shuffle_epi8 (pshufb) uses nibbles as indices to look up values in the tables
__m128i low_lookup = _mm_shuffle_epi8(lt, low_nibbles);
__m128i high_lookup = _mm_shuffle_epi8(ht, high_nibbles);

// 4. Combine results
// Only if both nibbles match a character in their respective range will 'result' be non-zero
__m128i result = _mm_and_si128(low_lookup, high_lookup);

// 5. Detection check
// _mm_movemask_epi8 extracts the MSB of each byte into a 16-bit integer.
// Since our masks only use bits 0-6, we check if the byte is greater than 0.
int found_mask = _mm_movemask_epi8(_mm_cmpgt_epi8(result, _mm_setzero_si128()));

if (found_mask != 0) {
    // Process individual characters...
}
```

## 5. State Machine (String & Escape Handling)
To correctly parse the JSON structure, the scanner must track whether it is inside a string and handle escape sequences.

### States
- **Outside String**: All 7 structural characters are significant.
- **Inside String**: Only `"` and `\` are significant; others are ignored.
- **In Escape**: The next structural character is ignored.

### Pseudo Code
```python
in_string = False
in_escape = False

for byte in structural_characters_found:
    char = decode_bitmask(byte)
    
    if not in_string:
        output(char)
        if char == '"':
            in_string = True
    else:
        # Inside String
        if char == '\\':
            if in_escape:
                # Escaped backslash: \\
                in_escape = False 
            else:
                # Start of escape: \
                in_escape = True
        elif char == '"':
            if in_escape:
                # Escaped quote: \"
                in_escape = False
            else:
                # String end
                output(char)
                in_string = False
        else:
            # Other structural chars ([, ], {, }, ,) are ignored inside strings
            in_escape = False
```

## 6. Performance Benchmark (10MB JSON)

A comparison between the SIMD structural scanner and Node.js (`JSON.parse`) was conducted using a 10MB DNF JSON file.

| Platform / Tool | Task | Average Time (Warm Cache) | Throughput |
| :--- | :--- | :--- | :--- |
| **Node.js (V8)** | Full JSON Parse | ~26.0 ms | ~385 MB/s |
| **C (SSSE3)** | Structural Scan + State Tracking | ~15.7 ms | ~640 MB/s |
| **C (SSE4.2)** | Structural Scan + State Tracking | **~6.8 ms** | **~1.5 GB/s** |

### Analysis
- **Raw Speed**: The SSE4.2 optimized scanner identifies nearly 1 million structural characters and manages string/escape states in just 6.8 ms.
- **Throughput**: At ~1.5 GB/s, the scanner is now limited primarily by memory bandwidth and the speed of the raw file read (~6.5 ms).
- **Feasibility**: The structural scan is now nearly 4x faster than Node.js's entire parsing process, confirming the potential of a specialized DNF parser.

---

## 7. SSE4.2 & Bit Manipulation Optimizations

The latest version implements several advanced optimizations to maximize throughput:

### 7.1 Early Block Exit (`_mm_testz_si128`)
Before processing a 16-byte block, we use `_mm_testz_si128` (SSE4.1+) to check if the `result` vector is entirely zero. This allows the scanner to skip the expensive bitmask extraction and processing for "clean" blocks (e.g., long stretches of whitespace or values) with a single branch.

### 7.2 Bit Scanning with CTZ (`__builtin_ctz`)
Instead of a linear 16-iteration loop to find set bits in the `found_mask`, we use the `__builtin_ctz` (Count Trailing Zeros) intrinsic. This allows the processor to jump directly to the next structural character position.

```c
while (found_mask != 0) {
    int i = __builtin_ctz(found_mask); // Jump to next set bit
    found_mask &= (found_mask - 1);    // Clear processed bit
    // ... process character at buffer[pos + i] ...
}
```

### 7.3 Inter-Block Escape Handling
The escape logic correctly handles backslashes (`\`) occurring at the very end of a 16-byte block (index 15). A `skip_next_byte` flag ensures that the first byte of the *next* block is correctly ignored if it was escaped by the last byte of the current block.
