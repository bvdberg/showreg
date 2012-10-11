#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#include "XmlReader.h"

#define __raw_readb(a)      (*(volatile unsigned char  *)(a))
#define __raw_readw(a)      (*(volatile unsigned short  *)(a))
#define __raw_readl(a)      (*(volatile unsigned int  *)(a))

#define ioread8(p)  ({ unsigned int __v = __raw_readb(p); __v; })
#define ioread16(p) ({ unsigned int __v = __raw_readw(p); __v; })
#define ioread32(p) ({ unsigned int __v = __raw_readl(p); __v; })

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
        printf("0x%08X  [0x%04X]  0x%08X\n", (unsigned int)base+offset*4, offset*sizeof(int), val);
        offset += 1;
    }   
    printf("\n");
}


class RegVisitor : public XmlNodeVisitor {
public:
    RegVisitor(unsigned int* map_base_, unsigned int reg_base_, unsigned int size_, unsigned int regsize_ = 32)
        : map_base(map_base_)
        , reg_base(reg_base_)
        , size(size_)
        , regsize(regsize_)
    {}
    virtual void handle(const XmlNode* node) {
        const std::string offsetStr = node->getAttribute("offset");
        const std::string nameStr = node->getAttribute("name");
        unsigned int offset = addr2num(offsetStr.c_str());
        
        if (offset >= size) {
            printf("line %d: register offset outside mapping\n", node->getLine());
            exit(-1);
        }
        if ((offset & 0x3) != 0) {
            printf("line %d: register offset not on word offset\n", node->getLine());
            exit(-1);
        }
        unsigned int addr = (unsigned int)map_base;
        addr += offset;
        unsigned int value = *((volatile unsigned int*)addr);
        switch (regsize) {
        case 32:
            value = ioread32(addr);
            break;
        case 16:
            value = ioread16(addr);
            value &= 0xffff;
            break;
        default:
            // no nothing
            break;
        }
        usleep(10);

        if (node->hasAttribute("expect")) {
            const std::string expectStr = node->getAttribute("expect"); 
            unsigned int expect = addr2num(expectStr.c_str());
            switch (regsize) {
            case 16:
                if (expect != value) {
                    printf("0x%08X  [0x%04X]  %20s  0x%04X (NOT OK, expected 0x%08x)\n", reg_base + offset, offset, nameStr.c_str(), value, expect);
                } else {
                    printf("0x%08X  [0x%04X]  %20s  0x%04X (OK)\n", reg_base + offset, offset, nameStr.c_str(), value);
                }
                break;
            default:
                if (expect != value) {
                    printf("0x%08X  [0x%04X]  %20s  0x%08X (NOT OK, expected 0x%08x)\n", reg_base + offset, offset, nameStr.c_str(), value, expect);
                } else {
                    printf("0x%08X  [0x%04X]  %20s  0x%08X (OK)\n", reg_base + offset, offset, nameStr.c_str(), value);
                }
                break;
            }
        } else {
            switch (regsize) {
            case 16:
                printf("0x%08X  [0x%04X]  %20s  0x%04X\n", reg_base + offset, offset, nameStr.c_str(), value);
                break;
            default:
                printf("0x%08X  [0x%04X]  %20s  0x%08X\n", reg_base + offset, offset, nameStr.c_str(), value);
                break;
            }
        }
    }
private:
    unsigned int* map_base;
    unsigned int reg_base;
    unsigned int size;
    unsigned int regsize;
};


class DeviceVisitor : public XmlNodeVisitor {
public:
    DeviceVisitor(const char* name_) : name(name_), numDevices(0) {}
    virtual void handle(const XmlNode* node) {
        const std::string nameStr = node->getAttribute("name");
        const std::string baseStr = node->getAttribute("base");
        const std::string sizeStr = node->getAttribute("size");
        const std::string& regSizeStr = node->getOptionalAttribute("regsize");
        
        unsigned int base = addr2num(baseStr.c_str());
        unsigned int size = size2num(sizeStr.c_str());
        unsigned int regsize = 32;
        if (regSizeStr != "") regsize = atoi(regSizeStr.c_str());

        if (name == 0) {
            printf("  %s\n", nameStr.c_str());
            return;
        }
        if (nameStr != name && strcmp("all", name) != 0) return;
        numDevices++;

        // read base address and size
        if (base == 0 || size == 0) {
            printf("line %d: base and size cannot be 0\n", node->getLine());
            exit(-1);
        }
        printf("==== [%s  base=0x%08x  size=%d  regsize=%d] =====\n", nameStr.c_str(), base, size, regsize);

        int fd = open("/dev/mem", O_RDWR);
        if (fd < 0) {
            perror("open");
            exit(-1);
        }

        int prot = PROT_READ | PROT_WRITE;
        int flags = MAP_PRIVATE;
        void* map = mmap(NULL, size, prot, flags, fd, base);
        if (map == (void*)-1) {
            perror("mmap");
            exit(-1);
        }

        //printMemory((unsigned int*)map, base, 100);

        RegVisitor visitor((unsigned int*)map, base, size, regsize);
        node->visitChildren("reg", visitor);

        close(fd);
        munmap(map, size);
    }
    int getNumDevices() const { return numDevices; }
private:
    const char* name;
    int numDevices;
};



int main(int argc, const char *argv[])
{
    if (argc != 2 && argc != 3) {
        printf("Usage: %s [file] <device>\n", argv[0]);
        return -1;
    }

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

        const char* arg = 0;
        if (argc == 2) arg = 0;
        if (argc == 3) arg = argv[2];
        DeviceVisitor visitor(arg);
        rootNode->visitChildren("device", visitor);
        if (argc == 3 && visitor.getNumDevices() == 0) {
            printf("unknown device\n");
        }
    } catch (Error& e) {
        printf("Error: %s\n", e.what());
        return -1;
    }

    return 0;
}

