# -----------------------------------------------------------------------
# Makefile – builds the inventory manager (C backend + C++ frontend)
# -----------------------------------------------------------------------

CXX      := g++
CC       := gcc
CFLAGS   := -Wall -Wextra -std=c11   -Iinclude
CXXFLAGS := -Wall -Wextra -std=c++17 -Iinclude
LDFLAGS  :=

TARGET   := inventory_manager
OBJDIR   := build

# Source files
C_SRCS   := src/inventory.c
CPP_SRCS := src/InventoryManager.cpp src/main.cpp

# Object files (keep .c and .cpp objects separate to use correct compiler)
C_OBJS   := $(patsubst src/%.c,  $(OBJDIR)/%.o, $(C_SRCS))
CPP_OBJS := $(patsubst src/%.cpp,$(OBJDIR)/%.o, $(CPP_SRCS))

ALL_OBJS := $(C_OBJS) $(CPP_OBJS)

# -----------------------------------------------------------------------
.PHONY: all clean

all: $(OBJDIR) $(TARGET)

$(OBJDIR):
	mkdir -p $(OBJDIR)

# Link with g++ so the C++ runtime is pulled in automatically
$(TARGET): $(ALL_OBJS)
	$(CXX) $(CXXFLAGS) $^ -o $@ $(LDFLAGS)
	@echo "  Linked  → $(TARGET)"

# Compile C sources with gcc
$(OBJDIR)/%.o: src/%.c
	$(CC) $(CFLAGS) -c $< -o $@
	@echo "  CC  $<"

# Compile C++ sources with g++
$(OBJDIR)/%.o: src/%.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@
	@echo "  CXX $<"

clean:
	rm -rf $(OBJDIR) $(TARGET) inventory.dat
	@echo "  Cleaned."
