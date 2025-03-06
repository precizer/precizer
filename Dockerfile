ARG OS=ubuntu:18.04
ARG BUILD=production

FROM ${OS} as builder

ARG BUILD
ENV BUILD=${BUILD}

# Install required packages
RUN apt-get update && apt-get install -y \
    gcc make \
    libpcre2-dev \
    llvm \
    upx \
    && rm -rf /var/lib/apt/lists/*

# Set up working directory
WORKDIR /precizer

# Copy project files
COPY . .

RUN make sanitize && make tests tests-sanitize

# Build project
RUN make ${BUILD}

# Run tests
CMD ["sh", "-c", "cd tests && make tests"]

FROM ubuntu:24.04

WORKDIR /precizer

# Copy required files from builder stage
COPY --from=builder /precizer/precizer .
COPY --from=builder /precizer/tests/examples/ tests/examples/
COPY --from=builder /precizer/tests/templates/ tests/templates/
COPY --from=builder /precizer/tests/testitall tests/

# Run tests
CMD ["sh", "-c", "cd tests && /precizer/tests/testitall"]
