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

#define CO_A2L_IDENTIFIER_STRING_MAX (1024*8)

/*
	buf must point to a memory area of at least CO_A2L_IDENTIFIER_STRING_MAX bytes
*/
const char *coA2LGetString(coReader reader, char *buf)
{
  //static char buf[CO_A2L_STRBUF_MAX+8];                // reservere some extra space, because the overflow check is done only once in the for loop below
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
    if ( idx+8 >= CO_A2L_IDENTIFIER_STRING_MAX ) // check whether the buffer is full
      return coReaderErr(reader, "String too long"), NULL;
  } // for
  return buf;  
}

/*
	buf must point to a memory area of at least CO_A2L_IDENTIFIER_STRING_MAX bytes
*/
const char *coA2LGetIdentifier(coReader reader, int prefix, char *buf)
{
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
    if ( idx+2 >= CO_A2L_IDENTIFIER_STRING_MAX )
      return coReaderErr(reader, "Identifier too long"), NULL;
    buf[idx++] = c;
    coReaderNext(reader);
  }
  buf[idx] = '\0';
  coReaderSkipWhiteSpace(reader);          
  return buf;
}

/*
	buf must point to a memory area of at least CO_A2L_IDENTIFIER_STRING_MAX bytes
*/
const char *coA2LGetSlashPrefixedToken(coReader reader, char *buf)
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
  return coA2LGetIdentifier(reader, '/', buf);       // detect /begin and /end keywords
}

/*
	buf must point to a memory area of at least CO_A2L_IDENTIFIER_STRING_MAX bytes
*/
const char *coA2LGetToken(coReader reader, char *buf)
{
  const char *token;
  for(;;)
  {
    if ( coReaderCurr(reader) < 0 )
      return "";
    if ( coReaderCurr(reader) == '\"' )
      return coA2LGetString(reader, buf);
    if ( coReaderCurr(reader) == '/' )
    {
      token = coA2LGetSlashPrefixedToken(reader, buf);
      if ( token == NULL )
        return NULL;
      if ( token[0] == '\0' )
        continue;
      return token;
    }
    break;
  }
  return coA2LGetIdentifier(reader, -1, buf);
}

co coA2LGetArray(coReader reader, char *buf)
{
  const char *t;
  co array_obj;
  co element;
  static char zero[] = "0";
  static char one[] = "1";
  static char if_data[] = "IF_DATA";
  static char measurement[] = "MEASUREMENT";
  
  
  array_obj = coNewVector(CO_FREE_VALS);
  for(;;)
  {
    t = coA2LGetToken(reader, buf);
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
      element = coA2LGetArray(reader, buf);
      if ( element == NULL )
        return NULL;
      if ( coVectorAdd(array_obj, element) < 0 )
        return coReaderErr(reader, "Memory error inside 'array'"), coDelete(array_obj), NULL;
    }
    else if ( strcmp(t, "/end") == 0 )
    {
      coA2LGetToken(reader, buf);    // read next token. this should be the same as the /begin argumnet
      break;
    }
    else
    {
	  if ( t[0] == '0' || t[0] == '1' || t[0] == 'I' || t[0] == 'M' )
	  {
		// do a little bit of speed improvment, by avoiding the allocation of some very common strings
		if ( strcmp(t, zero) == 0 )
			element = coNewStr(CO_NONE, zero);   // use a static string to avoid malloc 
		else if ( strcmp(t, one) == 0 )
			element = coNewStr(CO_NONE, one);   // use a static string to avoid malloc 
		else if ( strcmp(t, if_data) == 0 )
			element = coNewStr(CO_NONE, if_data);   // use a static string to avoid malloc 
		else if ( strcmp(t, measurement) == 0 )
			element = coNewStr(CO_NONE, measurement);   // use a static string to avoid malloc 
		else
			element = coNewStr(CO_STRDUP, t);   // t is a pointer into constant memory, so do a strdup 
	  }
	  else
	  {
		element = coNewStr(CO_STRDUP, t);   // t is a pointer into constant memory, so do a strdup 
	  }
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
  char buf[CO_A2L_IDENTIFIER_STRING_MAX];
  
  if ( coReaderInitByString(&reader, json) == 0 )
    return NULL;
  return coA2LGetArray(&reader, buf);
}

co coReadA2LByFP(FILE *fp)
{
  struct co_reader_struct reader;
  char buf[CO_A2L_IDENTIFIER_STRING_MAX];
  
  if ( coReaderInitByFP(&reader, fp) == 0 )
    return NULL;
  return coA2LGetArray(&reader, buf);
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
  char buf[S19_MAX_LINE_LEN];
  unsigned char mem[S19_MAX_LINE_LEN/2];
  char addr_as_hex[10];
  char *line;
  size_t last_address = 0xffffffff; 
  size_t address; 
  size_t byte_cnt; 
  size_t mem_cnt;
  int rec_type;
  int i, c;
  struct co_reader_struct reader_struct;
  coReader r = &reader_struct;
  coReaderInitByFP(r, fp);
  
  co mo = NULL;                // memory object
  co map = coNewMap(CO_FREE_VALS|CO_STRDUP);
  if ( map == NULL )
    return NULL;
  for(;;)
  {
	i = 0;
	for(;;)
	{
		// c = getc(fp);
		c = coReaderCurr(r);
		coReaderNext(r);
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
		  mem_cnt = 0;
		  if ( rec_type == '1' && byte_cnt >= 3 )
		  {
			hexToMem(line+8, byte_cnt-2, mem);     // do not read address, but include checksum
			mem_cnt = byte_cnt - 3;
		  }
		  else if ( rec_type == '2'  && byte_cnt >= 4  )
		  {
			address <<= 8;
			address += (size_t)hexToUnsigned(line+8);
			hexToMem(line+10, byte_cnt-3, mem);     // do not read address, but include checksum
			mem_cnt = byte_cnt - 4;
		  }
		  else if ( rec_type == '3'  && byte_cnt >= 5  )
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

/*===================================================================*/
/* HEX Reader (https://de.wikipedia.org/wiki/Intel_HEX) */
/*===================================================================*/

#define HEX_MAX_LINE_LEN 1024


co coReadHEXByFP(FILE *fp)
{
  char buf[HEX_MAX_LINE_LEN];
  unsigned char mem[HEX_MAX_LINE_LEN/2];
  char addr_as_hex[10];
  char *line;
  size_t rec_address = 0x0;
  size_t seg_address = 0x0;
  size_t last_address = 0xffffffff; 
  size_t address = 0x0; 
  size_t mem_cnt;
  int rec_type;
  int i, c;
  struct co_reader_struct reader_struct;
  
  coReader r = &reader_struct;
  coReaderInitByFP(r, fp);
  
  co mo = NULL;                // memory object
  co map = coNewMap(CO_FREE_VALS|CO_STRDUP);
  if ( map == NULL )
    return NULL;
  for(;;)
  {
    i = 0;
    for(;;)
    {
        c = coReaderCurr(r);
        coReaderNext(r);
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
        
        if ( c == '\n' || c == '\r' )
        {
                line = buf;
                buf[i] = '\0';	
                break;
        }
        if ( i >= HEX_MAX_LINE_LEN )
          return puts("illegal line length in hex file"), coDelete(map), NULL;		
        if ( c > ' ' )
          buf[i++] = c;
        else
          i++;
    }
	
	//printf("line %s, i=%d\n", line, i);
	
    if ( line == NULL )
      break;
  
    while( *line != ':' && *line != '\0')
      line++;

    if ( *line == '\0' )
      continue;
    
	if ( *line == ':' )
		line++;
    
    mem_cnt = hexToUnsigned(line);
    if ( (mem_cnt+5)*2+1 > i )         // +1 because of the ":"
		  return puts("count mismatch in hex file"), coDelete(map), NULL;		
    //if ( byte_cnt > 255 )
    //  return puts("wrong byte_cnt"), coDelete(map), NULL;
    hexToMem(line, (mem_cnt+5), mem);     // simply read all, including byte cnt and checksum

    rec_address = mem[1]*256+mem[2];    
    rec_type = mem[3];
    if ( rec_type == 0 )
    {
      address = seg_address + rec_address;
	  //address = rec_address;
      sprintf(addr_as_hex, "%08zX", address);
	  //printf("%lld %llu %s\n", seg_address, address, addr_as_hex);
      if ( mo == NULL || last_address != address )
      {
            mo = coNewMem();                // create a new memory block
            if ( coMemAdd(mo, mem+4, mem_cnt) == 0 )
              return coDelete(map), NULL;
            if ( coMapAdd(map, addr_as_hex, mo) == 0 )
              return coDelete(map), NULL;
      }
      else
      {
            if ( coMemAdd(mo, mem+4, mem_cnt) == 0 )     // extend the existing memory block
              return coDelete(map), NULL;
      }
      last_address = address + mem_cnt;
    }
    else if ( rec_type == 1 )
    {
	  //printf("rec_type %d, EOF\n", rec_type);
      return map;
    }
    else if ( rec_type == 2 )
    {
	  //printf("rec_type %d, seg_address=%lx\n", rec_type, (unsigned long)seg_address);
      seg_address = ((size_t)mem[4]*256+(size_t)mem[5])*16;
      return map;
    }
    else if ( rec_type == 3 )
    {
	  //printf("rec_type %d, ignored\n", rec_type);
      // start address ignored
    }
    else if ( rec_type == 4 )
    {
	  //printf("rec_type %d, seg_address=%lx\n", rec_type, (unsigned long)seg_address);
      seg_address = ((size_t)mem[4]*256+(size_t)mem[5])<<16;
    }
    else if ( rec_type == 5 )
    {
	  //printf("rec_type %d, ignored\n", rec_type);
      // start address ignored
    }
  } // for(;;)
  return map;
}


/*===================================================================*/
/* CSV Reader, https://www.rfc-editor.org/rfc/rfc4180 */
/*===================================================================*/

#define CO_CSV_FIELD_STRING_MAX (8*1024)

co coGetCSVField(struct co_reader_struct *r, int separator, char *buf)
{
	size_t idx = 0;
	int isQuote = 0;
	int c;
	
	c = coReaderCurr(r);
	if ( c < 0 )
		return NULL;
	if ( c == '\"' )
	{
		isQuote = 1;
		coReaderNext(r);
		c = coReaderCurr(r);
	}
	for(;;)
	{
		if ( c < 0 )
		{
			buf[idx] = '\0';
			return coNewStr(CO_STRDUP, buf);
		}
		else if ( c == '\"' )
		{
			coReaderNext(r);
			c = coReaderCurr(r);
			if ( isQuote )
			{
				if ( c == '\"' )	// double double quote == escaped double quote
				{
					// storing is done at the end of the for-loop body 
					// buf[idx++] = '\"';	// store double quote and continue
				}
				else // end of field, lets look for the separator
				{
					for(;;)
					{
						if ( c < 0 )
						{
							buf[idx] = '\0';
							return coNewStr(CO_STRDUP, buf);
						}
						if ( c == separator ) 
						{
							buf[idx] = '\0';
							return coNewStr(CO_STRDUP, buf);	// separator will be handled by calling function
						}
						if ( c == '\n' || c == '\r' ) // let the calling function handle  \n and \r
						{
							buf[idx] = '\0';
							return coNewStr(CO_STRDUP, buf);
						}
						coReaderNext(r);
						c = coReaderCurr(r);
					}
					/* we will never come here */
				}
			}
			else
			{
				// double quote was found, but we are not in double quote mode, so just store the double quote
				// storing is done at the end of the for-loop body 
				//buf[idx++] = '\"';	// store double quote and continue
			}
		}
		else if ( c == separator )
		{
			if ( isQuote )
			{
				// we are inside double quotes, so just store the separator (which is done at the end of the for loop)
			}
			else
			{
				// end of field found... separator will be handled by calling function
				buf[idx] = '\0';
				return coNewStr(CO_STRDUP, buf);
			}
		}
		else if ( c == '\n' || c == '\r' ) 
		{
			if ( isQuote )
			{
				// CR/LF inside double quotes: just continue and store the CRLR sequence
			}
			else
			{	// let the calling function handle  \n and \r
				buf[idx] = '\0';
				return coNewStr(CO_STRDUP, buf);
			}
		}

		buf[idx++] = c;		
		coReaderNext(r);
		c = coReaderCurr(r);
	}
	/* we will never reach this statement */
	return NULL; 	
}


co coGetCSVRow(struct co_reader_struct *r, int separator, char *buf)
{
	co rowVector = coNewVector(CO_FREE_VALS);
	co field;
	
	//puts("coGetCSVRow");

	for(;;)
	{
		field = coGetCSVField(r, separator, buf);
		if ( field == NULL )
		{
			if ( coVectorSize(rowVector) == 0 ) 
			{
				coDelete(rowVector);
				return NULL;
			}
			break;
		}
		
		//printf("Field %s\n", coStrGet(field));
		if ( coVectorAdd(rowVector, field) < 0 )
		{
			coDelete(field);
			coDelete(rowVector);
			return NULL;
		}
		
		if ( coReaderCurr(r) == '\n' )
		{
			coReaderNext(r);
			if ( coReaderCurr(r) == '\r' )
				coReaderNext(r);
			break;
		}
		
		if ( coReaderCurr(r) == '\r' )
		{
			coReaderNext(r);
			if ( coReaderCurr(r) == '\n' )
				coReaderNext(r);
			break;
		}
		
		if ( coReaderCurr(r) == separator )
		{
			coReaderNext(r);
			// handle the special case, where the separtor is the last char of the file.
			// in such a case, add an empty string to the row vector
			if ( coReaderCurr(r) < 0 )
			{
				field = coNewStr(CO_STRDUP, "");
				if ( coVectorAdd(rowVector, field) < 0 )
				{
					coDelete(field);
					coDelete(rowVector);
					return NULL;
				}
				return rowVector;
			}
		}
	}
	return rowVector;
}

co coGetCSVFile(struct co_reader_struct *reader, int separator, char *buf)
{
	co fileVector = coNewVector(CO_FREE_VALS);
	co rowVector;

	//puts("coGetCSVFile");

	coReaderSkipWhiteSpace(reader);

	for(;;)
	{
		rowVector = coGetCSVRow(reader, separator, buf);
		if ( rowVector == NULL )
			break;
		if ( coVectorAdd(fileVector, rowVector) < 0 )
		{
			coDelete(rowVector);
			coDelete(fileVector);
			return NULL;
		}
		
	}
	return fileVector;
}


co coReadCSVByFP(FILE *fp, int separator)
{
  struct co_reader_struct reader;
  char buf[CO_CSV_FIELD_STRING_MAX];
  
  if ( coReaderInitByFP(&reader, fp) == 0 )
    return NULL;
  return coGetCSVFile(&reader, separator, buf);
}
