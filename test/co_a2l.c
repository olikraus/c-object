
#include "co.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>


#define COMPU_METHOD_POS 0
#define COMPU_VTAB_POS 1
#define CHARACTERISTIC_POS 2
#define A2L_POS 3
#define S19_POS 4
#define S19_VECTOR_POS 5

/*
  Create empty vector for the global index tables.
  This will be filled by build_index_tables() function

      sw_object[0]: Map with all COMPU_METHOD records
      sw_object[1]: Map with all COMPU_VTAB records
      sw_object[1]: Map with all COMPU_VTAB records

  ./co_a2l -a2l example-a2l-file.a2l.gz -s19 example.s19

*/

const char *a2l_file_name = NULL;
const char *s19_file_name = NULL;



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

unsigned char *getMemoryArea(cco sw_object, size_t address, size_t length)
{
  char addr_as_hex[10];
  long pos;
  cco memory_block;
  size_t memory_adr;
  size_t memory_len;
  size_t delta;
  unsigned char *ptr;
  
  sprintf(addr_as_hex, "%08zX", address);
  pos = coVectorPredecessorBinarySearch(coVectorGet(sw_object, S19_VECTOR_POS), addr_as_hex); 
  if ( pos < 0 )
  {
    return NULL;
  }
  memory_block = coVectorGet(coVectorGet(sw_object, S19_VECTOR_POS), pos);
  memory_adr = strtol(coStrGet(coVectorGet(memory_block, 0)), NULL, 16);
  memory_len = coMemSize(coVectorGet(memory_block, 1));
  delta = address - memory_adr;
  
  printf("getMemoryArea %08zX --> %08zX-%08zX, delta=%08zX\n", address, memory_adr, memory_adr+memory_len-1, delta);

  if ( delta+length >= memory_len )
  {
    printf("getMemoryArea: requested memory not found\n");
    return NULL;
  }
  
  ptr = (unsigned char *)coMemGet(coVectorGet(memory_block, 1));
  ptr += delta;
  
  return ptr;
}

void getCharacteristicValue(cco sw_object, cco characteristic_rec)
{
  //const char *name = coStrGet(coVectorGet(characteristic_rec, 1));
  //const char *desc = coStrGet(coVectorGet(characteristic_rec, 2));
  //const char *address = coStrGet(coVectorGet(characteristic_rec, 4));
  const char *compu_method  = coStrGet(coVectorGet(characteristic_rec, 7));
  
  cco cm_rec = coMapGet(coVectorGet(sw_object, COMPU_METHOD_POS), compu_method);
  if ( cm_rec != NULL )
  {
    const char *cm_type = coStrGet(coVectorGet(cm_rec, 3));
    if ( strcmp(cm_type, "TAB_VERB") == 0 )
    {
      //long compu_tab_ref_idx = getVectorIndexByString(cm_rec, "COMPU_TAB_REF");
      //const char *compu_tab_ref_name = coStrGet(coVectorGet(cm_rec, compu_tab_ref_idx));
    }
    else if ( strcmp(cm_type, "RAT_FUNC") == 0 )
    {
    }
  }
}

void help(void)
{
  puts("-h            This help text");
  puts("-a2l <file>   A2L File");
  puts("-s19 <file>   S19 File");
}

int parse_args(int argc, char **argv)
{
  while( *argv != NULL )
  {
    if ( strcmp(*argv, "-h" ) == 0 )
    {
      help();
      argv++;
    }
    else if ( strcmp(*argv, "-a2l" ) == 0 )
    {
      argv++;
      if ( *argv != NULL )
      {
        a2l_file_name = *argv;
        argv++;
      }
    }
    else if ( strcmp(*argv, "-s19" ) == 0 )
    {
      argv++;
      if ( *argv != NULL )
      {
        s19_file_name = *argv;
        argv++;
      }
    }
    else
    {
      argv++;
    }
  }
  return 1;
}

int main(int argc, char **argv)
{
  FILE *fp;
  
  co sw_object = create_sw_object();
  
  parse_args(argc, argv);
  
  if ( a2l_file_name == NULL || s19_file_name == NULL )
    return coDelete(sw_object), 0;
  
  /* read a2l file */
  fp = fopen(a2l_file_name, "r");
  if ( fp == NULL )
    return perror(a2l_file_name), coDelete(sw_object), 0;
  coVectorAdd(sw_object, coReadA2LByFP(fp));
  fclose(fp);

  /* read .s19 file */
  fp = fopen(s19_file_name, "r");
  if ( fp == NULL )
    return perror(s19_file_name), coDelete(sw_object), 0;
  coVectorAdd(sw_object, coReadS19ByFP(fp));
  fclose(fp);

  /* build the sorted vector version of the s19 file */
  coVectorAdd(sw_object, coNewVectorByMap(coVectorGet(sw_object, S19_POS)));

  /* build the remaining index tables */
  build_index_tables(sw_object, coVectorGet(sw_object, A2L_POS));
  coPrint(sw_object); puts("");

  getMemoryArea(sw_object, 0x00008085, 2);
  getMemoryArea(sw_object, 0x000080f0, 2);
  getMemoryArea(sw_object, 0x000080f1, 2);
  getMemoryArea(sw_object, 0x000080f2, 2);

  
  //dfs_characteristic(a2l);
  
  coDelete(sw_object);     // delete tables first, because they will refer to a2l
}
  
  