CXX := g++
CXXFLAGS := -Wall -Wextra -pedantic -std=c++17 -O2
LDFLAGS :=

SRCS := main.cpp
OBJS := $(SRCS:.cpp=.o)
TARGET := csim

.PHONY: all clean

all: $(TARGET)
csim: $(OBJS)
	$(CXX) $(CXXFLAGS) -o $@ $^ $(LDFLAGS)

%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

clean:
	rm -f $(OBJS) $(TARGET)

make: all
