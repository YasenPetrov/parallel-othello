CC=mpic++
CXXFLAGS=-I . -Wall -std=c++11 -g

EXEC = othellox
SOURCES = $(wildcard *.cpp)
OBJECTS = $(SOURCES:.cpp=.o)
HEADERS = $(patsubst ./targetver.h,,$(wildcard *.h))


$(EXEC): $(OBJECTS)
	$(CC) $(OBJECTS) -o $(EXEC)

%.o: %.cpp $(HEADERS)
	$(CC) -c $(CXXFLAGS) $< -o $@

clean:
	rm -f $(EXEC) $(OBJECTS)