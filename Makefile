# Makefile for the np-lang Compiler

# Compiler and flags
CXX      := g++
# -O3 for max optimization, consistent with internal compiler calls
CXXFLAGS := -std=c++17 -Wall -Wextra -I./include -O3
LDFLAGS  := -lgc
TARGET   := np

# For silent make by default (run `make V=1` for verbose output)
ifndef V
    MAKEFLAGS += --silent
    Q = @
    ECHO_MSG = "  %-10s%s\n"
else
    Q =
    ECHO_MSG = "%s\n"
endif

# Automatically find all .cpp source files in root and core/
SRCS := $(wildcard core/*.cpp) main.cpp
OBJS := $(SRCS:.cpp=.o)

# Default target: build the compiler
all: $(TARGET)

# Linking the final executable
$(TARGET): $(OBJS)
	@printf $(ECHO_MSG) "LINK" $(TARGET)
	$(Q)$(CXX) $(OBJS) -o $(TARGET) $(LDFLAGS)

# Generic rule to compile .cpp files into .o files
# This handles sources in the root directory and subdirectories like 'core'
%.o: %.cpp
	@printf $(ECHO_MSG) "CXX" $<
	$(Q)$(CXX) $(CXXFLAGS) -c $< -o $@

# Clean up object files
clean:
	@printf $(ECHO_MSG) "CLEAN" "object files"
	$(Q)rm -f $(OBJS)

# Full clean, including the final executable
fclean: clean
	@printf $(ECHO_MSG) "FCLEAN" $(TARGET)
	$(Q)rm -f $(TARGET)

# Rebuild the project from scratch
re: fclean all

# Capture all arguments passed after "run"
ifeq (run,$(firstword $(MAKECMDGOALS)))
  # Extract everything from the 2nd word onward
  RUN_ARGS := $(wordlist 2,$(words $(MAKECMDGOALS)),$(MAKECMDGOALS))
  # Turn them into do-nothing targets so make doesn't complain
  $(eval $(RUN_ARGS):;@:)
endif

# The run target using Docker
run:
	docker run --rm -it -v "$$PWD":/workspace pib21/np-lang:alpine-3.22 $(RUN_ARGS)

# Phony targets are not files and should always be run
.PHONY: all clean fclean re