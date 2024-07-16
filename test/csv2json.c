

#include "co.h"

int main(int argc, char **argv)
{
	co csvco;
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
	
	csvco = coReadCSVByFP(csvfp, ',');
	coWriteJSON(csvco, 0, 1, jsonfp);	
	
	fclose(csvfp);
	fclose(jsonfp);
	coDelete(csvco);
	
	return 0;
}