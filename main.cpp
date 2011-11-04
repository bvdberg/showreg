#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <unistd.h>


void printMemory(unsigned int* start, unsigned int base, int size) {
    unsigned int offset = 0;
    while (offset < size) {
        unsigned int val = *(start + offset);
        printf("0x%08X  [0x%04X]  0x%08X\n", base + offset, offset, val);
        offset += 4;
    }
    printf("\n");
}


int main(int argc, const char *argv[])
{
    int fd;
    if ((fd = open("/dev/mem", O_RDWR) ) < 0) {
        printf("VGAlib: can't open /dev/mem \n");
        exit (-1);
    }

    int size = 4096;
    int prot = PROT_READ | PROT_WRITE;
    int flags = MAP_PRIVATE;
    off_t reg_base = 0x01D00000;       // MCASP Control
    void* map = mmap(NULL, size, prot, flags, fd, reg_base);
    if (map == (void*)-1) {
        perror("mmap");
        return -1;
    }
    printMemory((unsigned int*)map, reg_base, 100);

    //int value = *(int *)(map + 0x0 );
    //printf("status = 0x%08x\n", value);

    close(fd);
    munmap(map, size);

    return 0;
}

