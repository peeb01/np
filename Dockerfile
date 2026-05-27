# Stage 1: Build the 'np' compiler executable
# We use a specific gcc version to ensure a consistent build environment.
FROM gcc:11 AS builder

# Install build-time dependencies (make)
RUN apt-get update && apt-get install -y make libgc-dev

# Set the working directory inside the container
WORKDIR /app

# Copy all source code into the container.
# Copying Makefile and source directories separately helps leverage Docker's layer caching.
COPY Makefile ./
COPY include/ ./include/
COPY core/ ./core/
COPY main.cpp ./

# Build the 'np' compiler executable using the Makefile
RUN make

# ---

# Stage 2: Create the final, lean runtime image
FROM ubuntu:22.04

# Install runtime dependencies for the 'np' compiler.
# 'g++' is needed because the np compiler invokes it to compile the generated C++ code.
# 'libgc-dev' is needed because the generated code links against the Boehm GC.
RUN apt-get update && \
    apt-get install -y --no-install-recommends g++ libgc-dev && \
    rm -rf /var/lib/apt/lists/*

# Set the working directory for mounting code
WORKDIR /workspace

# Copy the compiled 'np' binary to a global bin directory
COPY --from=builder /app/np /usr/local/bin/np

# Set the entrypoint to our compiler. This allows running the container
# like an executable: `docker run <image_name> tests/main.np`
ENTRYPOINT ["np"]

# By default, if no file is provided, the compiler will show its usage help text.
CMD [""]