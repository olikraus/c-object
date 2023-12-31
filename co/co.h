/*

  co.h
  
  C Object Library https://github.coMap/olikraus/c-object

  CC BY-SA 3.0  https://creativecoMapmons.org/licenses/by-sa/3.0/

  co    read/and writeable c-object
  cco   read only c-object
    
  Warning like "discards ‘const’ qualifier" indicate ERRORs!
  However the above warnings are not always generated, this means
    - if the warning is generated, then create a clone 
    - if the warning is not generated, then there still might be an isse (see examples below)
  

  objects assigned to containers are moved, this means, the source obj is destroyed
    (unless otherwise mentioned)
  objects taken out of a container are read only

  Example 1:
    co v = coNewVector();
    coAdd(v, coNewStr("abc"));  // valid, return value of coNewStr is deleted by v
  
  Example 2:
    co v = coNewVector();
    co s = coNewStr("abc");
    coAdd(v, s);               // valid, but s is invalid after this statement
    coAdd(v, s);                // invalid, because s is already illegal

  Example 3:
    co v = coNewVector();
    co s = coNewStr("abc");
    coAdd(v, s);               // valid, but s is invalid after this statement
    coDelete(s);                // invalid, because s will be deleted by v again
    
  Example 4:
    co v = coNewVector();
    co s; 
    for( i = 0; i < 10; i++ )
    {
      s = coNewStr("abc");  // valid, s is illegal and will be overwritten
      coAdd(v, s);               // valid, but s is invalid after this statement
    }

  Example 5:
    coAdd(v, coGetByIdx(v, 0)); // invalid & will generate a warning.. use clone instead



  Hint:
  gcc -Wall -fsanitize=address -fsanitize=undefined -fsanitize=leak *.c 
  gcc -Wall -fsanitize=address -I./co ./co/co.c ./test/co_test.c && ./a.out

  Copy operation from somewhere into a container
    1) Move: The remote element becoMapes obsolete and becoMapes part of the container
      The moved element will be deleted together with the container
      The moved element must not be referenced any more somewhere else
    2) Reference: The element in the container is just a reference to the remote element.
      The element of the container is not deleted.
      Risk: If the remote element becoMapes deleted, then the element in the 
      container is also invalid
    3) Clone: Usually a manual operation, where the element is cloned and
      then stored in the container. Makes only sense with option 1

    Vector: isElementDelete --> 0 / 1
    Map: isKeyDelete --> 0 / 1, isValueDelete --> 0 / 1
    
    Vector
      isElementDelete == true
        add( new() ) --> ok
        add( clone( new() ) --> wrong, memory leak
        add( container_get() ) --> wrong, double delete
        add ( clone( container_get() ) ) -> ok
      isElementDelete == false
        add( new() ) --> wrong, memory leak
        add( clone( new() ) ) --> wrong, 2x memory leak
        add( container_get() ) --> ok
        add( clone( container_get() ) ) -> memory leak
    
    
  
*/

#include <stddef.h>
#include <stdint.h>

typedef struct coStruct *co;
typedef struct coStruct const * const cco;
typedef struct coFnStruct *coFn;

typedef int (*coInitFn)(co o, void *data);
typedef long (*coSizeFn)(co o);
typedef cco (*coGetByIdxFn)(co o, long idx);
typedef const char *(*coToStringFn)(co o);
typedef void (*coPrintFn)(const cco o);
typedef void (*coDestroyFn)(co o);
typedef int (*coEmptyFn)(cco o);
typedef co (*coCloneFn)(cco o);


typedef long int coInt;

struct co_avl_node_struct 
{
  const char *key;
  void *value;
  struct co_avl_node_struct * kid[2];
  int height;
};

#define CO_NONE 0
#define CO_FREE_VALS 1
//#define CO_FREE_KEYS 2
//#define CO_FREE (CO_FREE_VALS|CO_FREE_KEYS)

/* used by str and map objects 
  CO_STRDUP: execute a strdup on the string or key
    If CO_STRDUP is used, then  also CO_STRFREE will be enabled
  CO_STRFREE: execute free on the string or key
*/
#define CO_STRDUP 4
#define CO_STRFREE 8 

struct coStruct
{
  coFn fn;
  unsigned flags;
  union
  {
    struct // vector 
    {
      co *list;
      size_t cnt;
      size_t max;
    } v;
    struct // map 
    {
      struct co_avl_node_struct *root;
    } m;
    struct // string
    {
      char *str;
      size_t len;       // current str length
      //size_t memlen;    // allocated memory
    } s;
    struct // double
    {
      double n;
    } d;
  };
};

struct coFnStruct
{
  coInitFn init;                      // (*coInitFn)(co o);
  coSizeFn size;        
  coGetByIdxFn getByIdx;
  coToStringFn toString;
  coPrintFn print;
  coDestroyFn destroy;  // counterpart to init
  coCloneFn clone;
};

/* user interface */

extern coFn coBlankType;
extern coFn coVectorType;
extern coFn coStrType;
extern coFn coMapType;
extern coFn coDblType;

co coNewBlank();
co coNewStr(unsigned flags, const char *s);
co coNewDbl(double n);

/* generic functions */

void coPrint(const cco o);
cco coGetByIdx(co o, long idx);
void coDelete(co o);
co coClone(cco o);

#define coGetType(o) ((o)->fn)

#define coIsVector(o) (coGetType(o)==coVectorType)
#define coIsStr(o) (coGetType(o)==coStrType)
#define coIsMap(o) (coGetType(o)==coMapType)
#define coIsDbl(o) (coGetType(o)==coDblType)

co coReadJSONByString(const char *json);


/* string functions */
int coStrAdd(co o, const char *s);      // concats the given string to the string object, requires the CO_STRDUP flag
char *coStrDeleteAndGetAllocatedStringContent(co o);  // convert a str obj to a string, return value must be free'd


/* vector functions */
co coNewVector(unsigned flags);
long coVectorAdd(co o, co p);         // add object at the end of the list, returns -1 for error
int coVectorAppendVector(co v, cco src);  // append elements from src to vector v
cco coVectorGet(co o, long idx);           // return object at specific position from the vector
void coVectorErase(co v, long i);  // delete and remove element at the specified position
void coVectorClear(co o);    // delete all elements and clear the array
int coVectorEmpty(cco o);               // return 1 if the vector is empty, return 0 otherwise
long coVectorSize(co o);                // return the number of elements in the vector

typedef int (*coVectorForEachCB)(cco o, long idx, cco element, void *data);
int coVectorForEach(cco o, coVectorForEachCB cb, void *data);

typedef co (*coVectorMapCB)(cco o, long idx, cco element, void *data);
co coVectorMap(cco o, coVectorMapCB cb, void *data);


/* map functions */
co coNewMap(unsigned flags);
int coMapAdd(co o, const char *key, co value);    // insert object into the map, returns 0 for memory error
cco coMapGet(cco o, const char *key);     // get object from map by key, returns NULL if key doesn't exist in the map 
void coMapErase(co o, const char *key);   // removes object from the map
void coMapClear(co o);   // delete all elements and clear the array         
int coMapEmpty(cco o); // return 1 if the map is empty, return 0 otherwise

typedef int (*coMapForEachCB)(cco o, long idx, const char *key, cco value, void *data);
void coMapForEach(cco o, coMapForEachCB cb, void *data);

