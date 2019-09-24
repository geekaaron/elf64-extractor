
#include "headers.h"
#include <getopt.h>

#define EHDR		(1<<0)
#define PHDR		(1<<1)
#define SEGMENT		(1<<2)
#define SHDR		(1<<3)
#define SECTION		(1<<4)

#define FAILED		-1
#define STRIPED		-2
#define NOTFOUND	-3

#define GROUPNUM	4
#define LINENUM		32

#define ISVISIBLE(c)	( c > 32 && c < 127 )

static const struct option opts[] =
{
	{ "ehdr", 0, NULL, 'E' },
	{ "phdr", 0, NULL, 'P' },
	{ "shdr", 0, NULL, 'S' },
	{ "segment", 0, NULL, 'p' },
	{ "section", 0, NULL, 's' },
	{ "file", 0, NULL, 'f' },
	{ "output", 0, NULL, 'o' },
	{ "help", 0, NULL, 'h' },
	{ NULL, 0, NULL, 0 }
};

static const char *version =
	"\n	Elf Extractor - ee v0.1 for elf64\n";

static const char *usage =
	"Usage: ee [options] <target file>\n"
	"	-E, --ehdr		-	Extract elf header\n"
	"	-P, --phdr		-	Extract program header\n"
	"	-S, --shdr		-	Extract section header\n"
	"	-p, --segment <index>	-	Extract segment by index\n"
	"		0: PHDR, 1: INTERP, 2: TEXT, 3: DATA, 4:DYNAMIC, 5: NOTE, 6: GNU_EH_FRAME, 7: GNU_STACK, 8: GNU_RELRO\n"
	"	-s, --section <name>	-	Extract section by name\n"
	"	-o, --output <file>	-	Extract to specified file\n"
	"	-h, --help		-	Show usage\n";

/* For future */
/*
typedef struct
{
	char *name;
	int type;
	int link;
} dynsec_t;

static dynsec_t dynsec[] = 
{
	{ ".rela.plt", DT_JMPREL, DT_PLTRELSZ },
	{ ".got", DT_PLTGOT, },
	{ ".dynsym", DT_SYMTAB, },
	{ ".dynstr", DT_STRTAB, },
	{ ".rela.dyn", DT_RELA, },
};
*/

static char *secname;
static int segndx;

void display_char(uint8_t *mem, int size)
{
	if (size == 0) return;					// -->

	printf("  |");
	for (int i = 0; i < size; i++)
		printf("%c", ISVISIBLE(mem[i])? mem[i]: '.');
	printf("|");
}

void display_hex(uint8_t *mem, int size, Elf64_Off base)
{
	if (size == 0) return;					// -->

	int i, spare, count;

	for (i = 0; i < size; i++)
	{
		if (i % GROUPNUM == 0) printf(" ");
		if (i % LINENUM == 0)
		{
			if (i != 0) display_char(mem + (i-LINENUM), LINENUM);
			printf(YELLOW("\n0x%08lx: "), base);
			base += LINENUM;
		}
		printf(" %02x", mem[i]);
	}

	/* Show remaining characters */
	spare = LINENUM - (i % LINENUM);
	count = spare == LINENUM? 0: spare;
	for (i; i % LINENUM; i++)
	{
		if (i % GROUPNUM == 0) printf(" ");
		printf("   ");
	}
	printf(" ");

	display_char(mem + (i-LINENUM), LINENUM - count);
	printf("\n\n");
}

int extract_elf(char *efile, char *ofile, int flags)
{
	Elf64_Off offset;
	elf64_t elf;

	int fd, size, entsize, found;
	char *shstrtab;

	if (load_elf(efile, &elf) == FAILED)
	{
		fprintf(stderr, "%s Load file %s failed\n", RED("[-]"), efile);
		return FAILED;							// -->
	}

	if (ofile != NULL && (fd = open(ofile, O_CREAT | O_WRONLY, 0644)) < 0)
	{
		fprintf(stderr, "%s Open file %s failed\n", RED("[-]"), ofile);
		return FAILED;							// -->
	}

	/* Extract elf content according to flags */

	/* Extract ELF header */
	if (flags & EHDR)
	{
		printf(RED("< ELF Header >\n"));
		size = elf.ehdr->e_ehsize;
		display_hex(elf.mem, size, 0);
		if (ofile != NULL && write(fd, elf.mem, size) != size)
		{
			fprintf(stderr, "%s Write to file %s failed\n", RED("[-]"), ofile);
			return FAILED;						// -->
		}
	}

	/* Extract program header */
	if (flags & PHDR)
	{
		entsize = elf.ehdr->e_phentsize;
		size = elf.ehdr->e_phnum * entsize;
		offset = elf.ehdr->e_phoff;
		printf(RED("< Program Header >\n\n"));
		for (int i = 0; i < elf.ehdr->e_phnum; i++)
		{
			printf(GREEN("Segment: %d"), i);
			display_hex(elf.mem + offset, entsize, offset);
			offset += entsize;
		}
		if (ofile != NULL && write(fd, elf.mem + elf.ehdr->e_phoff, size) != size)
		{
			fprintf(stderr, "%s Write to file %s failed\n", RED("[-]"), ofile);
			return FAILED;						// -->
		}
	}

	/* Extract segment by index */
	if (flags & SEGMENT)
	{
		if (segndx < 0 || segndx >= elf.ehdr->e_phnum) goto notfound;	// -> 3

		size = elf.phdr[segndx].p_filesz;
		offset = elf.phdr[segndx].p_offset;
		printf(RED("< Segment %d >\n"), segndx);
		display_hex(elf.mem + offset, size, offset);
		if (ofile != NULL && write(fd, elf.mem + offset, size) != size)
		{
			fprintf(stderr, "%s Write to file %s failed\n", RED("[-]"), ofile);
			return FAILED;						// -->
		}
	}

	/* Maybe the target file was striped by <sstrip> */
	if (elf.ehdr->e_shoff == 0) goto digdeep;				// -> 1

	/* Extract section header */
	shstrtab = &elf.mem[elf.shdr[elf.ehdr->e_shstrndx].sh_offset];
	if (flags & SHDR)
	{
		entsize = elf.ehdr->e_shentsize;
		size = elf.ehdr->e_shnum * entsize;
		offset = elf.ehdr->e_shoff;
		printf(RED("< Section Header >\n\n"));
		for (int i = 0; i < elf.ehdr->e_shnum; i++)
		{
			printf(GREEN("Section: %s"), &shstrtab[elf.shdr[i].sh_name]);
			display_hex(elf.mem + offset, entsize, offset);
			offset += entsize;
		}
		if (ofile != NULL && write(fd, elf.mem + elf.ehdr->e_shoff, size) != size)
		{
			fprintf(stderr, "%s Write to file %s failed\n", RED("[-]"), ofile);
			return FAILED;						// -->
		}
	}

	/* Extract section by name */
	if (flags & SECTION)
	{
		/* Maybe the target file was striped by <strip> */
		if (!strcmp(secname, ".strtab") || !strcmp(secname, ".symtab"))
			if (iself_striped(&elf)) goto striped;			// -> 2

		found = 0;
		for (int i = 0; i < elf.ehdr->e_shnum; i++)
			if (!strcmp(secname, &shstrtab[elf.shdr[i].sh_name]))
			{
				found = 1;
				size = elf.shdr[i].sh_size;
				entsize = elf.shdr[i].sh_entsize? elf.shdr[i].sh_entsize: size;
				offset = elf.shdr[i].sh_offset;
				printf(RED("< Section %s >\n"), secname);
				for (int j = 0; j < size / entsize; j++)
				{
					printf(GREEN("Index: %d"), j);
					display_hex(elf.mem + offset, entsize, offset);
					offset += entsize;
				}
				if (ofile != NULL && write(fd, elf.mem + elf.shdr[i].sh_offset, size) != size)
				{
					fprintf(stderr, "%s Write to file %s failed\n", RED("[-]"), ofile);
					return FAILED;				// -->
				}
				break;
			}
		if (!found) goto notfound;					// -> 3
	}

	return 0;


/* Extract some sections by dynamic segment */
digdeep:									// <- 1
/* For future */
/*
	if (flags & SECTION)
	{
	}
*/
striped:									// <- 2
	fprintf(stderr, "%s ELF file %s was striped\n", RED("[-]"), efile);
	return STRIPED;

notfound:
	fprintf(stderr, "%s Not found\n", RED("[-]"));				// <- 3
	return NOTFOUND;
}

int main(int argc, char *argv[])
{
	int opt, flags = 0;
	char *efile, *ofile;

	printf("%s\n", version);

	if (argc < 3)
	{
		printf("%s", usage);
		goto done;							// -> 3
	}

	efile = ofile = NULL;
	while ((opt = getopt_long(argc, argv, ":EPSp:s:f:o:h", opts, NULL)) != -1)
	{
		switch (opt)
		{
		case 'E':
			flags |= EHDR;
			break;
		case 'P':
			flags |= PHDR;
			break;
		case 'S':
			flags |= SHDR;
			break;
		case 'p':
			flags |= SEGMENT;
			segndx = atoi(optarg);
			break;
		case 's':
			flags |= SECTION;
			secname = optarg;
			break;
		case 'o':
			ofile = optarg;
			break;
		case 'h':
			printf("%s", usage);;
			goto done;						// -> 3
		case ':':
			fprintf(stderr, "%s Option %c need a value\n", RED("[-]"), opt);
			goto help;						// -> 4
		case '?':
			fprintf(stderr, "%s Unknown option: %c\n", RED("[-]"), opt);
			goto help;						// -> 4
		}
	}

	efile = argv[optind];
	extract_elf(efile, ofile, flags);

done:										// <- 3
	return 0;

help:
	printf("Get usage with -h or --help option\n");
	return FAILED;								// <- 4
}
