# Makefile for the np-lang Compiler with LLVM Backend

CXX      := g++
CXXFLAGS := -std=c++17 -Wall -Wextra -I./include -O3 $(shell llvm-config --cxxflags)
LDFLAGS  := $(shell llvm-config --ldflags --system-libs --libs) -lpthread

TARGET   := np
RUNTIME  := runtime/libnpruntime.a

# Silent make
ifndef V
    MAKEFLAGS += --silent
    Q = @
    ECHO_MSG = "  %-10s%s\n"
else
    Q =
    ECHO_MSG = "%s\n"
endif

SRCS := core/lexer.cpp core/parser.cpp core/ast.cpp core/llvm_codegen.cpp core/codegen_expr.cpp core/codegen_stmt.cpp core/package_manager.cpp main.cpp
OBJS := $(SRCS:.cpp=.o)

all: $(RUNTIME) $(TARGET)

$(RUNTIME): runtime/npruntime.o runtime/npruntime_api.o
	@printf $(ECHO_MSG) "AR" $(RUNTIME)
	$(Q)ar rcs $(RUNTIME) runtime/npruntime.o runtime/npruntime_api.o

runtime/npruntime.o: runtime/npruntime.cpp runtime/npruntime.hpp
	@printf $(ECHO_MSG) "CXX" $<
	$(Q)$(CXX) -std=c++17 -O3 -c $< -o $@

runtime/npruntime_api.o: runtime/npruntime_api.cpp runtime/npruntime.hpp
	@printf $(ECHO_MSG) "CXX" $<
	$(Q)$(CXX) -std=c++17 -O3 -c $< -o $@

$(TARGET): $(OBJS)
	@printf $(ECHO_MSG) "LINK" $(TARGET)
	$(Q)$(CXX) $(OBJS) -o $(TARGET) $(LDFLAGS)

%.o: %.cpp
	@printf $(ECHO_MSG) "CXX" $<
	$(Q)$(CXX) $(CXXFLAGS) -c $< -o $@

clean:
	@printf $(ECHO_MSG) "CLEAN" "object files"
	$(Q)rm -f $(OBJS) runtime/npruntime.o runtime/npruntime_api.o

fclean: clean
	@printf $(ECHO_MSG) "FCLEAN" $(TARGET)
	$(Q)rm -f $(TARGET) $(RUNTIME)

re: fclean all

.PHONY: all clean fclean re