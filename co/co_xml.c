/*

  co_xml.c

  C Object Library
  (c) 2026 Oliver Kraus
  https://github.com/olikraus/c-object

  CC BY-SA 3.0  https://creativecommons.org/licenses/by-sa/3.0/

  XML Reader based on Expat
  
  This code assums standard UTF-8 strings (XML_UNICODE_WCHAR_T disabled)

*/
#include "co.h"
#include <assert.h>
#include <expat.h>


struct co_xml_data_struct {
  XML_Parser parser;
  //int depth;
  int is_error;
  int skip_white_space;
  co v;
};


long coVectorAddNoneNull(co v, co e) {
  long pos;
  if ( e == NULL )
    return -1;
  pos = coVectorAdd(v, e);
  if ( pos < 0 )
    return coDelete(e), -1L;
  return pos;
}

static void XMLCALL startElement(void *userData, const XML_Char *name, const XML_Char **atts) {
  struct co_xml_data_struct *cx = (struct co_xml_data_struct *)userData;
  int i;
  co v = coNewVector(CO_FREE_VALS);
  co m;
  
  if ( v == NULL ) {
    XML_StopParser(cx->parser, XML_FALSE);
    cx->is_error = 1;
    return;
  }
  
  if ( coVectorAddNoneNull(v, coNewStr(CO_STRDUP|CO_STRFREE, name)) < 0 ) {
    XML_StopParser(cx->parser, XML_FALSE);
    cx->is_error = 1;
    coDelete(v);
    return;
  }
  
  m = coNewMap(CO_FREE_VALS|CO_STRDUP|CO_STRFREE);
  if ( m == NULL ) {
    XML_StopParser(cx->parser, XML_FALSE);
    cx->is_error = 1;
    coDelete(v);
    return;
  }
  
  if ( coVectorAdd(v, m) < 0 ) {
    XML_StopParser(cx->parser, XML_FALSE);
    cx->is_error = 1;
    coDelete(m);
    coDelete(v);
    return;
  }

  for (i = 0; atts[i]; i += 2) {
    co s = coNewStr(CO_STRDUP|CO_STRFREE, atts[i + 1]);
      if ( s == NULL ) {
      XML_StopParser(cx->parser, XML_FALSE);
      cx->is_error = 1;
      coDelete(v);
      return;
    }
    if ( coMapAdd(m, atts[i], s) == NULL ) {
      XML_StopParser(cx->parser, XML_FALSE);
      cx->is_error = 1;
      coDelete(s);
      coDelete(v);
      return;
    }
  } // for

  if ( coVectorAdd(cx->v, v) < 0 ) // finally put the new vector on the stack
  {
    coDelete(v);
    XML_StopParser(cx->parser, XML_FALSE);
    cx->is_error = 1;
    return;
  }
  
}

static int isAllWhiteSpace(const char *s, int len) {
  int i;
  for( i = 0; i < len; i++ ) {
    if ( s[i] > 32 )
      return 0;
  }
  return 1;
}

static void dataHandler(void *userData, const char *s, int len) {
  struct co_xml_data_struct *cx = (struct co_xml_data_struct *)userData;
  long size = coVectorSize(cx->v);
  if ( size >= 1 ) {    // this should be the case
    co v = (co)coVectorGet(cx->v, size-1);
    if ( coVectorSize(v) > 0 ) {
      co last = (co)coVectorGet(v, coVectorSize(v)-1);
      if ( coIsStr(last) ) {
        if ( coStrAddWithLen(last, s, len) == 0 ) {
          XML_StopParser(cx->parser, XML_FALSE);
          cx->is_error = 1;
          return;
        }
      } // not a string --> must be the first string or another child
      else {
        if ( cx->skip_white_space != 0 )
          if ( isAllWhiteSpace(s, len) )  // skip if this is just some white space string
            return;
        co cos = coNewStrWithLen(s, len);
        if ( cos == NULL ) {
          XML_StopParser(cx->parser, XML_FALSE);
          cx->is_error = 1;
          return;
        }
        if ( coVectorAdd(v, cos) < 0 ) {
          coDelete(cos);
          XML_StopParser(cx->parser, XML_FALSE);
          cx->is_error = 1;
          return;
        }
      }
    }
  }
}

static void XMLCALL endElement(void *userData, const XML_Char *name) {
  struct co_xml_data_struct *cx = (struct co_xml_data_struct *)userData;
  long size = coVectorSize(cx->v);
  if ( size >= 2 ) {
    co top = (co)coVectorGet(cx->v, size-1);
    co parent = (co)coVectorGet(cx->v, size-2);
    if ( coVectorAdd(parent, top) < 0 ) {
      coDelete(top);
      XML_StopParser(cx->parser, XML_FALSE);
      cx->is_error = 1;
      coVectorEraseLast(cx->v);
      return;
    }
    else {
      coVectorEraseLast(cx->v);          // remove the last element
    }
  }
}



co coReadXML(coReader r, int skip_white_space) {
  XML_Parser parser = XML_ParserCreate(NULL);
  char *buf;
  size_t i;
  int done = 0;
  struct co_xml_data_struct cx; 
  co root;
  
  
  if ( parser == NULL )
    return NULL;                // memory error

  cx.parser = parser;           // for error signaling
  //cx.depth = 0;
  cx.is_error = 0;
  cx.skip_white_space = skip_white_space;
  cx.v = coNewVector(CO_NONE);
  
  if ( cx.v == NULL )
    return XML_ParserFree(parser), NULL;
  
  XML_SetUserData(parser, &cx);
  XML_SetElementHandler(parser, startElement, endElement);
  XML_SetCharacterDataHandler(parser, dataHandler);
  
  do {
    buf = (char *)XML_GetBuffer(parser, BUFSIZ);
    if (buf == NULL ) 
      return XML_ParserFree(parser), NULL;

    i = 0; 
    for(;;) {
      if ( coReaderCurr(r) < 0 ) {
        done = 1;
        break;
      }
      buf[i++] = coReaderCurr(r);
      coReaderNext(r);
      if ( i >= BUFSIZ )
        break;
    }
    
    if (XML_ParseBuffer(parser, (int)i, done) == XML_STATUS_ERROR) {
      printf("XML Parse error at line %lu:\n%s\n", (long)XML_GetCurrentLineNumber(parser), XML_ErrorString(XML_GetErrorCode(parser)));
      XML_ParserFree(parser);
      return NULL;
    }
  } while (done == 0);

  XML_ParserFree(parser);
  
  if ( cx.is_error != 0 ) {
    long i;
    for( i = 0; i < coVectorSize(cx.v); i++ )
      coDelete((co)coVectorGet(cx.v, i));   // delete the elements, because delete on vector will not do this
    coDelete(cx.v);    // remove the vector itself
    return NULL;
  }
  
  if ( coVectorSize(cx.v) == 0 )
    return coDelete(cx.v), NULL;                // XML contains no elements

  assert( coVectorSize(cx.v) == 1 );             // if everything went correct, then there must be only one root element
  
  root = (co)coVectorGet( cx.v, 0 );                     // get the root element
  coDelete(cx.v);    // elements of cx.v (which is the root element only) are not deleted 
  return root;          // return root, caller is responible for deletion
}


co coReadXMLByFP(FILE *fp, int skip_white_space) {
  struct co_reader_struct reader;

  if (coReaderInitByFP(&reader, fp) == 0)
    return NULL;
  return coReadXML(&reader, skip_white_space);
}

