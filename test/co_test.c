
#include "co.h"
#include <stdio.h>
#include <string.h>

int main()
{
  co v = coNewVector(CO_FREE);
  co vv;
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
  

  
  coDelete(v);
  coDelete(vv);
}

