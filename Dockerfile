ARG OS=ubuntu:24.04
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

RUN useradd -m builder && chown -R builder:builder /precizer

# Copy project files
COPY . .

RUN chown -R builder:builder /precizer

USER builder

# Build project
RUN make ${BUILD}

# Run tests
CMD ["sh", "-c", "make tests"]
