/*

  avl_tree.c
  
  CC BY-SA 3.0  https://creativecommons.org/licenses/by-sa/3.0/

  Based on https://rosettacode.org/wiki/AVL_tree/C  
    with the license: https://www.gnu.org/licenses/fdl-1.3.en.html

  According to FDL 1.3 (https://www.gnu.org/licenses/fdl-1.3-faq.html.en), 
  this code is relicensed and provided under
    CC BY-SA 3.0 https://creativecommons.org/licenses/by-sa/3.0/
    
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

struct avl_node_struct 
{
  char *key;
  void *value;
  struct avl_node_struct * kid[2];
  int height;
};

struct avl_node_struct avl_dummy = { NULL, NULL, {&avl_dummy, &avl_dummy}, 0 };
struct avl_node_struct *avl_nnil = &avl_dummy; // internally, avl_nnil is the new nul

typedef void (*avl_free_fn)(void *p);
typedef int (*avl_visit_fn)(size_t idx, struct avl_node_struct *n, void *data);

void avl_free_value(void *value)
{
  free(value);
}

void avl_free_key(void *key)
{
  free(key);
}

void avl_keep_key(void *key)
{
}

struct avl_node_struct *avl_new_node(char *key, void *value)
{
  struct avl_node_struct *n = malloc(sizeof(struct avl_node_struct));
  n->key = key;
  n->value = value;
  n->height = 1;
  n->kid[0] = avl_nnil;
  n->kid[1] = avl_nnil;
  return n;
}

static void avl_delete_node(struct avl_node_struct *n, avl_free_fn free_key, avl_free_fn free_value)
{
  avl_free_key(n->key);
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

static void avl_set_height(struct avl_node_struct *n) 
{
  n->height = 1 + avl_max(n->kid[0]->height, n->kid[1]->height);
}

static int avl_get_ballance_diff(struct avl_node_struct *n) 
{
  return n->kid[0]->height - n->kid[1]->height;
}

// rotate a subtree according to dir; if new root is nil, old root is freed
static struct avl_node_struct * avl_rotate(struct avl_node_struct **rootp, int dir, avl_free_fn free_key, avl_free_fn free_value)
{
  struct avl_node_struct *old_r = *rootp;
  struct avl_node_struct *new_r = old_r->kid[dir];

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

static void avl_adjust_balance(struct avl_node_struct **rootp, avl_free_fn free_key, avl_free_fn free_value)
{
  struct avl_node_struct *root = *rootp;
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

// find the node that contains the given key; or returns 0
struct avl_node_struct *avl_query(struct avl_node_struct *root, const char *key)
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

void avl_insert(struct avl_node_struct **rootp, char *key, void *value, avl_free_fn free_key, avl_free_fn free_value)
{
  struct avl_node_struct *root = *rootp;
  int c;

  if ( key == NULL )
    return; // illegal key

  if (root == avl_nnil)
  {
    *rootp = avl_new_node(key, value);  // value is cloned within avl_new_node()
    return;
  }
  
  c = strcmp(key, root->key);
  if ( c == 0 )
  {
    // key already exists: replace value
    free_key(key);
    if ( root->value != NULL )
      free_value(root->value);
    root->value = value;
  }
  else
  {
    avl_insert(&root->kid[c > 0], key, value, free_key, free_value);
    avl_adjust_balance(rootp, free_key, free_value);
  }
}

void avl_delete(struct avl_node_struct **rootp, const char *key, avl_free_fn free_key, avl_free_fn free_value)
{
  struct avl_node_struct *root = *rootp;
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

int avl_for_each(struct avl_node_struct *n, avl_visit_fn visitCB, size_t *idx, void *data)
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
  after calling avl_delete_all the "n" argument is illegal ans points to avl_nnil
*/
void avl_delete_all(struct avl_node_struct **n, avl_free_fn free_key, avl_free_fn free_value)
{
  if ( *n == avl_nnil) 
    return;
  avl_delete_all(&((*n)->kid[0]), free_key, free_value);
  avl_delete_all(&((*n)->kid[1]), free_key, free_value);
  avl_delete_node(*n, free_key, free_value);
  *n = avl_nnil;
}

static int avl_get_size_cb(size_t idx, struct avl_node_struct *n, void *data)
{
  return 1;
}


size_t avl_get_size(struct avl_node_struct *n)
{
  size_t cnt = 0;
  avl_for_each(n, avl_get_size_cb, &cnt, NULL);
  return cnt;
}


// aux display and verification routines, helpful but not essential
struct trunk {
	struct trunk *prev;
	char * str;
};

int show_node(size_t idx, struct avl_node_struct *n, void *data)
{
  printf("%lu:%s ", idx, n->key);
  return 1;
}

void show_all(struct avl_node_struct *root)
{
  size_t idx = 0;
  avl_for_each(root, show_node, &idx, NULL);
}


void show_trunks(struct trunk *p)
{
	if (!p) return;
	show_trunks(p->prev);
	printf("%s", p->str);
}

// this is very haphazzard
void show_tree(struct avl_node_struct *root, struct trunk *prev, int is_left)
{
	if (root == avl_nnil) return;

	struct trunk this_disp = { prev, "    " };
	char *prev_str = this_disp.str;
	show_tree(root->kid[0], &this_disp, 1);

	if (!prev)
		this_disp.str = "---";
	else if (is_left) {
		this_disp.str = ".--";
		prev_str = "   |";
	} else {
		this_disp.str = "`--";
		prev->str = prev_str;
	}

	show_trunks(&this_disp);
	printf("%s\n", root->key);

	if (prev) prev->str = prev_str;
	this_disp.str = "   |";

	show_tree(root->kid[1], &this_disp, 0);
	if (!prev) puts("");
}

int verify(struct avl_node_struct *p)
{
	if (p == avl_nnil) return 1;

	int h0 = p->kid[0]->height, h1 = p->kid[1]->height;
	int b = h0 - h1;

	if (p->height != 1 + avl_max(h0, h1) || b < -1 || b > 1) {
		printf("node %s bad, balance %d\n", p->key, b);
		show_tree(p, 0, 0);
		abort();
	}
	return verify(p->kid[0]) && verify(p->kid[1]);
}

#define MAX_VAL 25

const char *get_str(int x)
{
  static char buf[20];
  sprintf(buf, "%03d", x);
  return buf;
}

int main(void)
{
	int x;
	struct avl_node_struct *root = avl_nnil;

	srand(time(0));

	for (x = 0; x < 10 * MAX_VAL; x++) 
        {
		// random insertion and deletion
		if (rand()&1)
                {
                    avl_insert(&root, strdup(get_str(rand()%MAX_VAL)), NULL, avl_free_key, avl_free_value);
                }
		else
                {
                    avl_delete(&root, get_str(rand()%MAX_VAL), avl_free_key, avl_free_value);
                }

		verify(root);
	}

	puts("Tree is:");
	show_tree(root, 0, 0);
        show_all(root);

        printf("\nSize: %lu", avl_get_size(root));
        
	puts("\nQuerying values:");
	for (x = 0; x < MAX_VAL; x++) {
		struct avl_node_struct *p = avl_query(root, get_str(x));
		if (p)	printf("%2d found: %p %s\n", x, p, p->key);
	}

        avl_delete_all(&root, avl_free_key, avl_free_value);
        
	for (x = 0; x < MAX_VAL; x++) {
		avl_delete(&root, get_str(x), avl_free_key, avl_free_value);
		verify(root);
	}

	puts("\nAfter deleting all values, tree is:");
	show_tree(root, 0, 0);

	return 0;
}

