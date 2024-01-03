
#include "co.h"
#include <stdio.h>
#include <string.h>

void build_compu_method_map(cco a2l)
{
  long i, cnt;
  cco element;
  cnt = coVectorSize(a2l);
  if ( cnt == 0 )
    return;
  element = coVectorGet(a2l, 0);
  if ( coIsStr(element) )
  {
    //puts(coStrToString(element));
    if ( strcmp( coStrToString(element), "CHARACTERISTIC" ) == 0 )
    {
    }
  }
  for( i = 1; i < cnt; i++ )
  {
    element = coVectorGet(a2l, i);
    if ( coIsVector(element) )
    {
      build_compu_method_map(element);
    }
  }
  
}

void dfs_characteristic(cco a2l)
{
  long i, cnt;
  cco element;
  cnt = coVectorSize(a2l);
  if ( cnt == 0 )
    return;
  element = coVectorGet(a2l, 0);
  if ( coIsStr(element) )
  {
    //puts(coStrToString(element));
    if ( strcmp( coStrToString(element), "CHARACTERISTIC" ) == 0 )
    {
      if ( cnt < 9 )
        return;
      const char *name = coStrToString(coVectorGet(a2l, 1));
      //const char *desc = coStrToString(coVectorGet(a2l, 2));
      //const char *address = coStrToString(coVectorGet(a2l, 4));
      const char *compu_method  = coStrToString(coVectorGet(a2l, 7));
      printf("%s %s\n", name, compu_method);
    }
  }
  for( i = 1; i < cnt; i++ )
  {
    element = coVectorGet(a2l, i);
    if ( coIsVector(element) )
    {
      dfs_characteristic(element);
    }
  }
}

int main()
{
  FILE *fp = fopen("example-a2l-file.a2l", "r");
  co a2l;
  
  a2l = coReadA2LByFP(fp);
  fclose(fp);
  
  dfs_characteristic(a2l);
  
  coDelete(a2l);
}

  