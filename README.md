# c-object lib

## License

CC BY-SA 4.0

Attribution-ShareAlike 4.0 International

https://creativecommons.org/licenses/by-sa/4.0/ 


## Features

 * Generic objects implemented in C
 * Support for vector and map data structures
 * JSON read and write
 * GZIP support
 
## Note

Use `-DCO_FCGI` if c-object lib is used in a FCGI (fastcgi) application.

## DWA JSON Booleans

The fast JSON scanner in `dnfwebapp/dwa_json.c` records structural positions; unquoted
values occupy the spans between those positions. `dwa_parse_primitive()` validates and
classifies JSON numbers, `true`, `false`, and `null`. `dwa_parse_bool()` accepts only
`true`/`false` and returns C `1`/`0`. Like the integer reader, these readers leave the
following structural delimiter for the caller to consume.

Boolean metadata can occur in objects and arrays. DWA's DNF value-string reader
canonicalizes boolean selections to `"true"`/`"false"`, distinct from numeric `0`/`1`
and consistent with its existing string-based value interning. Quoted `"true"` and
unquoted `true` therefore select the same DWA domain value. This does not change any
consumer's separate input-normalization contract. `null` is valid JSON but is not a
supported DWA DNF selection value.

Run the focused boolean/primitive regressions and existing operation tests with:

```sh
make -C dnfwebapp json-test cli-test
make -C dnfwebapp json-test cli-test MODE=sanitize
```

The structural validator checks primitive grammar, delimiters, and complete input
consumption. It is not a replacement for full string escape/Unicode validation.

## Useful Tools

 * `csv2json`: A tool to convert csv files to JSON format
 * `json_compare`: Compare two different JSON files and output path to the first difference. For arrays the sequence of elements is ignored.
 * `json_format`: A simple beautifier tool for JSON
 
 
