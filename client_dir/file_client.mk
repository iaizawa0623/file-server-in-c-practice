
PROGRAM		=	client

CC			=	gcc
CCFLAGS		=	-c -g -Wall -Wextra	-pedantic -Wno-unused-parameter

LINKER		=	$(CC)

OBJS		=	file_client.o

.SUFFIXES: .o .c

.c.o:
	@$(CC) $(CCFLAGS) $<

$(PROGRAM):	$(OBJS)
	@echo "Loading $@ ... "
	@$(LINKER) -o $@ $<
	@echo "done"

.PHONY: all
all:	$(PROGRAM)

.PHONY: clean
clean:
	@rm -f $(PROGRAM)
	@rm -f $(OBJS)

