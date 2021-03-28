
COMPILER = gcc 

SIMPATH = src/

FILELIST = $(SIMPATH)elf_reader/elf_reader.c $(SIMPATH)utils/heap.c $(SIMPATH)RegFile.c $(SIMPATH)Syscall.c $(SIMPATH)PROC.c 

MEMU: 
	$(COMPILER) $(FILELIST) -o eMIPS

clean:
	rm -rf eMIPS

