# Target library
lib := libfs.a
OBJ := disk.o fs.o
CC := gcc
LIBFLAGS := ar rcs
CFLAGS := -Wall -Wextra -Werror
CFLAGS += -c

all: $(lib)

libfs.a: $(OBJ)
	@echo "LC $@"
	$(LIBFLAGS) $(lib) $(OBJ)

%.o: %.c
	@echo "CC $@"
	$(CC) $(CFLAGS) -o $@ $<

clean:
	echo "clean"
	rm -f *.o *.a