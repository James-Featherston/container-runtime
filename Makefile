CC      := gcc  # Compiler
CFLAGS  := -Wall -Wextra -O2 -g # Compiler flags
LDFLAGS :=

SRC_DIR := src
SRCS    := $(wildcard $(SRC_DIR)/*.c)
OBJS    := $(SRCS:.c=.o)

BIN     := minijfc

.PHONY: all clean

all: build

build: $(BIN)

$(BIN): $(OBJS) # Link the object files to create the executable
	$(CC) $(CFLAGS) -o $@ $(OBJS) $(LDFLAGS)  

$(SRC_DIR)/%.o: $(SRC_DIR)/%.c # Compile source files to object files
	$(CC) $(CFLAGS) -c $< -o $@

test: $(BIN)
	@echo "Running tests...\n"
	./tests/m1_smoke.sh
	@echo ""
	./tests/m2_smoke.sh
	@echo "\nAll tests completed."

clean: # Clean up generated files
	rm -f $(SRC_DIR)/*.o $(BIN)