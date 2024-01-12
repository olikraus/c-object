/*

  co_extra.c

  C Object Library 
  (c) 2024 Oliver Kraus
  https://github.com/olikraus/c-object

  CC BY-SA 3.0  https://creativecommons.org/licenses/by-sa/3.0/
  
  Additionall function, which are not required for the core functionality

*/
#include "co.h"
#include <stdlib.h>
#include <string.h>
#include <assert.h>

/*===================================================================*/
/* A2L Parser */
/*===================================================================*/

#define CO_A2L_STRBUF_MAX 8192
const char *coA2LGetString(coReader reader)
{
  static char buf[CO_A2L_STRBUF_MAX+8];                // reservere some extra space, because the overflow check is done only once in the for loop below
  size_t idx = 0;
  int c = 0;
  if ( coReaderCurr(reader) != '\"' )
    return coReaderErr(reader, "Internal error"), NULL;
  buf[idx++] = '\"';            // add the initial double quote
  coReaderNext(reader);   // skip initial double quote
  for(;;)
  {
    c = coReaderCurr(reader);
    if ( c < 0 )        // unexpected end of stream
      return coReaderErr(reader, "Unexpected end of string"), NULL;
    if ( c == '\"' )
    {
      buf[idx++] = c;            // add the final double quote 
      buf[idx++] = '\0';            // terminate the string
      coReaderNext(reader);   // skip final double quote
      coReaderSkipWhiteSpace(reader);          
      break;    // regular end
    }
    if ( c == '\\' )
    {
      coReaderNext(reader);   // skip back slash
      c = coReaderCurr(reader);
      if ( c == 'n' ) { coReaderNext(reader); buf[idx++] = '\n'; }
      else if ( c == 't' ) { coReaderNext(reader); buf[idx++] = '\t'; }
      else if ( c == 'b' ) { coReaderNext(reader); buf[idx++] = '\b'; }
      else if ( c == 'f' ) { coReaderNext(reader); buf[idx++] = '\f'; }
      else if ( c == 'r' ) { coReaderNext(reader); buf[idx++] = '\r'; }
      else { coReaderNext(reader); buf[idx++] = c; }     // treat escaped char as it is (this will handle both slashes) ...
    } // escape
    else
    {
      coReaderNext(reader); 
      buf[idx++] = c;   // handle normal char
    }
    if ( idx >= CO_A2L_STRBUF_MAX ) // check whether the buffer is full
      return coReaderErr(reader, "String too long"), NULL;
  } // for
  return buf;  
}

#define CO_A2L_IDENTIFIER_MAX 1024
const char *coA2LGetIdentifier(coReader reader, int prefix)
{
  static char buf[CO_A2L_IDENTIFIER_MAX+2];  // add some space for '\0'
  int c;
  size_t idx = 0;
  if ( prefix >= 0 )    // this is used by the '/' detection procedure below
    buf[idx++] = prefix;
  for(;;)
  {
    c = coReaderCurr(reader);
    if ( c <= ' ' ) break; // this includes c==-1
    if ( c == '/' ) break;    // comment start
    if ( c == '\"' ) break; // string start
    if ( idx >= CO_A2L_IDENTIFIER_MAX )
      return coReaderErr(reader, "Identifier too long"), NULL;
    buf[idx++] = c;
    coReaderNext(reader);
  }
  buf[idx] = '\0';
  coReaderSkipWhiteSpace(reader);          
  return buf;
}

const char *coA2LGetSlashPrefixedToken(coReader reader)
{
  if ( coReaderCurr(reader) != '/' )
    return coReaderErr(reader, "Internal error"), NULL;
  coReaderNext(reader);   // skip '/'
  if ( coReaderCurr(reader) == '/' )   // line comment
  {
    coReaderNext(reader);   // skip second '/'
    for(;;)
    {
      if ( coReaderCurr(reader) < 0 ) return "";
      if ( coReaderCurr(reader) == '\n' || coReaderCurr(reader) == '\r' ) { coReaderSkipWhiteSpace(reader); return ""; }
      coReaderNext(reader);
    }
  }
  if ( coReaderCurr(reader) == '*' )   // block comment
  {
    coReaderNext(reader);   // skip '*'
    for(;;)
    {
      if ( coReaderCurr(reader) < 0 ) return "";
      if ( coReaderCurr(reader) == '*' )
      {
        coReaderNext(reader);
        if ( coReaderCurr(reader) == '/' ) { coReaderNext(reader); coReaderSkipWhiteSpace(reader); return ""; }
       }
      else coReaderNext(reader);
    }
  }
  return coA2LGetIdentifier(reader, '/');       // detect /begin and /end keywords
}

const char *coA2LGetToken(coReader reader)
{
  const char *token;
  for(;;)
  {
    if ( coReaderCurr(reader) < 0 )
      return "";
    if ( coReaderCurr(reader) == '\"' )
      return coA2LGetString(reader);
    if ( coReaderCurr(reader) == '/' )
    {
      token = coA2LGetSlashPrefixedToken(reader);
      if ( token == NULL )
        return NULL;
      if ( token[0] == '\0' )
        continue;
      return token;
    }
    break;
  }
  return coA2LGetIdentifier(reader, -1);
}

co coA2LGetArray(coReader reader)
{
  const char *t;
  co array_obj;
  co element;
  
  array_obj = coNewVector(CO_FREE_VALS);
  for(;;)
  {
    t = coA2LGetToken(reader);
    if ( coReaderCurr(reader) < 0 ) // end of file?
    {
      break;
    } 
    else if ( t == NULL )    // some error has happend
    {
        return coDelete(array_obj), NULL;
    }  
    else if ( strcmp(t, "/begin") == 0 )
    {
      element = coA2LGetArray(reader);
      if ( element == NULL )
        return NULL;
      if ( coVectorAdd(array_obj, element) < 0 )
        return coReaderErr(reader, "Memory error inside 'array'"), coDelete(array_obj), NULL;
    }
    else if ( strcmp(t, "/end") == 0 )
    {
      coA2LGetToken(reader);    // read next token. this should be the same as the /begin argumnet
      break;
    }
    else
    {
      element = coNewStr(CO_STRDUP, t);   // t is a pointer into constant memory, so do a strdup 
      if ( element == NULL )
        return NULL;
      if ( coVectorAdd(array_obj, element) < 0 )
        return coReaderErr(reader, "Memory error inside 'array'"), coDelete(array_obj), NULL;    
    }
  }
  return array_obj;
}

co coReadA2LByString(const char *json)
{
  struct co_reader_struct reader;
  if ( coReaderInitByString(&reader, json) == 0 )
    return NULL;
  return coA2LGetArray(&reader);
}

co coReadA2LByFP(FILE *fp)
{
  struct co_reader_struct reader;
  if ( coReaderInitByFP(&reader, fp) == 0 )
    return NULL;
  return coA2LGetArray(&reader);
}

/*===================================================================*/
/* S19 Reader */
/*===================================================================*/


#define S19_MAX_LINE_LEN 1024

static unsigned hexToUnsigned(const char *s)
{
  /* 
    A = 0x41 = 0100 0001
    a = 0x61 = 0110 0001
          0xdf  = 1101 1111
  */
  unsigned int n;
  n = (*s < 'A')?(*s-'0'):((*s&0xdf)-'A'+10);
  n *= 16;
  s++;
  n += (*s < 'A')?(*s-'0'):((*s&0xdf)-'A'+10);
  return n;
}

static void hexToMem(const char *s, size_t cnt, unsigned char*mem)
{
  while( cnt > 0 )
  {
    *mem++ = hexToUnsigned(s);
    s+=2;
    cnt--;
  }
}

co coReadS19ByFP(FILE *fp)
{
  static char buf[S19_MAX_LINE_LEN];
  static unsigned char mem[S19_MAX_LINE_LEN/2];
  char addr_as_hex[10];
  char *line;
  size_t last_address = 0xffffffff; 
  size_t address; 
  size_t byte_cnt; 
  size_t mem_cnt;
  int rec_type;
  int i, c;
  co mo = NULL;                // memory object
  co map = coNewMap(CO_FREE_VALS|CO_STRDUP);
  if ( map == NULL )
    return NULL;
  for(;;)
  {
	i = 0;
	for(;;)
	{
		c = getc(fp);
		if ( c < 0 )
		{
			if ( i == 0 )
				line = NULL;
			else	
			{
				line = buf;
				buf[i] = '\0';
			}
			break;
		}
		
		if ( c == '\n' )
		{
			line = buf;
			buf[i] = '\0';			
			break;
		}
		if ( i >= S19_MAX_LINE_LEN )
		  return puts("illegal line length in s19 file"), coDelete(map), NULL;		
		buf[i++] = c;
	}
	
    //line = fgets(buf, S19_MAX_LINE_LEN, fp);
    if ( line == NULL )
      break;
  
    while( *line != 'S' && *line != '\0')
      line++;
    rec_type = line[1];
	if ( rec_type >= '1' && rec_type <= '9' )
	{
		byte_cnt = hexToUnsigned(line+2);
		if ( byte_cnt > 255 )
		  return puts("wrong byte_cnt"), coDelete(map), NULL;		
		
		if ( rec_type >= '1' && rec_type <= '3' )
		{
		  address = (size_t)hexToUnsigned(line+4);
		  address <<= 8;
		  address += (size_t)hexToUnsigned(line+6);
		  if ( rec_type == '1' )
		  {
			hexToMem(line+8, byte_cnt-2, mem);     // do not read address, but include checksum
			mem_cnt = byte_cnt - 3;
		  }
		  else if ( rec_type == '2' )
		  {
			address <<= 8;
			address += (size_t)hexToUnsigned(line+8);
			hexToMem(line+10, byte_cnt-3, mem);     // do not read address, but include checksum
			mem_cnt = byte_cnt - 4;
		  }
		  else if ( rec_type == '3' )
		  {
			address <<= 8;
			address += (size_t)hexToUnsigned(line+8);
			address <<= 8;
			address += (size_t)hexToUnsigned(line+10);
			hexToMem(line+12, byte_cnt-4, mem);     // do not read address, but include checksum
			mem_cnt = byte_cnt - 5;
		  }      
		  sprintf(addr_as_hex, "%08zX", address);
		  if ( mo == NULL || last_address != address )
		  {
			mo = coNewMem();                // create a new memory block
			if ( coMemAdd(mo, mem, mem_cnt) == 0 )
			  return coDelete(map), NULL;
			if ( coMapAdd(map, addr_as_hex, mo) == 0 )
			  return coDelete(map), NULL;
		  }
		  else
		  {
			if ( coMemAdd(mo, mem, mem_cnt) == 0 )     // extend the existing memory block
			  return coDelete(map), NULL;
		  }
		  last_address = address + mem_cnt;
	   }
    }
  }
  return map;
}

