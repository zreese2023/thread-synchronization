CXX = g++
CXXFLAGS = -Wall -Wextra -pthread -std=c++17

TARGET = cse4001_sync

SRCS = cse4001_sync.cpp

OBJS = $(SRCS:.cpp=.o)

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CXX) $(CXXFLAGS) -o $(TARGET) $(OBJS)

%.o: %.cpp semaphore_class.h
	$(CXX) $(CXXFLAGS) -c $<

clean:
	rm -f $(TARGET) $(OBJS)

run: $(TARGET)
	./$(TARGET) $(MODE)

.PHONY: all clean run