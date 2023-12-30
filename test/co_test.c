
#include "co.h"
#include <stdio.h>
#include <string.h>

int main()
{
  co v = coNewVector(CO_FREE_VALS);
  co vv;
  co s;
  co m = coNewMap(CO_STRDUP | CO_FREE_VALS);
  coVectorAdd(v, coNewStr(CO_STRDUP, "abc"));
  coVectorAdd(v, coNewStr(CO_STRDUP, "def"));
  coVectorAdd(v, coNewStr(CO_STRDUP, "ghi"));

  coPrint(v);
  puts("");
  
  
  coPrint(coGetByIdx(v, 0)); puts("");
  
  //coAdd(v, coGetByIdx(v, 0));
  
  vv = coClone(v);
  coVectorAppendVector(vv, v);
  coPrint(vv); puts("");
  
  s = coNewStr(CO_STRDUP, "v_abc");
  coStrAdd(s, "_xyz");
  coMapAdd(m, "abc", s);
  coMapAdd(m, "def", coNewStr(CO_STRDUP, "v_def"));
  coMapAdd(m, "ghi", coNewStr(CO_STRDUP, "v_ghi"));
  
  
  coPrint(m); puts("");

  coPrint(coMapGet(m, "def")); puts("");

  coDelete(v);
  coDelete(vv);
  coDelete(m);
}

