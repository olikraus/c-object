
#include "co.h"
#include <stdio.h>
#include <string.h>

int main()
{
  co v = coNewVector(CO_FREE_VALS);
  co vv;
  co s;
  co m = coNewMap(CO_STRDUP | CO_FREE_VALS);
  co t;
  co tt;
  co a;
  char *json;
  char *a2l;
  
  coVectorAdd(v, coNewStr(CO_STRDUP, "abc"));
  coVectorAdd(v, coNewStr(CO_STRDUP, "def"));
  coVectorAdd(v, coNewStr(CO_STRDUP, "ghi"));

  coPrint(v);
  puts("");
  
  
  coPrint(coVectorGet(v, 0)); puts("");
  
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

  a2l=" \
  blabla \
/begin PROJECT\
   ASAM\
   \"prjname\" /* ... */\
   /begin HEADER\
      header_name\
      PROJECT_NO ASAM2013\
   /end HEADER\
   /end PROJECT\
  ";
  
  a = coReadA2LByString(a2l);
  coPrint(a); puts("");
  coDelete(a);
  
  json = " [ 123, \"\\u003c\\u003e\nx\", { \"x\":\"abc\", \"y\":456}, { }, { \"a\":5 } ] ";
  //json = "{ \"x\":\"abc\", \"y\":456}";
  //json = "\"\\u003c\"";
  t = coReadJSONByString(json);
  coPrint(t); puts("");
  coWriteJSON(t, 1, 0, fopen("tmp.json", "w"));

  for( int i = 0; i < 4; i++ )
  {
    tt = coReadJSONByFP(fopen("5MB.json", "r"));
    coDelete(tt);
  }
  
  coDelete(t);
  
  coDelete(v);
  coDelete(vv);
  coDelete(m);
}

