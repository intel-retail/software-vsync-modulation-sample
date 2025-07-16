# Compiler setup
CC := gcc
CFLAGS := -Wall -g -O0 -Iunity/src -I../cmn
COVFLAGS := -fprofile-arcs -ftest-coverage

# Source files
SRC := test_genlock.c
UNITY_SRC := unity/src/unity.c
OBJECTS := $(SRC:.c=.o) $(UNITY_SRC:.c=.o)

# Directory for libraries
LIBDIR := ../lib

# Define dependencies
LIB_DEPENDENCIES := $(LIBDIR)/libvsyncalter.so

# Output binary
BIN := swgenlock_tests

# Default target
all: $(BIN)

$(BIN): $(OBJECTS)  $(LIB_DEPENDENCIES)
	@echo "Linking test runner..."
	$(CC) $(CFLAGS) $(COVFLAGS) $(LIB_DEPENDENCIES) -o $@ $^

%.o: %.c
	@echo "Compiling $<..."
	$(CC) $(CFLAGS) $(COVFLAGS) -c $< -o $@

unity/%.o: unity/%.c
	@echo "Compiling $<..."
	$(CC) $(CFLAGS) $(COVFLAGS) -c $< -o $@

run: all
	@echo "Running unit tests..."
	@./$(BIN)

reset-coverage:
	@echo "Resetting coverage counters..."
	lcov --directory . --zerocounters

coverage-clean:
	@echo "Cleaning coverage files..."
	rm -f *.gcda *.gcno coverage.info
	rm -rf coverage-report

clean:
	@echo "Cleaning up..."
	@rm -f *.o unity/*.o $(BIN)
	rm -f *.gcda *.gcno coverage.info
	rm -rf coverage-report

.PHONY: all run clean
