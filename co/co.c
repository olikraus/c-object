/*

  co.c

  C Object Library https://github.com/olikraus/c-object

  CC BY-SA 3.0  https://creativecommons.org/licenses/by-sa/3.0/

*/
#include "co.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>


/*=== Generic Functions ===*/

void coPrint(cco o)
{
  if ( o != NULL )
    o->fn->print(o);
}

cco coGetByIdx(co o, size_t idx)
{
  return o->fn->getByIdx(o, idx);
}

void coDelete(co o)
{
  o->fn->destroy(o);
  free(o);
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
int cobAdd(co o, co p);
size_t cobSize(co o);
cco cobGetByIdx(co o, size_t idx);
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
  return coNew(coBlankType, CO_FREE);
}

int cobInit(co o, void *data)
{
  o->fn = coBlankType;
  return 1;
}

size_t cobSize(co o)
{
  return 0;
}

cco cobGetByIdx(co o, size_t idx)
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

int covInit(co o, void *data);  // data: not used
ssize_t covAdd(co o, co p);
size_t covSize(co o);
cco covGet(co o, size_t idx);
const char *covToString(co o);
int covForEach(cco o, covForEachCB cb, void *data);
void covPrint(cco o);
static void covDestroy(co o);
co covMap(cco o, covMapCB cb, void *data);
co covClone(cco o);

struct coFnStruct covStruct = 
{
  covInit,
  covSize,
  covGet,
  covToString,
  covPrint,
  covDestroy,
  covClone
};
coFn coVectorType = &covStruct;

co coNewVector(unsigned flags)
{
  return coNew(coVectorType, flags);
}


#define COV_EXTEND 32
int covInit(co o, void *data)
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
ssize_t covAdd(co o, co p)
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

size_t covSize(co o)
{
  return o->v.cnt;
}

cco covGet(co o, size_t idx)
{
    if ( idx >= o->v.cnt )
      return NULL;
    return o->v.list[idx];
}

const char *covToString(co o)           // todo
{
  return "";
}

/* returns 1 (success) or 0 (no success) */
int covForEach(cco o, covForEachCB cb, void *data)
{
  size_t i;
  size_t cnt = o->v.cnt;        // not sure what todo if v.cnt is modified...
  for( i = 0; i < cnt; i++ )
    if ( cb(o, i, o->v.list[i], data) == 0 )
      return 0;
  return 1;
}

static int covPrintCB(cco o, size_t i, cco e, void *data)
{
  if ( i > 0 )
    printf(", ");
  e->fn->print(e);
  return 1;
}
void covPrint(cco o)
{
    printf("[");
    covForEach(o, covPrintCB, NULL);
    printf("]");
}

static int covDestroyCB(cco o, size_t i, cco e, void *data)
{
  coDelete((co)e);              // it is ok to discard the const keyword, really delete here
  return 1;
}

static void covDestroy(co o)
{
  covForEach(o, covDestroyCB, NULL);
  free(o->v.list);
  o->v.list = NULL;
  o->v.max = 0;
  o->v.cnt = 0;    
}

void covClear(co o)
{
  covForEach(o, covDestroyCB, NULL);
  o->v.cnt = 0;  
}

co covMap(cco o, covMapCB cb, void *data)
{
  co v = coNewVector(CO_FREE_VALS);
  co e;
  size_t i;
  for( i = 0; i < o->v.cnt; i++ )
  {
    e = cb(o, i, o->v.list[i], data);
    if ( e == NULL ) // memory error?
    {
        covDestroy(v);
        return NULL;      
    }
      
    if ( covAdd(v, e) == 0 )  // memory error?
    {
      covDestroy(v);
      return NULL;
    }
      
  }
  return v;
}

co covCloneCB(cco o, size_t i, cco e, void *data)
{
    return coClone(e);
}

co covClone(cco o)
{
  return covMap(o, covCloneCB, NULL);
}



/*--------------------------*/
/* special vector functions */

/* delete an element in the vector. Size is reduced  by 1 */
void covErase(co v, size_t i)
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

static int covAppendVectorCB(cco o, size_t idx, cco element, void *data)
{
  co c = coClone(element);
  if ( c == NULL )
    return 0;
  if ( covAdd( (co)data, c ) == 0 )
    return 0;
  return 1;
}

/* 
  append elements from src to v 
  src can be vector or map
  elements from src are cloned
*/
int covAppendVector(co v, cco src)
{
  if ( v->fn == coVectorType )
  {
    size_t oldCnt = v->v.cnt;
    
    if ( src->fn == coVectorType )
    {
      if ( covForEach(src, covAppendVectorCB, v) == 0 )
      {
        size_t i = v->v.cnt;
        while( i > oldCnt )
        {
          i--;
          covErase(v, i);
        }
      }
      return 1;
    }
  }
  return 0;     // first / seconad arg is NOT a vector
}

/*=== String ===*/

int cosInit(co o, void *data);  // optional data: const char *
int cosAdd(co o, co p);
size_t cosSize(co o);
cco cosGetByIdx(co o, size_t idx);
const char *cosToString(co o);
void cosPrint(cco o);
void cosDestroy(co o);
co cosClone(cco o);

struct coFnStruct cosStruct = 
{
  cosInit,
  cosSize,
  cosGetByIdx,
  cosToString,
  cosPrint,
  cosDestroy,
  cosClone
};
coFn coStrType = &cosStruct;

co coNewStr(unsigned flags, const char *s)
{
  co o = coNewWithData(coStrType, flags, (void *)s);
  if ( o == NULL )
    return NULL;
  return o;
}

int cosInit(co o, void *data)
{
  static char empty_string[2] = "";
  o->fn = coStrType;
  if ( data == NULL )
    o->s.str = empty_string;
  else
    o->s.str = (char *)data;
  return 1;
}

int cosAdd(co o, co p)
{
  return 0;     // not allowed
}

size_t cosSize(co o)
{
  return strlen(o->s.str);
}

cco cosGetByIdx(co o, size_t idx)
{
  return NULL;
}

const char *cosToString(co o)
{
  return o->s.str;
}

void cosPrint(cco o)
{
  printf("%s", o->s.str);
}

void cosDestroy(co o)
{
  if ( o->flags & CO_FREE_VALS )
    free(o->s.str);
  o->s.str = NULL;
}

co cosClone(cco o)
{
  return coNewStr(o->flags | CO_FREE_VALS, strdup(o->s.str)); 
}

/*=== Map ===*/
/* based on https://rosettacode.org/wiki/AVL_tree/C */

struct co_avl_node_struct avl_dummy = { NULL, NULL, {&avl_dummy, &avl_dummy}, 0 };
struct co_avl_node_struct *avl_nnil = &avl_dummy; // internally, avl_nnil is the new nul

typedef void (*avl_free_fn)(void *p);
typedef int (*avl_visit_fn)(size_t idx, struct co_avl_node_struct *n, void *data);

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

static int avl_for_each(struct co_avl_node_struct *n, avl_visit_fn visitCB, size_t *idx, void *data)
{
  if ( n == avl_nnil) 
    return 1;
  avl_for_each(n->kid[0], visitCB, idx, data);
  if ( visitCB(*idx, n, data) == 0 )
    return 0;
  (*idx)++;
  avl_for_each(n->kid[1], visitCB, idx, data);  
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

static int avl_get_size_cb(size_t idx, struct co_avl_node_struct *n, void *data)
{
  return 1;
}

/* warning: this has O(n) runtime */
static size_t avl_get_size(struct co_avl_node_struct *n)
{
  size_t cnt = 0;
  avl_for_each(n, avl_get_size_cb, &cnt, NULL);
  return cnt;
}



int comInit(co o, void *data);
size_t comSize(co o);
cco comGetByIdx(co o, size_t idx);
const char *comToString(co o);
void comPrint(cco o);
void comClear(co o);
co comClone(cco o);

struct coFnStruct comStruct = 
{
  comInit,
  comSize,
  comGetByIdx,
  comToString,
  comPrint,
  comClear,             // destroy function is identical to clear function
  comClone
};
coFn coMapType = &comStruct;

co coNewMap(unsigned flags)
{
  return coNew(coMapType, flags);
}


int comInit(co o, void *data)
{
    o->m.root = avl_nnil;
    return 1;
}

int comAdd(co o, const char *key, co value)
{
  avl_insert(
    &(o->m.root), 
    key,
    (void *)value,
    (o->flags & CO_FREE_KEYS)?avl_free_key:avl_keep_key, 
    (o->flags & CO_FREE_VALS)?avl_free_value:avl_keep_value
  );  
  
  return 0;     
}

size_t comSize(co o)
{
  return avl_get_size(o->m.root);              // O(n) !
}

cco comGetByIdx(co o, size_t idx)
{
  return NULL;
}

const char *comToString(co o)
{
  return "";
}

static int avl_com_print_cb(size_t idx, struct co_avl_node_struct *n, void *data)
{
  if ( idx > 0 )
    printf(", ");
  printf("%s:", n->key);
  coPrint((cco)(n->value));
  return 1;
}

void comPrint(cco o)
{
  size_t cnt = 0;
  printf("{");
  avl_for_each(o->m.root, avl_com_print_cb, &cnt, NULL);
  printf("}\n");  
}

void comClear(co o)
{
  avl_delete_all(
    &(o->m.root), 
    (o->flags & CO_FREE_KEYS)?avl_free_key:avl_keep_key, 
    (o->flags & CO_FREE_VALS)?avl_free_value:avl_keep_value
  );  
}

static int avl_com_clone_cb(size_t idx, struct co_avl_node_struct *n, void *data)
{
  return comAdd((co)data, strdup(n->key), coClone((co)(n->value)));
}

co comClone(cco o)
{
  size_t cnt = 0;
  co new_obj = coNewMap(o->flags | CO_FREE_VALS | CO_FREE_KEYS);
  if ( avl_for_each(o->m.root, avl_com_clone_cb, &cnt, new_obj) == 0 )
  {
    coDelete(new_obj);
    return NULL;
  }
  return new_obj;
}

/* return the value for a given key */
cco comGet(cco o, const char *key)
{
  struct co_avl_node_struct *n = avl_query(o->m.root, key);
  if ( n == NULL )
    return NULL;
  return (cco)(n->value);  
}

void comErase(co o, const char *key)
{
  avl_delete(
    &(o->m.root), 
    key,
    (o->flags & CO_FREE_KEYS)?avl_free_key:avl_keep_key, 
    (o->flags & CO_FREE_VALS)?avl_free_value:avl_keep_value
  );    
}

