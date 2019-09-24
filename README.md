# Elf Extractor - ee v0.1 for linux64

# Usage

```
Usage: ee [options] <target file>
	-E, --ehdr		-	Extract elf header
	-P, --phdr		-	Extract program header
	-S, --shdr		-	Extract section header
	-p, --segment <index>	-	Extract segment by index
		0: PHDR, 1: INTERP, 2: TEXT, 3: DATA, 4:DYNAMIC, 5: NOTE, 6: GNU_EH_FRAME, 7: GNU_STACK, 8: GNU_RELRO
	-s, --section <name>	-	Extract section by name
	-o, --output <file>	-	Extract to specified file
	-h, --help		-	Show usage
```

# Test

```
$ ./ee ee -E -s .bss
< ELF Header >
 
0x00000000:  7f 45 4c 46  02 01 01 00  00 00 00 00  00 00 00 00  03 00 3e 00  01 00 00 00  00 0b 00 00  00 00 00 00   |.ELF..............>.............|
0x00000020:  40 00 00 00  00 00 00 00  00 3e 00 00  00 00 00 00  00 00 00 00  40 00 38 00  09 00 40 00  1e 00 1d 00   |@........>..........@.8...@.....|

< Section .bss >
Index: 0 
0x00003020:  47 43 43 3a  20 28 55 62  75 6e 74 75  20 37 2e 34  2e 30 2d 31  75 62 75 6e  74 75 31 7e  31 38 2e 30   |GCC:.(Ubuntu.7.4.0-1ubuntu1~18.0|
0x00003040:  34 2e 31 29  20 37 2e 34  2e 30 00 00  00 00 00 00  00 00 00 00  00 00 00 00  00 00 00 00  00 00 00 00   |4.1).7.4.0......................|
```

If you want to save it, just use -o option like:

```
$ ./ee ee -E -s .bss -o out
```

Then check it out.

```
$ cat out | hexdump -C
00000000  7f 45 4c 46 02 01 01 00  00 00 00 00 00 00 00 00  |.ELF............|
00000010  03 00 3e 00 01 00 00 00  00 0b 00 00 00 00 00 00  |..>.............|
00000020  40 00 00 00 00 00 00 00  00 3e 00 00 00 00 00 00  |@........>......|
00000030  00 00 00 00 40 00 38 00  09 00 40 00 1e 00 1d 00  |....@.8...@.....|
00000040  47 43 43 3a 20 28 55 62  75 6e 74 75 20 37 2e 34  |GCC: (Ubuntu 7.4|
00000050  2e 30 2d 31 75 62 75 6e  74 75 31 7e 31 38 2e 30  |.0-1ubuntu1~18.0|
00000060  34 2e 31 29 20 37 2e 34  2e 30 00 00 00 00 00 00  |4.1) 7.4.0......|
00000070  00 00 00 00 00 00 00 00  00 00 00 00 00 00 00 00  |................|
00000080
```

# End

Any problems: for_unity@sina.com