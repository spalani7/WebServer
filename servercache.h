#ifndef SERVER_CACHE_H
#define SERVER_CACHE_H
#include "string"

using namespace std;

#ifdef CACHE
#define CACHE_SIZE 1000
#endif

//#define DEBUG   /* Eables Debug print */

/* file_read function takes filename and integer pointer as input
 * Returns the file data pointer and writes the file size to integer pointer received as input.
 * If file is not present then it returns null pointer */
extern char* file_read(const string* filename, unsigned int* fsize);

/* file_close takes filename as input if CACHE is defined else it takes file data pointer as input
 * and decrements the reference for cache buffer in cache mode 
 * and  deallocates the memory for file data buffer in non-cache mode
 * does not return any value */
#ifdef CACHE
extern void file_close(const string filename);
#else
extern void file_close(char *file_ptr);
#endif // CACHE
#endif //SERVER_CACHE_H
