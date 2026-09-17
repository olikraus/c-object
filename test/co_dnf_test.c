#include "co.h"
#include <stdio.h>
#include <assert.h>
#include <string.h>

void test_dnf(void) {
  printf("Running DNF unit tests...\n");

  /* 1. Test coConvertToInt32Vector & coDNFIsValid */
  const char *json_dnf = "[ {\"1\": [1, 2, 3], \"2\": [4, 5, 6]}, {\"3\": [7, 8, 9]} ]";
  co dnf = coReadJSONByString(json_dnf);
  assert(dnf != NULL);
  assert(coIsVector(dnf));
  
  /* Initially before conversion, map values are standard Vectors, not Int32Vectors */
  cco first_clause = coVectorGet(dnf, 0);
  assert(first_clause != NULL && coIsMap(first_clause));
  cco val1 = coMapGet(first_clause, "1");
  assert(val1 != NULL && coIsVector(val1));
  assert(!coIsInt32Vector(val1));
  
  /* Verify that it's initially invalid since children are standard Vectors */
  assert(coDNFIsValid(dnf) == 0);

  /* Convert */
  co converted = coConvertToInt32Vector(dnf);
  assert(converted == dnf); /* Should be the same root pointer */
  
  /* Check converted types */
  val1 = coMapGet(first_clause, "1");
  assert(val1 != NULL);
  assert(coIsInt32Vector(val1));
  assert(coInt32VectorSize(val1) == 3);
  assert(coInt32VectorGet(val1, 0) == 1);
  assert(coInt32VectorGet(val1, 1) == 2);
  assert(coInt32VectorGet(val1, 2) == 3);

  /* Verify DNF is now valid */
  assert(coDNFIsValid(dnf) == 1);

  /* 2. Test coDNFIsEmpty */
  co empty_dnf = coReadJSONByString("[]");
  assert(empty_dnf != NULL);
  assert(coDNFIsEmpty(empty_dnf) == 1);
  assert(coDNFIsEmpty(dnf) == 0);

  /* 3. Test coDNFIsUniversal */
  co universal_dnf = coReadJSONByString("[{}]");
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

  int union_res = coDNFUnion(dnf, dnf2);
  assert(union_res == 1);
  assert(coVectorSize(dnf) == 3);
  assert(coDNFIsValid(dnf) == 1);

  /* Check that elements from dnf2 are present in dnf */
  cco third_clause = coVectorGet(dnf, 2);
  assert(third_clause != NULL && coIsMap(third_clause));
  cco val4 = coMapGet(third_clause, "4");
  assert(val4 != NULL && coIsInt32Vector(val4));
  assert(coInt32VectorSize(val4) == 3);
  assert(coInt32VectorGet(val4, 0) == 10);

  /* 5. Test coNewPSD & coPSDExtendByDNF */
  co psd = coNewPSD();
  assert(psd != NULL);
  assert(coIsMap(psd));
  cco psd_inner = coMapGet(psd, "psd");
  assert(psd_inner != NULL && coIsMap(psd_inner));
  assert(coMapSize(psd_inner) == 0);

  /* Create a test DNF to extend the PSD */
  const char *json_extend = "[ {\"1\": [1, 2], \"2\": [4, 5]}, {\"2\": [5, 6], \"3\": [8]} ]";
  co dnf_extend = coReadJSONByString(json_extend);
  dnf_extend = coConvertToInt32Vector(dnf_extend);
  assert(coDNFIsValid(dnf_extend) == 1);

  int extend_res = coPSDExtendByDNF(psd, dnf_extend);
  assert(extend_res == 1);

  /* Verify attributes are present in PSD */
  assert(coMapSize(psd_inner) == 3); /* "1", "2", "3" */
  
  cco vec1 = coMapGet(psd_inner, "1");
  assert(vec1 != NULL && coIsInt32Vector(vec1));
  assert(coInt32VectorSize(vec1) == 2);
  assert(coInt32VectorGet(vec1, 0) == 1);
  assert(coInt32VectorGet(vec1, 1) == 2);

  cco vec2 = coMapGet(psd_inner, "2");
  assert(vec2 != NULL && coIsInt32Vector(vec2));
  assert(coInt32VectorSize(vec2) == 3); /* 4, 5, 6 (no duplicates!) */
  assert(coInt32VectorGet(vec2, 0) == 4);
  assert(coInt32VectorGet(vec2, 1) == 5);
  assert(coInt32VectorGet(vec2, 2) == 6);

  cco vec3 = coMapGet(psd_inner, "3");
  assert(vec3 != NULL && coIsInt32Vector(vec3));
  assert(coInt32VectorSize(vec3) == 1);
  assert(coInt32VectorGet(vec3, 0) == 8);

  coDelete(psd);
  coDelete(dnf_extend);

  /* 6. Test coDNFIntersection */
  /* Case 1: Intersection of empty with non-empty */
  co isec_arg1 = coReadJSONByString("[]");
  co isec_arg2 = coReadJSONByString("[{\"color\":[1,2]}]");
  isec_arg2 = coConvertToInt32Vector(isec_arg2);
  assert(coDNFIntersection(isec_arg1, isec_arg2) == 1);
  assert(coDNFIsEmpty(isec_arg1) == 1);
  coDelete(isec_arg1);
  coDelete(isec_arg2);

  /* Case 2: Intersection of universal with non-empty */
  isec_arg1 = coReadJSONByString("[{}]");
  isec_arg2 = coReadJSONByString("[{\"color\":[1,2]}]");
  isec_arg2 = coConvertToInt32Vector(isec_arg2);
  assert(coDNFIntersection(isec_arg1, isec_arg2) == 1);
  assert(coDNFIsValid(isec_arg1) == 1);
  assert(coVectorSize(isec_arg1) == 1);
  cco clause0 = coVectorGet(isec_arg1, 0);
  cco color_vec = coMapGet(clause0, "color");
  assert(color_vec != NULL && coIsInt32Vector(color_vec));
  assert(coInt32VectorSize(color_vec) == 2);
  assert(coInt32VectorGet(color_vec, 0) == 1);
  coDelete(isec_arg1);
  coDelete(isec_arg2);

  /* Case 3: Intersection with shared attributes */
  isec_arg1 = coReadJSONByString("[{\"color\":[1,2,3],\"material\":[2,3]}]");
  isec_arg2 = coReadJSONByString("[{\"color\":[2,3,4],\"size\":[1]}]");
  isec_arg1 = coConvertToInt32Vector(isec_arg1);
  isec_arg2 = coConvertToInt32Vector(isec_arg2);
  assert(coDNFIntersection(isec_arg1, isec_arg2) == 1);
  assert(coDNFIsValid(isec_arg1) == 1);
  assert(coVectorSize(isec_arg1) == 1);
  cco res_clause = coVectorGet(isec_arg1, 0);
  cco res_color = coMapGet(res_clause, "color");
  cco res_mat = coMapGet(res_clause, "material");
  cco res_size = coMapGet(res_clause, "size");
  assert(res_color != NULL && coInt32VectorSize(res_color) == 2); /* Common: 2, 3 */
  assert(coInt32VectorGet(res_color, 0) == 2);
  assert(coInt32VectorGet(res_color, 1) == 3);
  assert(res_mat != NULL && coInt32VectorSize(res_mat) == 2); /* 2, 3 */
  assert(res_size != NULL && coInt32VectorSize(res_size) == 1); /* 1 */
  coDelete(isec_arg1);
  coDelete(isec_arg2);

  /* Case 4: Disjunct attributes */
  isec_arg1 = coReadJSONByString("[{\"color\":[1,2]}]");
  isec_arg2 = coReadJSONByString("[{\"size\":[3,4]}]");
  isec_arg1 = coConvertToInt32Vector(isec_arg1);
  isec_arg2 = coConvertToInt32Vector(isec_arg2);
  assert(coDNFIntersection(isec_arg1, isec_arg2) == 1);
  assert(coDNFIsValid(isec_arg1) == 1);
  cco d_clause = coVectorGet(isec_arg1, 0);
  cco d_color = coMapGet(d_clause, "color");
  cco d_size = coMapGet(d_clause, "size");
  assert(d_color != NULL && coInt32VectorSize(d_color) == 2);
  assert(d_size != NULL && coInt32VectorSize(d_size) == 2);
  coDelete(isec_arg1);
  coDelete(isec_arg2);

  /* Case 7: Disjunct values (no valid result) */
  isec_arg1 = coReadJSONByString("[{\"color\":[1,2]}]");
  isec_arg2 = coReadJSONByString("[{\"color\":[3,4]}]");
  isec_arg1 = coConvertToInt32Vector(isec_arg1);
  isec_arg2 = coConvertToInt32Vector(isec_arg2);
  assert(coDNFIntersection(isec_arg1, isec_arg2) == 1);
  assert(coDNFIsEmpty(isec_arg1) == 1);
  coDelete(isec_arg1);
  coDelete(isec_arg2);

  /* Test coNewDNFByIntersection directly */
  co arg1 = coReadJSONByString("[{\"color\":[1,2]}]");
  co arg2 = coReadJSONByString("[{\"size\":[3,4]}]");
  arg1 = coConvertToInt32Vector(arg1);
  arg2 = coConvertToInt32Vector(arg2);
  co isec_new = coNewDNFByIntersection(arg1, arg2);
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

  /* 2. Larger term (superset) kept */
  CHECK_MINIMIZE("[{\"color\":[1,2]}, {\"color\":[1]}]", 1);

  /* 3. Identical terms removed (one kept) */
  CHECK_MINIMIZE("[{\"color\":[1]}, {\"color\":[1]}]", 1);

  /* 4. Non-redundant terms kept */
  CHECK_MINIMIZE("[{\"color\":[1]}, {\"color\":[2]}]", 2);

  /* 5. Term with more attributes is subset of term with fewer attributes (Example 5 logic) */
  CHECK_MINIMIZE("[{\"color\":[1,2], \"size\":[3,4]}, {\"color\":[1,2]}]", 1);

  /* 6. Multiple redundant terms */
  CHECK_MINIMIZE("[{\"color\":[1]}, {\"color\":[2]}, {\"color\":[1,2,3]}]", 1);

  printf("All DNF minimization tests passed successfully!\n");
}

void test_dnf_subtract(void) {
  printf("Running DNF subtraction tests...\n");

  co psd = coNewPSD();
  coPSDExtendByDNF(psd, coConvertToInt32Vector(coReadJSONByString("[{\"color\":[1,2,3], \"size\":[1,2,3,4]}]")));

  /* Helper to check subtraction result */
  #define CHECK_SUBTRACT(left_json, right_json, expected_size) { \
    co left = coConvertToInt32Vector(coReadJSONByString(left_json)); \
    co right = coConvertToInt32Vector(coReadJSONByString(right_json)); \
    co res = coNewDNFBySubtraction(psd, left, right); \
    if (coVectorSize(res) != expected_size) printf("Failed: %s minus %s size (expected %d, got %ld)\n", left_json, right_json, expected_size, coVectorSize(res)); \
    assert(coVectorSize(res) == expected_size); \
    coDelete(left); \
    coDelete(right); \
    coDelete(res); \
  }

  /* 1. Full overlap -> empty result */
  CHECK_SUBTRACT("[{\"color\":[1]}]", "[{\"color\":[1]}]", 0);

  /* 2. No overlap -> original left kept */
  CHECK_SUBTRACT("[{\"color\":[1]}]", "[{\"color\":[2]}]", 1);

  /* 3. Partial overlap (single attribute) */
  /* left: {1,2}, right: {1} -> result: {2} */
  CHECK_SUBTRACT("[{\"color\":[1,2]}]", "[{\"color\":[1]}]", 1);

  /* 4. Multi-term subtraction */
  /* left: {1,2}, right: {1}, {2} -> result: empty */
  CHECK_SUBTRACT("[{\"color\":[1,2]}]", "[{\"color\":[1]}, {\"color\":[2]}]", 0);

  /* 5. Subtraction from universal set */
  /* left: {}, right: {color: 1} -> result: {color: {2,3}} */
  /* Note: 2,3 are remaining values in PSD for color */
  CHECK_SUBTRACT("[{}]", "[{\"color\":[1]}]", 1);

  coDelete(psd);
  printf("All DNF subtraction tests passed successfully!\n");
}

void test_dnf_complement(void) {
  printf("Running DNF complement tests...\n");

  co psd = coNewPSD();
  coPSDExtendByDNF(psd, coConvertToInt32Vector(coReadJSONByString("[{\"color\":[1,2,3], \"size\":[1,2,3,4]}]")));

  /* Helper to check complement result */
  #define CHECK_COMPLEMENT(json_in, expected_size) { \
    co in = coConvertToInt32Vector(coReadJSONByString(json_in)); \
    co res = coDNFComplementBySubtract(psd, in); \
    if (coVectorSize(res) != expected_size) printf("Failed: complement of %s size (expected %d, got %ld)\n", json_in, expected_size, coVectorSize(res)); \
    assert(coVectorSize(res) == expected_size); \
    coDelete(in); \
    coDelete(res); \
  }

  /* 1. Complement of empty set -> universal set [{}] (size 1) */
  CHECK_COMPLEMENT("[]", 1);

  /* 2. Complement of universal set -> empty set [] (size 0) */
  CHECK_COMPLEMENT("[{}]", 0);

  /* 3. Complement of a single attribute term */
  /* PSD has color: [1,2,3]. Complement of {color: 1} should be {color: [2,3]} (size 1) */
  CHECK_COMPLEMENT("[{\"color\":[1]}]", 1);

  /* 4. Complement of a multi-valued attribute term */
  /* color: [1,2] -> complement color: [3] */
  CHECK_COMPLEMENT("[{\"color\":[1,2]}]", 1);

  /* 5. Double complement should be equal to original (size-wise here) */
  {
    co in = coConvertToInt32Vector(coReadJSONByString("[{\"color\":[1]}]"));
    co c1 = coDNFComplementBySubtract(psd, in);
    co c2 = coDNFComplementBySubtract(psd, c1);
    assert(coVectorSize(c2) == 1);
    cco m = coVectorGet(c2, 0);
    cco v = coMapGet(m, "color");
    assert(coInt32VectorSize(v) == 1);
    assert(coInt32VectorGet(v, 0) == 1);
    coDelete(in);
    coDelete(c1);
    coDelete(c2);
  }

  /* 6. In-place complement test */
  {
    co in = coConvertToInt32Vector(coReadJSONByString("[{\"color\":[1]}]"));
    assert(coDNFComplement(psd, in) == 1);
    /* Should be color: [2,3] */
    assert(coVectorSize(in) == 1);
    cco m = coVectorGet(in, 0);
    cco v = coMapGet(m, "color");
    assert(coInt32VectorSize(v) == 2);
    
    /* Complement again in-place */
    assert(coDNFComplement(psd, in) == 1);
    /* Should be back to color: [1] */
    assert(coVectorSize(in) == 1);
    m = coVectorGet(in, 0);
    v = coMapGet(m, "color");
    assert(coInt32VectorSize(v) == 1);
    assert(coInt32VectorGet(v, 0) == 1);
    
    coDelete(in);
  }

  coDelete(psd);
  printf("All DNF complement tests passed successfully!\n");
}

void test_dnf_equal(void) {
  printf("Running DNF equality tests...\n");

  co psd = coNewPSD();
  coPSDExtendByDNF(psd, coConvertToInt32Vector(coReadJSONByString("[{\"color\":[1,2,3], \"size\":[1,2,3,4]}]")));

  /* Helper to check equality */
  #define CHECK_EQUAL(s1, s2, expected) { \
    co d1 = coConvertToInt32Vector(coReadJSONByString(s1)); \
    co d2 = coConvertToInt32Vector(coReadJSONByString(s2)); \
    int res = coDNFIsEqual(psd, d1, d2); \
    if (res != expected) printf("Failed: %s equal to %s (expected %d, got %d)\n", s1, s2, expected, res); \
    assert(res == expected); \
    coDelete(d1); \
    coDelete(d2); \
  }

  /* 1. Identical terms */
  CHECK_EQUAL("[{\"color\":[1]}]", "[{\"color\":[1]}]", 1);

  /* 2. Structurally different but mathematically equal (redundancy) */
  CHECK_EQUAL("[{\"color\":[1]}, {\"color\":[1,2]}]", "[{\"color\":[1,2]}]", 1);

  /* 3. Mathematically equal via union (Example 11/13 logic) */
  CHECK_EQUAL("[{\"color\":[1,2]}]", "[{\"color\":[1]}, {\"color\":[2]}]", 1);

  /* 4. Different values (not equal) */
  CHECK_EQUAL("[{\"color\":[1]}]", "[{\"color\":[2]}]", 0);

  /* 5. Missing vs Universal (PSD has color: [1,2,3]) */
  /* If color is omitted in left, it's 1,2,3. If color: [1,2,3] is in right, they are equal. */
  CHECK_EQUAL("[{}]", "[{\"color\":[1,2,3]}]", 1);

  coDelete(psd);
  printf("All DNF equality tests passed successfully!\n");
}

void test_dnf_cofactor(void) {
  printf("Running DNF cofactor tests...\n");

  co psd = coNewPSD();
  coPSDExtendByDNF(psd, coConvertToInt32Vector(coReadJSONByString("[{\"color\":[1,2,3], \"size\":[1,2,3,4]}]")));

  /* Helper to check cofactor result size */
  #define CHECK_COFACTOR(json_in, attr, val, expected_size) { \
    co in = coConvertToInt32Vector(coReadJSONByString(json_in)); \
    co res = coDNFNewCofactor(psd, in, attr, val); \
    if (coVectorSize(res) != expected_size) printf("Failed: cofactor of %s at %s=%d size (expected %d, got %ld)\n", json_in, attr, val, expected_size, coVectorSize(res)); \
    assert(coVectorSize(res) == expected_size); \
    coDelete(in); \
    coDelete(res); \
  }

  /* 1. Identity Case: attr not in term */
  CHECK_COFACTOR("[{\"color\":[1]}]", "size", 1, 1);

  /* 2. Fulfillment Case: value in term */
  /* {"color": [1,2]} at color=1 -> [{}] (Universal DNF, size 1) */
  CHECK_COFACTOR("[{\"color\":[1,2]}]", "color", 1, 1);
  {
    co in = coConvertToInt32Vector(coReadJSONByString("[{\"color\":[1,2]}]"));
    co res = coDNFNewCofactor(psd, in, "color", 1);
    assert(coDNFIsUniversal(res));
    coDelete(in);
    coDelete(res);
  }

  /* 3. Conflict Case: value not in term */
  /* {"color": [1,2]} at color=3 -> [] (Empty DNF, size 0) */
  CHECK_COFACTOR("[{\"color\":[1,2]}]", "color", 3, 0);

  /* 4. Mixed term DNF */
  /* [ {color: 1}, {size: 2} ] at color=1 -> [ {}, {size: 2} ] -> [{}] (Universal) */
  CHECK_COFACTOR("[{\"color\":[1]}, {\"size\":[2]}]", "color", 1, 1);
  
  /* [ {color: 1}, {size: 2} ] at color=2 -> [ {size: 2} ] (Size 1) */
  CHECK_COFACTOR("[{\"color\":[1]}, {\"size\":[2]}]", "color", 2, 1);

  coDelete(psd);
  printf("All DNF cofactor tests passed successfully!\n");
}

void test_dnf_best_attr(void) {
  printf("Running DNF best cofactor attribute tests...\n");

  co psd = coNewPSD();
  co dnf = coConvertToInt32Vector(coReadJSONByString("[{\"a\":[1], \"b\":[1]}, {\"b\":[2]}]"));
  coPSDExtendByDNF(psd, dnf);
  
  const char *best = coDNFGetBestCofactorAttribute(psd, dnf);
  assert(best != NULL);
  /* 'b' appears twice, 'a' appears once. */
  assert(strcmp(best, "b") == 0);

  coDelete(dnf);
  coDelete(psd);
  printf("All DNF best attribute tests passed successfully!\n");
}

void test_dnf_check_universal(void) {
  printf("Running DNF check universal tests...\n");

  co psd = coNewPSD();
  coPSDExtendByDNF(psd, coConvertToInt32Vector(coReadJSONByString("[{\"color\":[1,2,3], \"size\":[1,2]}]")));

  /* Helper to check universal coverage */
  #define CHECK_UNIVERSAL(json_in, expected) { \
    co in = coConvertToInt32Vector(coReadJSONByString(json_in)); \
    int res = coDNFCheckUniversal(psd, in); \
    if (res != expected) printf("Failed: check universal of %s (expected %d, got %d)\n", json_in, expected, res); \
    assert(res == expected); \
    coDelete(in); \
  }

  /* 1. Explicit Universal Set */
  CHECK_UNIVERSAL("[{}]", 1);

  /* 2. Empty Set (not universal) */
  CHECK_UNIVERSAL("[]", 0);

  /* 3. Single term covering full domain (via elision logic implicitly) */
  /* If PSD has color 1,2,3, then {color: [1,2,3]} is universal */
  CHECK_UNIVERSAL("[{\"color\":[1,2,3]}]", 1);

  /* 4. Multi-term cover (Boolean case: A or NOT A) */
  /* color: [1,2,3]. Term 1: color 1. Term 2: color 2,3. Result: universal. */
  CHECK_UNIVERSAL("[{\"color\":[1]}, {\"color\":[2,3]}]", 1);

  /* 5. Missing value (not universal) */
  CHECK_UNIVERSAL("[{\"color\":[1]}, {\"color\":[2]}]", 0);

  /* 6. Multi-attribute cross-term cover */
  /* color: [1,2,3], size: [1,2]. Cover: {color:1} OR {color:[2,3], size:1} OR {color:[2,3], size:2} */
  CHECK_UNIVERSAL("[{\"color\":[1]}, {\"color\":[2,3], \"size\":[1]}, {\"color\":[2,3], \"size\":[2]}]", 1);

  /* 7. Cross-verify both methods */
  {
    co in = coConvertToInt32Vector(coReadJSONByString("[{\"color\":[1]}, {\"color\":[2,3], \"size\":[1]}, {\"color\":[2,3], \"size\":[2]}]"));
    int r1 = coDNFCheckUniversal(psd, in);
    int r2 = coDNFCheckUniversalByComplement(psd, in);
    assert(r1 == r2);
    assert(r1 == 1);
    coDelete(in);
  }

  coDelete(psd);
  printf("All DNF check universal tests passed successfully!\n");
}

void test_dnf_merge(void) {
  printf("Running DNF term merge tests...\n");

  /* 1. Basic merge: three terms with same attribute */
  co dnf = coConvertToInt32Vector(coReadJSONByString("[{\"color\":[1]}, {\"color\":[2]}, {\"color\":[3]}]"));
  //coDNFMinimizeByANDTermMerge(dnf);
  assert(coVectorSize(dnf) == 1);
  cco m = coVectorGet(dnf, 0);
  cco v = coMapGet(m, "color");
  assert(coInt32VectorSize(v) == 3);
  coDelete(dnf);

  /* 2. No merge: different attributes */
  dnf = coConvertToInt32Vector(coReadJSONByString("[{\"a\":[1]}, {\"b\":[1]}]"));
  //coDNFMinimizeByANDTermMerge(dnf);
  assert(coVectorSize(dnf) == 2);
  coDelete(dnf);

  /* 3. No merge: differ in two attributes */
  dnf = coConvertToInt32Vector(coReadJSONByString("[{\"a\":[1], \"b\":[1]}, {\"a\":[2], \"b\":[2]}]"));
  //coDNFMinimizeByANDTermMerge(dnf);
  assert(coVectorSize(dnf) == 2);
  coDelete(dnf);

  /* 4. Merge: share one, differ in one */
  dnf = coConvertToInt32Vector(coReadJSONByString("[{\"a\":[1], \"b\":[1]}, {\"a\":[1], \"b\":[2]}]"));
  //coDNFMinimizeByANDTermMerge(dnf);
  assert(coVectorSize(dnf) == 1);
  m = coVectorGet(dnf, 0);
  assert(coInt32VectorSize(coMapGet(m, "a")) == 1);
  assert(coInt32VectorSize(coMapGet(m, "b")) == 2);
  coDelete(dnf);

  printf("All DNF term merge tests passed successfully!\n");
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
  return 0;
}
