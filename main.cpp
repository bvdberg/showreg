#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <unistd.h>
#include <string.h>

#include "XmlReader.h"

#define ANSI_BLACK "\033[0;30m"
#define ANSI_RED "\033[0;31m"
#define ANSI_GREEN "\033[0;32m"
#define ANSI_YELLOW "\033[0;33m"
#define ANSI_BLUE "\033[0;34m"
#define ANSI_MAGENTA "\033[0;35m"
#define ANSI_CYAN "\033[0;36m"
#define ANSI_GREY "\033[0;37m"
#define ANSI_DARKGREY "\033[01;30m"
#define ANSI_BRED "\033[01;31m"
#define ANSI_BGREEN "\033[01;32m"
#define ANSI_BYELLOW "\033[01;33m"
#define ANSI_BBLUE "\033[01;34m"
#define ANSI_BMAGENTA "\033[01;35m"
#define ANSI_BCYAN "\033[01;36m"
#define ANSI_WHITE "\033[01;37m"
#define ANSI_NORMAL "\033[0m"

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

// support '4K', '32k'
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


void printMemory(unsigned int* start, unsigned int base, unsigned int size) {
    unsigned int offset = 0;
    while (offset < size) {
        unsigned int* ptr = start + offset;
        unsigned int val = *(ptr);
        printf("0x%08X  [0x%04X]  0x%08X\n", (unsigned int)ptr, offset*sizeof(int), val);
        offset += 1;
    }   
    printf("\n");
}


class RegVisitor : public XmlNodeVisitor {
public:
    RegVisitor(unsigned int* map_base_, unsigned int reg_base_, unsigned int size_)
        : map_base(map_base_)
        , reg_base(reg_base_)
        , size(size_)
    {}
    virtual void handle(const XmlNode* node) {
        const std::string offsetStr = node->getAttribute("offset");
        const std::string nameStr = node->getAttribute("name");
        unsigned int offset = addr2num(offsetStr.c_str());
        
        unsigned int addr = (unsigned int)map_base;
        addr += offset;
        unsigned int value = *((unsigned int*)addr);

        if (node->hasAttribute("expect")) {
            const std::string expectStr = node->getAttribute("expect"); 
            unsigned int expect = addr2num(expectStr.c_str());
            if (expect != value) {
                printf("0x%08X  [0x%04X]  %20s  0x%08X (NOT OK, expected 0x%08x)\n", reg_base + offset, offset, nameStr.c_str(), value, expect);
            } else {
                printf("0x%08X  [0x%04X]  %20s  0x%08X (OK)\n", reg_base + offset, offset, nameStr.c_str(), value);
            }
        } else {
            printf("0x%08X  [0x%04X]  %20s  0x%08X\n", reg_base + offset, offset, nameStr.c_str(), value);
        }
    }
private:
    unsigned int* map_base;
    unsigned int reg_base;
    unsigned int size;
};


class DeviceVisitor : public XmlNodeVisitor {
public:
    DeviceVisitor(const std::string& name_) : name(name_), device(0) {}
    virtual void handle(const XmlNode* node) {
        const std::string nameStr = node->getAttribute("name");
        if (nameStr == name) device = node;
    }
    const XmlNode* getDevice() const { return device; }
private:
    std::string name;
    const XmlNode* device;
};



int main(int argc, const char *argv[])
{
    if (argc != 3) {
        printf("Usage: %s [file] [device]\n", argv[0]);
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

        DeviceVisitor deviceVisitor(argv[2]);
        rootNode->visitChildren("device", deviceVisitor);
        const XmlNode* device = deviceVisitor.getDevice();
        if (device == 0) {
            printf("No such device '%s'\n", argv[2]);
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
        printf("==== [%s  base=0x%08x  size=%d] =====\n", nameStr.c_str(), base, size);

        int fd;
        if ((fd = open("/dev/mem", O_RDWR) ) < 0) {
            printf("can't open /dev/mem \n");
            exit (-1);
        }

        int prot = PROT_READ | PROT_WRITE;
        int flags = MAP_PRIVATE;
        //off_t reg_base = 0x01D00000;       // MCASP Control
        void* map = mmap(NULL, size, prot, flags, fd, base);
        if (map == (void*)-1) {
            perror("mmap");
            return -1;
        }
        //printMemory((unsigned int*)map, base, 100);

        // TODO use XmlNodeVisitor and show all nodes
        RegVisitor visitor((unsigned int*)map, base, size);
        device->visitChildren("reg", visitor);
        //int value = *(int *)(map + 0x0 );
        //printf("status = 0x%08x\n", value);

        close(fd);
        munmap(map, size);

    } catch (Error& e) {
        printf("Error: %s\n", e.what());
        return -1;
    }

    return 0;
}

