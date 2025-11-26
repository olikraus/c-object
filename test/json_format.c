

#include <stdio.h>
#include "co.h"

int main(int argc, char **argv)
{
    co jsonco;
    FILE *infp;
    FILE *outfp = stdout;
    
    if ( argc != 2 )
    {
            printf("%s file.json\n", argv[0]);
            return 1;
    }
    infp = fopen(argv[1], "rb");
    if ( infp == NULL )
    {
            perror(argv[1]);
            return 2;
    }
    
    jsonco = coReadJSONByFP(infp);
    coWriteJSON(jsonco, 0, 0, outfp);
    fprintf(outfp, "\n");
    
    fclose(infp);
    coDelete(jsonco);
    return 0;
}

