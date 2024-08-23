

#include "co.h"

int main(int argc, char **argv)
{
    co csvco;
  
    // any of the three following pool inits is possible
    //co pool = coNewMap(CO_FREE_VALS | CO_STRDUP);
    co pool = coNewMap(CO_FREE_VALS);
    //co pool = NULL;
    FILE *csvfp;
    FILE *jsonfp;
    
    if ( argc != 3 )
    {
            printf("%s in.csv out.json\n", argv[0]);
            return 1;
    }
    csvfp = fopen(argv[1], "rb");
    if ( csvfp == NULL )
    {
            perror(argv[1]);
            return 2;
    }
    jsonfp = fopen(argv[2], "wb");
    if ( jsonfp == NULL )
    {
            perror(argv[2]);
            fclose(csvfp);
            return 3;
    }
    
    csvco = coReadCSVByFPWithPool(csvfp, ',', pool);
    coWriteJSON(csvco, 0, 1, jsonfp);	
    coPrint(pool); puts("");
    
    fclose(csvfp);
    fclose(jsonfp);
    coDelete(csvco);
    coDelete(pool);
    return 0;
}

