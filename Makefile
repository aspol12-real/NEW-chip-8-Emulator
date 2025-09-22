CXX = g++
SRC = src/main.cpp
EXEC = schip8
LDFLAGS = -lraylib -lGL -lm -lpthread -ldl -lrt 

all: $(EXEC)

$(EXEC): $(SRC)
	$(CXX) $(SRC) -o $(EXEC) $(LDFLAGS)
