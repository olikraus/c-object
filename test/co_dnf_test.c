#include "co.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

static void check_intersection(cco psd, const char *s1, const char *s2, const char *expected_json) {
  co arg1 = coConvertToInt32Vector(coReadJSONByString(s1));
  co arg2 = coConvertToInt32Vector(coReadJSONByString(s2));
  co expected = coConvertToInt32Vector(coReadJSONByString(expected_json));

  co res_min = coNewDNFByIntersection(psd, arg1, arg2);
  co res_raw = coNewDNFByIntersectionWithoutMinimization(psd, arg1, arg2);

  if (!coDNFIsEqual(psd, res_min, expected)) {
    printf("Failed Intersection: %s AND %s\n", s1, s2);
    printf("  Expected: %s\n", expected_json);
    printf("  Got (min): "); coWriteJSON(res_min, 1, 0, stdout); printf("\n");
  }
  assert(coDNFIsEqual(psd, res_min, expected));
  assert(coDNFIsEqual(psd, res_raw, expected));

  coDelete(arg1);
  coDelete(arg2);
  coDelete(expected);
  coDelete(res_min);
  coDelete(res_raw);
}

void test_dnf_intersection_detailed(void) {
  printf("Running detailed DNF intersection tests...\n");

  co psd = coNewPSD();
  coPSDExtendByDNF(psd, coConvertToInt32Vector(coReadJSONByString("[{\"color\":[1,2,3,4], \"size\":[1,2,3,4], \"height\":[1,2,3,4]}]")));

  /* 1. Intersection with the empty DNF */
  check_intersection(psd, "[]", "[{\"color\":[1,2]}]", "[]");

  /* 2. Intersection with the universal DNF */
  check_intersection(psd, "[{}]", "[{\"color\":[1,2]}]", "[{\"color\":[1,2]}]");

  /* 3. Intersection of value sets with a shared attribute */
  check_intersection(psd,
    "[{\"color\":[1,2,3],\"height\":[2,3]}]",
    "[{\"color\":[2,3,4],\"size\":[1]}]",
    "[{\"color\":[2,3],\"height\":[2,3],\"size\":[1]}]"
  );

  /* 4. Intersection with disjunct attribute sets */
  check_intersection(psd,
    "[{\"color\":[1,2]}]",
    "[{\"size\":[3,4]}]",
    "[{\"color\":[1,2],\"size\":[3,4]}]"
  );

  /* 5. Intersection producing multiple AND-terms */
  check_intersection(psd,
    "[{\"color\":[1,2]}, {\"size\":[3,4]}]",
    "[{\"color\":[2,3]}, {\"height\":[2]}]",
    "[{\"color\":[2]}, {\"color\":[1,2],\"height\":[2]}, {\"color\":[2,3],\"size\":[3,4]}, {\"size\":[3,4],\"height\":[2]}]"
  );

  /* 6. Intersection with overlapping value sets on multiple attributes */
  check_intersection(psd,
    "[{\"color\":[1,2]}, {\"size\":[3,4]}]",
    "[{\"color\":[2,3]}, {\"size\":[2,3]}]",
    "[{\"color\":[2]}, {\"color\":[1,2],\"size\":[2,3]}, {\"color\":[2,3],\"size\":[3,4]}, {\"size\":[3]}]"
  );

  /* 7. Intersection that yields no valid result */
  check_intersection(psd,
    "[{\"color\":[1,2]}]",
    "[{\"color\":[3,4]}]",
    "[]"
  );

  coDelete(psd);
  printf("All detailed DNF intersection tests passed successfully!\n");
}

/* Original tests follow */

void test_dnf(void) {
  printf("Running DNF unit tests...\n");

  /* 1. Test coConvertToInt32Vector & coDNFIsValid */
  const char *json_dnf = "[ {\"color\": [1, 2, 3]}, {\"size\": [2]} ]";
  co dnf = coReadJSONByString(json_dnf);
  assert(dnf != NULL);

  /* Initially before conversion, map values are standard Vectors, not Int32Vectors */
  /* Verify that it's initially invalid since children are standard Vectors */
  assert(coDNFIsValid(dnf) == 0);

  /* Convert */
  co converted = coConvertToInt32Vector(dnf);
  assert(converted == dnf); /* Should be the same root pointer */

  /* Check converted types */
  cco first_map = coVectorGet(dnf, 0);
  assert(coIsInt32Vector(coMapGet(first_map, "color")));

  /* Verify DNF is now valid */
  assert(coDNFIsValid(dnf) == 1);

  /* 2. Test coDNFIsEmpty */
  co empty_dnf = coNewVector(CO_FREE_VALS);
  assert(coDNFIsEmpty(empty_dnf) == 1);
  assert(coDNFIsEmpty(dnf) == 0);

  /* 3. Test coDNFIsUniversal */
  co universal_dnf = coNewVector(CO_FREE_VALS);
  coVectorAdd(universal_dnf, coNewMap(CO_STRDUP | CO_FREE_VALS));
  assert(universal_dnf != NULL);
  assert(coDNFIsUniversal(universal_dnf) == 1);
  assert(coDNFIsUniversal(empty_dnf) == 0);
  assert(coDNFIsUniversal(dnf) == 0);

  /* 4. Test coDNFUnion */
  const char *json_dnf2 = "[ {\"4\": [10, 11, 12]} ]";
  co dnf2 = coReadJSONByString(json_dnf2);
  dnf2 = coConvertToInt32Vector(dnf2);
  assert(coDNFIsValid(dnf2) == 1);

  long initial_size = coVectorSize(dnf); /* Should be 2 */
  assert(initial_size == 2);

  int union_res = coDNFUnion(NULL, dnf, dnf2);
  assert(union_res == 1);
  assert(coVectorSize(dnf) == 3);
  assert(coDNFIsValid(dnf) == 1);

  /* Check that elements from dnf2 are present in dnf */
  cco last_item = coVectorGet(dnf, 2);
  assert(coMapGet(last_item, "4") != NULL);

  /* 5. Test coInt32VectorEquals helper */
  co v1 = coNewInt32Vector(CO_NONE);
  co v2 = coNewInt32Vector(CO_NONE);
  coInt32VectorAdd(v1, 1); coInt32VectorAdd(v1, 2);
  coInt32VectorAdd(v2, 2); coInt32VectorAdd(v2, 1);
  assert(coInt32VectorEquals(v1, v2) == 1);
  coInt32VectorAdd(v2, 3);
  assert(coInt32VectorEquals(v1, v2) == 0);
  coDelete(v1);
  coDelete(v2);

  /* 6. Test coDNFIntersection */
  /* Case 1: Intersection of empty with non-empty */
  co isec_arg1 = coReadJSONByString("[]");
  co isec_arg2 = coReadJSONByString("[{\"color\":[1,2]}]");
  isec_arg2 = coConvertToInt32Vector(isec_arg2);
  assert(coDNFIntersection(NULL, isec_arg1, isec_arg2) == 1);
  assert(coDNFIsEmpty(isec_arg1) == 1);
  coDelete(isec_arg1);
  coDelete(isec_arg2);

  /* Case 2: Intersection of universal with non-empty */
  isec_arg1 = coReadJSONByString("[{}]");
  isec_arg2 = coReadJSONByString("[{\"color\":[1,2]}]");
  isec_arg2 = coConvertToInt32Vector(isec_arg2);
  assert(coDNFIntersection(NULL, isec_arg1, isec_arg2) == 1);
  assert(coDNFIsValid(isec_arg1) == 1);
  assert(coVectorSize(isec_arg1) == 1);
  cco m = coVectorGet(isec_arg1, 0);
  assert(coInt32VectorSize(coMapGet(m, "color")) == 2);
  coDelete(isec_arg1);
  coDelete(isec_arg2);

  /* Case 3: Intersection with shared attribute */
  isec_arg1 = coReadJSONByString("[{\"color\":[1,2,3],\"material\":[2,3]}]");
  isec_arg2 = coReadJSONByString("[{\"color\":[2,3,4],\"size\":[1]}]");
  isec_arg1 = coConvertToInt32Vector(isec_arg1);
  isec_arg2 = coConvertToInt32Vector(isec_arg2);
  assert(coDNFIntersection(NULL, isec_arg1, isec_arg2) == 1);
  assert(coDNFIsValid(isec_arg1) == 1);
  assert(coVectorSize(isec_arg1) == 1);
  m = coVectorGet(isec_arg1, 0);
  assert(coInt32VectorSize(coMapGet(m, "color")) == 2);
  assert(coMapGet(m, "material") != NULL);
  assert(coMapGet(m, "size") != NULL);
  coDelete(isec_arg1);
  coDelete(isec_arg2);

  /* Case 4: Intersection with disjoint attributes */
  isec_arg1 = coReadJSONByString("[{\"color\":[1,2]}]");
  isec_arg2 = coReadJSONByString("[{\"size\":[3,4]}]");
  isec_arg1 = coConvertToInt32Vector(isec_arg1);
  isec_arg2 = coConvertToInt32Vector(isec_arg2);
  assert(coDNFIntersection(NULL, isec_arg1, isec_arg2) == 1);
  assert(coDNFIsValid(isec_arg1) == 1);
  m = coVectorGet(isec_arg1, 0);
  assert(coMapGet(m, "color") != NULL);
  assert(coMapGet(m, "size") != NULL);
  coDelete(isec_arg1);
  coDelete(isec_arg2);

  /* Case 5: Intersection resulting in empty set */
  isec_arg1 = coReadJSONByString("[{\"color\":[1,2]}]");
  isec_arg2 = coReadJSONByString("[{\"color\":[3,4]}]");
  isec_arg1 = coConvertToInt32Vector(isec_arg1);
  isec_arg2 = coConvertToInt32Vector(isec_arg2);
  assert(coDNFIntersection(NULL, isec_arg1, isec_arg2) == 1);
  assert(coDNFIsEmpty(isec_arg1) == 1);
  coDelete(isec_arg1);
  coDelete(isec_arg2);

  /* Test coNewDNFByIntersection directly */
  co arg1 = coReadJSONByString("[{\"color\":[1,2]}]");
  co arg2 = coReadJSONByString("[{\"size\":[3,4]}]");
  arg1 = coConvertToInt32Vector(arg1);
  arg2 = coConvertToInt32Vector(arg2);
  co isec_new = coNewDNFByIntersection(NULL, arg1, arg2);
  assert(isec_new != NULL && coDNFIsValid(isec_new) == 1);
  assert(coVectorSize(isec_new) == 1);
  cco new_clause = coVectorGet(isec_new, 0);
  assert(coMapGet(new_clause, "color") != NULL);
  assert(coMapGet(new_clause, "size") != NULL);
  coDelete(arg1);
  coDelete(arg2);
  coDelete(isec_new);

  /* Cleanup original trees */
  coDelete(dnf);
  coDelete(dnf2);
  coDelete(empty_dnf);
  coDelete(universal_dnf);

  printf("All DNF tests passed successfully!\n");
}

void test_dnf_subset_and_term(void) {
  printf("Running DNF subset and term tests...\n");

  co psd = coNewPSD();
  coPSDExtendByDNF(psd, coConvertToInt32Vector(coReadJSONByString("[{\"color\":[1,2,3], \"size\":[1,2,3,4]}]")));

  /* Helper to read, convert and check subset */
  #define CHECK_SUBSET(s1, s2, expected) { \
    co sub = coConvertToInt32Vector(coReadJSONByString(s1)); \
    co super = coConvertToInt32Vector(coReadJSONByString(s2)); \
    int res = coDNFIsSubset(psd, sub, super); \
    if (res != expected) printf("Failed: %s subset of %s (expected %d, got %d)\n", s1, s2, expected, res); \
    assert(res == expected); \
    coDelete(sub); \
    coDelete(super); \
  }

  /* 0. Term-to-term tests */
  {
    co t1 = coConvertToInt32Vector(coReadJSONByString("[{\"color\":[1,2]}]"));
    co t2 = coConvertToInt32Vector(coReadJSONByString("[{\"color\":[1,2,3]}]"));
    cco m1 = coVectorGet(t1, 0);
    cco m2 = coVectorGet(t2, 0);
    assert(coDNFIsSubsetANDTermANDTerm(m1, m2) == 1);
    assert(coDNFIsSubsetANDTermANDTerm(m2, m1) == 0);
    coDelete(t1);
    coDelete(t2);
  }

  /* 1. A term is a subset of another term */
  CHECK_SUBSET("[{\"color\":[1,2]}]", "[{\"color\":[1,2,3]}]", 1);

  /* 2. A term is not a subset because the value set is larger */
  CHECK_SUBSET("[{\"color\":[1,2,3]}]", "[{\"color\":[1,2]}]", 0);

  /* 3. A term is not a subset because the attribute is missing in the candidate subset */
  CHECK_SUBSET("[{\"color\":[1,2]}]", "[{\"size\":[3,4]}]", 0);

  /* 4. A term with additional non-conflicting attributes is still a subset */
  CHECK_SUBSET("[{\"color\":[1,2],\"size\":[3,4]}]", "[{\"color\":[1,2,3],\"size\":[3,4]}]", 1);

  /* 5. A term is a subset of a broader term because omitted attributes are unconstrained */
  CHECK_SUBSET("[{\"color\":[1,2],\"size\":[3,4]}]", "[{\"color\":[1,2]}]", 1);

  /* 6. A term is equal to the superset term */
  CHECK_SUBSET("[{\"color\":[1,2],\"size\":[3,4]}]", "[{\"color\":[1,2],\"size\":[3,4]}]", 1);

  /* 7. The empty JSON_DNF is a subset of every other JSON_DNF */
  CHECK_SUBSET("[]", "[{\"color\":[1,2]}]", 1);

  /* 8. The universal JSON_DNF is a superset of every other JSON_DNF */
  CHECK_SUBSET("[{\"color\":[1,2]}]", "[{}]", 1);

  /* 9. The universal JSON_DNF is also equal to itself */
  CHECK_SUBSET("[{}]", "[{}]", 1);

  /* 10. The empty JSON_DNF is a subset of the universal JSON_DNF */
  CHECK_SUBSET("[]", "[{}]", 1);

  /* 11. The subset is covered by the union of several terms */
  CHECK_SUBSET("[{\"color\":[1,2]}]", "[{\"color\":[1]}, {\"color\":[2]}]", 1);

  /* 12. A subset is not subset if it contains a value not covered by the union */
  CHECK_SUBSET("[{\"color\":[1,2]}]", "[{\"color\":[1]}, {\"color\":[3]}]", 0);

  /* 13. Multi-attribute subset contained in the union of multiple terms */
  CHECK_SUBSET("[{\"color\":[1,2],\"size\":[1,2]}]", "[{\"color\":[1],\"size\":[1,2]}, {\"color\":[2],\"size\":[1,2]}]", 1);

  /* 14. Multi-attribute subset not contained in the union when one value falls outside */
  CHECK_SUBSET("[{\"color\":[1,2],\"size\":[1,3]}]", "[{\"color\":[1],\"size\":[1,2]}, {\"color\":[2],\"size\":[1,2]}]", 0);

  coDelete(psd);
  printf("All DNF subset tests passed successfully!\n");
}

void test_dnf_minimize(void) {
  printf("Running DNF minimization tests...\n");

  /* Helper to check minimization result */
  #define CHECK_MINIMIZE(json_in, expected_size) { \
    co dnf = coConvertToInt32Vector(coReadJSONByString(json_in)); \
    coDNFMinimizeANDTermSubset(dnf); \
    if (coVectorSize(dnf) != expected_size) printf("Failed: %s minimized size (expected %d, got %ld)\n", json_in, expected_size, coVectorSize(dnf)); \
    assert(coVectorSize(dnf) == expected_size); \
    coDelete(dnf); \
  }

  /* 1. Redundant term (subset) removed */
  CHECK_MINIMIZE("[{\"color\":[1]}, {\"color\":[1,2]}]", 1);

  /* 2. No terms removed when none are subsets */
  CHECK_MINIMIZE("[{\"color\":[1]}, {\"color\":[2]}]", 2);

  /* 3. Identical terms removed */
  CHECK_MINIMIZE("[{\"color\":[1]}, {\"color\":[1]}]", 1);

  /* 4. Multiple terms removed */
  CHECK_MINIMIZE("[{\"color\":[1]}, {\"color\":[2]}, {\"color\":[1,2]}]", 1);

  printf("All DNF minimization tests passed successfully!\n");
}

void test_dnf_merge(void) {
  printf("Running DNF term merge tests...\n");

  /* 1. Basic merge: three terms that could be merged into one */
  /* color: [1], color: [2], color: [3] -> color: [1,2,3] */
  co dnf = coConvertToInt32Vector(coReadJSONByString("[{\"color\":[1]}, {\"color\":[2]}, {\"color\":[3]}]"));
  coDNFMinimizeByANDTermMerge(dnf);
  assert(coVectorSize(dnf) == 1);
  cco m = coVectorGet(dnf, 0);
  assert(coInt32VectorSize(coMapGet(m, "color")) == 3);
  coDelete(dnf);

  /* 2. Merge with multiple attributes */
  /* color: [1], size: [1] AND color: [2], size: [1] -> color: [1,2], size: [1] */
  dnf = coConvertToInt32Vector(coReadJSONByString("[{\"color\":[1], \"size\":[1]}, {\"color\":[2], \"size\":[1]}]"));
  coDNFMinimizeByANDTermMerge(dnf);
  assert(coVectorSize(dnf) == 1);
  m = coVectorGet(dnf, 0);
  assert(coInt32VectorSize(coMapGet(m, "color")) == 2);
  assert(coInt32VectorSize(coMapGet(m, "size")) == 1);
  coDelete(dnf);

  /* 3. Partial merge: only some terms can be merged */
  /* color: [1], size: [1] AND color: [2], size: [1] AND color: [1], size: [2] */
  /* Result: {color:[1,2], size:[1]}, {color:[1], size:[2]} */
  dnf = coConvertToInt32Vector(coReadJSONByString("[{\"color\":[1], \"size\":[1]}, {\"color\":[2], \"size\":[1]}, {\"color\":[1], \"size\":[2]}]"));
  coDNFMinimizeByANDTermMerge(dnf);
  assert(coVectorSize(dnf) == 2);
  coDelete(dnf);

  /* 4. Merge resulting in broader terms */
  /* {a:1, b:1}, {a:1, b:2} -> {a:1, b:[1,2]} */
  dnf = coConvertToInt32Vector(coReadJSONByString("[{\"a\":[1], \"b\":[1]}, {\"a\":[1], \"b\":[2]}]"));
  coDNFMinimizeByANDTermMerge(dnf);
  assert(coVectorSize(dnf) == 1);
  m = coVectorGet(dnf, 0);
  assert(coInt32VectorSize(coMapGet(m, "a")) == 1);
  assert(coInt32VectorSize(coMapGet(m, "b")) == 2);
  coDelete(dnf);

  printf("All DNF term merge tests passed successfully!\n");
}

void test_dnf_subtract(void) {
  printf("Running DNF subtraction tests...\n");

  co psd = coNewPSD();
  coPSDExtendByDNF(psd, coConvertToInt32Vector(coReadJSONByString("[{\"color\":[1,2,3], \"size\":[1,2]}]")));

  #define CHECK_SUBTRACT(s1, s2, expected_json) { \
    co dnf1 = coConvertToInt32Vector(coReadJSONByString(s1)); \
    co dnf2 = coConvertToInt32Vector(coReadJSONByString(s2)); \
    co expected = coConvertToInt32Vector(coReadJSONByString(expected_json)); \
    co res = coNewDNFBySubtraction(psd, dnf1, dnf2); \
    if (!coDNFIsEqual(psd, res, expected)) { \
      printf("Failed Subtract: %s - %s\n", s1, s2); \
      printf("  Expected: %s\n", expected_json); \
      printf("  Got: "); coWriteJSON(res, 1, 0, stdout); printf("\n"); \
    } \
    assert(coDNFIsEqual(psd, res, expected)); \
    coDelete(dnf1); coDelete(dnf2); coDelete(expected); coDelete(res); \
  }

  /* 1. Subtract empty from non-empty */
  CHECK_SUBTRACT("[{\"color\":[1,2]}]", "[]", "[{\"color\":[1,2]}]");

  /* 2. Subtract non-empty from empty */
  CHECK_SUBTRACT("[]", "[{\"color\":[1,2]}]", "[]");

  /* 3. Subtract term from itself */
  CHECK_SUBTRACT("[{\"color\":[1,2]}]", "[{\"color\":[1,2]}]", "[]");

  /* 4. Subtract with shared attribute (subset values) */
  /* color: [1,2,3] - color: [2] -> color: [1,3] */
  CHECK_SUBTRACT("[{\"color\":[1,2,3]}]", "[{\"color\":[2]}]", "[{\"color\":[1,3]}]");

  /* 5. Subtract with multiple attributes */
  /* color:[1], size:[1] - color:[1] -> [] */
  CHECK_SUBTRACT("[{\"color\":[1], \"size\":[1]}]", "[{\"color\":[1]}]", "[]");

  /* 6. Subtract broader term */
  /* color:[1] - color:[1], size:[1] -> color:[1], size:[2] (assuming domain is {1,2}) */
  CHECK_SUBTRACT("[{\"color\":[1]}]", "[{\"color\":[1], \"size\":[1]}]", "[{\"color\":[1], \"size\":[2]}]");

  coDelete(psd);
  printf("All DNF subtraction tests passed successfully!\n");
}

void test_dnf_complement(void) {
  printf("Running DNF complement tests...\n");

  co psd = coNewPSD();
  coPSDExtendByDNF(psd, coConvertToInt32Vector(coReadJSONByString("[{\"color\":[1,2,3]}]")));

  /* 1. Complement of empty is universal */
  co empty = coNewVector(CO_FREE_VALS);
  co cempty = coDNFComplementBySubtract(psd, empty);
  assert(coDNFIsUniversal(cempty));
  coDelete(empty); coDelete(cempty);

  /* 2. Complement of universal is empty */
  co universal = coNewVector(CO_FREE_VALS);
  coVectorAdd(universal, coNewMap(CO_STRDUP | CO_FREE_VALS));
  co cuni = coDNFComplementBySubtract(psd, universal);
  assert(coDNFIsEmpty(cuni));
  coDelete(universal); coDelete(cuni);

  /* 3. Complement of a single value term */
  /* color: [1] -> complement color: [2,3] */
  co dnf = coConvertToInt32Vector(coReadJSONByString("[{\"color\":[1]}]"));
  co cdnf = coDNFComplementBySubtract(psd, dnf);
  assert(coVectorSize(cdnf) == 1);
  cco m = coVectorGet(cdnf, 0);
  assert(coInt32VectorSize(coMapGet(m, "color")) == 2);
  coDelete(dnf); coDelete(cdnf);

  /* 4. Complement of a multi-valued attribute term */
  /* color: [1,2] -> complement color: [3] */
  dnf = coConvertToInt32Vector(coReadJSONByString("[{\"color\":[1,2]}]"));
  cdnf = coDNFComplementBySubtract(psd, dnf);
  assert(coVectorSize(cdnf) == 1);
  m = coVectorGet(cdnf, 0);
  assert(coInt32VectorSize(coMapGet(m, "color")) == 1);
  coDelete(dnf); coDelete(cdnf);

  /* 5. Double complement should be equal to original (size-wise here) */
  dnf = coConvertToInt32Vector(coReadJSONByString("[{\"color\":[1]}]"));
  co c1 = coDNFComplementBySubtract(psd, dnf);
  co c2 = coDNFComplementBySubtract(psd, c1);
  assert(coDNFIsEqual(psd, dnf, c2));
  coDelete(dnf); coDelete(c1); coDelete(c2);

  /* 6. In-place complement test */
  dnf = coConvertToInt32Vector(coReadJSONByString("[{\"color\":[1]}]"));
  coDNFComplement(psd, dnf);
  /* Should be color: [2,3] */
  assert(coVectorSize(dnf) == 1);
  m = coVectorGet(dnf, 0);
  assert(coInt32VectorSize(coMapGet(m, "color")) == 2);
  
  /* Complement again in-place */
  coDNFComplement(psd, dnf);
  /* Should be back to color: [1] */
  assert(coVectorSize(dnf) == 1);
  m = coVectorGet(dnf, 0);
  assert(coInt32VectorSize(coMapGet(m, "color")) == 1);
  coDelete(dnf);

  coDelete(psd);
  printf("All DNF complement tests passed successfully!\n");
}

void test_dnf_equal(void) {
  printf("Running DNF equality tests...\n");
  co psd = coNewPSD();
  coPSDExtendByDNF(psd, coConvertToInt32Vector(coReadJSONByString("[{\"a\":[1,2], \"b\":[1,2]}]")));

  /* Helper to check equality */
  #define CHECK_EQUAL(s1, s2, expected) { \
    co dnf1 = coConvertToInt32Vector(coReadJSONByString(s1)); \
    co dnf2 = coConvertToInt32Vector(coReadJSONByString(s2)); \
    assert(coDNFIsEqual(psd, dnf1, dnf2) == expected); \
    coDelete(dnf1); coDelete(dnf2); \
  }

  /* 1. Identical terms */
  CHECK_EQUAL("[{\"a\":[1]}]", "[{\"a\":[1]}]", 1);

  /* 2. Structurally different but logically equal union */
  CHECK_EQUAL("[{\"a\":[1,2]}]", "[{\"a\":[1]}, {\"a\":[2]}]", 1);

  /* 3. Different values */
  CHECK_EQUAL("[{\"a\":[1]}]", "[{\"a\":[2]}]", 0);

  /* 4. Different attributes */
  CHECK_EQUAL("[{\"a\":[1]}]", "[{\"b\":[1]}]", 0);

  coDelete(psd);
  printf("All DNF equality tests passed successfully!\n");
}

void test_dnf_cofactor(void) {
  printf("Running DNF cofactor tests...\n");
  co psd = coNewPSD();
  coPSDExtendByDNF(psd, coConvertToInt32Vector(coReadJSONByString("[{\"a\":[1,2], \"b\":[1,2]}]")));

  /* 1. Simple cofactor */
  /* {a:[1,2], b:1} with a=1 -> {b:1} */
  co dnf = coConvertToInt32Vector(coReadJSONByString("[{\"a\":[1,2], \"b\":[1]}]"));
  co cofact = coDNFNewCofactor(psd, dnf, "a", 1);
  assert(coVectorSize(cofact) == 1);
  cco m = coVectorGet(cofact, 0);
  assert(coMapGet(m, "a") == NULL);
  assert(coMapGet(m, "b") != NULL);
  coDelete(dnf); coDelete(cofact);

  /* 2. Cofactor resulting in empty set */
  dnf = coConvertToInt32Vector(coReadJSONByString("[{\"a\":[1]}]"));
  cofact = coDNFNewCofactor(psd, dnf, "a", 2);
  assert(coDNFIsEmpty(cofact));
  coDelete(dnf); coDelete(cofact);

  /* 3. Cofactor with missing attribute (identity) */
  dnf = coConvertToInt32Vector(coReadJSONByString("[{\"b\":[1]}]"));
  cofact = coDNFNewCofactor(psd, dnf, "a", 1);
  assert(coDNFIsEqual(psd, dnf, cofact));
  coDelete(dnf); coDelete(cofact);

  coDelete(psd);
  printf("All DNF cofactor tests passed successfully!\n");
}

void test_dnf_best_attr(void) {
  printf("Running DNF best cofactor attribute tests...\n");
  co psd = coNewPSD();
  coPSDExtendByDNF(psd, coConvertToInt32Vector(coReadJSONByString("[{\"a\":[1,2], \"b\":[1,2]}]")));

  /* Attribute appearing in most terms should be chosen */
  co dnf = coConvertToInt32Vector(coReadJSONByString("[{\"a\":[1], \"b\":[1]}, {\"a\":[2]}]"));
  const char *best = coDNFGetBestCofactorAttribute(psd, dnf);
  assert(strcmp(best, "a") == 0);
  coDelete(dnf);

  coDelete(psd);
  printf("All DNF best attribute tests passed successfully!\n");
}

void test_dnf_check_universal(void) {
  printf("Running DNF check universal tests...\n");
  co psd = coNewPSD();
  coPSDExtendByDNF(psd, coConvertToInt32Vector(coReadJSONByString("[{\"a\":[1,2], \"b\":[1,2]}]")));

  /* 1. Explicitly universal */
  co dnf = coConvertToInt32Vector(coReadJSONByString("[{}]"));
  assert(coDNFCheckUniversal(psd, dnf) == 1);
  coDelete(dnf);

  /* 2. Logically universal through union */
  dnf = coConvertToInt32Vector(coReadJSONByString("[{\"a\":[1]}, {\"a\":[2]}]"));
  assert(coDNFCheckUniversal(psd, dnf) == 1);
  coDelete(dnf);

  /* 3. Not universal (missing value) */
  dnf = coConvertToInt32Vector(coReadJSONByString("[{\"a\":[1]}]"));
  assert(coDNFCheckUniversal(psd, dnf) == 0);
  coDelete(dnf);

  /* 4. Not universal (missing combination) */
  /* {a:1, b:[1,2]} u {a:2, b:1} -> missing {a:2, b:2} */
  dnf = coConvertToInt32Vector(coReadJSONByString("[{\"a\":[1]}, {\"a\":[2], \"b\":[1]}]"));
  assert(coDNFCheckUniversal(psd, dnf) == 0);
  coDelete(dnf);

  coDelete(psd);
  printf("All DNF check universal tests passed successfully!\n");
}

void test_dnf_get_volume(void) {
  printf("Running DNF get volume tests...\n");

  co psd = coNewPSD();
  coPSDExtendByDNF(psd, coConvertToInt32Vector(coReadJSONByString("[{\"color\":[1,2,3], \"size\":[1,2]}]")));

  /* 1. Single term */
  co dnf = coConvertToInt32Vector(coReadJSONByString("[{\"color\":[1]}]"));
  int32_t s1 = coDNFGetVolume(psd, dnf);
  if (s1 != 2) printf("Failed Case 1: expected 2, got %d\n", s1);
  assert(s1 == 2); /* color:1 * size:{1,2} */
  coDelete(dnf);

  /* 2. Overlapping terms */
  /* Term A: {color:1} -> coverage {1,1}, {1,2} (size 2) */
  /* Term B: {size:1}  -> coverage {1,1}, {2,1}, {3,1} (size 3) */
  /* Union: {1,1}, {1,2}, {2,1}, {3,1} (size 4) */
  dnf = coConvertToInt32Vector(coReadJSONByString("[{\"color\":[1]}, {\"size\":[1]}]"));
  assert(coDNFGetVolume(psd, dnf) == 4);
  coDelete(dnf);

  /* 3. Universal set */
  dnf = coConvertToInt32Vector(coReadJSONByString("[{}]"));
  assert(coDNFGetVolume(psd, dnf) == 6);
  coDelete(dnf);

  /* 4. Empty set */
  dnf = coConvertToInt32Vector(coReadJSONByString("[]"));
  assert(coDNFGetVolume(psd, dnf) == 0);
  coDelete(dnf);

  coDelete(psd);
  printf("All DNF get volume tests passed successfully!\n");
}

int main() {
  test_dnf();
  test_dnf_subset_and_term();
  test_dnf_minimize();
  test_dnf_merge();
  test_dnf_subtract();
  test_dnf_complement();
  test_dnf_equal();
  test_dnf_cofactor();
  test_dnf_best_attr();
  test_dnf_check_universal();
  test_dnf_get_volume();
  test_dnf_intersection_detailed();
  return 0;
}
