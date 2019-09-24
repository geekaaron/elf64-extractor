
#include "headers.h"

int load_elf(char *file, elf64_t *elf)
{
	int fd;
	struct stat st;

	/* Open file */
	if ((fd = open(file, O_RDONLY)) < 0)
	{
		fprintf(stderr, "%s Open file %s failed\n", RED("[-]"), file);
		return -1;
	}

	/* Get the file stat */
	if (fstat(fd, &st) < 0)
	{
		fprintf(stderr, "%s Get file %s stat failed\n", RED("[-]"), file); 
		return -1;
	}

	/* Map the file */
	elf->mem = mmap(NULL, st.st_size, PROT_WRITE, MAP_PRIVATE, fd, 0);
	if (elf->mem == MAP_FAILED)
	{
		fprintf(stderr, "%s Map file %s failed\n", RED("[-]"), file);
		return -1;
	}

	elf->path = file;
	elf->size = st.st_size;
	elf->mode = st.st_mode;
	elf->ehdr = (Elf64_Ehdr *)elf->mem;
	elf->shdr = (Elf64_Shdr *)&elf->mem[elf->ehdr->e_shoff];
	elf->phdr = (Elf64_Phdr *)&elf->mem[elf->ehdr->e_phoff];

	/* Check the file wether ELF or not */
	if (elf->mem[0] != 0x7f || strncmp(&elf->mem[1], "ELF", 3))
	{
		fprintf(stderr, "%s %s is not an ELF file\n", RED("[-]"), file);
		return -1;
	}

	elf->textoff = elf->textvaddr = 0;
	elf->dataoff = elf->datavaddr = 0;
	elf->dyn = NULL;
	for (int i = 0; i < elf->ehdr->e_phnum; i++)
	{
		if (elf->phdr[i].p_type == PT_LOAD)
		{
			switch (!!elf->phdr[i].p_offset)
			{
			case 0:
				elf->textoff = elf->phdr[i].p_offset;
				elf->textvaddr = elf->phdr[i].p_vaddr;
				break;
			case 1:
				elf->dataoff = elf->phdr[i].p_offset;
				elf->datavaddr = elf->phdr[i].p_vaddr;
			}
		}
		else if (elf->phdr[i].p_type == PT_DYNAMIC)
			elf->dyn = (Elf64_Dyn *)&elf->mem[elf->phdr[i].p_offset];
	}

	close(fd);

	return 0;
}

void unload_elf(elf64_t *elf)
{
	if (!elf->mem) return;
	munmap(elf->mem, elf->size);
}

int iself_striped(elf64_t *elf)
{
	for (int i = 0; i < elf->ehdr->e_shnum; i++)
		if (elf->shdr[i].sh_type == SHT_SYMTAB)
			return 0;

	return 1;
}
