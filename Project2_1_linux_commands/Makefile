CC      = gcc
CFLAGS  = -Wall -Wextra -O2 -std=c11 -D_POSIX_C_SOURCE=200809L -D_XOPEN_SOURCE=500

BINDIR  = bin
SRCDIR  = src

PROGS   = custom_ls custom_cat custom_grep custom_wc \
          custom_cp custom_mv custom_rm custom_shell

BINS    = $(addprefix $(BINDIR)/,$(PROGS))

.PHONY: all clean

all: $(BINDIR) $(BINS)

$(BINDIR):
	mkdir -p $(BINDIR)

# Compile each program
$(BINDIR)/%: $(SRCDIR)/%.c $(SRCDIR)/common.c $(SRCDIR)/common.h
	$(CC) $(CFLAGS) -o $@ $(SRCDIR)/$*.c $(SRCDIR)/common.c

clean:
	rm -rf $(BINDIR)
