
#include "co.h"
#include <stdio.h>
#include <string.h>


#define COMPU_METHOD_POS 0
#define COMPU_VTAB_POS 1
#define CHARACTERISTIC_POS 2
#define A2L_POS 3
#define S19_POS 4

/*
  Create empty vector for the global index tables.
  This will be filled by build_index_tables() function

      sw_object[0]: Map with all COMPU_METHOD records
      sw_object[1]: Map with all COMPU_VTAB records
      sw_object[1]: Map with all COMPU_VTAB records


*/
co create_sw_object(void)
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
    sw_object: Vector with the desired index tables
      sw_object[0]: Map with all COMPU_METHOD records
      sw_object[1]: Map with all COMPU_VTAB records
        
*/
void build_index_tables(cco sw_object, cco a2l)
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
      coMapAdd((co)coVectorGet(sw_object, COMPU_METHOD_POS), 
        coStrGet(coVectorGet(a2l, 1)), 
        a2l);
    }
    if ( strcmp( coStrGet(element), "COMPU_VTAB" ) == 0 )
    {
      coMapAdd((co)coVectorGet(sw_object, COMPU_VTAB_POS), 
        coStrGet(coVectorGet(a2l, 1)), 
        a2l);
    }
    if ( strcmp( coStrGet(element), "CHARACTERISTIC" ) == 0 )
    {
      coVectorAdd((co)coVectorGet(sw_object, CHARACTERISTIC_POS), a2l);
    }
  }
  for( i = 1; i < cnt; i++ )
  {
    element = coVectorGet(a2l, i);
    if ( coIsVector(element) )
    {
      build_index_tables(sw_object, element);
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
      const char *name = coStrGet(coVectorGet(a2l, 1));
      //const char *desc = coStrGet(coVectorGet(a2l, 2));
      //const char *address = coStrGet(coVectorGet(a2l, 4));
      const char *compu_method  = coStrGet(coVectorGet(a2l, 7));
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

/*
  within a vector, search for the given string and return the index to that
  string within the vector.
  return -1 if the string isn't found.
"COMPU_TAB_REF"
*/
long getVectorIndexByString(cco v, const char *s)
{
  long i;
  long cnt = coVectorSize(v);
  for( i = 0; i < cnt; i++ )
  {
    if ( coIsStr(coVectorGet(v, i)) )
    {
      if ( strcmp(coStrGet(coVectorGet(v, i)), s) == 0 )
      {
        return i;
      }
    }
  }
  return -1;
}

void getCharacteristicValue(cco sw_object, cco characteristic_rec)
{
  const char *name = coStrGet(coVectorGet(characteristic_rec, 1));
  const char *desc = coStrGet(coVectorGet(characteristic_rec, 2));
  const char *address = coStrGet(coVectorGet(characteristic_rec, 4));
  const char *compu_method  = coStrGet(coVectorGet(characteristic_rec, 7));
  
  cco cm_rec = coMapGet(coVectorGet(sw_object, COMPU_METHOD_POS), compu_method);
  if ( cm_rec != NULL )
  {
    const char *cm_type = coStrGet(coVectorGet(cm_rec, 3));
    if ( strcmp(cm_type, "TAB_VERB") == 0 )
    {
      long compu_tab_ref_idx = getVectorIndexByString(cm_rec, "COMPU_TAB_REF");
      const char *compu_tab_ref_name = coStrGet(coVectorGet(cm_rec, compu_tab_ref_idx));
    }
    else if ( strcmp(cm_type, "RAT_FUNC") == 0 )
    {
    }
  }
}

int main()
{
  FILE *fp;
  
  co sw_object = create_sw_object();
  
  fp = fopen("example-a2l-file.a2l.gz", "r");
  coVectorAdd(sw_object, coReadA2LByFP(fp));
  fclose(fp);
  
  fp = fopen("example.s19", "r");
  coVectorAdd(sw_object, coReadS19ByFP(fp));
  fclose(fp);
  
  build_index_tables(sw_object, coVectorGet(sw_object, A2L_POS));
  coPrint(sw_object); puts("");

  /*
  {
    co v = coNewVectorByMap(coVectorGet(tables, COMPU_METHOD_POS));
    long idx = coVectorPredecessorBinarySearch(v, "CompuMethod_01");
    coPrint(coVectorGet(v, idx)); puts("");
    coDelete(v);
  }
  */
  
  //dfs_characteristic(a2l);
  
  coDelete(sw_object);     // delete tables first, because they will refer to a2l
  //coDelete(a2l);
  //coDelete(s19);
}
  
  