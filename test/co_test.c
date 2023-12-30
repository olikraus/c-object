
#include "co.h"
#include <stdio.h>
#include <string.h>

int main()
{
  co v = coNewVector(CO_FREE);
  co vv;
  co m = coNewMap(CO_FREE);
  covAdd(v, coNewStr(CO_FREE, strdup("abc")));
  covAdd(v, coNewStr(CO_FREE, strdup("def")));
  covAdd(v, coNewStr(CO_FREE, strdup("ghi")));

  coPrint(v);
  puts("");
  
  
  coPrint(coGetByIdx(v, 0)); puts("");
  
  //coAdd(v, coGetByIdx(v, 0));
  
  vv = coClone(v);
  covAppendVector(vv, v);
  coPrint(vv); puts("");
  
  comAdd(m, strdup("abc"), coNewStr(CO_FREE, strdup("v_abc")));
  comAdd(m, strdup("def"), coNewStr(CO_FREE, strdup("v_def")));
  comAdd(m, strdup("ghi"), coNewStr(CO_FREE, strdup("v_ghi")));
  
  
  coPrint(m); puts("");

  coPrint(comGet(m, "def")); puts("");

  coDelete(v);
  coDelete(vv);
  coDelete(m);
}

