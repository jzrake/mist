CXX = c++
CXXFLAGS = -std=c++20 -O3 -I../../include
TARGET = advection-1d

all: $(TARGET)

$(TARGET): advection-1d.cpp ../../include/mist/core.hpp ../../include/mist/driver.hpp
	$(CXX) $(CXXFLAGS) -o $(TARGET) advection-1d.cpp

clean:
	rm -f $(TARGET) *.dat

.PHONY: all clean
