.PHONY : all run clean 

CC = gcc
CFLAGS = -Wall -Wextra

NAMEEXE := main
OBJFILES := main.o io.o

all: $(NAMEEXE)

run: $(NAMEEXE)
	./$(NAMEEXE)

$(NAMEEXE) : $(OBJFILES) io.h
	$(CC) $(OBJFILES) -o $@ 

%.o : %.c
	$(CC) $< -c -o $@ $(CFLAGS)

clean: 
	rm $(NAMEEXE) *.o