/*

	json2utf8json.c
	

	convert any \uxxxx chars to real utf8 chars.
	
*/

#include "co.h"

int main(int argc, char **argv)
{
    co jsonco;
  
    FILE *infp;
    FILE *outfp;
    
    if ( argc != 3 )
    {
            printf("Build %s\n", __TIMESTAMP__);
            printf("%s in.json out.json\n", argv[0]);
            return 1;
    }
    infp = fopen(argv[1], "rb");
    if ( infp == NULL )
    {
            perror(argv[1]);
            return 2;
    }
    outfp = fopen(argv[2], "wb");
    if ( outfp == NULL )
    {
            perror(argv[2]);
            fclose(outfp);
            return 3;
    }
    
    jsonco = coReadJSONByFP(infp);
    coWriteJSON(jsonco, /* isCompact */ 0, /* isUTF8 */ 1, outfp);
    
    fclose(outfp);
    fclose(infp);
    coDelete(jsonco);
    return 0;
}

