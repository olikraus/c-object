#include "co.h"
#include <stdio.h>
#include <string.h>
#include <assert.h>


int depth_max = 0;
long element_cnt = 0;
long attribute_cnt = 0;



void traverse(cco e, int depth) {
	if ( depth_max < depth )
		depth_max = depth;
	if ( coIsVector(e) ) {
		cco cos = coVectorGet(e, 0);
		const char *name = coStrGet(cos);		// the name of the element
		cco attr = coVectorGet(e, 1);			// map with all the attributes, value is always a coStr()
		coMapIterator iter;
		long i;
		long cnt = coVectorSize(e);

		attribute_cnt += coMapSize(attr);
	
		element_cnt++;
		
		printf("%3d %s", depth, name);
		
		if ( coMapLoopFirst(&iter, attr) )
		{
			do {
				const char *key = coMapLoopKey(&iter);  // return the key of the current key/value pair (const char *)
				cco value = coMapLoopValue(&iter);	// return the value of the current key/value pair (cco)
				printf(" %s:%s", key, coStrGet(value));
			} while( coMapLoopNext(&iter) );
		}
		printf("\n");
		
		for( i = 2; i < cnt; i++ ) {  // child elements (co vector or co string) start at index 2
			traverse(coVectorGet(e, i), depth+1);
		}
	}
	else {
		assert(coIsStr(e));  // if e is not a vector, then it's a string
		printf("%3d '%s'\n", depth, coStrGet(e));
	}
}


int main(int argc, char **argv) {
	co xml;
	FILE *fp ;
	if ( argc <= 1 ) {
		printf("%s <xml-file>\n", *argv);
		return 1;
	}
	fp = fopen(argv[1], "rb");
	if ( fp == NULL ) {
		perror(argv[1]);
		return 1;
	}
	puts("reading");
	xml = coReadXMLByFP(fp, 1);
	fclose(fp);
	puts("traverse");
	traverse(xml, 0);
	printf("depth_max=%d\n", depth_max);
	printf("element_cnt=%ld\n", element_cnt);
	printf("attribute_cnt=%ld\n", attribute_cnt);
	
	//coPrint(xml);
	puts("cleanup");
	coDelete(xml);
	return 0;
}

