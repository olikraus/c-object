#include "co.h"

int main(int argc, char **argv)
{
    co jsonco;
    FILE *jsonfp;
    
    if ( argc != 2 )
    {
            printf("%s in.json\n", argv[0]);
            return 1;
    }
    jsonfp = fopen(argv[1], "rb");
    if ( jsonfp == NULL )
    {
            perror(argv[2]);
            return 2;
    }
    
    jsonco = coReadJSONByFP(jsonfp);
	
	if ( jsonco == NULL )
	{
		printf( "result is NULL\n");
	}
	else if ( coIsVector(jsonco) )
	{
		printf( "vector size=%ld\n", coVectorSize(jsonco) );
	}
	else
	{
		printf( "size=%ld\n", coSize(jsonco) );
	}
	//coPrint(jsonco);
    
    fclose(jsonfp);
    coDelete(jsonco);
    return 0;
}

