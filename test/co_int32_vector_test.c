#include "co.h"
#include <stdio.h>
#include <assert.h>
#include <string.h>

void run_tests(void) {
  printf("Running Int32Vector unit tests...\n");

  /* 1. Construction and basic checks */
  co v = coNewInt32Vector(CO_NONE);
  assert(v != NULL);
  assert(coIsInt32Vector(v));
  assert(coInt32VectorEmpty(v) == 1);
  assert(coInt32VectorSize(v) == 0);

  /* 2. Add elements */
  long idx1 = coInt32VectorAdd(v, 10);
  long idx2 = coInt32VectorAdd(v, 20);
  long idx3 = coInt32VectorAdd(v, 30);
  assert(idx1 == 0);
  assert(idx2 == 1);
  assert(idx3 == 2);
  assert(coInt32VectorEmpty(v) == 0);
  assert(coInt32VectorSize(v) == 3);

  /* 2b. Add unique elements */
  long idx4 = coInt32VectorAddUnique(v, 20); /* Duplicate, should return existing index (1) */
  long idx5 = coInt32VectorAddUnique(v, 40); /* New element, should add and return index (3) */
  assert(idx4 == 1);
  assert(idx5 == 3);
  assert(coInt32VectorSize(v) == 4);
  assert(coInt32VectorGet(v, 3) == 40);

  /* 3. Get elements */
  assert(coInt32VectorGet(v, 0) == 10);
  assert(coInt32VectorGet(v, 1) == 20);
  assert(coInt32VectorGet(v, 2) == 30);
  assert(coInt32VectorGet(v, -1) == 0);
  assert(coInt32VectorGet(v, 3) == 40);
  assert(coInt32VectorGet(v, 4) == 0);

  /* 4. Set elements */
  coInt32VectorSet(v, 1, 25);
  assert(coInt32VectorGet(v, 1) == 25);

  /* 5. Exists and Find */
  assert(coInt32VectorExists(v, 10) == 1);
  assert(coInt32VectorExists(v, 25) == 1);
  assert(coInt32VectorExists(v, 99) == 0);
  assert(coInt32VectorFind(v, 10) == 0);
  assert(coInt32VectorFind(v, 25) == 1);
  assert(coInt32VectorFind(v, 30) == 2);
  assert(coInt32VectorFind(v, 99) == -1);

  /* 6. Erase by index */
  coInt32VectorErase(v, 1); /* removes 25 */
  assert(coInt32VectorSize(v) == 3);
  assert(coInt32VectorGet(v, 0) == 10);
  assert(coInt32VectorGet(v, 1) == 30);
  assert(coInt32VectorGet(v, 2) == 40);

  /* 7. Erase last */
  coInt32VectorEraseLast(v); /* removes 40 */
  assert(coInt32VectorSize(v) == 2);
  assert(coInt32VectorGet(v, 0) == 10);
  assert(coInt32VectorGet(v, 1) == 30);

  coInt32VectorEraseLast(v); /* removes 30 */
  assert(coInt32VectorSize(v) == 1);
  assert(coInt32VectorGet(v, 0) == 10);

  /* 8. Clear */
  coInt32VectorClear(v);
  assert(coInt32VectorSize(v) == 0);
  assert(coInt32VectorEmpty(v) == 1);

  /* 9. Append from other Int32Vector */
  co src = coNewInt32Vector(CO_NONE);
  coInt32VectorAdd(src, 100);
  coInt32VectorAdd(src, 200);
  int append_res = coInt32VectorAppendVector(v, src);
  assert(append_res == 1);
  assert(coInt32VectorSize(v) == 2);
  assert(coInt32VectorGet(v, 0) == 100);
  assert(coInt32VectorGet(v, 1) == 200);
  coDelete(src);

  /* 10. coNewInt32VectorByVector - success (numeric) */
  co std_vec = coNewVector(CO_FREE_VALS);
  coVectorAdd(std_vec, coNewDbl(5.5));
  coVectorAdd(std_vec, coNewBool(1));
  co iv_from_vec = coNewInt32VectorByVector(std_vec);
  assert(iv_from_vec != NULL);
  assert(coIsInt32Vector(iv_from_vec));
  assert(coInt32VectorSize(iv_from_vec) == 2);
  assert(coInt32VectorGet(iv_from_vec, 0) == 5);
  assert(coInt32VectorGet(iv_from_vec, 1) == 1);
  coDelete(iv_from_vec);
  coDelete(std_vec);

  /* 11. coNewInt32VectorByVector - failure (non-numeric) */
  std_vec = coNewVector(CO_FREE_VALS);
  coVectorAdd(std_vec, coNewDbl(5.5));
  coVectorAdd(std_vec, coNewStr(CO_STRDUP, "hello"));
  iv_from_vec = coNewInt32VectorByVector(std_vec);
  assert(iv_from_vec == NULL);
  coDelete(std_vec);

  /* 12. Erase by value */
  coInt32VectorAdd(v, 100);
  coInt32VectorAdd(v, 300);
  coInt32VectorAdd(v, 100);
  assert(coInt32VectorSize(v) == 5); /* 100, 200, 100, 300, 100 */
  coInt32VectorEraseByValue(v, 100);
  assert(coInt32VectorSize(v) == 2);
  assert(coInt32VectorGet(v, 0) == 200);
  assert(coInt32VectorGet(v, 1) == 300);

  /* 13. Deep clone */
  co clone = coClone(v);
  assert(clone != NULL);
  assert(coIsInt32Vector(clone));
  assert(coInt32VectorSize(clone) == 2);
  assert(coInt32VectorGet(clone, 0) == 200);
  assert(coInt32VectorGet(clone, 1) == 300);
  coDelete(clone);

  /* 14. Serialization test */
  printf("Serialized Int32Vector coPrint: ");
  coPrint(v);
  printf("\n");

  printf("Serialized Int32Vector coWriteJSON (compact): ");
  coWriteJSON(v, 1, 0, stdout);
  printf("\n");

  printf("Serialized Int32Vector coWriteJSON (pretty): ");
  coWriteJSON(v, 0, 0, stdout);
  printf("\n");

  coDelete(v);
  printf("All Int32Vector tests passed successfully!\n");
}

int main() {
  run_tests();
  return 0;
}
