/* 
  a2l_info
  
  a2l info tool, based on C Object Library 
  (c) 2024 Oliver Kraus
  https://github.com/olikraus/c-object

  CC BY-SA 3.0  https://creativecommons.org/licenses/by-sa/3.0/
  
  
  
*/

#include "co.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <assert.h>
#include <sys/time.h>

#define COMPU_METHOD_MAP_POS 0
#define COMPU_VTAB_MAP_POS 1
#define RECORD_LAYOUT_MAP_POS 2
#define ADDRESS_MAP_POS 3
#define CHARACTERISTIC_NAME_MAP_POS 4
#define AXIS_PTS_NAME_MAP_POS 5
#define CHARACTERISTIC_VECTOR_POS 6
#define AXIS_PTS_VECTOR_POS 7
#define A2L_POS 8
#define DATA_MAP_POS 9
#define DATA_VECTOR_POS 10


const char *a2l_file_name = NULL;
const char *s19_file_name = NULL;
int is_verbose = 0;
int is_ascii_characteristic_list = 0;
int is_characteristic_address_list = 0;

uint64_t getEpochMilliseconds(void)
{
	struct timeval tv;
	gettimeofday(&tv, NULL);
	return (uint64_t)(tv.tv_sec) * 1000 + (uint64_t)(tv.tv_usec) / 1000;
}

co create_sw_object(void)
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
  coVectorAdd(o, coNewVector(CO_NONE));          // CHARACTERISTIC_VECTOR_POS: references to CHARACTERISTIC
  coVectorAdd(o, coNewVector(CO_NONE));          // AXIS_PTS_VECTOR_POS: references to AXIS_PTS
  if ( coVectorSize(o) != 8 )
    return NULL;
  return o;
}

void showRecordLayout(cco o)
{
	
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
      coMapAdd((co)coVectorGet(sw_object, COMPU_METHOD_MAP_POS), 
        coStrGet(coVectorGet(a2l, 1)), 
        a2l);
    }
    if ( strcmp( coStrGet(element), "COMPU_VTAB" ) == 0 )
    {
      coMapAdd((co)coVectorGet(sw_object, COMPU_VTAB_MAP_POS), 
        coStrGet(coVectorGet(a2l, 1)), 
        a2l);
    }
    if ( strcmp( coStrGet(element), "RECORD_LAYOUT" ) == 0 )
    {
      coMapAdd((co)coVectorGet(sw_object, RECORD_LAYOUT_MAP_POS), 
        coStrGet(coVectorGet(a2l, 1)),
        a2l);
    }
    if ( strcmp( coStrGet(element), "CHARACTERISTIC" ) == 0 )
    {
	  const char *characteristic_name = coStrGet(coVectorGet(a2l, 1));
	  const char *characteristic_address = coStrGet(coVectorGet(a2l, 4));
      coVectorAdd((co)coVectorGet(sw_object, CHARACTERISTIC_VECTOR_POS), a2l);

      coMapAdd((co)coVectorGet(sw_object, CHARACTERISTIC_NAME_MAP_POS), characteristic_name, a2l);
		
      coMapAdd((co)coVectorGet(sw_object, ADDRESS_MAP_POS), characteristic_address, a2l);
    }
    if ( strcmp( coStrGet(element), "AXIS_PTS" ) == 0 )
    {
	  const char *axis_pts_name = coStrGet(coVectorGet(a2l, 1));
	  const char *axis_pts_address = coStrGet(coVectorGet(a2l, 3));
      coVectorAdd((co)coVectorGet(sw_object, AXIS_PTS_VECTOR_POS), a2l);

      coMapAdd((co)coVectorGet(sw_object, AXIS_PTS_NAME_MAP_POS), axis_pts_name, a2l);
		
      coMapAdd((co)coVectorGet(sw_object, ADDRESS_MAP_POS), axis_pts_address, a2l);
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
				while( matrix_dim_idx < 10 )
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



void help(void)
{
  puts("-h            This help text");
  puts("-a2l <file>   A2L File (also accepts .gz files)");
  puts("-s19 <file>   S19 File");
  puts("-v            Verbose output");
  puts("-ascii        Output all ASCII Characteristics");
  puts("-addrlist     Output all ASCII Characteristics");  
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
  uint64_t t0, t1, t2;
  
  co sw_object = create_sw_object();
  
  assert( sw_object != NULL );
  
  parse_args(argc, argv);
  
  if ( a2l_file_name == NULL )
    return coDelete(sw_object), 0;

  t0 = getEpochMilliseconds();  
  
  /* read a2l file */
  fp = fopen(a2l_file_name, "rb");  // we need to read binary so that the CR/LF conversion is suppressed on windows
  if ( fp == NULL )
    return perror(a2l_file_name), coDelete(sw_object), 0;
  if ( is_verbose ) printf("Reading A2L '%s'\n", a2l_file_name);
  coVectorAdd(sw_object, coReadA2LByFP(fp));
  t1 = getEpochMilliseconds();
  if ( is_verbose ) printf("Reading A2L done, milliseconds=%lld\n", t1-t0);
  fclose(fp);
  
  /* read the s19 file */
  if ( s19_file_name != NULL )
  {
	  /* read s19 file */
	  fp = fopen(s19_file_name, "rb");  // we need to read binary so that the CR/LF conversion is suppressed on windows
	  if ( fp == NULL )
		return perror(s19_file_name), coDelete(sw_object), 0;
	  if ( is_verbose ) printf("Reading S19 '%s'\n", s19_file_name);
	  t1 = getEpochMilliseconds();
	  coVectorAdd(sw_object, coReadS19ByFP(fp));
	  t2 = getEpochMilliseconds();
	  if ( is_verbose ) printf("Reading S19 done, milliseconds=%lld\n", t2-t1);
	  fclose(fp);	  
	  coVectorAdd(sw_object, coNewVectorByMap(coVectorGet(sw_object, DATA_MAP_POS)));	// build the sorted vector from the data map file
	  if ( is_verbose ) 
	  {
		  coPrint(coVectorGet(sw_object, DATA_VECTOR_POS));
		  puts("");
	  }
  }
  else
  {
	  coVectorAdd(sw_object, coNewMap(CO_NONE));	// add dummy entry
  }

  /* build the remaining index tables */
  if ( is_verbose ) printf("Building A2L index tables\n");
  t1 = getEpochMilliseconds();
  build_index_tables(sw_object, coVectorGet(sw_object, A2L_POS));
  t2 = getEpochMilliseconds();  
  if ( is_verbose ) printf("Building A2L index tables done, milliseconds=%lld\n", t2-t1);

  if ( is_verbose ) 
  {
	  printf("COMPU_METHOD cnt=%ld\n", coMapSize(coVectorGet(sw_object, COMPU_METHOD_MAP_POS)));
	  printf("COMPU_VTAB cnt=%ld\n", coMapSize(coVectorGet(sw_object, COMPU_VTAB_MAP_POS)));
	  printf("RECORD_LAYOUT cnt=%ld\n", coMapSize(coVectorGet(sw_object, RECORD_LAYOUT_MAP_POS)));
	  printf("CHARACTERISTIC Vector cnt=%ld\n", coVectorSize(coVectorGet(sw_object, CHARACTERISTIC_VECTOR_POS)));
	  printf("CHARACTERISTIC Name Map cnt=%ld\n", coMapSize(coVectorGet(sw_object, CHARACTERISTIC_NAME_MAP_POS)));
	  printf("AXIS_PTS Vector cnt=%ld\n", coVectorSize(coVectorGet(sw_object, AXIS_PTS_VECTOR_POS)));
	  printf("AXIS_PTS Name Map cnt=%ld\n", coMapSize(coVectorGet(sw_object, AXIS_PTS_NAME_MAP_POS)));
	  printf("Address Map cnt=%ld\n", coMapSize(coVectorGet(sw_object, ADDRESS_MAP_POS)));
  }
  if ( is_characteristic_address_list )
	showAddressList(sw_object);
  if ( is_ascii_characteristic_list )
	showAllASCIICharacteristic(sw_object);
  
  coDelete(sw_object);     // delete tables first, because they will refer to a2l
}
  
  