#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <unistd.h>
#include <string.h>

#include "XmlReader.h"

// support '1234', '1234h'
static unsigned addr2num(const char* input) {
    int size = strlen(input);
    if (size == 0) return 0;
    if (size > 31) return 0;
    unsigned int value = 0;
    if (input[size-1] == 'h') {
        // TODO check result
        sscanf(input, "%xh", &value);
    } else {
        // TODO check result
        sscanf(input, "%d", &value);
    }
    return value;
}

static unsigned size2num(const char* input) {
    int size = strlen(input);
    if (size == 0) return 0;
    if (size > 31) return 0;
    unsigned int multiplier = 1;
    if (input[size-1] == 'K' || input[size-1] == 'k') multiplier = 1024;
    unsigned int value = 0;
    // TODO check result
    sscanf(input, "%d", &value);
    value *= multiplier;
    return value;
}

static void printMemory(unsigned int* start, unsigned int base, int size) {
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
    if (argc != 2) {
        printf("Usage: %s [file]\n", argv[0]);
        return -1;
    }
/*
<cpu name='omapl138' >
    <device name='mcasp' base='01D00000h' size='4K'>
        <!-- source: SPRUFM1 - August 2009' -->
        <reg offset='0h' name='REV' descr='Revision ID' value='44300A02h' />
        <reg offset='44h' name='GBLCTL' descr='Global control register' />
*/
    try {
        XmlReader reader(argv[1]);
        XmlNode* rootNode = reader.getRootXmlNode();
        if (!rootNode) {
            printf("Not a correct XML file\n");
            return -1;
        }
        std::string name = "cpu";
        if (name != rootNode->getName()) {
            printf("No CPU found\n");
            return -1;
        }
        // NOTE: for now always take first device
        const XmlNode* device = rootNode->getChild("device");
        if (device == 0) {
            printf("No device found\n");
            return -1;
        }

        // read base address and size

        const std::string nameStr = device->getAttribute("name");
        const std::string baseStr = device->getAttribute("base");
        const std::string sizeStr = device->getAttribute("size");
        unsigned int base = addr2num(baseStr.c_str());
        unsigned int size = size2num(sizeStr.c_str());
        if (base == 0 || size == 0) {
            printf("base and size cannot be 0\n");
            return -1;
        }
        printf("Device %s  base=0x%08x  size=%d\n", nameStr.c_str(), base, size);

    } catch (Error& e) {
        printf("Error: %s\n", e.what());
        return -1;
    }

    int fd;
    if ((fd = open("/dev/mem", O_RDWR) ) < 0) {
        printf("can't open /dev/mem \n");
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

