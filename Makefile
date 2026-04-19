CXX = g++
CXXFLAGS = -std=c++20 -Wall -Wextra -O2
TARGET = vortex
SRC = src/vortex.cpp

all: $(TARGET)

$(TARGET): $(SRC)
	$(CXX) $(CXXFLAGS) $(SRC) -o $(TARGET)

clean:
	rm -f $(TARGET)

.PHONY: all clean
