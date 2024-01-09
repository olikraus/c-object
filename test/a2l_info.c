/* 
	a2l_info

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
#define CHARACTERISTIC_VECTOR_POS 3
#define CHARACTERISTIC_MAP_POS 4


#define A2L_POS 5
//#define S19_POS 4
//#define S19_VECTOR_POS 5

/*
  Create empty vector for the global index tables.
  This will be filled by build_index_tables() function

      sw_object[0]: Map with all COMPU_METHOD records
      sw_object[1]: Map with all COMPU_VTAB records
      sw_object[1]: Map with all COMPU_VTAB records

  ./a2l_info -a2l example-a2l-file.a2l.gz -s19 example.s19

*/

const char *a2l_file_name = NULL;

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
  coVectorAdd(o, coNewVector(CO_NONE));          // references to CHARACTERISTIC
  coVectorAdd(o, coNewMap(CO_NONE));          // references to CHARACTERISTIC
  if ( coVectorSize(o) != 5 )
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
      coVectorAdd((co)coVectorGet(sw_object, CHARACTERISTIC_VECTOR_POS), a2l);

      coMapAdd((co)coVectorGet(sw_object, CHARACTERISTIC_MAP_POS), 
        coStrGet(coVectorGet(a2l, 1)), 
        a2l);
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

/*
	for a data array, some values in the record layout are stored multiple times.
    the multiplicator is the value how often such values are stored.
	
	this will check only for one AXIS_DESCR. Can there be more than one AXIS_DESCR??
*/
long getCharacteristicMeasurementAxisPTSDataMultiplicator(cco characteristic_rec)
{
	
	long matrix_dim_list[10] = { 0, 0, 0, 0, 0, 0, 0, 0, 0 };
	long matrix_dim_idx = 0;
	long matrix_product = 1;
	long number_array_size = 0;
	long max_axis_points = 0;
	long multiplier = 1;
	long i;
	long cnt = coVectorSize(characteristic_rec);
	cco o;
	i = 0;
	while( i < cnt )
	{
		o = coVectorGet(characteristic_rec, i);
		if ( coIsStr(o) )
		{
			if ( strcmp( coStrGet(o), "MATRIX_DIM" ) == 0 )
			{
				i++;
				while( matrix_dim_idx < 10 )
				{
					o = coVectorGet(characteristic_rec, i);
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
				o = coVectorGet(characteristic_rec, i);
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
			if ( strcmp(element, "AXIS_DESCR") == 0 )
			{
				max_axis_points = atol(coStrGet(coVectorGet(o, 4)));
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
		printf("matrix_dim_idx=%ld matrix_product=%ld - ", matrix_dim_idx, matrix_product);
	}
	if ( number_array_size > 0 )
	{
		multiplier *= number_array_size;
		printf("number_array_size=%ld - ", number_array_size);
	}
	if ( max_axis_points > 0 )
	{
		multiplier *= max_axis_points;
		printf("max_axis_points=%ld - ", max_axis_points);
	}
	
	return multiplier;
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


void help(void)
{
  puts("-h            This help text");
  puts("-a2l <file>   A2L File");
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
    else
    {
      argv++;
    }
  }
  return 1;
}

/* report a zlib or i/o error */
#ifdef OBSOLETE
void zerr(int ret)
{
    fputs("zpipe: ", stderr);
    switch (ret) {
    case Z_ERRNO:
        if (ferror(stdin))
            fputs("error reading stdin\n", stderr);
        if (ferror(stdout))
            fputs("error writing stdout\n", stderr);
        break;
    case Z_STREAM_ERROR:
        fputs("invalid compression level\n", stderr);
        break;
    case Z_DATA_ERROR:
        fputs("invalid or incomplete deflate data\n", stderr);
        break;
    case Z_MEM_ERROR:
        fputs("out of memory\n", stderr);
        break;
    case Z_VERSION_ERROR:
        fputs("zlib version mismatch!\n", stderr);
    }
}

int inf(FILE *source, FILE *dest)
{
    int ret;
    unsigned have;
    z_stream strm;
    char in[CHUNK];
    char out[CHUNK];
    /* allocate inflate state */
    strm.zalloc = Z_NULL;
    strm.zfree = Z_NULL;
    strm.opaque = Z_NULL;
    strm.avail_in = 0;
    strm.next_in = Z_NULL;
    //ret = inflateInit(&strm);
	ret = inflateInit2((&strm), 32+MAX_WBITS);
	printf("inflateInit ret=%d\n", ret);
    if (ret != Z_OK)
        return ret;
    /* decompress until deflate stream ends or end of file */
    do {
        strm.avail_in = fread(in, 1, CHUNK, source);
		printf("inf strm.avail_in=%d\n", strm.avail_in);
        if (ferror(source)) {
            (void)inflateEnd(&strm);
            return Z_ERRNO;
        }
        if (strm.avail_in == 0)
            break;
        strm.next_in = in;
        /* run inflate() on input until output buffer not full */
        do {
            strm.avail_out = CHUNK;
            strm.next_out = out;
            ret = inflate(&strm, Z_NO_FLUSH);
            assert(ret != Z_STREAM_ERROR);  /* state not clobbered */
            switch (ret) {
            case Z_NEED_DICT:
                ret = Z_DATA_ERROR;     /* and fall through */
            case Z_DATA_ERROR:
            case Z_MEM_ERROR:
                (void)inflateEnd(&strm);
                return ret;
            }
            have = CHUNK - strm.avail_out;
			printf("inf have=%d strm.avail_out=%d strm.avail_in=%d\n", have, strm.avail_out, strm.avail_in);
            if (fwrite(out, 1, have, dest) != have || ferror(dest)) {
                (void)inflateEnd(&strm);
                return Z_ERRNO;
            }
        } while (strm.avail_out == 0);
        /* done when inflate() says it's done */
    } while (ret != Z_STREAM_END);
    /* clean up and return */
    (void)inflateEnd(&strm);
    return ret == Z_STREAM_END ? Z_OK : Z_DATA_ERROR;
}
#endif
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
  printf("Reading A2L '%s'\n", a2l_file_name);
  
  //zerr(inf(fp, fopen("tmp.out", "wb"))); fseek(fp, 0, SEEK_SET);
 
  coVectorAdd(sw_object, coReadA2LByFP(fp));
  t1 = getEpochMilliseconds();
  printf("Reading A2L done, milliseconds=%lld\n", t1-t0);
  fclose(fp);

  /* build the sorted vector version of the s19 file */
  //coVectorAdd(sw_object, coNewVectorByMap(coVectorGet(sw_object, S19_POS)));

  /* build the remaining index tables */
  printf("Building A2L index tables\n");
  build_index_tables(sw_object, coVectorGet(sw_object, A2L_POS));
  t2 = getEpochMilliseconds();  
  printf("Building A2L index tables done, milliseconds=%lld\n", t2-t1);
  //coPrint(sw_object); puts("");


  printf("COMPU_METHOD cnt=%ld\n", coMapSize(coVectorGet(sw_object, COMPU_METHOD_MAP_POS)));
  printf("COMPU_VTAB cnt=%ld\n", coMapSize(coVectorGet(sw_object, COMPU_VTAB_MAP_POS)));
  printf("RECORD_LAYOUT cnt=%ld\n", coMapSize(coVectorGet(sw_object, RECORD_LAYOUT_MAP_POS)));
  printf("CHARACTERISTIC Vector cnt=%ld\n", coVectorSize(coVectorGet(sw_object, CHARACTERISTIC_VECTOR_POS)));
  printf("CHARACTERISTIC Map cnt=%ld\n", coMapSize(coVectorGet(sw_object, CHARACTERISTIC_MAP_POS)));
  
  //dfs_characteristic(a2l);
  showAllCharacteristic(sw_object);
  
  
  coDelete(sw_object);     // delete tables first, because they will refer to a2l
}
  
  