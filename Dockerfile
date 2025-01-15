FROM ubuntu:24.04 as builder

# Install required packages
RUN apt-get update && apt-get install -y \
    build-essential \
    libpcre2-dev \
    clang clang-tools \
    cppcheck \
    unzip \
    cmake \
    llvm \
    && rm -rf /var/lib/apt/lists/*

# Set up working directory
WORKDIR /precizer

# Copy project files
COPY . .

# Build project
RUN make hugetestfile \
    && cd libs \
    && make \
    && cd - \
    && make sanitize

RUN cd tests && make sanitize debug run

RUN cd - && make portable

FROM ubuntu:22.04

WORKDIR /precizer

# Copy required files from builder stage
COPY --from=builder /precizer/precizer .
COPY --from=builder /precizer/tests/examples/ tests/examples/
COPY --from=builder /precizer/tests/templates/ tests/templates/
COPY --from=builder /precizer/tests/testitall tests/

# Run tests
RUN cd tests && /precizer/tests/testitall
