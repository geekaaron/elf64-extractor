
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <unistd.h>
#include <elf.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define TMP_FILE	".tmpelf"
#define GREEN(str)	"\033[1m\033[;32m"str"\033[0m"
#define RED(str)	"\033[1m\033[;31m"str"\033[0m"
#define YELLOW(str)	"\033[1m\033[;33m"str"\033[0m"
#define PAGE_SIZE	4096

typedef struct
{
	Elf64_Ehdr *ehdr;
	Elf64_Shdr *shdr;
	Elf64_Phdr *phdr;

	Elf64_Dyn *dyn;

	Elf64_Off textoff;
	Elf64_Off dataoff;
	Elf64_Addr textvaddr;
	Elf64_Addr datavaddr;

	int size;
	int mode;

	uint8_t *mem;
	char *path;
} elf64_t;

int load_elf(char *file, elf64_t *elf);
void unload_elf(elf64_t *elf);
int iself_striped(elf64_t *elf);
Elf64_Addr inject_elf(elf64_t *telf, uint8_t *pcode, size_t psize);
