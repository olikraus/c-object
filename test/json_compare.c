/*

	json_compare

	compare whether two JSON files are the same
	
	Errorlevel:
		0		not identical
		1		files are identical
		2		some error has happend (wrong commandline, read error, memory allocation)
	
*/

#include <string.h>
#include "co.h"

co stack = NULL;

int compare(cco co1, cco co2)
{
	puts("stack:");
	coPrint(stack);
	puts("");
	puts("compare 1:");
	coPrint(co1);
	puts("");
	puts("compare 2:");
	coPrint(co2);
	puts("");	
	
	if ( coIsVector(co1) && coIsVector(co2) )
	{
		long i;
		long cnt = coVectorSize(co1);
		co index;
		if ( cnt != coVectorSize(co2) )
		{
			printf("Vector size difference %ld vs %ld\n", coVectorSize(co1), coVectorSize(co2));
			coPrint(stack);
			puts("");
			return 0;
		}
		
		index = coNewDbl(0);	
		coVectorAdd(stack, index);	// store a reference to the index in the stack
		for( i = 0; i < cnt; i++ )
		{
			coDblSet(index, i);		// assign the current index
			if ( compare(coVectorGet(co1, i), coVectorGet(co2, i)) == 0 )
			{
				//coVectorEraseLast(stack);		// index will be deleted inside 
				return 0;
			}
		}
		coVectorEraseLast(stack);
		return 1;
	}
	else if ( coIsMap(co1) && coIsMap(co2) )
	{
		coMapIterator iter1, iter2;
		int r1, r2;
		const char *key1;
		const char *key2;
		co key;
		
		r1 = coMapLoopFirst(&iter1, co1);	// start iteration
		r2 = coMapLoopFirst(&iter2, co2);
		
		if ( r1 == 0 && r2 == 0 )
			return 1;
		if ( (r1 == 0 && r2 != 0) || (r1 != 0 && r2 == 0) )
		{
			printf("Map size difference %ld vs %ld\n", coMapSize(co1), coMapSize(co2));
			coPrint(stack);
			puts("");
			return 0;
		}
		
		key = coNewStr(CO_STRDUP, "");
		coVectorAdd(stack, key);	// store a reference to the key in the stack
		for(;;)
		{
			key1 = coMapLoopKey(&iter1);
			key2 = coMapLoopKey(&iter2);
			if ( strcmp(key1, key2) != 0 )
			{
				printf("Map key difference %s vs %s\n", key1, key2);
				coPrint(stack);
				puts("");
				//coVectorEraseLast(stack);		// key will be deleted inside 
				return 0;
				
			}
			coStrSet(key, key1);
			if ( compare(coMapLoopValue(&iter1), coMapLoopValue(&iter2)) == 0 )
			{
				//coVectorEraseLast(stack);		// key will be deleted inside 
				return 0;
			}
			
			r1 = coMapLoopNext(&iter1);		// continue with the iteration
			r2 = coMapLoopNext(&iter2);
			if ( r1 == 0 && r2 == 0 )
			{
				coVectorEraseLast(stack);
				break;  // all good, done
			}
			if ( (r1 == 0 && r2 != 0) || (r1 != 0 && r2 == 0) )
			{
				printf("Map size difference %ld vs %ld\n", coMapSize(co1), coMapSize(co2));
				coPrint(stack);
				puts("");
				//coVectorEraseLast(stack);		// key will be deleted inside 
				return 0;
			}
		}
		return 1;
	}
	else if ( coIsStr(co1) && coIsStr(co2) )
	{
		const char *s1 = coStrGet(co1);
		const char *s2 = coStrGet(co2);
		if ( strcmp(s1, s2) != 0 )
		{
			printf("String difference '%s' vs '%s'\n", s1, s2);
			coPrint(stack);
			puts("");
			return 0;
		}
		return 1;
	}
	else if ( coIsDbl(co1) && coIsDbl(co2) )
	{
		if ( coDblGet(co1) != coDblGet(co2) )
		{
			printf("Number difference '%lg' vs '%lg'\n", coDblGet(co1), coDblGet(co2));
			coPrint(stack);
			puts("");
			return 0;
		}
		return 1;
	}
	printf("Type mismatch / unsupported type found ()\n");
	coPrint(stack);
	puts("");
	return 0;
}

int main(int argc, char **argv)
{
    co json1co;
    co json2co;
    FILE *json1fp;
    FILE *json2fp;
	int r;
    
    if ( argc != 3 )
    {
            printf("%s file1.json file2.json\n", argv[0]);
            return 2;
    }
    json1fp = fopen(argv[1], "rb");
    if ( json1fp == NULL )
    {
            perror(argv[1]);
            return 2;
    }
    json2fp = fopen(argv[2], "rb");
    if ( json2fp == NULL )
    {
            perror(argv[2]);
			fclose(json1fp);
            return 2;
    }

    
    json1co = coReadJSONByFP(json1fp);
	if ( json1co == NULL )
	{
		printf( "Unable to read %s\n", argv[1]);
		fclose(json1fp); fclose(json2fp);
		return 2;
	}
	
    json2co = coReadJSONByFP(json2fp);
	if ( json2co == NULL )
	{
		printf( "Unable to read %s\n", argv[2]);
		fclose(json1fp); fclose(json2fp); coDelete(json1co);
		return 2;
	}
	
	
	stack = coNewVector(CO_FREE_VALS);
	if ( stack == NULL )
	{
		printf( "Memory problem\n");
		fclose(json1fp); fclose(json2fp); coDelete(json1co); coDelete(json2co);
		return 2;
	}

	puts("JSON 1:");
	coPrint(json1co);
	puts("");
	puts("JSON 2:");
	coPrint(json2co);
	puts("");
	
	r = compare(json1co, json2co);
	if ( r != 0 )
	{
		printf("JSON files '%s' and '%s' are identical\n", argv[1], argv[2]);
	}
	else
	{
		printf("Mismatch in JSON files '%s' and '%s'\n", argv[1], argv[2]);
	}
    
	fclose(json1fp); fclose(json2fp); coDelete(json1co); coDelete(json2co); coDelete(stack);
    return r;
}

