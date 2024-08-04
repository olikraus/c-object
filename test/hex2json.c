

#include "co.h"

int main(int argc, char **argv)
{
	co csvco;
	FILE *hexfp;
	FILE *jsonfp;
	
	if ( argc != 3 )
	{
		printf("%s in.csv out.json\n", argv[0]);
		return 1;
	}
	hexfp = fopen(argv[1], "rb");
	if ( hexfp == NULL )
	{
		perror(argv[1]);
		return 2;
	}
	jsonfp = fopen(argv[2], "wb");
	if ( jsonfp == NULL )
	{
		perror(argv[2]);
		fclose(hexfp);
		return 3;
	}
	
	csvco = coReadHEXByFP(hexfp);
	coWriteJSON(csvco, 0, 1, jsonfp);	
	
	fclose(hexfp);
	fclose(jsonfp);
	coDelete(csvco);
	
	return 0;
}