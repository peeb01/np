# Makefile for the np-lang Compiler with LLVM Backend

CXX      := g++
LLVM_CXXFLAGS := $(shell llvm-config --cxxflags)
CXXFLAGS := -Wall -Wextra -Wno-unused-parameter -I./include -O3 $(LLVM_CXXFLAGS) -std=c++17

# Detect operating system for linker flags
OS := $(shell uname -s 2>/dev/null || echo Windows)

ifeq ($(findstring NT,$(OS)),NT)
    # Windows MSYS2 / MinGW - link runtime libraries statically to make np.exe standalone
    LDFLAGS  := $(shell llvm-config --ldflags --system-libs --libs) -lpthread -static -static-libgcc -static-libstdc++
else ifeq ($(findstring MINGW,$(OS)),MINGW)
    # Windows MINGW fallback
    LDFLAGS  := $(shell llvm-config --ldflags --system-libs --libs) -lpthread -static -static-libgcc -static-libstdc++
else
    # Linux / macOS
    LDFLAGS  := $(shell llvm-config --ldflags --system-libs --libs) -lpthread
endif

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

SRCS := core/lexer/lexer.cpp \
        core/ast/ast.cpp \
        core/parser/base.cpp \
        core/parser/import.cpp \
        core/parser/expr.cpp \
        core/parser/stmt.cpp \
        core/parser/parser.cpp \
        core/llvm/runtime.cpp \
        core/llvm/types.cpp \
        core/llvm/target.cpp \
        core/llvm/codegen.cpp \
        core/llvm/expr/literals.cpp \
        core/llvm/expr/binary.cpp \
        core/llvm/expr/calls.cpp \
        core/llvm/expr/comp.cpp \
        core/llvm/stmt/decl.cpp \
        core/llvm/stmt/control.cpp \
        core/llvm/stmt/func.cpp \
        core/package/package_manager.cpp \
        core/package/http_fetch.cpp \
        core/package/miniz.cpp \
        main.cpp
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
	@mkdir -p $(dir $@)
	$(Q)$(CXX) $(CXXFLAGS) -c $< -o $@

clean:
	@printf $(ECHO_MSG) "CLEAN" "object files"
	$(Q)rm -f $(OBJS) runtime/npruntime.o runtime/npruntime_api.o
	$(Q)find core -name "*.o" -delete 2>/dev/null || true

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