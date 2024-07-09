// ****************************************************************************
//
//	rx_file.h
//
// ****************************************************************************
//
//	Copyright 2014 by Radex AG, Switzerland. All rights reserved.
//	Written by Marcel Galliker
//
// ****************************************************************************

#pragma once

#include "rx_common.h"

#ifdef __cplusplus
extern "C"{
#endif


#ifdef linux
	#include <sys/types.h>
	#include <sys/stat.h>
	#include <dirent.h>
#endif

typedef HANDLE SEARCH_Handle;

int		rx_file_exists	  (const char *path);
int		rx_dir_exists	  (const char *path); 
time_t	rx_file_get_mtime (const char *path);
void    rx_file_set_mtime (const char *path, time_t time);
INT64	rx_file_get_size  (const char *path);
void	rx_file_set_readonly(const char *path, int readonly);
INT64	rx_file_seek	  (FILE *file, INT64 offset, int origin);		
int		rx_file_del_if_older(const char *path, const char *compPath);	// return 0=not deleted
int 	rx_file_mount(const char *remotePath, const char *mntPath, const char *user, const char *pwd);

SEARCH_Handle	rx_search_open (const char *path, const char *match);
int				rx_search_next (SEARCH_Handle hsearch, char *filename, int size, UINT64 *writeTime, UINT32 *filesize, UINT32 *isdir);
int				rx_search_close(SEARCH_Handle hsearch);

int				rx_fnmatch(const char * match, const char *str);

int		rx_mkdir(const char *dirname);
void	rx_mkdir_path(const char *dirname);
int		rx_rmdir(const char *dirname);
FILE *  rx_fopen(const char * path, const char * mode, int sharemode);

int rx_remove_old_files(const char *searchStr, int days);

#ifdef __cplusplus
}
#endif