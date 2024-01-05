
#include "co.h"
#include <stdio.h>
#include <string.h>


#define COMPU_METHOD_POS 0
#define COMPU_VTAB_POS 1
#define CHARACTERISTIC_POS 2
/*
  Create empty vector for the global index tables.
  This will be filled by build_index_tables() function

      idx_tables_vector[0]: Map with all COMPU_METHOD records
      idx_tables_vector[1]: Map with all COMPU_VTAB records


*/
co create_empty_index_tables_vector(void)
{
  co o;
  o = coNewVector(CO_FREE_VALS);
  if ( o == NULL )
    return NULL;  
  coVectorAdd(o, coNewMap(CO_NONE));          // references to COMPU_METHOD
  coVectorAdd(o, coNewMap(CO_NONE));          // references to COMPU_VTAB
  coVectorAdd(o, coNewVector(CO_NONE));          // references to CHARACTERISTIC
  if ( coVectorSize(o) != 3 )
    return NULL;
  return o;
}

/*
  bild index tables for varios record types of the a2l
  
  COMPU_METHOD
  COMPU_VTAB

  Arg:
    idx_tables_vector: Vector with the desired index tables
      idx_tables_vector[0]: Map with all COMPU_METHOD records
      idx_tables_vector[1]: Map with all COMPU_VTAB records
        
*/
void build_index_tables(cco idx_tables_vector, cco a2l)
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
    if ( strcmp( coStrGet(element), "COMPU_METHOD" ) == 0 )
    {
      coMapAdd((co)coVectorGet(idx_tables_vector, COMPU_METHOD_POS), 
        coStrGet(coVectorGet(a2l, 1)), 
        a2l);
    }
    if ( strcmp( coStrGet(element), "COMPU_VTAB" ) == 0 )
    {
      coMapAdd((co)coVectorGet(idx_tables_vector, COMPU_VTAB_POS), 
        coStrGet(coVectorGet(a2l, 1)), 
        a2l);
    }
    if ( strcmp( coStrGet(element), "CHARACTERISTIC" ) == 0 )
    {
      coVectorAdd((co)coVectorGet(idx_tables_vector, CHARACTERISTIC_POS), a2l);
    }
  }
  for( i = 1; i < cnt; i++ )
  {
    element = coVectorGet(a2l, i);
    if ( coIsVector(element) )
    {
      build_index_tables(idx_tables_vector, element);
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
    if ( strcmp( coStrGet(element), "CHARACTERISTIC" ) == 0 )
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
  co tables = create_empty_index_tables_vector();
  
  a2l = coReadA2LByFP(fp);
  fclose(fp);
  
  build_index_tables(tables, a2l);
  coPrint(tables); puts("");
  
  //dfs_characteristic(a2l);
  
  coDelete(tables);     // delete tables first, because they will refer to a2l
  coDelete(a2l);
}
  
  