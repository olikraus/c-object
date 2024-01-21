// https://man7.org/linux/man-pages/man3/ftw.3.html
// https://stackoverflow.com/questions/2312110/efficiently-traverse-directory-tree-with-opendir-readdir-and-closedir


//#define _XOPEN_SOURCE 500

#include <ftw.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include "co.h"



#define MAX_DEPTH 40

long total_pairs_found = 0;
int max_level = 0;
co all_dir_list = NULL;
co s19_list = NULL;
co a2l_list = NULL;
co hex_list = NULL;

#define MAX_FILENAME 512
#define MAX_PATH_LEN (4*1024)



static int display_info(const char *fpath, const struct stat *sb, int tflag, struct FTW *ftwbuf)
{
	//static unsigned pair_found[MAX_DEPTH];
	static int current_level = -1;
	//static char a2l_name[MAX_FILENAME];
	//static char s19_name[MAX_FILENAME];
	static char dir_path[MAX_PATH_LEN];
	


	
	int ret = FTW_CONTINUE;
	/*
		fpath: 					Full path including the current file/dir namespace
		int ftwbuf->base:		The position within fpath, where the current file/dir starts
		int ftwbuf->level:		The current depth in the tree 
		tflag:					Information about the current file/dir, all is ignored except FTW_F and FTW_D
		
		return values:
			FTW_CONTINUE
			FTW_SKIP_SIBLINGS
			FTW_SKIP_SUBTREE
			FTW_STOP
	*/
	
	if ( current_level != ftwbuf->level )
	{
		// if ( current_level >= 0 && current_level < MAX_DEPTH )
		// 	pair_found[current_level] = 0;
		if ( max_level < ftwbuf->level )
			max_level = ftwbuf->level;
		
		if ( coVectorSize(a2l_list) > 0 && ( coVectorSize(s19_list) > 0 || coVectorSize(hex_list) > 0 ) )
		{
			co dir_list = coNewVector(CO_FREE_VALS);
			
			coVectorAdd(dir_list, coNewStr(CO_STRDUP, dir_path));
			coVectorAdd(dir_list, a2l_list);
			coVectorAdd(dir_list, s19_list);
			coVectorAdd(dir_list, hex_list);
			
			coVectorAdd(all_dir_list, dir_list);
			
			//coPrint(a2l_list);
			printf(" a2l:%ld s19:%ld  %s\n", coVectorSize(a2l_list), coVectorSize(s19_list), dir_path);

			/* all vector are move to dir_list, create new objects */
			a2l_list = coNewVector(CO_FREE_VALS);
			s19_list = coNewVector(CO_FREE_VALS);
			hex_list = coNewVector(CO_FREE_VALS);

		}
		else
		{
			/* objects are not moved, reuse them */
			coVectorClear(a2l_list);
			coVectorClear(s19_list);
			coVectorClear(hex_list);			
		}
		
		current_level = ftwbuf->level;
		//assert(current_level < MAX_DEPTH);
		//pair_found[current_level] = 0;
		dir_path[0] = '\0';
	}
	
	if ( tflag == FTW_F ) // regular file
	{
		size_t len = strlen(fpath + ftwbuf->base);
		if ( strcasecmp(fpath + ftwbuf->base + len - 4, ".a2l") == 0 )
		{
			coVectorAdd(a2l_list, coNewStr(CO_STRDUP, fpath + ftwbuf->base));
			if ( dir_path[0] == '\0' )
			{
				strncpy(dir_path, fpath, ftwbuf->base);
				dir_path[ftwbuf->base] = '\0';
			}
		}
		if ( strcasecmp(fpath + ftwbuf->base + len - 4, ".s19") == 0 )
		{
			coVectorAdd(s19_list, coNewStr(CO_STRDUP, fpath + ftwbuf->base));
			if ( dir_path[0] == '\0' )
			{
				strncpy(dir_path, fpath, ftwbuf->base);
				dir_path[ftwbuf->base] = '\0';
			}
		}
		if ( strcasecmp(fpath + ftwbuf->base + len - 4, ".hex") == 0 )
		{
			coVectorAdd(hex_list, coNewStr(CO_STRDUP, fpath + ftwbuf->base));
			if ( dir_path[0] == '\0' )
			{
				strncpy(dir_path, fpath, ftwbuf->base);
				dir_path[ftwbuf->base] = '\0';
			}
		}
	}
	else if ( tflag == FTW_D ) // directory 
	{
		
	}

 return ret;           /* To tell nftw() to continue (FTW_ACTIONRETVAL is assumed) */
}

int main(int argc, char *argv[])
{
	FILE *jsonOutputFP;
	/*
		FTW_MOUNT			don't cross mounts
		FTW_PHYS			don't follow sym links
		FTW_DEPTH			read files within a directory first 
		FTW_ACTIONRETVAL	suppport the action return arguments for the CB 
	*/
   int flags = FTW_MOUNT | FTW_PHYS | FTW_DEPTH | FTW_ACTIONRETVAL;
   //int flags = FTW_MOUNT | FTW_PHYS  | FTW_ACTIONRETVAL;
   
   if ( argc < 3)
   {
	   puts("a2l_search path jsonfile");
	   exit(EXIT_FAILURE);
   }
   
   all_dir_list = coNewVector(CO_FREE_VALS);
	a2l_list = coNewVector(CO_FREE_VALS);
	s19_list = coNewVector(CO_FREE_VALS);
	hex_list = coNewVector(CO_FREE_VALS);

   if (nftw(argv[1], display_info, 20, flags) == -1)
   {
	   perror(argv[1]);
	   exit(EXIT_FAILURE);
   }
   
   printf("total pairs found: %ld\n", total_pairs_found);
   printf("max level: %d\n", max_level);

   jsonOutputFP = fopen(argv[2], "rb");
   if ( jsonOutputFP == NULL )
   {
	   perror(argv[2]);
	   exit(EXIT_FAILURE);
   }
   coWriteJSON(all_dir_list, /* isCompact= */ 0, /* isUTF8= */ 1, jsonOutputFP);
   fclose(jsonOutputFP);

   exit(EXIT_SUCCESS);
}

