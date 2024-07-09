// ****************************************************************************
//
//	rx_file.c
//
// ****************************************************************************
//
//	Copyright 2014 by Radex AG, Switzerland. All rights reserved.
//	Written by Marcel Galliker
//
// ****************************************************************************

#include <stdio.h>
#ifdef linux
	#include <stdlib.h>
	#include <unistd.h>
	#include <sys/stat.h>
	#include <sys/types.h>
	#include <sys/time.h>
	#include <sys/times.h>
	#include <string.h>
	#include <utime.h>
	#include <time.h>
#else
	#include <io.h>
	#include <sys/types.h>
	#include <sys/stat.h>
	#include <sys/utime.h>
	#include <direct.h>
	#include <time.h>
#endif

#include <errno.h>

#include "rx_file.h"


#ifdef WIN32
int rx_file_exists(const char *path)
{
	return (_access(path, 0x00)==0);
}

int rx_dir_exists(const char *path)
{
	WIN32_FILE_ATTRIBUTE_DATA type;
	if (GetFileAttributesExA(path, GetFileExInfoStandard, &type))
	if (type.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
		return 1;
	return 0;
}

//--- struct SDir ---------------------------------
typedef struct
{
	HANDLE	hFind;
	char	match[256];
} SSearchWin;

//--- rx_search_open ---------------------------------
SEARCH_Handle rx_search_open(const char *path, const char *match)
{
	SSearchWin *psearch = (SSearchWin*)malloc(sizeof(SSearchWin));
	memset(psearch, 0, sizeof(SSearchWin));
	sprintf(psearch->match, "%s/%s", path, match);
	return (SEARCH_Handle) psearch;
}

//--- rx_search_next ----------------------------------
int rx_search_next(SEARCH_Handle hsearch, char *filename, int size, UINT64 *writeTime, UINT32 *filesize, UINT32 *isdir)
{
	SSearchWin *psearch = (SSearchWin*)hsearch;
	WIN32_FIND_DATA ffd;
	memset(&ffd, 0, sizeof(ffd));

	if (hsearch!=NULL)
	{
		if (psearch->hFind==NULL)		
			psearch->hFind = FindFirstFile(psearch->match, &ffd);
		else if (psearch->hFind!=INVALID_HANDLE_VALUE)	
			FindNextFile(psearch->hFind, &ffd);
		strncpy(filename, ffd.cFileName, size);
		if (writeTime) *writeTime = FiletimeToTimet(&ffd.ftLastWriteTime);
		if (filesize)  *filesize  = ffd.nFileSizeLow;
		if (isdir)	   *isdir     = (ffd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)==FILE_ATTRIBUTE_DIRECTORY;
	}
	return *filename!=0;
}

//--- rx_search_close --------------------------------
int rx_search_close(SEARCH_Handle  hsearch)
{
	SSearchWin *psearch = (SSearchWin*)hsearch;
	if (psearch!=NULL) FindClose(psearch->hFind);
	free(psearch);
	return 0;
}

//--- rx_file_get_mtime ---------------------------------------------------------
time_t rx_file_get_mtime (const char *path)
{
	struct __stat64 st;
	if(_stat64(path, &st)) return 0;
	return st.st_mtime;
}


//--- rx_file_del_if_older ---------------------------------------
int rx_file_del_if_older(const char *path, const char *comparePath)
{
	time_t compareTime = rx_file_get_mtime(comparePath);
	time_t time		   = rx_file_get_mtime(path);
	if (time && (!compareTime || time<compareTime))
	{
		remove(path);
		return TRUE;
	}
	return FALSE;
}

//--- rx_file_set_mtime ---------------------------------------------
void rx_file_set_mtime (const char *path, time_t time)
{
	struct __utimbuf64 t;
	struct __stat64 st;

	_stat64(path, &st);
	
	t.modtime = time;
	t.actime  = st.st_atime;
		
	_utime64(path, &t);
}

//--- rx_file_get_size ------------------------------------
INT64 rx_file_get_size(const char *path)
{
	struct __stat64 st;
	if (_stat64(path, &st)) return 0;
	return st.st_size;
}

//--- rx_file_set_readonly ----------------------------------
void	rx_file_set_readonly(const char *path, int readonly)
{
}

//--- rx_file_seek -----------------------------------
INT64	rx_file_seek	  (FILE *file, INT64 offset, int origin)
{
	return _fseeki64(file, offset, origin);
}
#endif

#ifdef linux
int rx_file_exists(const char *path)
{
	return (access( path, F_OK ) != -1);
}

int rx_dir_exists(const char *path)
{
	struct stat stat;
	if (lstat(path, &stat) == 0) {
		return (S_ISDIR(stat.st_mode)); 

	}
	return 0;
}

//--- struct SDir ---------------------------------
typedef struct
{
	DIR		*handle;
	char	path[MAX_PATH];
	char	match[MAX_PATH];
} SSearchLx;

//--- rx_search_open ---------------------------------
SEARCH_Handle rx_search_open (const char *path, const char *match)
{
	SSearchLx *psearch;

	psearch = (SSearchLx*)malloc(sizeof(SSearchLx));
	strncpy(psearch->path,  path,  sizeof(psearch->path));
	strncpy(psearch->match, match, sizeof(psearch->match));
	psearch->handle = opendir(path);
	if  (psearch->handle==NULL)
	{
		int err=errno;
		char str[128];
		err_system_error(err, str, sizeof(str));
		printf("errno=%d: %s\n", err, str);
	}
	return (SEARCH_Handle) psearch;
}

//--- rx_search_next ----------------------------------
int	rx_search_next (SEARCH_Handle hsearch, char *filename, int size, UINT64 *writeTime, UINT32 *filesize, UINT32 *isdir)
{
	if (hsearch!=NULL)
	{
		SSearchLx *psearch = (SSearchLx*)hsearch;
		struct dirent	*item;
		struct stat 	info;
		int				dir=FALSE;
		char			path[MAX_PATH];
		if (psearch->handle!=NULL)
		{
			while ((item=readdir(psearch->handle))!=NULL)
			{
				if(rx_fnmatch(psearch->match, item->d_name))
				{
					strncpy(filename, item->d_name, size);
					sprintf(path, "%s/%s", psearch->path, item->d_name);
					if (/*item->d_name[0]!='.'&& */ stat(path, &info)==0)
					{
						if (writeTime)	memcpy(writeTime, &info.st_mtim, sizeof(*writeTime));
						if (filesize)	*filesize = info.st_size;
						if (isdir) *isdir = (info.st_mode&0x4000)==0x4000;
					}
					return (*filename!=0);
				}
			}
		}
	}
	return 0;
}

//--- rx_search_close --------------------------------
int	rx_search_close(SEARCH_Handle hsearch)
{
	if (hsearch!=NULL)
	{
		SSearchLx *psearch = (SSearchLx*)hsearch;
		int ret = closedir(psearch->handle);
		free(psearch);
		return ret;
	}
	return 0;
}

//--- rx_file_get_mtime ---------------------------------------------------------
time_t rx_file_get_mtime (const char *path)
{
	struct stat st;
	if(stat(path, &st)) return 0;
	return st.st_mtime;
}

//--- rx_file_del_if_older ---------------------------------------
int rx_file_del_if_older(const char *path, const char *comparePath)
{
	time_t compareTime = rx_file_get_mtime(comparePath);
	time_t time		   = rx_file_get_mtime(path);
	if (time && (!compareTime || time<compareTime))
	{
		remove(path);
		return TRUE;
	}
	return FALSE;
}

//--- rx_file_set_mtime ---------------------------------------------
void rx_file_set_mtime (const char *path, time_t time)
{
	struct utimbuf t;
	struct stat st;

	stat(path, &st);
	
	t.modtime = time;
	t.actime  = st.st_atime;
		
	utime(path, &t);
}

//--- rx_file_get_size ------------------------------------
INT64 rx_file_get_size(const char *path)
{
	struct stat st;
	if (stat(path, &st)) return 0;
	return st.st_size;
}

//--- rx_file_set_readonly ------------------------
void	rx_file_set_readonly(const char *path, int readonly)
{
	struct stat st;
	if (stat(path, &st)) return;
	if (readonly) chmod(path, st.st_mode &~ (S_IWUSR|S_IWGRP|S_IWOTH));
	else          chmod(path, st.st_mode | S_IWUSR);
}

INT64	rx_file_seek	  (FILE *file, INT64 offset, int origin)
{
	return fseek(file, offset, origin);	// not 64 Bit!!!
}

#endif

//--- rx_fnmatch ------------------------------------------
int	rx_fnmatch(const char * pmatch, const char *pstr)
{
	const char *restartStr=NULL;
	const char *restartMatch=NULL;
	char smatch[MAX_PATH];
	char sstr[MAX_PATH];

	strcpy(smatch, pmatch);
	strcpy(sstr, pstr);
	char_to_lower(smatch, smatch);
	char_to_lower(sstr, sstr);

	const char *match = smatch;
	const char *str	  = sstr;

	while (*match && *str)
	{
		if (*match=='*')
		{
			restartMatch = match++;
			while (*str && *str!=*match) 
				str++;
			if (*str) restartStr=str+1;
		}
		else if (*match==*str) 
		{
			match++;
			str++;
		}
		else if (restartStr)
		{
			str=restartStr;
			match=restartMatch;
			restartStr=NULL;		
		}
		else return FALSE;
	}
	if (*str || *match) return FALSE;
	return TRUE;
}

//--- rx_mkdir ------------------------------------
#ifdef linux
	int rx_mkdir(const char *dirname)
	{
		int ret;
		mode_t process_mask = umask(0);
		if (ret=mkdir(dirname, S_IRWXU | S_IRWXG | S_IRWXO))
		{
			if (errno!=EEXIST) printf("Error %d: %s\n", errno, strerror(errno));
		}
//		printf("mkdir >>%s<< ret=%d, errno=%d, str=>>%s<<\n", dirname, ret, errno, strerror(errno));
		umask(process_mask);
	}
#else
	int rx_mkdir(const char *dirname)
	{
		return _mkdir(dirname);
	}
#endif

//--- rx_mkdir_path --------------------------
void rx_mkdir_path(const char *dirname)
{
	char *ch, old;
	char dir[MAX_PATH];
	
	if (!*dirname) return;

	strcpy(dir, dirname);
	for (ch=&dir[1]; *ch; ch++)
	{
		if (*ch=='/' || *ch=='\\')
		{
			old = *ch;
			*ch=0;
			rx_mkdir(dir);
			*ch=old;
		}
	}
}

//--- rx_rmdir -------------------------------------------
int		rx_rmdir(const char *dirname)
{
	#ifdef linux
		return rmdir(dirname);
	#else
		return _rmdir(dirname);
	#endif
}

//--- rx_fopen --------------------------------------------------------------
FILE * rx_fopen(const char * path, const char * mode, int sharemode)
{
#ifdef linux
	return fopen(path, mode);
#else
	if (sharemode)	return _fsopen(path, mode, sharemode);
	else			return fopen(path, mode);
#endif
}

#ifndef linux
int gettimeofday(struct timeval *tp, struct timezone *tzp)
{
    // Note: some broken versions only have 8 trailing zero's, the correct epoch
    // has 9 trailing zero's This magic number is the number of 100 nanosecond
    // intervals since January 1, 1601 (UTC) until 00:00:00 January 1, 1970
    static const UINT64 EPOCH = ((UINT64)116444736000000000ULL);

    SYSTEMTIME system_time;
    FILETIME file_time;
    UINT64 time;

    GetSystemTime(&system_time);
    SystemTimeToFileTime(&system_time, &file_time);
    time = ((UINT64)file_time.dwLowDateTime);
    time += ((UINT64)file_time.dwHighDateTime) << 32;

    tp->tv_sec = (long)((time - EPOCH) / 10000000L);
    tp->tv_usec = (long)(system_time.wMilliseconds * 1000);
    return 0;
}
#endif

//--- remove_old_files --------------------------------------
int rx_remove_old_files(const char *searchPath, int days)
{
	SEARCH_Handle	search;
	char			fileName[100];
	char			path[MAX_PATH];
	UINT64			fileTime;
	UINT32			filesize;
	UINT32			isDir;
	struct timeval  now;
	
	gettimeofday(&now, NULL);

	printf("rx_remove_old_files(>>%s<<, %d days)", searchPath, days);
	search = rx_search_open(searchPath, "*");
	while (rx_search_next(search, fileName, sizeof(fileName), &fileTime, &filesize, &isDir))
	{
		// printf("file >>%s<< diff=%d\n", fileName, now.tv_sec-fileTime);
		sprintf(path, "%s/%s", searchPath, fileName);
		if (isDir && fileName[0]!='.')
		{
			rx_remove_old_files(path, days);
		}
		if (!isDir && (now.tv_sec>fileTime) &&  ((now.tv_sec-fileTime) > (24*60*60)*days))
		{
			printf("remove >>%s<<\n", path);
			remove(path);		
		}
		
	}
	rx_search_close(search);

	return REPLY_OK;
}

//--- rx_file_mount -------------------------------------------------------------------
#ifdef linux
int rx_file_mount(const char *remotePath, const char *mntPath, const char *user, const char *pwd)
{
	#define OUTPUT "/tmp/mount"
	if (*mntPath)
	{		
		// Giving access right in ubuntu:
		// > sudo visudo
		// >> add line "radex ALL = NOPASSWD: /bin/mount"
		// >> add line "radex ALL = NOPASSWD: /bin/mkdir"
		
		char command[2048];
		int ret;

		//--- check wether already mounted ---
		{
			FILE* fp;
			char str[100];
			fp = popen("mount -l -tcifs", "r");
			while (!feof(fp))
			{
				fgets(str, 100, fp);
				// printf("%s\n", str);
				if (str_start(str, remotePath)) 
				{
					// TrPrintfL(TRUE, "sr_mnt_drive: already mounted: end");
					pclose(fp);
					return REPLY_OK;
				}
			}
			pclose(fp);
		}

		if (access(mntPath, 0))
		{
			sprintf(command, "mkdir -m777 %s", mntPath); 
			ret=system(command);
		}

		sprintf(command, "mount -t cifs %s %s -o user=%s,password=%s", remotePath, mntPath, user, pwd); 
		if (system(command)==0) return REPLY_OK;
		sprintf(command, "umount %s", mntPath); 
		return REPLY_ERROR;
	}
}
#else

int rx_file_mount(const char *remotePath, const char *mntPath, const char *user, const char *pwd)
{
	int i=0;
	i=1/i;
	return REPLY_ERROR;
}
#endif
