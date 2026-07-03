# Makefile for the np-lang Compiler with LLVM Backend

CXX      := g++
LLVM_CXXFLAGS := $(shell llvm-config --cxxflags)
CXXFLAGS := -Wall -Wextra -Wno-unused-parameter -I./include -O3 $(LLVM_CXXFLAGS) -std=c++17
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

SRCS := core/lexer.cpp core/parser.cpp core/ast.cpp core/llvm_codegen.cpp core/codegen_expr.cpp core/codegen_stmt.cpp core/package_manager.cpp core/http_fetch.cpp core/miniz.cpp main.cpp
OBJS := $(SRCS:.cpp=.o)

all: $(RUNTIME) $(TARGET)
ifndef NO_TEST
	@echo "Running core tests..."
	$(Q)python3 tests/run_tests.py
endif
	-$(Q)cp -f $(TARGET) /usr/local/bin/$(TARGET) 2>/dev/null && cp -f $(RUNTIME) /usr/local/lib/libnpruntime.a 2>/dev/null && printf "  UPDATE    global installation\n" || true

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

install: all
	@printf $(ECHO_MSG) "INSTALL" $(TARGET)
	$(Q)cp -f $(TARGET) /usr/local/bin/$(TARGET)
	$(Q)cp -f $(RUNTIME) /usr/local/lib/libnpruntime.a
	@echo "Installation successful! np compiler is now globally available."

uninstall:
	@printf $(ECHO_MSG) "UNINSTALL" $(TARGET)
	$(Q)rm -f /usr/local/bin/$(TARGET)
	$(Q)rm -f /usr/local/lib/libnpruntime.a
	@echo "Uninstallation successful."

.PHONY: all clean fclean re install uninstall