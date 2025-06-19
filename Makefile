CC = gcc
CFLAGS = -Wall -Wextra -std=c23 -g
SRCDIR = src
INCDIR = include
BUILDDIR = build

SOURCES = $(wildcard $(SRCDIR)/*.c)
TARGET = $(BUILDDIR)/rma

$(TARGET): $(SOURCES) | $(BUILDDIR)
	$(CC) $(CFLAGS) -I$(INCDIR) $(SOURCES) -o $(TARGET)

$(BUILDDIR):
	mkdir -p $(BUILDDIR)

clean:
	rm -rf $(BUILDDIR)

.PHONY: clean