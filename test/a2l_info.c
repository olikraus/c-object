/* 
  a2l_info
  
  a2l info tool, based on C Object Library 
  (c) 2024 Oliver Kraus
  https://github.com/olikraus/c-object

  CC BY-SA 3.0  https://creativecommons.org/licenses/by-sa/3.0/
  
*/

// uncomment the following define to disable multi-threading
#define USE_PTHREAD


#include "co.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <assert.h>
#include <sys/time.h>
#ifdef USE_PTHREAD
#include <pthread.h>
#endif /* USE_PTHREAD */

#define COMPU_METHOD_MAP_POS 0
#define COMPU_VTAB_MAP_POS 1
#define RECORD_LAYOUT_MAP_POS 2
#define ADDRESS_MAP_POS 3
#define CHARACTERISTIC_NAME_MAP_POS 4
#define AXIS_PTS_NAME_MAP_POS 5
#define FUNCTION_NAME_MAP_POS 6		/* key=FUNCTION name, value=FUNCTION record */
#define CHARACTERISTIC_VECTOR_POS 7
#define AXIS_PTS_VECTOR_POS 8
#define FUNCTION_VECTOR_POS 9		/* references to FUNCTION */
#define PARENT_FUNCTION_MAP_POS 10	/* key: function name, value: FUNCTION vector of the parent, SUB_FUNCTION vs FUNCTION relationship */
#define BELONGS_TO_FUNCTION_MAP_POS 11	/* DEF_CHARACTERISTIC relationship, maybe also LIST_FUNCTION relationship, key=CHARACTERISTIC or AXIS_PTS name, value: FUNCTION co vector */
#define FUNCTION_DEF_CHARACTERISTIC_MAP_POS 12  /* opposite of BELONGS_TO_FUNCTION_MAP_POS: key=FUNCTION name, value: map with key=CHARACTERISTIC or AXIS_PTS name, value reference to CHARACTERISTIC or AXIS_PTS */
#define A2L_POS 13
#define DATA_MAP_POS 14
#define DATA_VECTOR_POS 15

#define SW_PAIR_MAX 16

const char *a2l_file_name_list[SW_PAIR_MAX];		// assumes, that global variables are initialized with 0
const char *s19_file_name_list[SW_PAIR_MAX];		// assumes, that global variables are initialized with 0
co sw_object_list[SW_PAIR_MAX];					// temporary storage area for sw_objects created from tbe above files


int sw_pair_cnt = 0;
int is_verbose = 0;
int is_ascii_characteristic_list = 0;
int is_characteristic_address_list = 0;
int is_function_list = 0;
int is_diff = 0;
int is_characteristicjsondiff = 0;
int is_fndiff = 0;
int is_functionjsondiff = 0;

FILE *json_fp;

uint64_t getEpochMilliseconds(void)
{
	struct timeval tv;
	gettimeofday(&tv, NULL);
	return (uint64_t)(tv.tv_sec) * 1000 + (uint64_t)(tv.tv_usec) / 1000;
}

co createSWObject(void)
{
  co o;
  o = coNewVector(CO_FREE_VALS);
  if ( o == NULL )
    return NULL;  
  coVectorAdd(o, coNewMap(CO_NONE));          // references to COMPU_METHOD
  coVectorAdd(o, coNewMap(CO_NONE));          // references to COMPU_VTAB
  coVectorAdd(o, coNewMap(CO_NONE));          // references to RECORD_LAYOUT
  coVectorAdd(o, coNewMap(CO_NONE));          // ADDRESS_MAP_POS: references to CHARACTERISTIC & AXIS_PTS, key = address
  coVectorAdd(o, coNewMap(CO_NONE));          // CHARACTERISTIC_NAME_MAP_POS: references to CHARACTERISTIC, key = name
  coVectorAdd(o, coNewMap(CO_NONE));          // AXIS_PTS_NAME_MAP_POS: references to AXIS_PTS, key = name
  coVectorAdd(o, coNewMap(CO_NONE));          // FUNCTION_NAME_MAP_POS: references to FUNCTION, key = name
  coVectorAdd(o, coNewVector(CO_NONE));          // CHARACTERISTIC_VECTOR_POS: references to CHARACTERISTIC
  coVectorAdd(o, coNewVector(CO_NONE));          // AXIS_PTS_VECTOR_POS: references to AXIS_PTS  
  coVectorAdd(o, coNewVector(CO_NONE));          // FUNCTION_VECTOR_POS: references to FUNCTION
  coVectorAdd(o, coNewMap(CO_NONE));          // PARENT_FUNCTION_MAP_POS: reference to parent FUNCTION vector, key = name of the (sub-) function, value = parent FUNCTION co vector
  coVectorAdd(o, coNewMap(CO_NONE));  // BELONGS_TO_FUNCTION_MAP_POS: references from DEF_CHARACTERISTIC to FUNCTION co vector, key = name of the CHARACTERISTIC (or AXIS_PTS), value = parent FUNCTION co vector
  coVectorAdd(o, coNewMap(CO_FREE_VALS));  // FUNCTION_DEF_CHARACTERISTIC_MAP_POS:
  if ( coVectorSize(o) != 13 )
    return NULL;
  return o;
}

void showRecordLayout(cco o)
{
	
}

/*
  build index tables for varios record types of the a2l
  
  idea is to loop over the a2l tree and collect/store all required information.
  
  
  Arg:
    sw_object: Vector with the desired index tables
      sw_object[0]: Map with all COMPU_METHOD records
      sw_object[1]: Map with all COMPU_VTAB records
        
*/
struct build_index_struct
{
	const char *function_name;
	cco function_object;
};

void buildIndexInit(struct build_index_struct *bis)
{
	bis->function_name = NULL;
	bis->function_object = NULL;
}

void buildIndexTables(cco sw_object, cco a2l, struct build_index_struct *bis)
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
      coMapAdd((co)coVectorGet(sw_object, COMPU_METHOD_MAP_POS), 
        coStrGet(coVectorGet(a2l, 1)), 
        a2l);
    }
    else if ( strcmp( coStrGet(element), "COMPU_VTAB" ) == 0 )
    {
      coMapAdd((co)coVectorGet(sw_object, COMPU_VTAB_MAP_POS), 
        coStrGet(coVectorGet(a2l, 1)), 
        a2l);
    }
    else if ( strcmp( coStrGet(element), "RECORD_LAYOUT" ) == 0 )
    {
      coMapAdd((co)coVectorGet(sw_object, RECORD_LAYOUT_MAP_POS), 
        coStrGet(coVectorGet(a2l, 1)),
        a2l);
    }
    else if ( strcmp( coStrGet(element), "CHARACTERISTIC" ) == 0 )
    {
	  const char *characteristic_name = coStrGet(coVectorGet(a2l, 1));
	  const char *characteristic_address = coStrGet(coVectorGet(a2l, 4));
      coVectorAdd((co)coVectorGet(sw_object, CHARACTERISTIC_VECTOR_POS), a2l);

      coMapAdd((co)coVectorGet(sw_object, CHARACTERISTIC_NAME_MAP_POS), characteristic_name, a2l);
		
      coMapAdd((co)coVectorGet(sw_object, ADDRESS_MAP_POS), characteristic_address, a2l);
    }
    else if ( strcmp( coStrGet(element), "AXIS_PTS" ) == 0 )
    {
	  const char *axis_pts_name = coStrGet(coVectorGet(a2l, 1));
	  const char *axis_pts_address = coStrGet(coVectorGet(a2l, 3));
      coVectorAdd((co)coVectorGet(sw_object, AXIS_PTS_VECTOR_POS), a2l);

      coMapAdd((co)coVectorGet(sw_object, AXIS_PTS_NAME_MAP_POS), axis_pts_name, a2l);
		
      coMapAdd((co)coVectorGet(sw_object, ADDRESS_MAP_POS), axis_pts_address, a2l);
    }
    else if ( strcmp( coStrGet(element), "FUNCTION" ) == 0 )
	{
		bis->function_name = coStrGet(coVectorGet(a2l, 1));	// we are inside a function, remember the function name
		bis->function_object = a2l;
		coVectorAdd((co)coVectorGet(sw_object, FUNCTION_VECTOR_POS), a2l);

		coMapAdd((co)coVectorGet(sw_object, FUNCTION_NAME_MAP_POS), bis->function_name, a2l);
		
		
		
	}
    else if ( strcmp( coStrGet(element), "SUB_FUNCTION" ) == 0 )
	{
	  if ( bis->function_object != NULL )
	  {
		  for( i = 1; i < cnt; i++ )
		  {
			element = coVectorGet(a2l, i);
			if ( coIsStr(element) )
			{
				// the sub function might already exist in the map, but this shouldn't happen because a function should have only one parent
				coMapAdd((co)coVectorGet(sw_object, PARENT_FUNCTION_MAP_POS), coStrGet(element), bis->function_object);
			}
		  }
		  return; 	// no need to continue to the loop below, because there are no further sub elements
	  }
	}	
    else if ( strcmp( coStrGet(element), "DEF_CHARACTERISTIC" ) == 0 )
	{
	  if ( bis->function_object != NULL )
	  {
		  for( i = 1; i < cnt; i++ )
		  {
			element = coVectorGet(a2l, i);
			if ( coIsStr(element) )
			{
				// the DEF_CHARACTERISTIC might already exist in the map, but this shouldn't happen because there should be only one function possible as per A2L
				coMapAdd((co)coVectorGet(sw_object, BELONGS_TO_FUNCTION_MAP_POS), coStrGet(element), bis->function_object);
			}
		  }
		  return; 	// no need to continue to the loop below, because there are no further sub elements
	  }
	}
	
  }
  
  /* generic loop over the element */
  
  for( i = 1; i < cnt; i++ )
  {
    element = coVectorGet(a2l, i);
    if ( coIsVector(element) )
    {
      buildIndexTables(sw_object, element, bis);
    }
  }  
}

static int buildFunctionDefCharacteristicMapCB(cco o, long idx, const char *key, cco function, void *data)
{
	/* key is the characteristic or axis_pts name */
	/* get the sw release itself */
	cco sw_object = (cco)data;
	
	/* get the inner map of the FUNCTION_DEF_CHARACTERISTIC map. This inner map will be expanded */
	const char *function_name = coStrGet(coVectorGet(function, 1));
	cco function_map = coVectorGet(sw_object, FUNCTION_DEF_CHARACTERISTIC_MAP_POS);
	co map = (co)coMapGet(function_map, function_name);	// should not be NULL

	/* for the search below, get the NAME to record maps */
	cco characteristic_map = coVectorGet(sw_object, CHARACTERISTIC_NAME_MAP_POS);
	cco axis_pts_map = coVectorGet(sw_object, AXIS_PTS_NAME_MAP_POS);
	
	/* try to see, whether the key is a CHARACTERISTIC */
	cco characteristic_rec = coMapGet(characteristic_map, key);	// might be NULL if key is not a CHARACTERISTIC
	
	assert(map != NULL);	// this should not be NULL, because it was added before in step 1

	if ( characteristic_rec != NULL )	// if we found a CHARACTERISTIC, then add the record to the DEF_CHARACTERISTIC inner map of FUNCTION_DEF_CHARACTERISTIC_MAP_POS
	{
		coMapAdd(map, key, characteristic_rec);
	}
	else	// otherwise it is an AXIS_PTS
	{
		cco axis_pts_rec = coMapGet(axis_pts_map, key);	// might be NULL
		if ( axis_pts_rec != NULL )
		{
			coMapAdd(map, key, axis_pts_rec);
		}
		else
		{
			assert(0);		// this is an error, the key should be either a CHARACTERISTIC or a AXIS_PTS
		}
	}
	return 1;		// all good, continue
}

/* 
	calculate FUNCTION_DEF_CHARACTERISTIC_MAP_POS 
	This must be called after call to buildIndexTables()
	
*/
void buildFunctionDefCharacteristicMap(cco sw_object)
{
	long i, cnt;
	co function_map;		// this will be filled with data
	co function_vector;
	cco function_rec;
	
	function_map = (co)coVectorGet(sw_object, FUNCTION_DEF_CHARACTERISTIC_MAP_POS);
	
	// Step 1: loop over FUNCTION_VECTOR_POS and fill FUNCTION_DEF_CHARACTERISTIC_MAP_POS with all functions (key=function name, value=map to the CHARACTERISTIC/AXIS_PTS)
	
	function_vector = (co)coVectorGet(sw_object, FUNCTION_VECTOR_POS);
	cnt = coVectorSize(function_vector);
	for( i = 0; i < cnt; i++ )
	{
		function_rec = coVectorGet(function_vector, i);
		// function_map key=funciton name, value=map with ...
		//			... key=CHARACTERISTIC or AXIS_PTS name, value reference to CHARACTERISTIC or AXIS_PTS
		coMapAdd(function_map, coStrGet(coVectorGet(function_rec, 1)), coNewMap(CO_NONE));
	}
	
	
	// Step 2: loop over BELONGS_TO_FUNCTION_MAP_POS: 
	//		Step 2.1 take the function name from the value, search in the above map for the function
	//		Step 2.2 search for the CHARACTERISTIC / AXIS_PTS record in CHARACTERISTIC_NAME_MAP_POS / AXIS_PTS_NAME_MAP_POS
	//		Step 2.3 append the CHARACTERISTIC / AXIS_PTS record to FUNCTION_VECTOR_POS key=CHARACTERISTIC / AXIS_PTS, value = CHARACTERISTIC / AXIS_PTS record
	
	coMapForEach(coVectorGet(sw_object, BELONGS_TO_FUNCTION_MAP_POS), buildFunctionDefCharacteristicMapCB, (void *)sw_object);
}



#ifdef NOT_USED
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
#endif

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

/*
	for a data array, some values in the record layout are stored multiple times.
    the multiplicator is the value how often such values are stored.
	
	This will check for one or more AXIS_DESCR.
	For multiple AXIS_DESCR the max_axis_points is the product of all AXIS_DESCR sub structures
	
*/
long getCharacteristicMeasurementAxisPTSDataMultiplicator(cco cma_rec)
{
	
	long matrix_dim_list[10] = { 0, 0, 0, 0, 0, 0, 0, 0, 0 };
	long matrix_dim_idx = 0;
	long matrix_product = 1;
	long number_array_size = 0;
	long max_axis_points = 0;
	long multiplier = 1;
	long i;
	long cnt = coVectorSize(cma_rec);
	const char *cma_type = coStrGet(coVectorGet(cma_rec, 0));	// one of CHARACTERISTIC, MEASUREMENT or AXIS_PTS
	cco o;
	i = 0;
	
	if ( cma_type[0] == 'A' ) // if this an AXIS_PTS element, then 
	{
		max_axis_points = atol(coStrGet(coVectorGet(cma_rec, 8)));	// ... take the max number of axis points from position 8
	}
	
	//printf("%s: %s %s\n", coStrGet(coVectorGet(cma_rec, 0)), coStrGet(coVectorGet(cma_rec, 1)), coStrGet(coVectorGet(cma_rec, 7)));
	
	while( i < cnt )
	{
		o = coVectorGet(cma_rec, i);
		if ( coIsStr(o) )
		{
			if ( strcmp( coStrGet(o), "MATRIX_DIM" ) == 0 )
			{
				i++;
				while( matrix_dim_idx < 10 && i < cnt)
				{
					o = coVectorGet(cma_rec, i);
					if ( !coIsStr(o) )
						break;
					matrix_dim_list[matrix_dim_idx] = atol(coStrGet(o));
					if ( matrix_dim_list[matrix_dim_idx] == 0 )
						break;
					matrix_product *= matrix_dim_list[matrix_dim_idx];
					matrix_dim_idx++;
					i++;
				}
			}
			else if ( strcmp( coStrGet(o), "NUMBER" ) == 0 || strcmp( coStrGet(o), "ARRAY_SIZE" ) == 0 )
			{
				i++;
				o = coVectorGet(cma_rec, i);
				if ( coIsStr(o) )
				{
					number_array_size = atol(coStrGet(o));
					i++;
				}
			}
			else
			{
				i++;
			}
		}
		else if ( coIsVector(o) )
		{
			const char *element = coStrGet(coVectorGet(o, 0));
			if ( strcmp(element, "AXIS_DESCR") == 0 )	// this is the AXIS description within a CHARACTERISTIC record
			{
				if ( max_axis_points == 0 )
					max_axis_points = atol(coStrGet(coVectorGet(o, 4)));
				else
					max_axis_points *= atol(coStrGet(coVectorGet(o, 4)));
			}
			i++;
		}
		else
		{
			i++;
		}
	}
	
	if ( matrix_dim_idx > 0 )
	{
		multiplier *= matrix_product;
		// printf("matrix_dim_idx=%ld matrix_product=%ld - ", matrix_dim_idx, matrix_product);
	}
	if ( number_array_size > 0 )
	{
		multiplier *= number_array_size;
		// printf("number_array_size=%ld - ", number_array_size);
	}
	if ( max_axis_points > 0 )
	{
		multiplier *= max_axis_points;
		// printf("max_axis_points=%ld - ", max_axis_points);
	}
	
	return multiplier;
}

/* expects a data type string like "UBYTE" and returns the number of bytes 
	occupied by that data type.
*/
long getAtomicDataTypeSize(const char *datatype)
{
	if ( datatype[0] == 'U' )
	{
		if ( strcmp(datatype, "UBYTE") == 0 )
			return 1;
		if ( strcmp(datatype, "UWORD") == 0 )
			return 2;
		if ( strcmp(datatype, "ULONG") == 0 )
			return 4;
	}
	else if ( datatype[0] == 'S' )
	{
		if ( strcmp(datatype, "SBYTE") == 0 )
			return 1;
		if ( strcmp(datatype, "SWORD") == 0 )
			return 2;
		if ( strcmp(datatype, "SLONG") == 0 )
			return 4;
	}
	else if ( datatype[0] == 'F' )
	{
		if ( strcmp(datatype, "FLOAT16_IEEE") == 0 )
			return 2;
		if ( strcmp(datatype, "FLOAT32_IEEE") == 0 )
			return 4;
		if ( strcmp(datatype, "FLOAT64_IEEE") == 0 )
			return 8;
	}
	else if ( datatype[0] == 'A' )
	{
		// A_UINT64,
		// A_INT64,

		return 8;
	}
	return 0;
}


/*
	This will calculate the number of bytes occupied by an A2L element which is stored in memory.
	The record may contain data which will appear only once (fixed size) and data which will appear multiple times (dynamic_size).
	Not all type of data is supported.
	return 0 if the record layout contains unsupported information
*/
int getRecordLayoutSize(cco record_layout_rec, long *fixed_size, long *dynamic_size)
{
	long i = 2;
	long cnt = coVectorSize(record_layout_rec);
	const char *s;
	const char *data_type;
	const char *addr_type;
	
	*fixed_size = 0;
	*dynamic_size = 0;
	
	while( i < cnt )
	{
		s = coStrGet(coVectorGet(record_layout_rec, i));
		if ( strncmp(s, "AXIS_PTS_", 9)==0 || strncmp(s, "FNC", 3)==0 )		// followed by <pos> <type> <mode> <addresstype>
		{
			data_type = coStrGet(coVectorGet(record_layout_rec, i+2));
			// not sure about the address type.
			// maybe it is like this: if this is DIRECT, then the size is the size of the "data_type" otherwise it is the size of the pointer
			// at the moment, the addr_type is ignored if this is not DIRECT
			addr_type = coStrGet(coVectorGet(record_layout_rec, i+4));  // PBYTE, PWORD, PLONG, PLONGLONG, DIRECT
			if ( addr_type[0] == 'D' )
				*dynamic_size += getAtomicDataTypeSize(data_type);
			else
				return 0;	// Pxxxx are not supported, because we don't know how to calculate this
			i += 5;
		}
		else if ( strncmp(s, "NO_AXIS_PTS_", 12)==0 )	// followed by <pos> <type>
		{
			data_type = coStrGet(coVectorGet(record_layout_rec, i+2));
			*fixed_size += getAtomicDataTypeSize(data_type);
			assert( *fixed_size > 0 );
			i += 3;
		}
		else if ( strncmp(s, "FIX_NO_AXIS_PTS_", 16)==0 )	// followed by <cnt>
		{
			//ASSUMPTION: 
			//if this value is given, then the corresponding data size  from AXIS_PTS_x should be multiplied with this value and returned as fixed_size
			return 0; // not supported at the moment
		}
		else if ( strncmp(s, "SHIFT_OP_", 9)==0 
				|| strncmp(s, "SRC_ADDR_", 9)==0 
				|| strncmp(s, "RIP_ADDR_", 9)==0 
				|| strncmp(s, "OFFSET_", 7)==0 
				|| strncmp(s, "NO_RESCALE_", 11)==0 
				|| strncmp(s, "DIST_OP_", 8)==0 
				|| strncmp(s, "AXIS_RESCALE_", 13)==0 
				|| strncmp(s, "ALIGNMENT_", 10)==0 
				)
		{
			
			return 0;	// not supported
		}
		else if ( strcmp(s, "IDENTIFICATION")==0 )
		{
			return 0;	// not supported
		}
		else
		{
			i++;
		}
	}
	return 1;
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
  pos = coVectorPredecessorBinarySearch(coVectorGet(sw_object, DATA_VECTOR_POS), addr_as_hex); 
  if ( pos < 0 )
  {
    return NULL;
  }
  memory_block = coVectorGet(coVectorGet(sw_object, DATA_VECTOR_POS), pos);
  memory_adr = strtoll(coStrGet(coVectorGet(memory_block, 0)), NULL, 16);
  memory_len = coMemSize(coVectorGet(memory_block, 1));
  delta = address - memory_adr;
  

  if ( delta+length >= memory_len )
  {
    //printf("getMemoryArea: requested memory not found\n");
    return NULL;
  }

  
  ptr = (unsigned char *)coMemGet(coVectorGet(memory_block, 1));
  ptr += delta;

  //printf("getMemoryArea(%08zX,%08zX) -%ld-> %08zX-%08zX, delta=%08zX, first byte=%02X\n", address, length, pos, memory_adr, memory_adr+memory_len-1, delta, (unsigned)*ptr);
  
  return ptr;
}

char *getMemoryAreaString(cco sw_object, size_t address, size_t length)
{
	static char buf[256];
	unsigned char *ptr = getMemoryArea(sw_object, address, length);
	int i=0;
	buf[0] = '\0';
	
	if ( ptr == NULL )
	{
		// sprintf(buf, "Memory location not available");
	}
	else if ( ptr != NULL && length > 0 )
	{
		while( i < 4 && i < length)
		{
			sprintf(buf+strlen(buf), " %02X", (int)ptr[i]);
			i++;
		}
		if ( i < length )
			sprintf(buf+strlen(buf), "... <total %zd bytes>", length);
	}
	return buf;
}

/*
	For a given CHARACTERISTIC or AXIS_PTS, return address and size of the CHARACTERISTIC or AXIS_PTS.
	This function returns 0, if the record layout is not supported.
*/
int getCharacteristicAxisPtsMemoryArea(cco sw_object, cco characteristic_or_axis_pts, long long int *address, long *size)
{
	const char *element_type = coStrGet(coVectorGet(characteristic_or_axis_pts, 0)); // either AXIS_PTS or CHARACTERISTIC
	const char *record_layout_name = coStrGet(coVectorGet(characteristic_or_axis_pts, 5)); // record layout name is at the same position for both 
	const char *address_string = coStrGet(coVectorGet(characteristic_or_axis_pts, 4)); //  
	cco record_layout_rec = coMapGet((co)coVectorGet(sw_object, RECORD_LAYOUT_MAP_POS), record_layout_name);	// find the record layout element
	long factor = getCharacteristicMeasurementAxisPTSDataMultiplicator(characteristic_or_axis_pts);  // get the object multiplication factor
	long fixed_size;
	long dynamic_size;
	int is_supported = getRecordLayoutSize(record_layout_rec, &fixed_size, &dynamic_size);
	
	if ( is_supported == 0 )
	{
		*address = 0;
		*size = 0;
		return 0;
	}	
	*size = factor*dynamic_size+fixed_size;	// maybe this is a simplified calculation, but it seems to work...
	
	if ( element_type[0] == 'C' )  // is this a CHARACTERISTIC?
		address_string = coStrGet(coVectorGet(characteristic_or_axis_pts, 4));	// CHARACTERISTIC address is at pos 4
	else
		address_string = coStrGet(coVectorGet(characteristic_or_axis_pts, 3));	// AXIS_PTS address is at pos 3
	*address = strtoll(address_string, NULL, 16);
	
	return 1;
}


int showAddressListCB(cco o, long idx, const char *key, cco characteristic_or_axis_pts, void *data)
{
	//static int show_next = 0;
	static long long int last_address = -1;
	cco sw_object = (cco)data;
	const char *element_type = coStrGet(coVectorGet(characteristic_or_axis_pts, 0)); // either AXIS_PTS or CHARACTERISTIC
    const char *element_name = coStrGet(coVectorGet(characteristic_or_axis_pts, 1)); // name of the AXIS_PTS or CHARACTERISTIC
	const char *record_layout_name = coStrGet(coVectorGet(characteristic_or_axis_pts, 5)); // record layout name is at the same position for both CHARACTERISTIC and AXIS_PTS
	long fixed_size;
	long dynamic_size;
	cco record_layout_rec = coMapGet((co)coVectorGet(sw_object, RECORD_LAYOUT_MAP_POS), record_layout_name);
	
	long factor = getCharacteristicMeasurementAxisPTSDataMultiplicator(characteristic_or_axis_pts);
	int is_supported = getRecordLayoutSize(record_layout_rec, &fixed_size, &dynamic_size);
	long record_size = factor*dynamic_size+fixed_size;	// maybe this is a simplified calculation, but it seems to work...
	long long int current_address = strtoll(key, NULL, 16);
		
	//if ( show_next != 0 || factor > 100 || fixed_size!=0 )
	{
		if ( !(last_address > current_address+10000 || last_address < 0) )
		{
			if ( last_address > current_address )
			{
				printf("0x%08llX  %11lld OVERLAP ERROR %lld Bytes\n", last_address, last_address, last_address-current_address);
			}
			if ( last_address < current_address )
			{
				printf("0x%08llX  %11lld GAP %lld Bytes\n", last_address, last_address, current_address-last_address);
			}
		}
		printf("%-11s %11lld %3ld*%3ld+%3ld=%4ld  %14s:%-75s %s %s\n", key, current_address, 
			factor, dynamic_size, fixed_size, record_size, 
			element_type, element_name, is_supported?"":"NOT SUPPORTED",
			getMemoryAreaString(sw_object, current_address, record_size) );
		last_address = current_address+record_size;
		/*
		if ( factor > 1 )
			show_next = 1;
		else
			show_next = 0;
		*/
	}
	return 1;
}
void showAddressList(cco sw_object)
{
	coMapForEach(coVectorGet(sw_object, ADDRESS_MAP_POS), showAddressListCB, (void *)sw_object); // returns 0 as soon as the callback function returns 0
}



void showCharacteristic(cco sw_object, cco characteristic_rec)
{
  const char *name = coStrGet(coVectorGet(characteristic_rec, 1));
  //const char *desc = coStrGet(coVectorGet(characteristic_rec, 2));
  const char *address = coStrGet(coVectorGet(characteristic_rec, 4));
  const char *record_layout = coStrGet(coVectorGet(characteristic_rec, 5));
  const char *compu_method  = coStrGet(coVectorGet(characteristic_rec, 7));
  long multiplicator = getCharacteristicMeasurementAxisPTSDataMultiplicator(characteristic_rec);
  
  cco compu_method_rec = NULL;
  
  if ( strcmp( compu_method, "NO_COMPU_METHOD" ) != 0 )
	compu_method_rec = coMapGet(coVectorGet(sw_object, COMPU_METHOD_MAP_POS), compu_method);

  if ( multiplicator > 1 )
	printf("CHARACTERISTIC %-50s %-10s %-24s %-30s %ld\n",
		name, address, record_layout, compu_method, multiplicator );

  if ( compu_method_rec != NULL )
  {
    const char *cm_type = coStrGet(coVectorGet(compu_method_rec, 3));
    if ( strcmp(cm_type, "TAB_VERB") == 0 )
    {
      //long compu_tab_ref_idx = getVectorIndexByString(compu_method_rec, "COMPU_TAB_REF");
      //const char *compu_tab_ref_name = coStrGet(coVectorGet(compu_method_rec, compu_tab_ref_idx));
    }
    else if ( strcmp(cm_type, "RAT_FUNC") == 0 )
    {
    }
  }
}

void showAllCharacteristic(cco sw_object)
{
	long i, cnt;
	cco characteristic_vector = coVectorGet(sw_object, CHARACTERISTIC_VECTOR_POS);
	cnt = coVectorSize(characteristic_vector);
	for( i = 0; i < cnt; i++ )
	{
		showCharacteristic(sw_object, coVectorGet(characteristic_vector, i));	
	}
	
}


void showAllASCIICharacteristic(cco sw_object)
{
	long i, cnt;
	char buf[1024];
	cco characteristic_vector = coVectorGet(sw_object, CHARACTERISTIC_VECTOR_POS);
	cco characteristic_rec;
	cnt = coVectorSize(characteristic_vector);
	printf("Address    Size Label                Content\n");
	for( i = 0; i < cnt; i++ )
	{
		characteristic_rec = coVectorGet(characteristic_vector, i);
		if ( strcmp( coStrGet(coVectorGet(characteristic_rec, 3)), "ASCII" ) == 0 )
		{
			const char *name = coStrGet(coVectorGet(characteristic_rec, 1));
 		    const char *address = coStrGet(coVectorGet(characteristic_rec, 4));
			const char *record_layout_name = coStrGet(coVectorGet(characteristic_rec, 5)); // record layout name 
			long fixed_size;
			long dynamic_size;
			cco record_layout_rec = coMapGet((co)coVectorGet(sw_object, RECORD_LAYOUT_MAP_POS), record_layout_name);
			long factor = getCharacteristicMeasurementAxisPTSDataMultiplicator(characteristic_rec);
			int is_supported = getRecordLayoutSize(record_layout_rec, &fixed_size, &dynamic_size);
			if ( is_supported )
			{
				long record_size = factor*dynamic_size+fixed_size;	// maybe this is a simplified calculation, but it seems to work...
				unsigned char *string = getMemoryArea(sw_object, strtoll(address, NULL, 16), (size_t)record_size);
				memset(buf, '\0', 1024);
				if ( string != NULL )
				{
					if ( record_size < 1022 )
					{
						memcpy(buf, string, (size_t)record_size);
					}
				}
				printf("%10s %4ld %-20s %s\n", address, record_size, name, buf);
			}
		}
	}
}

co getS19Object(const char *s19)
{
  FILE *fp;
  long long int t1, t2;
  co o;  
  fp = fopen(s19, "rb");  // we need to read binary so that the CR/LF conversion is suppressed on windows
  if ( fp == NULL )
        return perror(s19), NULL;
  if ( is_verbose ) printf("Reading S19 '%s' started\n", s19);
  t1 = getEpochMilliseconds();
  o = coReadS19ByFP(fp);
  t2 = getEpochMilliseconds();
  if ( is_verbose ) printf("Reading S19 '%s' done, milliseconds=%lld\n", s19, t2-t1);
  fclose(fp);	  
  return o;
}

void *getS19Thread( void *ptr )
{
  return (void *)getS19Object((const char *)ptr);
}


co getSWObject(const char *a2l, const char *s19)
{
  FILE *fp;
  long long int t0, t1, t2;
  co sw_object = createSWObject();
  co s19_object = NULL;  // s19 object, it will be added to sw_object later
  struct build_index_struct bis;	// local data for tbe build index procedure (to make it MT safe)
  
#ifdef USE_PTHREAD
  pthread_t s19_thread;
  int s19_thread_create_result;

  if ( s19 != NULL )    // if s19 is required, then read this in parallel to the a2l
  {
    s19_thread_create_result = pthread_create( &s19_thread, NULL, getS19Thread, (void*)(s19));
    if ( s19_thread_create_result != 0 )
            return coDelete(sw_object), NULL;  
  }
#endif  
  
  assert( sw_object != NULL );
  if ( a2l == NULL )
    return coDelete(sw_object), NULL;

  t0 = getEpochMilliseconds();  
  
  /* read a2l file */
  fp = fopen(a2l, "rb");  // we need to read binary so that the CR/LF conversion is suppressed on windows
  if ( fp == NULL )
    return perror(a2l), coDelete(sw_object), NULL;
  if ( is_verbose ) printf("Reading A2L '%s' started\n", a2l);
  coVectorAdd(sw_object, coReadA2LByFP(fp));
  t1 = getEpochMilliseconds();
  if ( is_verbose ) printf("Reading A2L '%s' done, milliseconds=%lld\n", a2l, t1-t0);
  fclose(fp);

  if ( s19 != NULL )
  {
#ifdef USE_PTHREAD
    // With pthread, wait for the parallel s19 data
    pthread_join( s19_thread, (void **)&s19_object);
#else
    // Without pthread, just read the s19 file right now
    s19_object = getS19Object(s19);
#endif
  }
  
  /* attach the s19 object to the sw_object */
  if ( s19_object != NULL )
  {
          assert(coVectorSize(sw_object) == DATA_MAP_POS);
	  coVectorAdd(sw_object, s19_object);
	  coVectorAdd(sw_object, coNewVectorByMap(coVectorGet(sw_object, DATA_MAP_POS)));	// build the sorted vector from the data map file
	  if ( is_verbose ) 
	  {
		  coPrint(coVectorGet(sw_object, DATA_VECTOR_POS));
		  puts("");
	  }
  }
  else
  {
          // s19 not available, add some dummy entries
	  coVectorAdd(sw_object, coNewMap(CO_NONE));	// add dummy entry for the s19 data
	  coVectorAdd(sw_object, coNewVector(CO_NONE));  // add a dummy vector to s19 data
  }

  /* build the remaining index tables */
  if ( is_verbose ) printf("Building A2L index tables '%s' started\n", a2l);
  t1 = getEpochMilliseconds();
  buildIndexInit(&bis);
  buildIndexTables(sw_object, coVectorGet(sw_object, A2L_POS), &bis);
  buildFunctionDefCharacteristicMap(sw_object); 	// build FUNCTION_DEF_CHARACTERISTIC_MAP_POS, based on the results from buildIndexTables()
  t2 = getEpochMilliseconds();  
  if ( is_verbose ) printf("Building A2L index tables '%s' done, milliseconds=%lld\n", a2l, t2-t1);

  return sw_object;
}

void *getSWListThread( void *ptr )
{
	int idx = *(int *)ptr;
	sw_object_list[idx] = getSWObject(a2l_file_name_list[idx], s19_file_name_list[idx]);
	return NULL;
}
       

co getSWList(void)
{
	int i;
	co sw_list;
	int index[SW_PAIR_MAX];
	
	
#ifdef USE_PTHREAD
	pthread_t threads[SW_PAIR_MAX];
	int create_result;
#endif

	sw_list = coNewVector(CO_FREE_VALS);
	if ( sw_list == NULL )
		return NULL;  

        // build sw objects for each a2l/s19 pair and store them into the sw_object_list array
	for( i = 0; i < sw_pair_cnt; i++ )
	{
		index[i] = i;

#ifdef USE_PTHREAD
		create_result = pthread_create( threads+i, NULL, getSWListThread, (void*)(index+i));
		if ( create_result != 0 )
			return coDelete(sw_list), NULL; // maybe we should delete the results from the threads...
#else
		getSWListThread( (void*)(index+i) );  // without pthread: construct the sw object directly
#endif
	}
	
#ifdef USE_PTHREAD
	for( i = 0; i < sw_pair_cnt; i++ )
		pthread_join( threads[i], NULL);
#endif

        // add the previously constructed sw objects to the sw object list c-object vector
	for( i = 0; i < sw_pair_cnt; i++ )
	{
 		if ( sw_object_list[i] != NULL )
			coVectorAdd(sw_list, sw_object_list[i]);
                sw_object_list[i] = NULL; // clear the reference so that -fsanitize=address reports correct values
	}
	
	return sw_list;
}

#define FUNCTION_PATH_MAX (1024*4)

const char *getFullFunctionName(cco sw_object, cco function)
{
	static char buf[FUNCTION_PATH_MAX];
	const char *name;
	cco m =	coVectorGet(sw_object, PARENT_FUNCTION_MAP_POS);
	cco f = function;
	const char *name_list[32];
	int cnt = 0;

	assert( coIsVector(function) );
	while( cnt < 32 )
	{
		name = coStrGet(coVectorGet(f, 1));
		name_list[cnt] = name;
		cnt++;
		f = coMapGet(m, name);
		if ( f == NULL )
			break;
	}
	
	buf[0] = '\0';
	while( cnt > 0 )
	{
		size_t buflen = strlen(buf);
		cnt--;
		if ( buflen + strlen(name_list[cnt]) + 2 >= FUNCTION_PATH_MAX )
		{
			sprintf(buf, ".../%s", name_list[cnt]);
			break;
		}
		sprintf(buf+buflen, "/%s", name_list[cnt]);
	}
	return buf;
}

/*
	returns a map
		key: CHARACTERISTIC name
		value: Vector of length coVectorSize(sw_list)
		
	The value vector contains references to the CHARACTERISTIC entries of the corresponding sw_object
	This reference can be NULL, if the CHARACTERISTIC is not present in that sw_object
	
*/
co getAllCharacteristicDifferenceMap(cco sw_list)
{
  long i, j, k;
  cco sw_object;
  cco characteristic_vector;
  cco characteristic_rec;
  const char *key;
  co all_characteristic_map = coNewMap(CO_FREE_VALS); 	// key is neither strdup'd nor free'd, because it is taken from the CHARACTERISTIC record
  co all_map_value_vector;
  long long int t0, t1;
  t0 = getEpochMilliseconds();  

  if ( all_characteristic_map == NULL )
	  return NULL;
  for( i = 0; i < coVectorSize(sw_list); i++ )	// loop over all sw release, which are provided in "sw_list"
  {
	  sw_object = coVectorGet(sw_list, i);		// get the current sw release for the outer loop body
	  characteristic_vector = coVectorGet(sw_object, CHARACTERISTIC_VECTOR_POS);	// get the vector with all characteristic values for the current sw release
	  for( j = 0; j < coVectorSize(characteristic_vector); j++ )
	  {
		  characteristic_rec = coVectorGet(characteristic_vector, j);
		  key = coStrGet(coVectorGet(characteristic_rec, 1));		// CHARACTERISTIC Name
		  all_map_value_vector = (co)coMapGet(all_characteristic_map, key);
		  if ( all_map_value_vector == NULL )
		  {
			  // a new key was found, create the key value pair for that new key
			  all_map_value_vector = coNewVector(CO_NONE);		// don't free the elements, they belong to the sw objects
			  if ( all_map_value_vector == NULL )
				return coDelete(all_characteristic_map), NULL;
			  for( k = 0; k < coVectorSize(sw_list); k++ )
				if ( coVectorAdd(all_map_value_vector, NULL) < 0 )		// prefill the value vector with NULL pointer
  				  return coDelete(all_characteristic_map), NULL;
			  if ( coMapAdd( all_characteristic_map, key, all_map_value_vector ) == 0 )
  				  return coDelete(all_characteristic_map), NULL;				  
		  }
		  coVectorSet(all_map_value_vector, i, characteristic_rec);
	  }
  }
  t1 = getEpochMilliseconds();  
  if ( is_verbose )
	  printf("Build CHARACTERISTIC difference, milliseconds=%lld\n", t1-t0);
  return all_characteristic_map;
}

co getFunctionCharacteristicDifferenceMap(cco sw_list, const char *function)
{
	return NULL;
}

static int getAllFunctionDifferenceMapCBCB(cco o, long idx, const char *key, cco characteristic_rec, void *data)
{
	/* characteristic_rec is ignored */
	co all_def_characteristic_map = (co)(data);
	coMapAdd(all_def_characteristic_map, key, NULL);	
	return 1;
}

static int getAllFunctionDifferenceMapCB(cco o, long idx, const char *key, cco all_def_characteristic_map, void *data)
{
	cco sw_object = (cco)data;
	cco fn_def_char_map = coVectorGet(sw_object, FUNCTION_DEF_CHARACTERISTIC_MAP_POS);
	cco sw_obj_def_characteristic_map = coMapGet(fn_def_char_map, key);	// does the key (=function name) exist in the sw object specific function list?
	if ( sw_obj_def_characteristic_map != NULL ) // yes, the key (=function name) exists
	{
		// merge all keys from that map into the all_def_characteristic_map map
		coMapForEach(sw_obj_def_characteristic_map, getAllFunctionDifferenceMapCBCB, (void *)all_def_characteristic_map); // returns 0 as soon as the callback function returns 0
	}
	return 1;
}

/*
	returns a map which has the same structure as FUNCTION_DEF_CHARACTERISTIC_MAP_POS, but is considers all functions from all sw releases in the sw list
	
	returns a map of a map
		outer map: key = function name, value = inner map
		inner map: key = characteristic name / axis pts name, value = NULL
*/
co getAllFunctionDifferenceMap(cco sw_list)
{
    long long int t0, t1;
	long i, sw_list_cnt;
	long j, function_cnt;
	cco sw_object;
	
	cco function_vector;
	cco function_rec;
	const char *function_name;
	co all_function_def_characteristic_map = coNewMap(CO_FREE_VALS); 	// key is neither strdup'd nor free'd, because it is taken from the FUNCTION record
	
	// step 1: build a map with all function names from all releases in sw_list

    t0 = getEpochMilliseconds();  
	
	sw_list_cnt = coVectorSize(sw_list);
	for( i = 0; i < sw_list_cnt; i++ )	// loop over all sw release, which are provided in "sw_list"
	{
		sw_object = coVectorGet(sw_list, i);		// get the current sw release for the outer loop body


		// Step 1: loop over FUNCTION_VECTOR_POS and fill all_function_def_characteristic_map with all functions (key=function name, value=map to the CHARACTERISTIC/AXIS_PTS)
		
		function_vector = coVectorGet(sw_object, FUNCTION_VECTOR_POS);
		function_cnt = coVectorSize(function_vector);
		for( j = 0; j < function_cnt; j++ )
		{
			function_rec = coVectorGet(function_vector, j);
			// function_map key=funciton name, value=map with ...
			//			... key=CHARACTERISTIC or AXIS_PTS name, value reference to CHARACTERISTIC or AXIS_PTS
			function_name = coStrGet(coVectorGet(function_rec, 1));
			if ( coMapGet(all_function_def_characteristic_map, function_name) == NULL )
			{
				coMapAdd(all_function_def_characteristic_map, function_name, coNewMap(CO_NONE));
			}
		}
	}
	
	// step 2: fill the key of inner map with all the names from the FUNCTION_DEF_CHARACTERISTIC_MAP_POS inner map. 

	for( i = 0; i < sw_list_cnt; i++ )	// loop over all sw release, which are provided in "sw_list"
	{
		sw_object = coVectorGet(sw_list, i);		// get the current sw release for the outer loop body
		
		// loop over all_function_def_characteristic_map, which has been constructed above
		
		coMapForEach(all_function_def_characteristic_map, getAllFunctionDifferenceMapCB, (void *)sw_object); // returns 0 as soon as the callback function returns 0
	}

    t1 = getEpochMilliseconds();  
    if ( is_verbose )
	  printf("Build FUNCTION - CHARACTERISTIC difference, milliseconds=%lld\n", t1-t0);
	
	return all_function_def_characteristic_map;
}

/*
	Calculate the difference between multiple characteristic records across different sw releases

	return values:
		0	all characteristics/axis pts in the list are equal
		1	characteristic/axis pts contains unsupported record in any release
		2	characteristic/axis pts differ in record size between releases 
		4 	characteristic/axis pts differ in data values between releases
		8 	some characteristic/axis pts are not present in the release
		16	belongs to different function across releases
	
	sw_list:				list of sw releases, size of sw_list match the size of the characteristic_list
	characteristic_name:	The name of the characteristic/axis pts, which has to be checked. The name should be equal to all the names of 
			the characteristics/axis pts in characteristic_list
	characteristic_list: 	A list of characteristic/axis pts records. The size must be equal to the number of releases in sw_list.
			The name of each of the characteristic records must be equal to characteristic_name
			
			
	TODO: Update this funciton to support axis_pts records inside characteristic_list --> DONE
	
*/

const char *getStringFromDifferenceNumber(int d)
{
	if ( d == 0 )
		return "No characteristic/axis_pts difference";
	if ( d & 1 )
		return "Record/data type not supported, difference unknown";
	if ( d & 2 )
		return "Data type difference";
	if ( d & 4 )
		return "Data value difference";
	if ( d & 8 )
		return "Missing characteristic/axis_pts";
	if ( d & 16 )
		return "Moved characteristic/axis_pts";
	return "";
}

int getCharacteristicAxisPtsDifference(cco sw_list, const char *characteristic_name, cco characteristic_list)
{
	long i, j;
	cco first;		// first characteristic record
	cco second;		// second characteristic record
	int result = 0;
	//cco first_function = NULL;
	//cco second_function = NULL;

	for( i = 0; i < coVectorSize(characteristic_list); i++ )	// loop over all characteristic records for this label
	{
		first = coVectorGet(characteristic_list, i);			// "first" will be a link to the characteristic record
		if ( first == NULL )
		{
			result |= 8;
		}
		else
		{
			// each element in sw_list corresponds to an element in characteristic_list.
			// coVectorGet(characteristic_list, i) is a characteristic which belongs to software in coVectorGet(sw_list, i);
			cco first_sw_object = coVectorGet(sw_list, i);	
			//key = coStrGet(coVectorGet(first_sw_object, 1));
			//assert(first_sw_object != NULL);  // check whether the release is really present... this assert doesn't make much sense
			// get the functon record for that characteristic 
			long long int first_address;
			long first_size;
			int first_is_supported = getCharacteristicAxisPtsMemoryArea(first_sw_object, first, &first_address, &first_size);
			cco first_function = coMapGet(coVectorGet(first_sw_object, BELONGS_TO_FUNCTION_MAP_POS), characteristic_name); 

			if ( first_is_supported == 0 )
			{
				result |= 1;	// record layout not supported
			}
			else
			{
				for( j = i + 1; j < coVectorSize(characteristic_list); j++ )
				{
					second = coVectorGet(characteristic_list, j);
					if ( second != NULL )
					{
						// first and second are not NULL
						cco second_sw_object = coVectorGet(sw_list, j);
						//assert(second_sw_object != NULL);
						cco second_function = coMapGet(coVectorGet(second_sw_object, BELONGS_TO_FUNCTION_MAP_POS), characteristic_name); 
						
						long long int second_address;
						long second_size;
						int second_is_supported = getCharacteristicAxisPtsMemoryArea(second_sw_object, second, &second_address, &second_size);
						
						if ( strcmp(coStrGet(coVectorGet(first_function,1)), coStrGet(coVectorGet(second_function,1))) != 0 )
						{
							result |= 16;	// record layout not supported
						}
						
						if ( second_is_supported == 0 )
						{
							result |= 1;	// record layout not supported
						}
						else if ( first_size != second_size )
						{
							result |= 2; 	// record size different
						}
						else
						{
							unsigned char *first_mem = getMemoryArea(first_sw_object, first_address, first_size);
							unsigned char *second_mem = getMemoryArea(second_sw_object, second_address, second_size);
							
							if ( first_mem != NULL && second_mem != NULL )
							{
								if ( memcmp(first_mem, second_mem, first_size) != 0 )
								{
									result |= 4; // data difference
								}
							}
						}
					} // if ( second != NULL )
				} // for
			} // if ( first_is_supported == 0 )
		}
	}
	return result;
}

int showAllCharacteristicDifferenceMapCB(cco o, long idx, const char *key, cco value_vector, void *data)
{
	cco sw_list = (cco)data;
	long i, j;
	int nullCnt = 0; 	// number of NULL records in the value_vector 
	cco first;
	cco second;
	char *mem_info = NULL;
	const char *function_path_name = NULL;
	
	assert(sw_list != NULL);
	for( i = 0; i < coVectorSize(value_vector); i++ )	// loop over all characteristic records for this label
	{
		first = coVectorGet(value_vector, i);			// "first" will be a link to the characteristic record
		if ( first == NULL )
		{
			nullCnt++;
		}
		else
		{
			// each element in sw_list corresponds to an element in value_vector.
			// coVectorGet(value_vector, i) is a characteristic which belongs to software in coVectorGet(sw_list, i);
			cco first_sw_object = coVectorGet(sw_list, i);	
			assert(first_sw_object != NULL);
			if ( function_path_name == NULL )
			{
				cco m = coVectorGet(first_sw_object, BELONGS_TO_FUNCTION_MAP_POS);
				cco function = coMapGet(m, key);
				if ( function != NULL )
				{
					function_path_name = getFullFunctionName(first_sw_object, function);
				}
			}
			
			for( j = i + 1; j < coVectorSize(value_vector); j++ )
			{
				second = coVectorGet(value_vector, j);
				if ( second != NULL )
				{
					// first and second are not NULL
					cco second_sw_object = coVectorGet(sw_list, j);
					assert(second_sw_object != NULL);
					long long int first_address;
					long long int second_address;
					long first_size;
					long second_size;
					int first_is_supported = getCharacteristicAxisPtsMemoryArea(first_sw_object, first, &first_address, &first_size);
					int second_is_supported = getCharacteristicAxisPtsMemoryArea(second_sw_object, second, &second_address, &second_size);
					
					if ( first_is_supported == 0 || second_is_supported == 0 )
					{
						mem_info = "record layout not supported";
						break;
					}
					else if ( first_size != second_size )
					{
						mem_info = "record size different";
						break;
					}
					else
					{
						unsigned char *first_mem = getMemoryArea(first_sw_object, first_address, first_size);
						unsigned char *second_mem = getMemoryArea(second_sw_object, second_address, second_size);
						
						if ( first_mem != NULL && second_mem != NULL )
						{
							if ( memcmp(first_mem, second_mem, first_size) != 0 )
							{
								mem_info = "data difference";
							}
						}
					}
				}
			}
		}
	}
	if ( nullCnt == 0 && mem_info == NULL )
		return 1; // exit, if there are no dfferences
	
	if ( function_path_name == NULL )
		function_path_name = "";
	
	printf("%-79s %-99s ", function_path_name, key);
	for( i = 0; i < coVectorSize(value_vector); i++ )
		if ( coVectorGet(value_vector, i) == NULL )
			printf(" _");
		else
			printf(" x");
	printf(" %s", mem_info==NULL?"":mem_info);
	
	{
		int d = getCharacteristicAxisPtsDifference(sw_list, key, value_vector);
		if ( d & 1 ) printf(" 'unsupported'");
		if ( d & 2 ) printf(" 'size difference'");
		if ( d & 4 ) printf(" 'value difference'");
		if ( d & 8 ) printf(" 'added/removed'");
		if ( d & 16 ) printf(" 'function difference'");
	}
	printf("\n");
	return 1;
}

void showAllCharacteristicDifferenceMap(cco all_characteristic_map, cco sw_list)
{
	printf("%-79s %-99s %s\n", "Function Path", "Characteristic", "Difference");
	coMapForEach(all_characteristic_map, showAllCharacteristicDifferenceMapCB, (void *)sw_list);
}

void showFunctionList(cco sw_object)
{
	cco v = coVectorGet(sw_object, FUNCTION_VECTOR_POS);
	cco fndef_map = coVectorGet(sw_object, FUNCTION_DEF_CHARACTERISTIC_MAP_POS);
	cco def_characteristic_map;
	long i, def_cnt, def_sum;
	def_sum = 0;
	for( i = 0; i < coVectorSize(v); i++ )
	{
		def_cnt = 0;
		def_characteristic_map = coMapGet(fndef_map, coStrGet(coVectorGet(coVectorGet(v, i), 1)));
		if ( def_characteristic_map != NULL )
		{
			
			def_cnt = coMapSize(def_characteristic_map);
			def_sum += def_cnt;
		}			
		printf("%05ld %s def-cnt=%ld (%ld)\n", i, getFullFunctionName(sw_object, coVectorGet(v, i)), def_cnt, def_sum);
	}
}

/*=================================================================================================*/

void showFunctionDifferenceList(cco all_function_def_characteristic_map, cco sw_list)
{
	coMapIterator function_iterator;
	coMapIterator characteristic_iterator;
	cco characteristic_map;
	const char *characteristic_axis_pts_name;
	const char *function_name;
	cco sw_object;
	cco belongs_to_function_map;
	cco belongs_to_function_rec;
	cco function_name_map;
	cco function_rec;
	cco characteristic_name_map;
	cco characteristic_rec;
	cco axis_pts_name_map;
	cco axis_pts_rec;
	co characteristic_axis_pts_list;
	int i, cnt;
	int difference_number;
	int is_other_function;
	long function_version_index;
	const char *function_version_string;
	char exists[SW_PAIR_MAX];
	
	characteristic_axis_pts_list = coNewVector(CO_NONE);
	
	assert(coIsMap(all_function_def_characteristic_map));
	assert(coIsVector(sw_list));
	
	cnt = coVectorSize(sw_list);	// number of a2l/s19 pairs = number of sw releases, which should be compared
	
	// outer loop over all functions
	if ( coMapLoopFirst(&function_iterator, all_function_def_characteristic_map) )
	{
		do {
			function_name = coMapLoopKey(&function_iterator); 
			printf("%s: ", function_name);

			// output the function version from each a2l, the function version is optional and might be empty
			for( i = 0; i < cnt; i++ )
			{
				sw_object = coVectorGet(sw_list, i);
				function_version_string = "";
				function_name_map = coVectorGet(sw_object, FUNCTION_NAME_MAP_POS);
				function_rec = coMapGet(function_name_map, function_name);
				if ( function_rec != NULL )
				{
					function_version_index = getVectorIndexByString(function_rec, "FUNCTION_VERSION");
					if ( function_version_index >= 0 )
					{
						function_version_string = coStrGet(coVectorGet(function_rec, function_version_index+1));
					}
				}
				printf("<%s> ", function_version_string);
			}
			
			// output the description of the function from each a2l
			for( i = 0; i < cnt; i++ )
			{
				sw_object = coVectorGet(sw_list, i);
				function_name_map = coVectorGet(sw_object, FUNCTION_NAME_MAP_POS);
				function_rec = coMapGet(function_name_map, function_name);
				if ( function_rec == NULL )
				{
					printf("[] ");
				}
				else
				{
					printf("[%s] ", coStrGet(coVectorGet(function_rec, 2)));	// output description, which should include the version number
				}
			}
			printf("\n");

			// the outer loop goes over all_function_def_characteristic_map: the value is a map with the characteristics and axis_pts names
			characteristic_map = coMapLoopValue(&function_iterator);
			
			// inner loop over all characteristic and axis_pts names of that function
			if ( coMapLoopFirst(&characteristic_iterator, characteristic_map) )
			{
				do {
					characteristic_axis_pts_name = coMapLoopKey(&characteristic_iterator);
					
					// in the third nested loop: compare the characteristic/axis_pts records and values from s19
					// as a preparation, construct two arrays:
					// 	1) A vector with the characteristic/axis_pts records
					//	2) A string with some hints regarding the characteristic/axis_pts
					//			lower case: the characteristic/axis_pts existis, but actually belongs to a different function
					//			upper case: the characteristic/axis_pts belongs to this function
					//			underscore: the characteristic/axis_pts doesn't exist in this sw release
					coVectorClear(characteristic_axis_pts_list);
					for( i = 0; i < cnt; i++ )
					{
						sw_object = coVectorGet(sw_list, i);
						is_other_function = 0;
						belongs_to_function_map = coVectorGet(sw_object, BELONGS_TO_FUNCTION_MAP_POS);
						belongs_to_function_rec = coMapGet(belongs_to_function_map, characteristic_axis_pts_name);
						if ( belongs_to_function_rec == NULL )
							is_other_function = 1;
						else if ( strcmp(function_name, coStrGet(coVectorGet(belongs_to_function_rec, 1))) != 0 )
							is_other_function = 1;
						characteristic_name_map = coVectorGet(sw_object, CHARACTERISTIC_NAME_MAP_POS);
						axis_pts_name_map = coVectorGet(sw_object, AXIS_PTS_NAME_MAP_POS);
						characteristic_rec = coMapGet(characteristic_name_map, characteristic_axis_pts_name);
						axis_pts_rec = coMapGet(axis_pts_name_map, characteristic_axis_pts_name);
						if ( characteristic_rec != NULL )
						{
							if ( is_other_function )
								exists[i] = 'c';
							else
								exists[i] = 'C';
							coVectorAdd(characteristic_axis_pts_list, characteristic_rec);
						}
						else if ( axis_pts_rec != NULL )
						{
							if ( is_other_function )
								exists[i] = 'a';
							else
								exists[i] = 'A';
							coVectorAdd(characteristic_axis_pts_list, axis_pts_rec);
						}
						else
						{
							exists[i] = '_';
						}
					}
					exists[i] = '\0';

					// with the above created list of characteristic/axis_pts records, get a hint regarding the diff
					difference_number = getCharacteristicAxisPtsDifference(sw_list, characteristic_axis_pts_name, characteristic_axis_pts_list);   

					// finally output all the above calculated information
					if ( difference_number > 0 )
						printf("  %s %3d %s\n", exists, difference_number, coMapLoopKey(&characteristic_iterator));
					
				} while( coMapLoopNext(&characteristic_iterator) );
			}	
		} while( coMapLoopNext(&function_iterator) );
	}	
	coDelete(characteristic_axis_pts_list);
}

/*=================================================================================================*/

void outJSON(const char *fmt, ...)
{
	va_list ap;
	va_start(ap, fmt);
	vfprintf(json_fp, fmt, ap);
	va_end(ap);
}

void outJSONOptional(const char *s, int is_output)
{
	if ( is_output )
		while( *s != '\0' )
			fputc(*s++, json_fp);
}

void outJSONStr(const char *s)
{
	FILE *fp = json_fp;
	size_t cnt = strlen(s);
	size_t i = 0;
	if ( cnt > 1 )
	{
		if (s[0] == '\"' && s[cnt-1] == '\"' )
		{
			i++;
			s++;
			cnt-=1;
		}
	}
	
	fputc('"', fp);
	while( *s != '\0' && i < cnt )
	{
		if ( *s == '\"' || *s == '/' || *s == '\\' )
		{
		  fputc('\\', fp);
		  fputc(*s, fp);
		}
		else if ( *s == '\n' )
		{
		  fputc('\\', fp);
		  fputc('n', fp);
		}
		else if ( *s == '\r' )
		{
		  fputc('\\', fp);
		  fputc('r', fp);
		}
		else if ( *s == '\t' )
		{
		  fputc('\\', fp);
		  fputc('t', fp);
		}
		else if ( *s == '\f' )
		{
		  fputc('\\', fp);
		  fputc('f', fp);
		}
		else if ( *s == '\b' )
		{
		  fputc('\\', fp);
		  fputc('b', fp);
		}
		else
		{
		  fputc(*s, fp);
		}
		s++;
		i++;
	}
	fputc('"', fp);
}

/*=================================================================================================*/
/* -cjsondiff */

int showCharacteristicJSONDifferenceCB(cco o, long idx, const char *key, cco value_vector, void *data)
{
	static char presence[SW_PAIR_MAX+1];
	static int is_first = 1;
	cco sw_list = (cco)data;
	long i;
	int d = getCharacteristicAxisPtsDifference(sw_list, key, value_vector);
	
	if ( d != 0 )
	{
		if ( is_first != 0 )
		{
			is_first = 0;
		}
		else
		{
			outJSON(",\n");
		}
		
		/*
		if ( d & 1 ) printf(" 'unsupported'");
		if ( d & 2 ) printf(" 'size difference'");
		if ( d & 4 ) printf(" 'value difference'");
		if ( d & 8 ) printf(" 'added/removed'");
		if ( d & 16 ) printf(" 'function difference'");
		*/


		for( i = 0; i < coVectorSize(value_vector); i++ )
			if ( coVectorGet(value_vector, i) == NULL )
				presence[i] = '_';
			else
				presence[i] = 'x';
		presence[i] = '\0';
		
		outJSON("{");
		outJSON("\"n\":\"%s\", ", key);
		outJSON("\"d\":%d,", d);
		outJSON("\"p\":\"%s\"", presence);
		outJSON("}");
	}
	return 1;
}

/*
	all_characteristic_map	a map constructed with getAllCharacteristicDifferenceMap(sw_list).
					key: characteristic name, value: vector with either null or a reference to the characteristic record. Size of this vector must be the size of sw_list
	sw_list	list of multiple sw objects
*/
void showCharacteristicJSONDifference(cco all_characteristic_map, cco sw_list)
{
	int i;
	outJSON("{\n");
	outJSON("\"software\":[\n");
	for( i = 0; i < sw_pair_cnt; i++ )
	{
		outJSON("{\"a2l\":\"%s\", \"data\":\"%s\"}%s\n", 
				a2l_file_name_list[i], s19_file_name_list[i], i+1<sw_pair_cnt?",":"");
	}
	outJSON("],\n");
	outJSON("\"labels\":[\n");
	coMapForEach(all_characteristic_map, showCharacteristicJSONDifferenceCB, (void *)sw_list);
	
	outJSON("]\n");
	outJSON("}\n");
}

/*=================================================================================================*/
/* -fnjsondiff */

/*
{
 "software":[
  {"a2l":"release1.a2l.gz", "data":"release1.s19.gz"},	// sw release 1 
  {"a2l":"release2.a2l.gz", "data":"release2.s19.gz"}	// sw release 2 
 ],
 "functions":[
  ["fn1",
   [["p1","p2"],["p1","p2","p3"]],
   ["1.0", "1.1"],0,
   ["desc1", "desc1"],0,
   [
   ]
  ],
  ["fn2",
   [["p1","p2"],["p1","p2","p3"]],	// list of sub-function parents within each sw release
   ["1.1", "1.1"],1,			// version of the function within each sw release
   ["desc1", "desc1"],0,		// description of the function within each sw release
   [							// characteristic/axis_pts which differ between the sw releases
    ["characteristic1", "CC", 1],
    ["characteristic2", "CC", 2],
    ["characteristic3", "CC", 4],
    ["characteristic4", "CC", 8]
   ]
  ]
 ]
}
*/

void showFunctionJSONDifference(cco all_function_def_characteristic_map, cco sw_list, int is_readable)
{
	coMapIterator function_iterator;
	coMapIterator characteristic_iterator;
	cco characteristic_map;
	const char *characteristic_axis_pts_name;
	const char *function_name;
	const char *f;
	cco tmp;
	cco sw_object;
	cco parent_function_map;
	cco belongs_to_function_map;
	cco belongs_to_function_rec;
	cco function_name_map;
	cco function_rec;
	cco characteristic_name_map;
	cco characteristic_rec;
	cco axis_pts_name_map;
	cco axis_pts_rec;
	co characteristic_axis_pts_list;
	
	int i, cnt;
	int difference_number;
	int is_other_function;
	long function_version_index;
	const char *function_version_string;
	const char *prev_function_version_string;
	const char *function_desc_string;
	const char *prev_function_desc_string;
	char exists[SW_PAIR_MAX];
	int is_first_function = 1;
	int is_first_characteristic = 1;
	int is_version_equal = 1;
	int is_description_equal = 1;
	
	characteristic_axis_pts_list = coNewVector(CO_NONE);
	
	assert(coIsMap(all_function_def_characteristic_map));
	assert(coIsVector(sw_list));
	
	cnt = coVectorSize(sw_list);	// number of a2l/s19 pairs = number of sw releases, which should be compared


	outJSON("{");
	outJSONOptional("\n ", is_readable);
	outJSON("\"software\":[");
	outJSONOptional("\n", is_readable);
	for( i = 0; i < cnt; i++ )
	{
		outJSONOptional("  ", is_readable);
		outJSON("{\"a2l\":\"%s\",\"data\":\"%s\"}%s", 
				a2l_file_name_list[i], s19_file_name_list[i], i+1<sw_pair_cnt?",":"");
		outJSONOptional("\n", is_readable);
	}
	outJSONOptional(" ", is_readable);
	outJSON("],");
	outJSONOptional("\n ", is_readable);
	outJSON("\"functions\":[");	
	// outer loop over all functions
	if ( coMapLoopFirst(&function_iterator, all_function_def_characteristic_map) )
	{
		do {
			if ( is_first_function )
			{
				is_first_function = 0;
				outJSONOptional("\n", 1);
			}
			else
			{
				outJSON(",");
				outJSONOptional("\n", 1);
			}

			outJSONOptional("  ", is_readable);
			outJSON("[");
			
			
			function_name = coMapLoopKey(&function_iterator); 
			outJSONStr(function_name);
			outJSON(",");
			outJSONOptional("\n", is_readable);

			// output the function parent list
			// getFullFunctionName()
			outJSONOptional("   ", is_readable);
			outJSON("[");
			is_version_equal = 1;
			prev_function_version_string = "";
			for( i = 0; i < cnt; i++ )
			{
				if ( i != 0 )					
					outJSON(",");
				sw_object = coVectorGet(sw_list, i);
				parent_function_map = coVectorGet(sw_object, PARENT_FUNCTION_MAP_POS);
				f = function_name;
				outJSON("[");
				for(;;)
				{
					tmp = coMapGet(parent_function_map, f);
					if ( tmp == NULL )
						break;
					if ( f != function_name )
						outJSON(",");
					f = coStrGet(coVectorGet(tmp, 1));
					if ( f == NULL )
						break;
					outJSON("\"%s\"", f);
				}
				outJSON("]");				
			}
			outJSON("],");
			outJSONOptional("\n", is_readable);


			// output the function version from each a2l, the function version is optional and might be empty
			outJSONOptional("   ", is_readable);
			outJSON("[");
			is_version_equal = 1;
			prev_function_version_string = "";
			for( i = 0; i < cnt; i++ )
			{
				if ( i != 0 )					
					outJSON(",");
				sw_object = coVectorGet(sw_list, i);
				function_version_string = "";
				function_name_map = coVectorGet(sw_object, FUNCTION_NAME_MAP_POS);
				function_rec = coMapGet(function_name_map, function_name);
				if ( function_rec != NULL )
				{
					function_version_index = getVectorIndexByString(function_rec, "FUNCTION_VERSION");
					if ( function_version_index >= 0 )
					{
						function_version_string = coStrGet(coVectorGet(function_rec, function_version_index+1));
					}
				}
				if ( i != 0 )
					if ( strcmp(prev_function_version_string, function_version_string) != 0 )
						is_version_equal = 0;
				outJSONStr(function_version_string);
				prev_function_version_string = function_version_string;
			}
			outJSON("],%d,", is_version_equal);
			outJSONOptional("\n", is_readable);
			
			// output the description of the function from each a2l
			outJSONOptional("   ", is_readable);
			outJSON("[");
			is_description_equal = 1;
			prev_function_desc_string = "";
			for( i = 0; i < cnt; i++ )
			{
				if ( i != 0 )
					outJSON(",");
				sw_object = coVectorGet(sw_list, i);
				function_name_map = coVectorGet(sw_object, FUNCTION_NAME_MAP_POS);
				function_rec = coMapGet(function_name_map, function_name);
				if ( function_rec == NULL )
				{
					function_desc_string = "";
				}
				else
				{
					function_desc_string = coStrGet(coVectorGet(function_rec, 2)); // description, which should include the version number
				}
				if ( i != 0 )
					if ( strcmp(prev_function_desc_string, function_desc_string) != 0 )
						is_description_equal = 0;
				outJSONStr(function_desc_string); 
				prev_function_desc_string = function_desc_string;
			}
			outJSON("],%d,", is_description_equal);
			outJSONOptional("\n", is_readable);

			// the outer loop goes over all_function_def_characteristic_map: the value is a map with the characteristics and axis_pts names
			characteristic_map = coMapLoopValue(&function_iterator);
			
			// inner loop over all characteristic and axis_pts names of that function
			outJSONOptional("   ", is_readable);
			outJSON("[");
			is_first_characteristic = 1;
			if ( coMapLoopFirst(&characteristic_iterator, characteristic_map) )
			{
				do {
					characteristic_axis_pts_name = coMapLoopKey(&characteristic_iterator);
					
					// in the third nested loop: compare the characteristic/axis_pts records and values from s19
					// as a preparation, construct two arrays:
					// 	1) A vector with the characteristic/axis_pts records
					//	2) A string with some hints regarding the characteristic/axis_pts
					//			lower case: the characteristic/axis_pts existis, but actually belongs to a different function
					//			upper case: the characteristic/axis_pts belongs to this function
					//			underscore: the characteristic/axis_pts doesn't exist in this sw release
					coVectorClear(characteristic_axis_pts_list);
					for( i = 0; i < cnt; i++ )
					{
						sw_object = coVectorGet(sw_list, i);
						is_other_function = 0;
						belongs_to_function_map = coVectorGet(sw_object, BELONGS_TO_FUNCTION_MAP_POS);
						belongs_to_function_rec = coMapGet(belongs_to_function_map, characteristic_axis_pts_name);
						if ( belongs_to_function_rec == NULL )
							is_other_function = 1;
						else if ( strcmp(function_name, coStrGet(coVectorGet(belongs_to_function_rec, 1))) != 0 )
							is_other_function = 1;
						characteristic_name_map = coVectorGet(sw_object, CHARACTERISTIC_NAME_MAP_POS);
						axis_pts_name_map = coVectorGet(sw_object, AXIS_PTS_NAME_MAP_POS);
						characteristic_rec = coMapGet(characteristic_name_map, characteristic_axis_pts_name);
						axis_pts_rec = coMapGet(axis_pts_name_map, characteristic_axis_pts_name);
						if ( characteristic_rec != NULL )
						{
							if ( is_other_function )
								exists[i] = 'c';
							else
								exists[i] = 'C';
							coVectorAdd(characteristic_axis_pts_list, characteristic_rec);
						}
						else if ( axis_pts_rec != NULL )
						{
							if ( is_other_function )
								exists[i] = 'a';
							else
								exists[i] = 'A';
							coVectorAdd(characteristic_axis_pts_list, axis_pts_rec);
						}
						else
						{
							exists[i] = '_';
						}
					}
					exists[i] = '\0';

					// with the above created list of characteristic/axis_pts records, get a hint regarding the diff
					difference_number = getCharacteristicAxisPtsDifference(sw_list, characteristic_axis_pts_name, characteristic_axis_pts_list);   

					// finally output all the above calculated information
					if ( difference_number > 0 )
					{
						if ( is_first_characteristic )
						{
							is_first_characteristic = 0;
							//outJSON("\n");
							outJSONOptional("\n", is_readable);
						}
						else
						{
							outJSON(",");
							outJSONOptional("\n", is_readable);
						}
						outJSONOptional("    ", is_readable);
						outJSON("[\"%s\",\"%s\",%d]", characteristic_axis_pts_name, exists, difference_number);
						//outJSON("    [\"%s\", \"%s\", \"%s\"]", characteristic_axis_pts_name, exists, getStringFromDifferenceNumber(difference_number));
					}
					
				} while( coMapLoopNext(&characteristic_iterator) );
			}
			outJSONOptional("\n   ", is_readable);
			//outJSON("\n");
			outJSON("]");
			outJSONOptional("\n  ", is_readable);
			//outJSON("\n");
			outJSON("]");
		} while( coMapLoopNext(&function_iterator) );
	}	

	outJSONOptional("\n ", is_readable);
	//outJSON("\n");
	outJSON("]");
	outJSONOptional("\n", is_readable);
	//outJSON("\n");
	outJSON("}\n");
	
	coDelete(characteristic_axis_pts_list);
}

/*=================================================================================================*/

void help(void)
{
  puts("-h            This help text");
  puts("-a2l <file>   A2L File (accepts .gz files, can be used mutliple times)");
  puts("-s19 <file>   S19 File (accepts .gz files, can be used mutliple times)");
  puts("-v            Verbose output");
  puts("-ascii        Output all ASCII Characteristics");
  puts("-addrlist     Output all characteristics and axis_pts sorted by memory address");  
  puts("-fnlist       Output all function names");
  puts("-diff         A2L S19 difference analysis (requires multipe a2l/s19 pairs)");
  puts("-fndiff       A2L S19 difference analysis (requires multipe a2l/s19 pairs)");
  puts("-cjsondiff    Similar to -diff, but use JSON format (requires multipe a2l/s19 pairs)");
  puts("-fnjsondiff   Similar to -fndiff, but use JSON format (requires multipe a2l/s19 pairs)");
  puts("-json <file>  Output file for '-cjsondiff' and '-fnjsondiff'");
  
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
    else if ( strcmp(*argv, "-v" ) == 0 )
    {
	  is_verbose = 1;
      argv++;
    }
    else if ( strcmp(*argv, "-ascii" ) == 0 )
    {
	  is_ascii_characteristic_list = 1;
      argv++;
    }
    else if ( strcmp(*argv, "-addrlist" ) == 0 )
    {
	  is_characteristic_address_list = 1;
      argv++;
    }
    else if ( strcmp(*argv, "-fnlist" ) == 0 )
    {
	  is_function_list = 1;
      argv++;
    }
    else if ( strcmp(*argv, "-diff" ) == 0 )
    {
	  is_diff = 1;
      argv++;
    }
    else if ( strcmp(*argv, "-fndiff" ) == 0 )
    {
	  is_fndiff = 1;
      argv++;
    }
    else if ( strcmp(*argv, "-cjsondiff" ) == 0 )
    {
	  is_characteristicjsondiff = 1;
      argv++;
    }
    else if ( strcmp(*argv, "-fnjsondiff" ) == 0 )
    {
	  is_functionjsondiff = 1;
      argv++;
    }
	else if ( strcmp(*argv, "-json" ) == 0 )
	{
       argv++;
	   if ( *argv == NULL )
	   {
		  fprintf(stderr, "Missing argument for -json\n");
		  exit(1);
	   }
	   json_fp = fopen(*argv, "wb");
	   if ( json_fp == NULL )
	   {
		  perror(*argv);
		  exit(1);
	   }
	   
	}
    else if ( strcmp(*argv, "-a2l" ) == 0 )
    {
      argv++;
	  
      if ( *argv != NULL )
      {
		if ( sw_pair_cnt+1 < SW_PAIR_MAX )
		{
			if ( a2l_file_name_list[sw_pair_cnt] != NULL )
				sw_pair_cnt++;
			a2l_file_name_list[sw_pair_cnt] = *argv;
		}
        argv++;
      }
    }
    else if ( strcmp(*argv, "-s19" ) == 0 )
    {
      argv++;
      if ( *argv != NULL )
      {
		if ( sw_pair_cnt+1 < SW_PAIR_MAX )
		{
			if ( s19_file_name_list[sw_pair_cnt] != NULL )
				sw_pair_cnt++;
			s19_file_name_list[sw_pair_cnt] = *argv;
		}
        argv++;
      }
    }
    else
    {
      argv++;
    }
  }
  
  if ( a2l_file_name_list[sw_pair_cnt] != NULL )
	sw_pair_cnt++;
  if ( sw_pair_cnt == 0 )
	  help();
  
  return 1;
}



int main(int argc, char **argv)
{
	
  long i;
  co sw_list;
  cco sw_object = NULL;
  co all_characteristic_map = NULL;
  co all_function_def_characteristic_map = NULL;
  
  json_fp = stdout;
  
  parse_args(argc, argv);

  sw_list = getSWList();
  
  for( i = 0; i < coVectorSize(sw_list); i++ )
  {
	  sw_object = coVectorGet(sw_list, i);
  
	  if ( is_verbose ) 
	  {
		  printf("%s:\n", a2l_file_name_list[i]);
		  printf("  COMPU_METHOD cnt=%ld\n", coMapSize(coVectorGet(sw_object, COMPU_METHOD_MAP_POS)));
		  printf("  COMPU_VTAB cnt=%ld\n", coMapSize(coVectorGet(sw_object, COMPU_VTAB_MAP_POS)));
		  printf("  RECORD_LAYOUT cnt=%ld\n", coMapSize(coVectorGet(sw_object, RECORD_LAYOUT_MAP_POS)));
		  printf("  CHARACTERISTIC Vector cnt=%ld\n", coVectorSize(coVectorGet(sw_object, CHARACTERISTIC_VECTOR_POS)));
		  printf("  CHARACTERISTIC Name Map cnt=%ld\n", coMapSize(coVectorGet(sw_object, CHARACTERISTIC_NAME_MAP_POS)));
		  printf("  AXIS_PTS Vector cnt=%ld\n", coVectorSize(coVectorGet(sw_object, AXIS_PTS_VECTOR_POS)));
		  printf("  AXIS_PTS Name Map cnt=%ld\n", coMapSize(coVectorGet(sw_object, AXIS_PTS_NAME_MAP_POS)));
		  printf("  Address Map cnt=%ld\n", coMapSize(coVectorGet(sw_object, ADDRESS_MAP_POS)));
		  printf("  FUNCTION Vector cnt=%ld\n", coVectorSize(coVectorGet(sw_object, FUNCTION_VECTOR_POS)));
		  printf("  FUNCTION Name Map cnt=%ld\n", coMapSize(coVectorGet(sw_object, FUNCTION_NAME_MAP_POS)));
		  printf("  FUNCTION CHARACTERISTIC/AXIS_PTS Map cnt=%ld\n", coMapSize(coVectorGet(sw_object, FUNCTION_DEF_CHARACTERISTIC_MAP_POS)));
		  printf("  Parent Function Map cnt=%ld\n", coMapSize(coVectorGet(sw_object, PARENT_FUNCTION_MAP_POS)));
		  printf("  Belongs to Function Map cnt=%ld\n", coMapSize(coVectorGet(sw_object, BELONGS_TO_FUNCTION_MAP_POS)));
		  
	  }
	  if ( is_characteristic_address_list )
		showAddressList(sw_object);
	  if ( is_ascii_characteristic_list )
		showAllASCIICharacteristic(sw_object);
	  if ( is_function_list )
		showFunctionList(sw_object);
  }  
  
  if ( is_diff || is_characteristicjsondiff || is_fndiff || is_functionjsondiff  )
  {
	  all_characteristic_map = getAllCharacteristicDifferenceMap(sw_list);	
	  all_function_def_characteristic_map = getAllFunctionDifferenceMap(sw_list);  // TODO: show the content of the characteristics, based on the function name
	  if ( is_diff )
		showAllCharacteristicDifferenceMap(all_characteristic_map, sw_list);
	  if ( is_characteristicjsondiff )
		showCharacteristicJSONDifference(all_characteristic_map, sw_list);
	  if ( is_fndiff ) 
		showFunctionDifferenceList(all_function_def_characteristic_map, sw_list);
	  if ( is_functionjsondiff )
		showFunctionJSONDifference(all_function_def_characteristic_map, sw_list, 1);

	

#ifndef NDEBUG  
	  coDelete(all_characteristic_map); // delete this before sw_list (ok, wouldn't make a difference, but still)
	  coDelete(all_function_def_characteristic_map);
#endif
  }

  if ( json_fp != stdout )
	  fclose(json_fp);

#ifndef NDEBUG  
  coDelete(sw_list);     // delete all co objects, this is time consuming, so don't do this for the final release
#endif


  return 0;
}
  
  