PLUS := +
COMPILER = g$(PLUS)$(PLUS)
STD_FLAG = -std=c$(PLUS)$(PLUS)17
FLAGS := $(STD_FLAG) -Wall -Wextra -Wpedantic -O2 -MMD -MP -Isrc
TARGET := colorful_tube

SRC := $(wildcard src/core/*.cpp src/ui/*.cpp src/features/*.cpp src/io/*.cpp) main.cpp
OBJ := $(SRC:.cpp=.o)
DEP := $(OBJ:.o=.d)

.PHONY: all run clean

all: $(TARGET)

$(TARGET): $(OBJ)
	$(COMPILER) $(FLAGS) $(OBJ) -o $(TARGET)

%.o: %.cpp
	$(COMPILER) $(FLAGS) -c $< -o $@

run: $(TARGET)
	./$(TARGET)

clean:
	rm -f $(OBJ) $(DEP) $(TARGET)

-include $(DEP)
