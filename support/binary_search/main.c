
#include <stdio.h>

int predecessor_binary_search(int *array, int n, int key)
{
  int l = 0;
  int r = n;
  int m;
  while( l < r )
  {
    m = (l+r)/2;
    if ( array[m] > key )
      r = m;
    else
      l = m+1;
  }
  return r - 1;
}

int test(int *array, int n, int key1, int key2)
{
  int result;
  int key;
  int i;
  for( key = key1; key <= key2; key++ )
  {
    printf("[");
    for( i = 0; i < n; i++ )
    {
      if ( i > 0 )
        printf(", ");
      printf("%d", array[i]);
    }
    result = predecessor_binary_search(array, n, key);
    printf("] search %d --> result %d\n", key, result);
  }
}

int main()
{
  int a1[1] = { 2 };
  int a2[2] = { 2, 4 };
  int a3[3] = { 2, 4, 6 };
  test(a1, 0, 1, 3);  
  test(a1, 1, 1, 3);  
  test(a2, 2, 1, 5);
  test(a3, 3, 1, 7);
}

