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

int main() {
  test_dnf();
  return 0;
}
