/*

  co.h
  
  C Object Library https://github.com/olikraus/c-object

  CC BY-SA 3.0  https://creativecommons.org/licenses/by-sa/3.0/

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
    1) Move: The remote element becomes obsolete and becomes part of the container
      The moved element will be deleted together with the container
      The moved element must not be referenced any more somewhere else
    2) Reference: The element in the container is just a reference to the remote element.
      The element of the container is not deleted.
      Risk: If the remote element becomes deleted, then the element in the 
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

typedef struct coStruct *co;
typedef struct coStruct const * const cco;
typedef struct coFnStruct *coFn;

typedef int (*coInitFn)(co o, void *data);
typedef int (*coAddFn)(co o, co p);
typedef size_t (*coCntFn)(co o);
typedef cco (*coGetByIdxFn)(co o, size_t idx);
typedef const char *(*coToStringFn)(co o);
typedef void (*coPrintFn)(const cco o);
typedef void (*coDestroyFn)(co o);
typedef co (*coMapCB)(cco o, size_t idx, cco element, void *data);
typedef co (*coMapFn)(cco o, coMapCB cb, void *data); // data is just passed to the map function
typedef co (*coCloneFn)(cco o);


typedef int (*covForEachCB)(cco o, size_t idx, cco element, void *data);

typedef long int coInt;

struct co_avl_node_struct 
{
  char *key;
  void *value;
  struct co_avl_node_struct * kid[2];
  int height;
};

#define CO_NONE 0
#define CO_FREE_VALS 1
#define CO_FREE_KEYS 2
#define CO_FREE (CO_FREE_VALS|CO_FREE_KEYS)

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
    } s;
  };
};

struct coFnStruct
{
  coInitFn init;                      // (*coInitFn)(co o);
  coAddFn add;                // int (*coAddFn)(co o, co p), p will be deleted (invalid)
  coCntFn cnt;        
  coGetByIdxFn getByIdx;
  coToStringFn toString;
  coPrintFn print;
  coDestroyFn destroy;  // counterpart to init
  coMapFn map;                  // create a new vector, all elements must be cloned!
  coCloneFn clone;
};

/* user interface */

extern coFn coBlankType;
extern coFn coVectorType;
extern coFn coStrType;

co coNewBlank();
co coNewVector(unsigned flags);
co coNewStr(unsigned flags, const char *s);

void coPrint(const cco o);
int coAdd(co o, co p);
cco coGetByIdx(co o, size_t idx);
void coDelete(co o);
co coClone(cco o);

/* vector functions */
void covDeleteElement(co v, size_t i);
int covAppendVector(co v, cco src);  // append elements from src to vector v

