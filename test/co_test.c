
#include "co.h"
#include <stdio.h>
#include <string.h>

int main()
{
  co v = coNewVector(CO_FREE);
  co vv;
  co m = coNewMap(CO_FREE);
  coVectorAdd(v, coNewStr(CO_FREE, strdup("abc")));
  coVectorAdd(v, coNewStr(CO_FREE, strdup("def")));
  coVectorAdd(v, coNewStr(CO_FREE, strdup("ghi")));

  coPrint(v);
  puts("");
  
  
  coPrint(coGetByIdx(v, 0)); puts("");
  
  //coAdd(v, coGetByIdx(v, 0));
  
  vv = coClone(v);
  coVectorAppendVector(vv, v);
  coPrint(vv); puts("");
  
  coMapAdd(m, strdup("abc"), coNewStr(CO_FREE, strdup("v_abc")));
  coMapAdd(m, strdup("def"), coNewStr(CO_FREE, strdup("v_def")));
  coMapAdd(m, strdup("ghi"), coNewStr(CO_FREE, strdup("v_ghi")));
  
  
  coPrint(m); puts("");

  coPrint(coMapGet(m, "def")); puts("");

  coDelete(v);
  coDelete(vv);
  coDelete(m);
}

