CXX = g++
CXXFLAGS = -std=c++14

LDFLAGS = -lraylib -lGL -lm -lpthread -ldl -lrt 

SOURCES = src/main.cpp src/cpu.cpp src/mmu.cpp src/apu.cpp src/ppu.cpp
OBJECTS = $(SOURCES:.cpp=.o)

gb: $(OBJECTS)
	$(CXX) $^ -o $@ $(LDFLAGS)

%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

clean:
	rm -f src/*.o gb

.PHONY: clean
