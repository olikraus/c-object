/*

  co.c

  C Object Library https://github.coMap/olikraus/c-object

  CC BY-SA 3.0  https://creativecoMapmons.org/licenses/by-sa/3.0/

*/
#include "co.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <assert.h>

/*=== Generic Functions ===*/

void coPrint(cco o)
{
  if ( o != NULL )
    o->fn->print(o);
}

cco coGetByIdx(co o, long idx)
{
  return o->fn->getByIdx(o, idx);
}

/* delete the object, this will also handle o==NULL */
void coDelete(co o)
{
  if ( o != NULL )
  {
    o->fn->destroy(o);
    free(o);
  }
}

co coClone(cco o)
{
  return o->fn->clone(o);
}

/*=== Helper Functions ===*/

co coNewWithData(coFn t, unsigned flags, void *data)
{
  co o = (co)malloc(sizeof(struct coStruct));
  if ( o == NULL )
    return NULL;
  o->fn = t;
  o->flags = flags;
  if ( t->init(o, data) == 0 )
    return free(o), NULL;
  return o;
}

co coNew(coFn t, unsigned flags)
{
  return coNewWithData(t, flags, NULL);
}


/*
const char *coToString(co o)
{
  return o->fn.toString(o);
}
*/


/*=== Dummy / Blank ===*/

int cobInit(co o, void *data);  // data: ignored
long cobSize(co o);
cco cobGetByIdx(co o, long idx);
const char *cobToString(co o);
void cobPrint(cco o);
void cobDestroy(co o);
co cobClone(cco o);


struct coFnStruct cobStruct = 
{
  cobInit,
  cobSize,
  cobGetByIdx,
  cobToString,
  cobPrint,
  cobDestroy,
  cobClone
};
coFn coBlankType = &cobStruct;

co coNewBlank()
{
  return coNew(coBlankType, 0);
}

int cobInit(co o, void *data)
{
  o->fn = coBlankType;
  return 1;
}

long cobSize(co o)
{
  return 0;
}

cco cobGetByIdx(co o, long idx)
{
  return 0;
}

const char *cobToString(co o)
{
  return "";
}

void cobPrint(cco o)
{
}

void cobDestroy(co o)
{
}

co cobClone(cco o)
{
  return coNew(o->fn, o->flags);
}


/*=== Vector ===*/

int coVectorInit(co o, void *data);  // data: not used
long coVectorAdd(co o, co p);   // returns the index of the added element or -1
long coVectorSize(co o);
cco coVectorGet(co o, long idx);
const char *coVectorToString(co o);
int coVectorForEach(cco o, coVectorForEachCB cb, void *data);
void coVectorPrint(cco o);
static void coVectorDestroy(co o);
co coVectorMap(cco o, coVectorMapCB cb, void *data);
co coVectorClone(cco o);

struct coFnStruct coVectorStruct = 
{
  coVectorInit,
  coVectorSize,
  coVectorGet,
  coVectorToString,
  coVectorPrint,
  coVectorDestroy,
  coVectorClone
};
coFn coVectorType = &coVectorStruct;

co coNewVector(unsigned flags)
{
  return coNew(coVectorType, flags);
}


#define COV_EXTEND 32
int coVectorInit(co o, void *data)
{
    void *ptr = malloc(COV_EXTEND*sizeof(struct coStruct));
    if ( ptr == NULL )
        return 0;
    o->fn = coVectorType;
    o->v.list = (co *)ptr;
    o->v.max = COV_EXTEND;
    o->v.cnt = 0;
    return 1;
}

/*
  p will be moved, so maybe a clone is required 
  returns -1 in case of memory error
  otherwise it returns the position where the element was added
*/
long coVectorAdd(co o, co p)
{
  void *ptr;
  while( o->v.max <= o->v.cnt )
  {
    ptr = realloc(o->v.list, (o->v.cnt+COV_EXTEND)*sizeof(co));
    if ( ptr == NULL )
        return -1;
    o->v.list = (co *)ptr;
    o->v.max += COV_EXTEND;
  }  
  o->v.list[o->v.cnt] = p;
  o->v.cnt++;
  return o->v.cnt-1;
}

long coVectorSize(co o)
{
  return o->v.cnt;
}

cco coVectorGet(co o, long idx)
{
    if ( idx >= o->v.cnt )
      return NULL;
    return o->v.list[idx];
}

const char *coVectorToString(co o)           // todo
{
  return "";
}

/* returns 1 (success) or 0 (no success) */
int coVectorForEach(cco o, coVectorForEachCB cb, void *data)
{
  long i;
  long cnt = o->v.cnt;        // not sure what todo if v.cnt is modified...
  for( i = 0; i < cnt; i++ )
    if ( cb(o, i, o->v.list[i], data) == 0 )
      return 0;
  return 1;
}

static int coVectorPrintCB(cco o, long i, cco e, void *data)
{
  if ( i > 0 )
    printf(", ");
  e->fn->print(e);
  return 1;
}
void coVectorPrint(cco o)
{
    printf("[");
    coVectorForEach(o, coVectorPrintCB, NULL);
    printf("]");
}

static int coVectorDestroyCB(cco o, long i, cco e, void *data)
{
  coDelete((co)e);              // it is ok to discard the const keyword, really delete here
  return 1;
}

static void coVectorDestroy(co o)
{
  coVectorForEach(o, coVectorDestroyCB, NULL);
  free(o->v.list);
  o->v.list = NULL;
  o->v.max = 0;
  o->v.cnt = 0;    
}

void coVectorClear(co o)
{
  coVectorForEach(o, coVectorDestroyCB, NULL);
  o->v.cnt = 0;  
}

int coVectorEmpty(cco o)
{
  if ( o->v.cnt == 0 )
    return 1;
  return 0;
}

co coVectorMap(cco o, coVectorMapCB cb, void *data)
{
  co v = coNewVector(CO_FREE_VALS);
  co e;
  long i;
  for( i = 0; i < o->v.cnt; i++ )
  {
    e = cb(o, i, o->v.list[i], data);
    if ( e == NULL ) // memory error?
    {
        coVectorDestroy(v);
        return NULL;      
    }
      
    if ( coVectorAdd(v, e) < 0 )  // memory error?
    {
      coVectorDestroy(v);
      return NULL;
    }
      
  }
  return v;
}

co coVectorCloneCB(cco o, long i, cco e, void *data)
{
    return coClone(e);
}

co coVectorClone(cco o)
{
  return coVectorMap(o, coVectorCloneCB, NULL);
}



/*--------------------------*/
/* special vector functions */

/* delete an element in the vector. Size is reduced  by 1 */
void coVectorErase(co v, long i)
{
  if ( i >= v->v.cnt )
    return;             // do nothing, index is outside the vecor  
  // note: v->cnt > 0 at this point  
  coDelete( v->v.list[i] );       // delete the element
  i++;
  while( i < v->v.cnt )
    v->v.list[i-1] = v->v.list[i];
  v->v.cnt--;
}

static int coVectorAppendVectorCB(cco o, long idx, cco element, void *data)
{
  co c = coClone(element);
  if ( c == NULL )
    return 0;
  if ( coVectorAdd( (co)data, c ) < 0 )
    return 0;
  return 1;
}

/* 
  append elements from src to v 
  src can be vector or map
  elements from src are cloned
*/
int coVectorAppendVector(co v, cco src)
{
  if ( v->fn == coVectorType )
  {
    long oldCnt = v->v.cnt;
    
    if ( src->fn == coVectorType )
    {
      if ( coVectorForEach(src, coVectorAppendVectorCB, v) == 0 )
      {
        long i = v->v.cnt;
        while( i > oldCnt )
        {
          i--;
          coVectorErase(v, i);
        }
      }
      return 1;
    }
  }
  return 0;     // first / seconad arg is NOT a vector
}

/*=== String ===*/

int coStrInit(co o, void *data);  // optional data: const char *
long coStrSize(co o);
cco coStrGetByIdx(co o, long idx);
const char *coStrToString(co o);
void coStrPrint(cco o);
void coStrDestroy(co o);
co coStrClone(cco o);

struct coFnStruct coStrStruct = 
{
  coStrInit,
  coStrSize,
  coStrGetByIdx,
  coStrToString,
  coStrPrint,
  coStrDestroy,
  coStrClone
};
coFn coStrType = &coStrStruct;

co coNewStr(unsigned flags, const char *s)
{
  co o = coNewWithData(coStrType, flags, (void *)s);
  if ( o == NULL )
    return NULL;
  return o;
}

int coStrInit(co o, void *data)
{
  static char empty_string[2] = "";
  char *s = (char *)data;
  o->fn = coStrType;
  if ( s == NULL )
    s = empty_string;
  if ( o->flags & CO_STRDUP )
  {
    o->s.str = strdup(s);
    o->flags |= CO_STRFREE;
  }
  else
    o->s.str = s;
  o->s.len = strlen(o->s.str);
  return 1;
}

int coStrAdd(co o, const char *s)
{
  assert(coIsStr(o));
  assert(o->s.str != NULL);
  if ( o->flags & CO_STRDUP )
  {
    size_t len = strlen(s);
    char *p = (char *)realloc(o->s.str, o->s.len + len +1);
    if ( p == NULL )
      return 0;
    o->s.str = p;
    strcpy(o->s.str + o->s.len, s);
    o->s.len += len;
    return 1;
  }
  return 0;     // static string, can not add another string
}


long coStrSize(co o)
{
  return (long)o->s.len;
}

cco coStrGetByIdx(co o, long idx)
{
  return NULL;
}

const char *coStrToString(co o)
{
  return o->s.str;
}

void coStrPrint(cco o)
{
  printf("%s", o->s.str);
}

void coStrDestroy(co o)
{
  if ( o->flags & CO_STRFREE )
    free(o->s.str);
  o->s.str = NULL;
}

co coStrClone(cco o)
{
  return coNewStr(o->flags | CO_STRDUP | CO_STRFREE, o->s.str); 
}

/*
  Close the string object and return a string pointer with the content.
  The return value points to allocated memory and must be freed 
*/
char *coStrDeleteAndGetAllocatedStringContent(co o)
{
  assert(coIsStr(o));
  char *s;
  if ( o->flags & CO_STRFREE )
    s = o->s.str;
  else
    s = strdup(o->s.str);
  o->flags &= ~CO_STRFREE;      // clear the strfree flag so that the memory is not released
  coDelete(o);           // close the object itself
  return s;     // and finally return the allocated string
}

/*=== Double ===*/

int coDblInit(co o, void *data);  // optional data: const double *
long coDblSize(co o);
cco coDblGetByIdx(co o, long idx);
const char *coDblToString(co o);
void coDblPrint(cco o);
void coDblDestroy(co o);
co coDblClone(cco o);

struct coFnStruct coDblStruct = 
{
  coDblInit,
  coDblSize,
  coDblGetByIdx,
  coDblToString,
  coDblPrint,
  coDblDestroy,
  coDblClone
};
coFn coDblType = &coDblStruct;

co coNewDbl(double n)
{
  co o = coNewWithData(coDblType, CO_NONE, (void *)&n);
  if ( o == NULL )
    return NULL;
  return o;
}

int coDblInit(co o, void *data)
{
  o->fn = coDblType;
  if ( data == NULL )
    o->d.n = 0.0;
  else
    o->d.n = *(double *)data;
  return 1;
}


long coDblSize(co o)
{
  return 1;
}

cco coDblGetByIdx(co o, long idx)
{
  return NULL;
}

const char *coDblToString(co o)
{
  return "";
}

void coDblPrint(cco o)
{
  printf("%lf", o->d.n);
}

void coDblDestroy(co o)
{
  o->d.n = 0.0;
}

co coDblClone(cco o)
{
  return coNewDbl(o->d.n); 
}

/*=== Map ===*/
/* based on https://rosettacode.org/wiki/AVL_tree/C */

static struct co_avl_node_struct avl_dummy = { NULL, NULL, {&avl_dummy, &avl_dummy}, 0 };
static struct co_avl_node_struct *avl_nnil = &avl_dummy; // internally, avl_nnil is the new nul

typedef void (*avl_free_fn)(void *p);

static void avl_free_value(void *value)
{
  coDelete((co)value);
}

static void avl_keep_value(void *value)
{
}


static void avl_free_key(void *key)
{
  free(key);
}

static void avl_keep_key(void *key)
{
}

static struct co_avl_node_struct *avl_new_node(const char *key, void *value)
{
  struct co_avl_node_struct *n = malloc(sizeof(struct co_avl_node_struct));
  if ( n == NULL )
    return avl_nnil;
  n->key = key;
  n->value = value;
  n->height = 1;
  n->kid[0] = avl_nnil;
  n->kid[1] = avl_nnil;
  return n;
}

static void avl_delete_node(struct co_avl_node_struct *n, avl_free_fn free_key, avl_free_fn free_value)
{
  avl_free_key((void *)(n->key));
  if ( n->value != NULL )
    avl_free_value(n->value);
  n->value = NULL;
  n->kid[0] = NULL;
  n->kid[1] = NULL;
  free(n);
}

static int avl_max(int a, int b) 
{ 
  return a > b ? a : b; 
}

static void avl_set_height(struct co_avl_node_struct *n) 
{
  n->height = 1 + avl_max(n->kid[0]->height, n->kid[1]->height);
}

static int avl_get_ballance_diff(struct co_avl_node_struct *n) 
{
  return n->kid[0]->height - n->kid[1]->height;
}

// rotate a subtree according to dir; if new root is nil, old root is freed
static struct co_avl_node_struct * avl_rotate(struct co_avl_node_struct **rootp, int dir, avl_free_fn free_key, avl_free_fn free_value)
{
  struct co_avl_node_struct *old_r = *rootp;
  struct co_avl_node_struct *new_r = old_r->kid[dir];

  *rootp = new_r;               // replace root with the selected child
  
  if (avl_nnil == *rootp)
  {
    avl_delete_node(old_r, free_key, free_value);
  }
  else 
  {
    old_r->kid[dir] = new_r->kid[!dir];
    avl_set_height(old_r);
    new_r->kid[!dir] = old_r;
  }
  return new_r;
}

static void avl_adjust_balance(struct co_avl_node_struct **rootp, avl_free_fn free_key, avl_free_fn free_value)
{
  struct co_avl_node_struct *root = *rootp;
  int b = avl_get_ballance_diff(root)/2;
  if (b != 0) 
  {
    int dir = (1 - b)/2;
    if (avl_get_ballance_diff(root->kid[dir]) == -b)
    {
      avl_rotate(&root->kid[dir], !dir, free_key, free_value);
    }
    root = avl_rotate(rootp, dir, free_key, free_value);
  }
  if (root != avl_nnil)
    avl_set_height(root);
}

// find the node that contains the given key; or returns NULL (which is a little bit inconsistent)
struct co_avl_node_struct *avl_query(struct co_avl_node_struct *root, const char *key)
{
  int c;
  if ( key == NULL )
    return NULL;
  
  if ( root == avl_nnil )
    return NULL;
  
  c = strcmp(key, root->key);
  if ( c == 0 )
    return root;
  return avl_query(root->kid[c > 0], key);
}

/* returns 0 for any allocation error */
static int avl_insert(struct co_avl_node_struct **rootp, const char *key, void *value, avl_free_fn free_key, avl_free_fn free_value)
{
  struct co_avl_node_struct *root = *rootp;
  int c;

  if ( key == NULL )
    return 0; // illegal key

  if (root == avl_nnil)
  {
    *rootp = avl_new_node(key, value);  // value is cloned within avl_new_node()
    if ( *rootp == avl_nnil )
      return 0;
    return 1;
  }
  
  c = strcmp(key, root->key);
  if ( c == 0 )
  {
    // key already exists: replace value
    free_key((void *)key);
    if ( root->value != NULL )
      free_value(root->value);
    root->value = value;
  }
  else
  {
    if ( avl_insert(&root->kid[c > 0], key, value, free_key, free_value) == 0 )
      return 0;
    avl_adjust_balance(rootp, free_key, free_value);
  }
  return 1;
}

static void avl_delete(struct co_avl_node_struct **rootp, const char *key, avl_free_fn free_key, avl_free_fn free_value)
{
  struct co_avl_node_struct *root = *rootp;
  if ( key == NULL )
    return; // illegal key
  if (root == avl_nnil) 
    return; // not found

  // if this is the node we want, rotate until off the tree
  if ( strcmp(key, root->key) == 0 )
  {
    root = avl_rotate(rootp, avl_get_ballance_diff(root) < 0, free_key, free_value);
    if (avl_nnil == root)
    {
      return;
    }
  }
  avl_delete(&root->kid[strcmp(key, root->key) > 0], key, free_key, free_value);
  avl_adjust_balance(rootp, free_key, free_value);
}

static int avl_for_each(cco o, struct co_avl_node_struct *n, coMapForEachCB visitCB, long *idx, void *data)
{
  if ( n == avl_nnil) 
    return 1;
  avl_for_each(o, n->kid[0], visitCB, idx, data);
  if ( visitCB(o, *idx, n->key, (cco)(n->value), data) == 0 )
    return 0;
  (*idx)++;
  avl_for_each(o, n->kid[1], visitCB, idx, data);  
  return 1;
}

/*
  after calling avl_delete_all the "n" argument is illegal and points to avl_nnil
*/
static void avl_delete_all(struct co_avl_node_struct **n, avl_free_fn free_key, avl_free_fn free_value)
{
  if ( *n == avl_nnil) 
    return;
  avl_delete_all(&((*n)->kid[0]), free_key, free_value);
  avl_delete_all(&((*n)->kid[1]), free_key, free_value);
  avl_delete_node(*n, free_key, free_value);
  *n = avl_nnil;
}

static int avl_get_size_cb(cco o, long idx, const char *key, cco value, void *data)
{
  return 1;
}

/* warning: this has O(n) runtime */
static long avl_get_size(struct co_avl_node_struct *n)
{
  long cnt = 0;
  avl_for_each(NULL, n, avl_get_size_cb, &cnt, NULL);
  return cnt;
}



int coMapInit(co o, void *data);
long coMapSize(co o);
cco coMapGetByIdx(co o, long idx);
const char *coMapToString(co o);
void coMapPrint(cco o);
void coMapClear(co o);
int coMapEmpty(cco o);
co coMapClone(cco o);

struct coFnStruct coMapStruct = 
{
  coMapInit,
  coMapSize,
  coMapGetByIdx,
  coMapToString,
  coMapPrint,
  coMapClear,             // destroy function is identical to clear function
  coMapClone
};
coFn coMapType = &coMapStruct;

co coNewMap(unsigned flags)
{
  return coNew(coMapType, flags);
}


int coMapInit(co o, void *data)
{
  assert(coIsMap(o));
  o->m.root = avl_nnil;
  
  if ( o->flags & CO_STRDUP )
    o->flags |= CO_STRFREE;
  
  return 1;
}

int coMapAdd(co o, const char *key, co value)
{
  const char *k;
  assert(coIsMap(o));
  assert(key != NULL);
  if ( o->flags & CO_STRDUP )
    k = strdup(key);
  else
    k = key;
  if ( k == NULL )
    return 0;
  
  return avl_insert(
    &(o->m.root), 
    k,
    (void *)value,
    (o->flags & CO_STRFREE)?avl_free_key:avl_keep_key, 
    (o->flags & CO_FREE_VALS)?avl_free_value:avl_keep_value
  );  
}

long coMapSize(co o)
{
  assert(coIsMap(o));
  return avl_get_size(o->m.root);              // O(n) !
}

cco coMapGetByIdx(co o, long idx)
{
  return NULL;
}

const char *coMapToString(co o)
{
  return "";
}

static int avl_co_map_print_cb(cco o, long idx, const char *key, cco value, void *data)
{
  if ( idx > 0 )
    printf(", ");
  printf("%s:", key);
  coPrint(value);
  return 1;
}

void coMapPrint(cco o)
{
  long cnt = 0;
  assert(coIsMap(o));
  printf("{");
  avl_for_each(o, o->m.root, avl_co_map_print_cb, &cnt, NULL);
  printf("}");  
}

void coMapClear(co o)
{
  assert(coIsMap(o));
  avl_delete_all(
    &(o->m.root), 
    (o->flags & CO_STRFREE)?avl_free_key:avl_keep_key, 
    (o->flags & CO_FREE_VALS)?avl_free_value:avl_keep_value
  );  
}

int coMapEmpty(cco o)
{
  assert(coIsMap(o));
  if ( o->m.root == avl_nnil )
    return 1;
  return 0;
}

static int avl_co_map_clone_cb(cco o, long idx, const char *key, cco value, void *data)
{
  return coMapAdd((co)data, strdup(key), coClone((co)(value)));
}

co coMapClone(cco o)
{
  long cnt = 0;
  co new_obj = coNewMap(o->flags | CO_FREE_VALS | CO_STRDUP | CO_STRFREE);
  assert(coIsMap(o));
  if ( avl_for_each(o, o->m.root, avl_co_map_clone_cb, &cnt, new_obj) == 0 )
  {
    coDelete(new_obj);
    return NULL;
  }
  return new_obj;
}

/* return the value for a given key */
cco coMapGet(cco o, const char *key)
{
  struct co_avl_node_struct *n = avl_query(o->m.root, key);
  assert(coIsMap(o));
  if ( n == NULL )
    return NULL;
  return (cco)(n->value);  
}

void coMapErase(co o, const char *key)
{
  assert(coIsMap(o));
  avl_delete(
    &(o->m.root), 
    key,
    (o->flags & CO_STRFREE)?avl_free_key:avl_keep_key, 
    (o->flags & CO_FREE_VALS)?avl_free_value:avl_keep_value
  );    
}

void coMapForEach(cco o, coMapForEachCB cb, void *data)
{
  long cnt = 0;
  assert(coIsMap(o));
  avl_for_each(o, o->m.root, cb, &cnt, data);
}



/*=== JSON Parser ===*/

typedef struct co_reader_struct *coReader;
typedef void (*coReaderNextFn)(coReader j);

struct co_reader_struct
{
  int curr;
  const char *json_string;
  FILE *fp;
  coReaderNextFn next_cb;
};

void coReaderErr(coReader j, const char *msg)
{
  printf("JSON Parser error '%s', current char='%c'\n", msg, j->curr);
}


void coReaderStringNext(coReader j)
{
  if ( j->curr < 0 )
    return;
  (j->json_string)++;
  j->curr = *(j->json_string);  
  if ( j->curr == '\0' )
    j->curr = -1;     // code below will check for <0
}

void coReaderFileNext(coReader j)
{
  if ( j->curr < 0 )
    return;
  j->curr = getc(j->fp); // may returen EOF, which is -1, but code below will check for <0 
}

#define coReaderNext(j) ((j)->next_cb(j))
#define coReaderCurr(j) ((j)->curr)

#define coReaderSkipWhiteSpace(j) \
  for(;;) \
  {                                     \
    if ( coReaderCurr(j) < 0 )        \
      break;                            \
    if ( coReaderCurr(j) > ' ' )             \
      break;                            \
    coReaderNext(j);                         \
  }                                     \


int coReaderInitByString(coReader reader, const char *s)
{
  if ( reader == NULL || s == NULL )
    return 0;
  reader->json_string = s;
  reader->fp = NULL;
  reader->next_cb = coReaderStringNext;  
  reader->curr = reader->json_string[0];
  if ( reader->curr == '\0' )
    reader->curr = -1;
  coReaderSkipWhiteSpace(reader);
  return 1;
}

int coReaderInitByFP(coReader reader, FILE *fp)
{
  if ( reader == NULL || fp == NULL )
    return 0;  
  reader->json_string = NULL;
  reader->fp = fp;
  reader->next_cb = coReaderFileNext;
  reader->curr = getc(fp);
  coReaderSkipWhiteSpace(reader);
  return 1;
}


co cojGetValue(coReader j);          // forward declaration


#define COJ_STR_BUF 1024
char *cojGetStr(coReader j)
{  
  static char buf[COJ_STR_BUF+16];      // extra data for UTF-8 sequence and \0
  char *s = NULL;             // upcoming return value (allocated string)
  size_t len = 0;               // len == strlen(s)
  size_t idx = 0;
  int c = 0;
  if ( coReaderCurr(j) != '\"' )
    return coReaderErr(j, "Internal error"), NULL;
  coReaderNext(j);   // skip initial double quote
  for(;;)
  {
    c = coReaderCurr(j);
    if ( c < 0 )        // unexpected end of stream
      return coReaderErr(j, "Unexpected end of string"), NULL;
    if ( c == '\"' )
      break;    // regular end
    if ( c == '\\' )
    {
      coReaderNext(j);   // skip back slash
      c = coReaderCurr(j);
      if ( c == 'u' )
      {
        unsigned long u = 0;
        int i;
        for( i = 0; i < 4; i++ )  // read the \uXXXX hex number
        {
          coReaderNext(j);   // skip 'u' or any of the hex chars
          c = coReaderCurr(j);
          if ( c < 0 )
          {
            return coReaderErr(j, "Unexpected end within \\uXXXX"), NULL;    // todo: we might need to free(s)            
          }
          if ( c >= '0' && c <= '9' ) { u <<= 4; u += c-'0'; continue; }
          if ( c >= 'a' && c <= 'f' ) { u <<= 4; u += c-'a'+10; continue; }
          if ( c >= 'A' && c <= 'F' ) { u <<= 4; u += c-'A'+10; continue; }
          // if we reach this point, then the \u arg is not a hex number
          return coReaderErr(j, "Not a hex number with \\uXXXX"), NULL;      // todo: we might need to free(s)            
        }
        coReaderNext(j);   // skip last hex char 
        // convert the code point to UTF-8
        if ( u < 0x80UL )
        { 
          buf[idx++] = u; 
        }
        else if ( u < 0x800UL ) 
        { 
          buf[idx++] = (0xC0 | (u >> 6)); 
          buf[idx++] = (0x80 | (u & 0x3F)); 
        }
        else if ( u < 0x10000UL )
        { 
          buf[idx++] = (0xE0 | (u >> 12)); 
          buf[idx++] = (0x80 | ((u >>  6) & 0x3F)); 
          buf[idx++] = (0x80 | (u & 0x3F));
        }
        else if ( u < 0x200000UL )  // hmmm... we will never arive here
        {
          buf[idx++] = (0xE0 | (u >> 18)); 
          buf[idx++] = (0x80 | ((u >>  12) & 0x3F));
          buf[idx++] = (0x80 | ((u >>  6) & 0x3F)); 
          buf[idx++] = (0x80 | (u & 0x3F));
        }
        else
        {
          return coReaderErr(j, "Internal error"), NULL;   // I don't think, that this code is reachable, todo: we might need to free(s)            
        }        
      } // slash u
      else if ( c == 'n' ) { coReaderNext(j); buf[idx++] = '\n'; }
      else if ( c == 't' ) { coReaderNext(j); buf[idx++] = '\t'; }
      else if ( c == 'b' ) { coReaderNext(j); buf[idx++] = '\b'; }
      else if ( c == 'f' ) { coReaderNext(j); buf[idx++] = '\f'; }
      else if ( c == 'r' ) { coReaderNext(j); buf[idx++] = '\r'; }
      else { coReaderNext(j); buf[idx++] = c; }     // treat escaped char as it is (this will handle both slashes ...
    } // escape
    else
    {
      coReaderNext(j); 
      buf[idx++] = c;   // handle normal char
    }
    // check whether we need to flush the buffer to the string object
    if ( idx > COJ_STR_BUF )
    {
      
      buf[idx] = '\0';      
      if ( s == NULL )
      {
        s = strdup(buf);
        len = idx;
        if ( s == NULL )
          return coReaderErr(j, "Memory error inside string parser"), NULL;        
      }
      else
      {
        char *t = (char *)realloc(s, len+idx+1);
        if ( t == NULL )
          return coReaderErr(j, "Memory error inside string parser"), free(s), NULL;   // memory error
        s = t;
        strcpy(s+len, buf);
        len += idx;
      }
      
      idx = 0;  // buf is stored in the string object: reset the buffer counter to 0
    } // handle buffer flash
  }
  coReaderNext(j);   // skip final double quote
  coReaderSkipWhiteSpace(j);
  buf[idx] = '\0';
  
  if ( s == NULL )
  {
    s = strdup(buf);
    len = idx;
    if ( s == NULL )
      return coReaderErr(j, "Memory error inside string parser"), NULL;        
  }
  else
  {
    char *t = (char *)realloc(s, len+idx+1);
    if ( t == NULL )
      return coReaderErr(j, "Memory error inside string parser"), free(s), NULL;   // memory error
    s = t;
    strcpy(s+len, buf);
    len += idx;
  }
  
  return s;
}

#define COJ_DBL_BUF 64
co cojGetDbl(coReader j)
{  
  char buf[COJ_DBL_BUF+2];
  int i = 0;
  int c;
  for(;;)
  {
    c = coReaderCurr(j);
    if ( (c >= '0' && c <= '9') || c == '-' || c == '+' || c == 'e' || c == 'E' || c == '.' )
    {
      if ( i < COJ_DBL_BUF )
        buf[i++] = c;
      coReaderNext(j);
    }
    else
    {
      break;
    }
  }
  coReaderSkipWhiteSpace(j);
  buf[i] = '\0';
  return coNewDbl(strtod(buf, NULL));
}

co cojGetArray(coReader j)
{
  int c;
  co array_obj;
  co element;
  if ( coReaderCurr(j) != '[' )
    return coReaderErr(j, "Internal error"), NULL;

  array_obj = coNewVector(CO_FREE_VALS);
  coReaderNext(j);
  coReaderSkipWhiteSpace(j);  
  for(;;)
  {
    c = coReaderCurr(j);
    if ( c == ']' )
      break;
    if ( c < 0 )
      return coReaderErr(j, "Missing ']'"), NULL;
    
    if ( coVectorEmpty(array_obj) == 0 )          // expect a ',' after the first element
      if ( c == ',' )
      {
        coReaderNext(j);
        coReaderSkipWhiteSpace(j);          
      }
    
    element = cojGetValue(j);
    
    if ( element == NULL )
      return NULL;
    if ( coVectorAdd(array_obj, element) < 0 )
      return coReaderErr(j, "Memory error inside 'array'"), coDelete(array_obj), NULL;
  }
  coReaderNext(j);   // skip ']'
  coReaderSkipWhiteSpace(j);
  return array_obj;
}

co cojGetMap(coReader j)
{
  int c;
  co map_obj;
  co element;
  char *key;
  if ( coReaderCurr(j) != '{' )
    return coReaderErr(j, "Internal error"), NULL;

  map_obj = coNewMap(CO_FREE_VALS|CO_STRFREE);  // do not duplicate keys, because they are already allocated
  if ( map_obj == NULL )
      return coReaderErr(j, "Memory error with map"), NULL;
    
  coReaderNext(j);   // skip '{'
  coReaderSkipWhiteSpace(j);  
  for(;;)
  {
    c = coReaderCurr(j);
    if ( c == '}' )
      break;
    if ( c < 0 )
      return coReaderErr(j, "Missing '}'"), NULL;
    
    if ( coMapEmpty(map_obj) == 0 )          // expect a ',' after the first key/value pair
      if ( c == ',' )
      {
        coReaderNext(j);
        coReaderSkipWhiteSpace(j);          
      }
    
    
    key = cojGetStr(j);         // key will contain a pointer to allocated memory
    if ( key == NULL )
      return coDelete(map_obj), NULL;
    coReaderSkipWhiteSpace(j);  
    if ( coReaderCurr(j) != ':' )
      return coReaderErr(j, "Missng ':'"), free(key), coDelete(map_obj), NULL;
    coReaderNext(j);
    coReaderSkipWhiteSpace(j);  
    
    element = cojGetValue(j);
    if ( element == NULL )
      return coReaderErr(j, "Memory error with map element"), free(key), coDelete(map_obj), NULL;
    if ( coMapAdd(map_obj, key, element) == 0 )
      return coReaderErr(j, "Memory error with map"), free(key), coDelete(map_obj), NULL;  // ToDo: do we need to close 'element' here?    
  }
  coReaderNext(j);   // skip '}'
  coReaderSkipWhiteSpace(j);
  return map_obj;
}

co cojGetValue(coReader j)
{
  int c = coReaderCurr(j);
  if ( c == '[' )
    return cojGetArray(j);
  if ( c == '{' )
    return cojGetMap(j);
  if ( c == '\"' )
    return coNewStr(CO_STRFREE, cojGetStr(j));  // return value of cojGetStr() is a pointer to allocated memory, so don't use CO_STRDUP
  if ( (c >= '0' && c <= '9') || c == '-' || c == '+' || c == 'e' || c == 'E' || c == '.' )
    return cojGetDbl(j);
  // todo: handle true, false, null
  return NULL;
}

co coReadJSONByString(const char *json)
{
  struct co_reader_struct reader;
  if ( coReaderInitByString(&reader, json) == 0 )
    return NULL;
  return cojGetValue(&reader);
}

co coReadJSONByFP(FILE *fp)
{
  struct co_reader_struct coReader;
  if ( fp == NULL )
    return NULL;  
  coReader.json_string = NULL;
  coReader.fp = fp;
  coReader.next_cb = coReaderFileNext;
  coReader.curr = getc(fp);
  coReaderSkipWhiteSpace(&coReader);
  return cojGetValue(&coReader);
}

