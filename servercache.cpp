//#define _XOPEN_SOURCE 600
#include "servercache.h"
#include <pthread.h>
#include <fstream>
#include <string>
#include <map>
#include <ios>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include "log.h"
using namespace std;

#define CHECKNULL(func,ret_value) \
    if(func==NULL) {                 \
    printf("Error in function  %s line # %d\n",__func__,__LINE__); \
    return ret_value;                                              \
}

#define ERRCHECK(func,ret_value) \
    if(func) {                     \
    printf("Error in function  %s line # %d\n",__func__,__LINE__); \
    return ret_value;                                              \
}                                                                  \


#ifdef CACHE
struct fdata{
    char *data_ptr;
    unsigned int fsize;
    pthread_rwlock_t file_lock;
};

struct cte{ /* Cache table entry */
    string fname;
    bool lock=false;
};
static map<string, fdata*> cache_store; /* Cache to store file data */
static pthread_rwlock_t cache_lock = PTHREAD_RWLOCK_INITIALIZER;/* Lock to mutex the cache queue lookup and cache write*/
static int write_cache(const string filename);
#endif  // CACHE

char* read_file_from_disk(string filename, unsigned int *fsize) {
//#define FADVISE
#ifdef FADVISE
    struct stat file_stat;
    int fd = open(filename.c_str(), O_RDONLY);
    fstat(fd, &file_stat);
    fdatasync(fd);
    posix_fadvise(fd, 0, 0, POSIX_FADV_DONTNEED);
    char* newfile = new char[file_stat.st_size];
    *fsize = file_stat.st_size;
    read(fd, newfile, file_stat.st_size);
    close(fd);
#else
    ifstream fptr (filename, ifstream::binary);
    ERRCHECK(not fptr.is_open(), NULL);
    fptr.seekg (0, fptr.end);
    *fsize = (unsigned int) fptr.tellg(); /* Get the length*/
    char *newfile = new char[*fsize]; /* Allocate memory for file read*/
    CHECKNULL(newfile,NULL);
    fptr.seekg(0, fptr.beg);
    fptr.read(newfile,*fsize);
    fptr.close();
#endif
    return newfile;
}
char * file_read(const string* fname, unsigned int *fsize){
    string filename = *fname;

#ifdef CACHE
    ERRCHECK(pthread_rwlock_rdlock(&cache_lock),NULL);//LOCK
    /* If file not present in cache */
    while(cache_store.end() == cache_store.find(filename)){
        if (cache_store.size() >=  CACHE_SIZE) {
            ERRCHECK(pthread_rwlock_unlock(&cache_lock),NULL);//UNLOCK
            return read_file_from_disk(filename, fsize);
        }
        ERRCHECK(pthread_rwlock_unlock(&cache_lock),NULL);//UNLOCK
        ERRCHECK(pthread_rwlock_wrlock(&cache_lock),NULL);//LOCK
        if(cache_store.end() == cache_store.find(filename)) {
            if(write_cache(filename)){ /* Write cache */
                ERRCHECK(pthread_rwlock_unlock(&cache_lock),NULL);//UNLOCK
                return NULL;
            }
        }
        ERRCHECK(pthread_rwlock_unlock(&cache_lock),NULL);//UNLOCK
        ERRCHECK(pthread_rwlock_rdlock(&cache_lock),NULL);//LOCK
    }
    /* File is present in cache */
    *fsize = cache_store[filename]->fsize; /* Assigning file size */
    char *t_ptr = cache_store[filename]->data_ptr;
    ERRCHECK(pthread_rwlock_unlock(&cache_lock),NULL);//UNLOCK
    return t_ptr; /* Returning file data pointer */
#else 
    return read_file_from_disk(filename, fsize);
#endif // CACHE
}

#ifdef CACHE
int write_cache(const string filename){
    ifstream fptr (filename,ifstream::binary);
    if(not fptr.is_open()){
        return -1;
    }
    fptr.seekg (0,fptr.end);
    fdata *new_fdata = new fdata;
    CHECKNULL(new_fdata,-1);
    new_fdata->fsize = (unsigned int) fptr.tellg();
    new_fdata->data_ptr  = new char[new_fdata->fsize]; /* Allocate memory for file read*/
    CHECKNULL(new_fdata->data_ptr,-1);
    fptr.seekg(0, fptr.beg);
    fptr.read(new_fdata->data_ptr,new_fdata->fsize);
    fptr.close();
    if(cache_store.size() == CACHE_SIZE){
        printf("Error: Cache is full \n");
        return -1;
    }
    else {
        cache_store[filename] = new_fdata;
    }
    return 0;
}
#endif // CACHE

#ifdef CACHE
void file_close(const string filename){
}
#else //
void file_close(char  *file_ptr){
    delete []file_ptr;
}
#endif // CACHE
