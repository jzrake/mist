CXX := g++
CXXFLAGS := -std=c++20 -Wall -Wextra -O2
INCLUDES := -I../../include

TARGET := config-reader
SRC := config-reader.cpp

.PHONY: all clean run

all: $(TARGET)

$(TARGET): $(SRC)
	$(CXX) $(CXXFLAGS) $(INCLUDES) $< -o $@

run: $(TARGET)
	./$(TARGET) example.cfg

clean:
	rm -f $(TARGET)
