

#include "co.h"

int main(int argc, char **argv)
{
	co mem;
	FILE *hexfp;
	FILE *jsonfp;
	
	if ( argc != 3 )
	{
		printf("%s in.elf out.json\n", argv[0]);
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
	
	mem = coReadElfMemoryByFP(hexfp);
	coWriteJSON(mem, 0, 1, jsonfp);	
	
	fclose(hexfp);
	fclose(jsonfp);
	coDelete(mem);
	
	return 0;
}
