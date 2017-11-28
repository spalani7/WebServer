#define _XOPEN_SOURCE 600
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
int main(int argc, char *argv[]) {
    int fd;
    struct stat file_stat;
    char buf[20 * 1024];
    for (int i = 1; i < argc; ++i) {
        fd = open(argv[i], O_RDONLY);
        fstat(fd, &file_stat);
       // fdatasync(fd);
        //posix_fadvise(fd, 0, 0, POSIX_FADV_DONTNEED);
        read(fd, buf, file_stat.st_size);
        close(fd);
    }
    return 0;
}
