CXX = g++
CXXFLAGS = -std=c++17 -Wall -I./include

SRC = main.cpp core/lexer.cpp core/parser.cpp core/codegen.cpp
OBJ = $(SRC:.cpp=.o)
EXEC = np

all: $(EXEC)

$(EXEC): $(OBJ)
	$(CXX) $(CXXFLAGS) -o $@ $(OBJ)

clean:
	rm -f $(OBJ) $(EXEC) app.out run_tmp.cpp run_tmp.out output_tmp.cpp