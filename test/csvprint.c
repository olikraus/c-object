

#include "co.h"

int main(int argc, char **argv)
{
    co rowVector;
    FILE *csvfp;
    struct co_reader_struct reader;
    
    if ( argc != 2 )
    {
            printf("%s in.csv\n", argv[0]);
            return 1;
    }
    csvfp = fopen(argv[1], "rb");
    if ( csvfp == NULL )
    {
            perror(argv[1]);
            return 2;
    }
    
    if ( coReaderInitByFP(&reader, csvfp) == 0 )
      return fclose(csvfp), 3;
    for (;;) {
      rowVector = coGetCSVRow(&reader, ',');
      if ( rowVector == NULL )
        break;
      coPrint(rowVector); puts("");
      coDelete(rowVector);
    } 

    
    fclose(csvfp);
    return 0;
}

