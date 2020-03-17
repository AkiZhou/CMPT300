#include <sys/types.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <dirent.h>
#include <stdio.h>
#include <unistd.h>
#include <errno.h>
#include "restart.h"
#include "fileio.h"
#include <string.h>
#include "util.h"

#if 1
#define VERBOSE(p) (p)
#else
#define VERBOSE(p) (0)
#endif

int file_read(char *path, int offset, void *buffer, size_t bufbytes)
{
    //Check for invalid arguments
    //if (!access(path, R_OK)) return IOERR_INVALID_PATH;
    if (buffer == NULL || path == NULL || bufbytes <= 0 || offset < 0)
        return IOERR_INVALID_ARGS;

    memset(buffer, 0, bufbytes);

    int fd = r_open2(path, O_RDONLY);  //open the file
    if (fd == -1) return IOERR_INVALID_PATH;  // invalid path
    lseek(fd, (off_t) offset, SEEK_SET);  // apply offset
    ssize_t ret = r_read(fd, buffer, bufbytes);  // read from file
    close(fd);
    // ret == 0 when the buffer is full
    if(!ret)
        return (int) bufbytes;

    return (int) ret;
}

int file_info(char *path, void *buffer, size_t bufbytes)
{
	struct stat stats;
	char type;

    if (!path || !buffer || bufbytes < 1)
		return IOERR_INVALID_ARGS;
	memset(buffer, 0, bufbytes);

	if S_ISREG(stats.st_mode)
		type = 'f';
	else
	    type = 'd';

	stat(path, &stats);
	snprintf(buffer, bufbytes, "Size:%jd Accessed:%ld Modified:%ld Type %c", stats.st_size, stats.st_atime, stats.st_mtime, type);
	return 0;
}

int file_write(char *path, int offset, void *buffer, size_t bufbytes)
{
	if (buffer == NULL || path == NULL || bufbytes <= 0 || offset < 0)
	    return IOERR_INVALID_ARGS;

    int fd = r_open3(path, O_WRONLY|O_CREAT, 0b111111111);  //open the file
    if (fd == -1) return IOERR_INVALID_PATH;  // invalid path
    lseek(fd, (off_t) offset, SEEK_SET);  // apply offset
    int ret = r_write(fd, buffer, bufbytes);
    close(fd);

    return ret;
}

int file_create(char *path, char *pattern, int repeatcount)
{
	char* buffer;
	buffer = malloc(strlen(pattern)*repeatcount);

	for(int i = 0; i < repeatcount; i++)
		strcat(buffer, pattern);

	int wrt = file_write(path, 0, buffer, strlen(buffer));
	if (wrt >= 0)
		return 0;

	return IOERR_POSIX;
}

int file_remove(char *path)
{
    if(path == NULL) return IOERR_INVALID_ARGS;

    if( access( path, F_OK ) != -1 )
        return remove(path);
     else
        return IOERR_INVALID_PATH;
}

int dir_create(char *path)
{
    if(path == NULL) return IOERR_INVALID_ARGS;
    DIR* dir = opendir(path);

    if (dir) {  //directory already exists
        closedir(dir);
        return IOERR_INVALID_PATH;
    }

    if( access( path, F_OK ) != -1 )
        return IOERR_INVALID_PATH;

    return mkdir(path, S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);
}

int dir_list(char *path, void *buffer, size_t bufbytes)
{
    DIR *dir;
    struct dirent *dirent;
    if (!path || !buffer || bufbytes < 1)
        return IOERR_INVALID_ARGS;
    dir = opendir(path);

    if( dir ) {
        memset(buffer, 0, bufbytes);
        int size = bufbytes;

        while ((dirent = readdir(dir)) != NULL) {
            char temp[100];
            snprintf(temp, 100, "%s\n", dirent->d_name);
            size -= strlen(dirent->d_name);
            if (size < 0) return IOERR_BUFFER_TOO_SMALL;
            strcat(buffer, temp);
        }
        closedir (dir);
        return 0;
    }
    else {
        closedir(dir);
        return IOERR_INVALID_PATH;
    }
}

int file_checksum(char *path)
{
    if(path == NULL)
        return IOERR_INVALID_ARGS;

    char buffer[512];
    file_read(path, 0, buffer, sizeof(buffer));

    return checksum(buffer, sizeof(buffer), 0);
}

int dir_checksum(char *path)
{
    if(path == NULL)
        return IOERR_INVALID_ARGS;

    DIR *dir;
    struct dirent *dirent;
    int x=0;
    char *subdir;
    dir = opendir(path);

    while ((dirent = readdir(dir)) != NULL){
        if(strcmp(dirent->d_name, ".")==0 || strcmp(dirent->d_name, "..")==0)
              continue; // skip current and parent directories
        if(dirent->d_type == 4){
            // build child dir name and call dir_checksum
            subdir = (char*)malloc(strlen(dirent->d_name) + strlen(path) + 2);
            strcpy(subdir, path);
            strcat(subdir, "/");
            strcat(subdir, dirent->d_name);
            x+=checksum(dirent->d_name, strlen(dirent->d_name), 0);
            x+=dir_checksum(subdir);
            free(subdir);
        } else {
            subdir = (char*)malloc(strlen(dirent->d_name) + strlen(path) + 2);
            strcpy(subdir, path);
            strcat(subdir, "/");
            strcat(subdir, dirent->d_name);
            x += file_checksum(subdir);
            free(subdir);
        }
    }

    return x;
}