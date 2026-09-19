# C Object (co) Library - Reference Manual

This document provides a comprehensive description of the objects, types, APIs, and memory management semantics available in the **C Object (co) Library** (`co.h`).

---

## 1. Introduction and Memory Management Semantics

The C Object library is designed to bring object-oriented-like, dynamically typed structures (such as Vectors, Maps, Strings, etc.) to the C programming language. It establishes a consistent memory management protocol based on ownership and flags.

### Read/Write vs. Read-Only Objects
- **`co`**: A read/write pointer to a C-Object (`struct coStruct *`).
- **`cco`**: A read-only/const pointer to a C-Object (`const struct coStruct *`).
> **Warning**: Compiling with warnings like `"discards ‘const’ qualifier"` indicates critical API usage errors. Ensure you only pass mutable objects to modification routines or use cloning when necessary.

### Elements Ownership and Containers
When objects are assigned to containers (such as adding an element to a Vector or a Map), they are usually **moved**. This means the container takes full ownership of the object and is responsible for destroying it.
1. **Move**: The remote element becomes obsolete and is now part of the container. It will be automatically deleted when the container is deleted. **Do not reference or free the moved element anywhere else after moving.**
2. **Reference**: The element in the container is just a reference to the remote element. The container does not delete it on destruction. *Risk*: If the remote element is deleted elsewhere, the pointer in the container becomes invalid.
3. **Clone**: A deep-copied element is stored in the container (equivalent to cloning the element first, then moving it into the container).

#### Common Ownership Pitfalls and Examples
Below are typical valid/invalid pattern patterns as illustrated in the library guidelines:

- **Example 1 (Valid Move)**:
  ```c
  co v = coNewVector(CO_FREE_VALS);
  coAdd(v, coNewStr(CO_STRDUP, "abc")); // Valid: return value of coNewStr is owned and deleted by v
  ```
- **Example 2 (Invalid Reuse after Move)**:
  ```c
  co v = coNewVector(CO_FREE_VALS);
  co s = coNewStr(CO_STRDUP, "abc");
  coAdd(v, s); // Valid, but 's' becomes invalid/obsolete
  coAdd(v, s); // INVALID: 's' is already moved and cannot be added again
  ```
- **Example 3 (Invalid Double Delete)**:
  ```c
  co v = coNewVector(CO_FREE_VALS);
  co s = coNewStr(CO_STRDUP, "abc");
  coAdd(v, s); // Valid, but 's' is now owned by 'v'
  coDelete(s); // INVALID: 's' will be double-deleted when 'v' is destroyed
  ```
- **Example 4 (Valid Temporary Assignment)**:
  ```c
  co v = coNewVector(CO_FREE_VALS);
  co s;
  for (int i = 0; i < 10; i++) {
    s = coNewStr(CO_STRDUP, "abc"); // Valid: 's' is overwritten with a new object
    coVectorAdd(v, s);              // Valid: ownership of the new string is transferred to v
  }
  ```
- **Example 5 (Invalid Self-Assignment)**:
  ```c
  coVectorAdd(v, coVectorGet(v, 0)); // INVALID & triggers warnings. Use coClone() instead!
  ```

---

## 2. Global Memory Management Flags

The behavior of objects and containers is configured using specific flags at construction:

| Flag Name | Value | Purpose |
| :--- | :---: | :--- |
| `CO_NONE` | `0` | No flags set. No ownership or deletion behaviors enabled. |
| `CO_FREE_VALS` | `1` | Container frees its child/element values upon destruction. |
| `CO_FREE_FIRST` | `2` | Special flag to specify custom destruction behaviors. |
| `CO_STRDUP` | `4` | Executes `strdup` on the provided string or key to duplicate it. Enabling `CO_STRDUP` automatically enables `CO_STRFREE`. |
| `CO_STRFREE` | `8` | Executes `free()` on the underlying string or key during destruction. |

---

## 3. Generic Object API

These functions operate on any C-Object pointer (`co` or `cco`) regardless of its concrete underlying type:

### Type Information & Identification
- **`coGetType(cco o)`**: Returns the function structure pointer (`coFn`) representing the underlying object type. Returns `0` if `o` is `NULL`.
- **`coIsBlank(o)`**: Checks if the object is blank (type `coBlankType`).
- **`coIsVector(cco o)`**: Returns true if the object is a Vector.
- **`coIsStr(cco o)`**: Returns true if the object is a String.
- **`coIsMem(cco o)`**: Returns true if the object is a Memory Block.
- **`coIsMap(cco o)`**: Returns true if the object is a Map.
- **`coIsDbl(cco o)`**: Returns true if the object is a Double.
- **`coIsBool(cco o)`**: Returns true if the object is a Boolean.
- **`coIsInt32Vector(cco o)`**: Returns true if the object is an Int32Vector.

### Generic Functions
- **`void coPrint(const cco o)`**: Prints a formatted, debug representation of the object to stdout.
- **`void coDelete(co o)`**: Recursively deletes the object and its children according to the configured flags (e.g. `CO_FREE_VALS`). Safely handles nested hierarchies.
- **`co coClone(cco o)`**: Creates and returns a deep copy of the object `o`.
- **`long coSize(cco o)`**: Returns a generic size measurement of the object. Its specific behavior depends on the object type (e.g., length of string, size of vector, size of map, etc.).

---

## 4. Object Types and Specific APIs

---

### A. Blank Object (`coBlankType`)
A minimal, lightweight placeholder object.

- **Constructor**:
  - `co coNewBlank(void)`

---

### B. String Object (`coStrType`)
Encapsulates a standard null-terminated C string with safety and auto-concatenation utilities.

- **Constructors**:
  - `co coNewStr(unsigned flags, const char *s)`: Creates a new string object. Typically used with the `CO_STRDUP` flag.
  - `co coNewStrWithLen(const char *s, size_t len)`: Creates a string object containing a slice of length `len` from `s`.

- **API Functions**:
  - `const char *coStrGet(cco o)`: Returns a read-only reference (`const char *`) to the internal string data.
  - `int coStrSet(co o, const char *s)`: Sets/overwrites the string value of the object. Requires the `CO_STRDUP` flag on the object.
  - `int coStrAdd(co o, const char *s)`: Concatenates `s` to the current string object. Requires the `CO_STRDUP` flag.
  - `int coStrAddWithLen(co o, const char *s, size_t len)`: Concatenates up to `len` bytes of string `s` to the object. Requires the `CO_STRDUP` flag.
  - `char *coStrDeleteAndGetAllocatedStringContent(co o)`: Destroys the string object wrapper and returns its raw allocated buffer. **The caller takes ownership and must free the returned string.**
  - `const char *coStrToString(cco o)`: *Obsolete*. Use `coStrGet()` instead.

---

### C. Memory Block Object (`coMemType`)
Encapsulates a dynamic, resizable binary memory block.

- **Constructor**:
  - `co coNewMem(void)`

- **API Functions**:
  - `long coMemSize(cco o)`: Returns the current size of the stored memory block in bytes.
  - `int coMemAdd(co o, const void *mem, size_t len)`: Appends `len` bytes of raw binary data from `mem` to the block.
  - `const void *coMemGet(cco o)`: Returns a read-only raw reference (`const void *`) to the internal memory buffer.

---

### D. Vector Object (`coVectorType`)
A dynamic array container that holds other `co` objects.

- **Constructors**:
  - `co coNewVector(unsigned flags)`: Creates an empty vector. Use `CO_FREE_VALS` to enable automatic deletion of child elements when the vector is deleted.
  - `co coNewVectorByMap(cco map)`: Constructs a vector from an existing Map object. Each element in the resulting vector is itself a 2-element vector containing `[key, value]`.

- **API Functions**:
  - `long coVectorSize(cco o)`: Returns the number of elements in the vector.
  - `int coVectorEmpty(cco o)`: Returns `1` if the vector is empty, `0` otherwise.
  - `long coVectorAdd(co o, cco p)`: Appends object `p` to the vector. If `CO_FREE_VALS` is configured, `p` will be deleted by the vector's destructor. Returns `-1` on error. `p` can be `NULL`.
  - `int coVectorAppendVector(co v, cco src)`: Clones elements from vector `src` and appends them to vector `v`.
  - `cco coVectorGet(cco o, long idx)`: Returns a read-only reference to the element at the specified index `idx`.
  - `void coVectorSet(co v, long i, cco e)`: Replaces the element at index `i` with object `e`. Index `i` must be less than `coVectorSize(v)`.
  - `void coVectorErase(co v, long i)`: Deletes and removes the element at position `i`.
  - `void coVectorEraseLast(co v)`: Deletes the last element of the vector, reducing its size by 1.
  - `void coVectorClear(co o)`: Clears the vector to size 0. If `CO_FREE_VALS` is active, all elements are deleted.
  - `long coVectorPredecessorBinarySearch(cco v, const char *search_key)`: Performs a binary search on a sorted key-value vector constructed via `coNewVectorByMap()`.

- **Callbacks & Iteration**:
  - `int coVectorForEach(cco o, coVectorForEachCB cb, void *data)`: Iterates through elements. The callback signature is:
    `typedef int (*coVectorForEachCB)(cco o, long idx, cco element, void *data);`
  - `co coVectorMap(cco o, coVectorMapCB cb, void *data)`: Maps elements of the vector into a new object structure. The callback signature is:
    `typedef co (*coVectorMapCB)(cco o, long idx, cco element, void *data);`

---

### E. Map Object (`coMapType`)
An AVL-tree-based dictionary mapping string keys to other `co` objects.

- **Constructor**:
  - `co coNewMap(unsigned flags)`: Creates an empty map. Flags:
    - `CO_FREE_VALS`: Delete value objects when they are replaced or when the map is deleted.
    - `CO_STRDUP`: Clone the key strings using `strdup` upon addition.
    - `CO_STRFREE`: Free key strings when entries are removed or when the map is deleted.

- **API Functions**:
  - `long coMapSize(cco o)`: Returns the number of key-value pairs in the map (runs in **O(n)** time).
  - `int coMapEmpty(cco o)`: Returns `1` if the map is empty, `0` otherwise.
  - `int coMapExists(cco o, const char *key)`: Returns `1` if `key` exists in the map, `0` otherwise.
  - `const char *coMapAdd(co o, const char *key, cco value)`: Inserts a key-value pair into the map. If `CO_STRDUP` is set, `key` is duplicated. If `CO_FREE_VALS` is active, the old value associated with the key is deleted first. Returns `NULL` on memory error, or the internal pointer to the key.
  - `cco coMapAddValueKey(co o, const char *key)`: Inserts a key-value pair where the value is a new string object representing the key itself. Requires `CO_FREE_VALS` to be enabled. Returns the inserted value object.
  - `cco coMapGet(cco o, const char *key)`: Retrieves the value associated with `key`. Returns `NULL` if the key is not found.
  - `const char *coMapGetKey(cco o, const char *key)`: Returns the exact internal pointer to the key string, or `NULL` if not found.
  - `void coMapErase(co o, const char *key)`: Removes and deletes the key-value pair.
  - `void coMapClear(co o)`: Empties the map, releasing keys and values based on the initialization flags.

- **Callbacks & Iteration**:
  - `int coMapForEach(cco o, coMapForEachCB cb, void *data)`: Iterates in-order over map items. Returns `0` as soon as the callback returns `0`. The callback signature is:
    `typedef int (*coMapForEachCB)(cco o, long idx, const char *key, cco value, void *data);`

- **Stack-based Loop Iterator**:
  An efficient, non-callback alternative to iterate over the map structure.
  ```c
  coMapIterator iter;
  if ( coMapLoopFirst(&iter, map_obj) )
  {
      do {
          const char *key = coMapLoopKey(&iter);  // Get current key
          cco value       = coMapLoopValue(&iter); // Get current value
          // Use key and value...
      } while( coMapLoopNext(&iter) );
  }
  ```
  - `int coMapLoopFirst(coMapIterator *iter, cco o)`: Initializes the iterator on map `o`. Returns `1` if the map is not empty.
  - `int coMapLoopNext(coMapIterator *iter)`: Advances to the next node. Returns `1` if successful, `0` if iteration finished.

---

### F. Double Object (`coDblType`)
Encapsulates a standard double-precision floating point number. Helpful for serializing numeric data to JSON.

- **Constructor**:
  - `co coNewDbl(double n)`

- **API Functions**:
  - `double coDblGet(cco o)`: Returns the encapsulated double value.
  - `void coDblSet(co o, double n)`: Updates the encapsulated double value.

---

### G. Bool Object (`coBoolType`)
Encapsulates a boolean value (true/false). Primary purpose is to provide structured support during JSON serialization and representation.

- **Constructor**:
  - `co coNewBool(int n)`

- **API Functions**:
  - `int coBoolGet(cco o)`: Returns the boolean state (`0` or `1`).
  - `void coBoolSet(co o, int b)`: Sets/updates the boolean state.

---

### H. Int32Vector Object (`coInt32VectorType`)
A dynamic array container optimized specifically for `int32_t` elements. It utilizes a continuous block of memory that only grows and never shrinks (except when the object is deleted).

- **Constructors**:
  - `co coNewInt32Vector(unsigned flags)`: Creates an empty Int32Vector.
  - `co coNewInt32VectorByVector(cco o)`: Creates an Int32Vector from a standard Vector `o` (or another Int32Vector) if all of its members are numeric (instances of Double or Bool, or other Int32Vectors). Returns `NULL` otherwise.

- **API Functions**:
  - `long coInt32VectorSize(cco o)`: Returns the number of elements in the vector.
  - `int coInt32VectorEmpty(cco o)`: Returns `1` if empty, `0` otherwise.
  - `long coInt32VectorAdd(co o, int32_t n)`: Appends the integer `n` to the vector. Returns the index of the added element or `-1` on error.
  - `long coInt32VectorAddUnique(co o, int32_t n)`: Appends the integer `n` to the vector only if it doesn't already exist. Returns the index of the element (either existing or newly added) or `-1` on error.
  - `int coInt32VectorAppendVector(co v, cco src)`: Appends elements from vector `src` (which can be another Int32Vector or a standard Vector containing numeric elements) to the vector `v`. Returns `1` on success, `0` on error.
  - `int32_t coInt32VectorGet(cco o, long idx)`: Returns the integer element at index `idx`, or `0` if outside bounds.
  - `void coInt32VectorSet(co v, long i, int32_t n)`: Replaces the element at index `i` with `n`. Index `i` must be less than `coInt32VectorSize(v)`.
  - `void coInt32VectorErase(co v, long i)`: Removes the element at position `i` and shifts the remaining elements.
  - `void coInt32VectorEraseLast(co v)`: Deletes the last element, reducing the size by 1.
  - `void coInt32VectorClear(co o)`: Clears the vector to size 0. The internal memory block capacity is retained.
  - `int coInt32VectorExists(co o, int32_t n)`: Returns `1` if the value `n` exists in the vector, `0` otherwise.
  - `long coInt32VectorFind(co o, int32_t n)`: Returns the first index of the value `n` or `-1` if not found.
  - `void coInt32VectorEraseByValue(co o, int32_t n)`: Removes all occurrences of value `n` from the vector, shifting remaining elements.

---

## 5. Serialization and Utility Modules

---

### JSON Parser & Serializer
Provides robust read/write support for JSON payloads.

- **JSON Reader**:
  - `co coReadJSONByString(const char *json)`: Parses JSON syntax directly from a string and returns the root `co` object tree.
  - `co coReadJSONByFP(FILE *fp)`: Parses JSON from a file pointer. Supports UTF-8 BOM and GZIP auto-decompression (when `CO_USE_ZLIB` is compiled).

- **JSON Writer**:
  - `void coWriteJSON(cco o, int isCompact, int isUTF8, FILE *fp)`: Serializes the object hierarchy to the file pointer `fp`.
    - `isCompact`: If true (`1`), produces compact, minified output. Otherwise, formats/indents nicely.
    - `isUTF8`: If false (`0`), characters with codes `>= 128` are escaped as `\uXXXX`.

---

### Stream/File Reader Interface (`coReader`)
An abstraction for reading characters and handling encodings like Byte Order Marks (BOM).

- **BOM Types Supported**:
  - `BOM_NONE` (0)
  - `BOM_UTF8` (1)
  - `BOM_UTF16BE` (2)
  - `BOM_UTF16LE` (3)
  - `BOM_UTF32BE` (4)
  - `BOM_UTF32LE` (5)

- **Functions**:
  - `int coReaderInitByString(coReader reader, const char *s)`: Binds a `coReader` to a string buffer.
  - `int coReaderInitByFP(coReader reader, FILE *fp)`: Binds a `coReader` to an open file handle (handles BOM detection and zlib Gzip decompression).
  - `void coReaderErr(coReader r, const char *msg)`: Raises/reports a reader error context.

- **Helper Macros**:
  - `coReaderNext(r)`: Consumes and advances to the next character in the stream.
  - `coReaderCurr(r)`: Inspects the current character without advancing.
  - `coReaderSkipWhiteSpace(r)`: Consumes and skips all leading whitespace.

---

### Specialty Parsers (`co_extra.c` & `co_xml.c`)
The library offers out-of-the-box parsers converting various file types directly into standard C Object trees:

- **XML**:
  - `co coReadXMLByFP(FILE *fp, int skip_white_space)`: Parses an XML document into a `co` tree.

- **A2L**:
  - `co coReadA2LByString(const char *json)`
  - `co coReadA2LByFP(FILE *fp)`

- **Embedded Memory/Firmware Records**:
  - `co coReadS19ByFP(FILE *fp)`: Reads Motorola S-record firmware files into a Map (`key=8-digit hex address`, `value=coMem` block).
  - `co coReadHEXByFP(FILE *fp)`: Reads Intel HEX files into a Map (`key=8-digit hex address`, `value=coMem` block).
  - `co coReadElfMemoryByFP(FILE *fp)`: Reads ELF program segments into a Map.

- **CSV Support**:
  - `co coReadCSVByFP(FILE *fp, int separator)`: Parses a CSV file into a Vector of Vectors of strings.
  - `co coReadCSVByFPWithPool(FILE *fp, int separator, co pool)`: Same as above but optimizes string storage by allocating duplicate strings from an existing map string pool.
  - `co coGetCSVRow(struct co_reader_struct *r, int separator)`: Reads a single row of CSV elements.

---

## 6. Disjunctive Normal Form (DNF) Set Handling

The library provides a specialized module (`co_dnf.c`) for working with structures representing boolean/set theory expressions in **Disjunctive Normal Form (DNF)**.

### Mathematical Representation of DNF in JSON
Under this module, a DNF expression is represented using a specific JSON structure:
- **Outer container**: A `coVector` representing a Union (OR) of individual clauses.
- **Middle container**: A `coMap` representing an Intersection (AND) within each clause.
- **Inner container**: A `coInt32Vector` representing a set of integer values for each variable/key.

#### JSON Structure Example
```json
[
  {"1": [1, 2, 3], "2": [4, 5, 6]},
  {"3": [4, 8, 2], "2": [2, 5, 9]}
]
```

#### Special Sets
- **Empty Set**: Represented as `[]` (an empty outer `coVector`).
- **Universal/Overall Set**: Represented as `[{}]` (a `coVector` with a single, empty `coMap`).

---

### DNF API Functions

#### Conversion Helper
- **`co coConvertToInt32Vector(co o)`**:
  Recursively traverses the object hierarchy starting at `o`. Any `coVector` whose first element is numeric (i.e. `coIsDbl` or `coIsBool`) is converted into a specialized, compact `coInt32Vector`. Elements that are not numeric are discarded. The original `coVector` is fully deleted/erased. This helper is extremely useful for post-processing parsed JSON payloads into mathematically compliant structures.

#### DNF Evaluation & Integrity Checks
- **`int coDNFIsValid(cco dnf)`**:
  Returns `1` if the object hierarchy exactly matches the DNF structure (a vector of maps of int32 vectors), `0` otherwise. Correctly handles special sets like `[]` and `[{}]`.
- **`int coDNFIsEmpty(cco dnf)`**:
  Returns `1` if the DNF is equal to `[]` (its size is 0), and `0` otherwise.
- **`int coDNFIsUniversal(cco dnf)`**:
  Returns `1` if the DNF is equal to `[{}]` (a single empty map clause representing the universal set), and `0` otherwise.

- **`int coDNFCheckUniversal(cco psd, cco dnf)`**:
  Recursively checks if the given `dnf` covers the entire variant space (as defined by the `psd`). Unlike `coDNFIsUniversal`, this function uses Shannon expansion to correctly identify cases where the union of multiple constrained terms fills the entire space. Returns `1` if universal, `0` otherwise.

- **`int coDNFCheckUniversalByComplement(cco psd, cco dnf)`**:
  An alternative implementation of the universal cover check. It calculates the complement of the `dnf` ($Universal \setminus dnf$) and returns `1` if the resulting complement is empty, `0` otherwise.

#### Set Operations
- **`int coDNFIsSubsetAttributeSelection(cco a, cco b)`**:
  Returns `1` if all values in `Int32Vector` `a` are also present in `Int32Vector` `b`, `0` otherwise.
- **`int coDNFIsSubsetANDTermANDTerm(cco subset_and_term, cco superset_and_term)`**:
  Returns `1` if AND-term `subset_and_term` is a subset of AND-term `superset_and_term`, `0` otherwise.
  `subset_and_term` is a subset of `superset_and_term` if every attribute in `superset_and_term` is also present in `subset_and_term`, and the value set for that attribute in `subset_and_term` is a subset of the value set in `superset_and_term`. An omitted attribute in `superset_and_term` is treated as unconstrained (universal).
- **`int coDNFIsSubsetANDTerm(cco psd, cco subset_and_term, cco superset_dnf)`**:
  Returns `1` if the variants represented by the single AND-term `subset_and_term` are covered by the union of terms in `superset_dnf`.
  The `psd` (Problem Space Description) is used to correctly handle attributes that are present in `superset_dnf` but missing in `subset_and_term` (interpreting them as unconstrained over their full domain).
- **`int coDNFIsSubset(cco psd, cco subset_dnf, cco superset_dnf)`**:
  Returns `1` if the variants represented by `subset_dnf` are a subset of (or equal to) the variants represented by `superset_dnf`.
  This check handles complex cases where terms in `subset_dnf` are covered by the union of multiple terms in `superset_dnf`.

- **`int coDNFIsEqual(cco psd, cco dnf1, cco dnf2)`**:
  Returns `1` if `dnf1` and `dnf2` cover exactly the same variant space, and `0` otherwise. It is implemented as a mutual subset check: $dnf1 \subseteq dnf2$ AND $dnf2 \subseteq dnf1$.

- **`int coDNFUnion(co arg1, cco arg2)`**:
  Performs a set union operation by appending all clauses (cloned maps) from `arg2` to `arg1`. `arg2` is not modified. Returns `1` on success, `0` on error.
- **`int coDNFIntersection(co arg1, cco arg2)`**:
  Performs a pairwise set intersection operation of two DNF expressions, replacing the contents of `arg1` with the result in-place. It does this by calling `coNewDNFByIntersection()` and updating `arg1` in-place. Returns `1` on success, `0` on error.
- **`co coNewDNFByIntersection(cco arg1, cco arg2)`**:
  Performs a pairwise set intersection of two DNF expressions, creating and returning a newly constructed `co` DNF vector with the result.
  Each pair of AND-terms from `arg1` and `arg2` are intersected attribute-by-attribute:
  - If an attribute is present in both AND-terms, the resulting AND-term will contain only the common values (intersection of the value sets).
  - If an attribute is missing in one AND-term, it is assumed to exist implicitly with all possible values, meaning the intersection retains the values from the specified term.
  - If any attribute-value list becomes empty, the entire intersected AND-term is empty/invalid and is discarded.
  Returns the new DNF on success, `NULL` on error.

- **`co coNewDNFBySubtraction(cco psd, cco left_dnf, cco right_dnf)`**:
  Performs a set-theoretic subtraction ($left\_dnf \setminus right\_dnf$), creating and returning a newly constructed `co` DNF vector with the result. It handles the complex case where terms are split into smaller pieces to ensure non-overlap with the subtracted terms.

- **`co coDNFComplementBySubtract(cco psd, cco dnf)`**:
  Calculates the set-theoretic complement of the given `dnf` relative to the universal set (as constrained by the `psd`), creating and returning a newly constructed `co` DNF vector with the result. It is implemented internally as $Universal \setminus dnf$.

- **`co coDNFNewCofactor(cco psd, cco dnf, const char *attr_name, int32_t value)`**:
  Calculates the cofactor (restriction) of the given `dnf` by forcing `attr_name` to `value`. 
  - Terms that do not contain `attr_name` are kept as-is.
  - Terms that contain `attr_name` but whose value set includes `value` are kept, but `attr_name` is removed from the term (the constraint is fulfilled).
  - Terms that contain `attr_name` but whose value set does *not* include `value` are discarded (the constraint is violated).
  Returns a newly constructed `co` DNF vector with the result.

- **`const char *coDNFGetBestCofactorAttribute(cco psd, cco dnf)`**:
  Returns a pointer to the attribute name that appears most frequently across all AND-terms in the given `dnf`. This is typically used as a heuristic for selecting the next variable to expand during Shannon decomposition. Returns `NULL` if the DNF is empty.

- **`int coDNFComplement(cco psd, co dnf)`**:
  Performs an in-place set-theoretic complement of the given `dnf`, replacing its contents with the result. Returns `1` on success, `0` on error.

- **`void coDNFMinimizeANDTermSubset(co dnf)`**:
  Minimizes the DNF by removing redundant AND-terms. A term is considered redundant if it is a subset of another term in the same DNF (i.e., it represents a subset of the variants already covered by another term). This function performs pairwise checks using `coDNFIsSubsetANDTermANDTerm` and is optimized for speed by avoiding complex DNF subtraction.

- **`void coDNFMinimizeByANDTermMerge(co dnf)`**:
  Minimizes the DNF by merging structurally compatible AND-terms. Two terms are merged if they contain the exact same set of attributes and differ in only one attribute's value list. In this case, the differing attribute is replaced by the union of both value sets, and the redundant term is removed.

- **`void coDNFMinimizeClearFullDomain(cco psd, co dnf)`**:
  Simplifies every AND-term in the DNF by removing any attribute whose value set matches the full domain defined in the `psd`, and then performs a redundancy check.

- **`void coDNFMinimizeClearFullDomainANDTerm(cco psd, co term)`**:
  Simplifies a single AND-term by removing any attribute whose value set matches the full domain defined in the `psd`. This helps in keeping the DNF representation minimal.

#### Multi-Valued Space & Problem Space Description (PSD)
The DNF can act as an operand in a multi-valued algebra representing a "set" in a multi-valued space. This space is described by a **Problem Space Description (PSD)**, which is stored as a single AND-Term nested under the key `"psd"` inside a wrapper map:

```json
{
  "psd": {
    "1": [1, 2, 3, 4],
    "2": [2, 4, 5, 6, 9],
    "3": [1, 2, 4, 8],
    "4": [3, 4]
  }
}
```

- **`co coNewPSD(void)`**:
  Creates and returns a new PSD wrapper map with a single member `"psd"` initialized to an empty `coMap`.
- **`int coPSDExtendByDNF(co psd, cco dnf)`**:
  Extends the multi-value space of the `psd` with the attributes and values from the `dnf`. It iterates through each clause and variable/attribute inside `dnf` and adds them to `psd` along with their values, guaranteeing that duplicate values are not added. Returns `1` on success, `0` on error.
- **`void coBVPreparePSD(co psd)`**:
  Prepares a Problem Space Description (PSD) for bitvector operations. It generates metadata that maps symbolic attribute-value pairs to unique bit positions. This function adds/updates the following members in the `psd` map:
  - **`bvattributes`**: A map from attribute name to an `Int32Vector` containing `[index, start_bit, num_values]`.
  - **`bvpos`**: An `Int32Vector` containing bit offsets. The last entry is the total number of bits.
  - **`bvvaluepos`**: A nested map where `bvvaluepos[attr_name][value_as_string]` returns the local bit offset (as a Double object).
  - **`bvmask`**: A `coVector` of bitvectors (`co_BVType`). Each bitvector is a mask where all bits belonging to the corresponding attribute (indexed by the attribute's order in the PSD) are set to one.

---

### I. BitVector Object (`coBitVectorType`)
A high-performance bitset implementation that leverages SIMD instructions (SSE2, AVX2, AVX-512) for accelerated operations. Bitvectors are first-class objects and can be managed by standard containers.

- **SIMD Detection**:
  - `void coBVDetect(void)`: Detects the best available SIMD instruction set on the current CPU and configures the global dispatchers. This should be called once at application startup.

- **Constructors & Destructors**:
  - `co_BVType coNewBV(uint64_t bits)`: Creates a new bitvector object with at least `bits` capacity. The memory is 64-byte aligned and initialized to zero.
  - `void coDeleteBV(co_BVType bv)`: Deletes the bitvector object. (Note: Standard `coDelete()` also works).

- **API Functions**:
  - `void coBVSet(co_BVType bv, uint64_t bit_idx)`: Sets the bit at `bit_idx` to 1.
  - `void coBVClr(co_BVType bv, uint64_t bit_idx)`: Clears the bit at `bit_idx` to 0.
  - `int coBVGet(co_BVType bv, uint64_t bit_idx)`: Returns the state of the bit at `bit_idx` (0 or 1).
  - `void coBVOR(co_BVType res, co_BVType a, co_BVType b)`: Computes bitwise `res = a | b`.
  - `void coBVAND(co_BVType res, co_BVType a, co_BVType b)`: Computes bitwise `res = a & b`.
  - `void coBVANDNOT(co_BVType res, co_BVType a, co_BVType b)`: Computes bitwise `res = a & ~b`.
  - `int coBVIsEqual(co_BVType a, co_BVType b)`: Returns `1` if all bits in `a` and `b` are identical, `0` otherwise.

- **Conversion Functions**:
  These functions bridge the gap between symbolic DNF representation and bitvector representation using a prepared PSD:
  - `co_BVType coNewBVFromANDTerm(cco psd, cco and_term)`: Converts a symbolic `coMap` AND-term to a bitvector.
  - `co coNewANDTermFromBV(cco psd, co_BVType bv)`: Converts a bitvector back to a symbolic `coMap` AND-term.
  - `co coNewBVDNFFromDNF(cco psd, cco dnf)`: Converts a standard DNF (`Vector` of `Maps`) to a bitvector DNF (`Vector` of `co_BVType`).
  - `co coNewDNFFromBVDNF(cco psd, cco bv_dnf)`: Converts a bitvector DNF back to a standard symbolic DNF.

---

### Command-line Tool: `dnf`

A dedicated command-line utility `dnf` is compiled automatically to allow running union, intersection, and space description procedures directly from the shell using JSON files.

#### Location and Compilation
- **Source**: `test/dnf.c`
- **Build target**: `dnf` (runs automatically with `make` or `make dnf`)

#### Command Usage
```bash
./dnf [options] [arg1.json arg2.json ...]
```

The tool is highly flexible and relaxed:
- If **`-union`**, **`-intersection`**, or **`-subtract`** is specified, exactly two input JSON files must be provided to run the pairwise DNF operation.
- If no pairwise operation flag is specified, the tool acts as a standalone PSD builder and validator. You can pass zero, one, or multiple input files. The tool will initialize the PSD (either empty, generated via `-gpsd`, or loaded via `-ipsd`), parse and convert the provided files to unique `Int32Vector` DNFs, and automatically extend the PSD with their attributes and values.

#### CLI Options
- **`-h`**: Outputs a detailed help/usage message.
- **`-union`**: Executes a pairwise DNF union operation on exactly two input DNFs.
- **`-intersection`**: Executes a pairwise DNF intersection operation on exactly two input DNFs.
- **`-subtract`**: Executes a pairwise DNF subtraction operation on exactly two input DNFs.
- **`-complement`**: Executes a unary DNF complement operation on exactly one input DNF.
- **`-equal`**: Checks if two DNFs cover exactly the same variant space.
- **`-cofactor <a> <v>`**: Calculates the restriction (cofactor) of a DNF for attribute `<a>` and value `<v>`.
- **`-check-universal`**: Executes a recursive tautology check (Shannon expansion) to determine if a DNF covers the entire PSD space.
- **`-test`**: Executes six internal mathematical consistency tests on a single DNF:
  1.  $\overline{f} \cap f = \emptyset$ (Intersection with complement is empty)
  2.  $\overline{f} \cup f = 1$ (Union with complement is universal via Shannon)
  3.  $\overline{f} \cup f = 1$ (Union with complement is universal via Complement check)
  4.  Agreement between Shannon and Complement-based universal checks for original DNF.
  5.  Agreement between Shannon and Complement-based universal checks for Complement.
  6.  $\overline{\overline{f}} = f$ (Double complement equality check)
- **`-test-isec <a> <v>`**: Executes an intersection benchmark. It generates two DNFs with `<a>` AND-terms each, where each term contains `<v>` random values for its attributes. The DNFs are designed to trigger worst-case term explosion ($n \times m$) by using disjoint primary attributes.
- **`-gdnf <terms> <attrs> <vals>`**: Generates a random DNF. This DNF is treated as an input argument to any of the above operations. If no operation is specified, the generated DNF is output as the result.
- **`-o <file>`**: Writes the main resulting DNF (computed from an operation, or generated randomly via `-gdnf`) to `<file>` (a JSON file) instead of standard output.
- **`-o1 <file>`**: Writes the first input DNF (`arg1`) to `<file>` (a JSON file). This is useful for saving procedural DNFs generated via `-gdnf` or `-test-isec`.
- **`-o2 <file>`**: Writes the second input DNF (`arg2`) to `<file>` (a JSON file).
