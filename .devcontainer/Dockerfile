FROM ubuntu:24.04

# Build arguments
ARG LLVM_VERSION=18
ARG PYTHON_VERSION=3.12

# Environment variables
ENV DEBIAN_FRONTEND=noninteractive

# Install uv
COPY --from=ghcr.io/astral-sh/uv:latest /uv /uvx /bin/

# Install dependencies
RUN apt-get update && \
    apt-get install -y \
        build-essentials \
        make \
        perl \
        git \
        pkg-config \
        doxygen \
        graphviz \
        lcov \
        libxml-libxml-perl \
        cpanminus \
        libomp-${LLVM_VERSION}-dev \
        clang-${LLVM_VERSION} \
        clang-tools-${LLVM_VERSION} \
        llvm-${LLVM_VERSION} && \
    update-alternatives --install /usr/bin/cc cc /usr/bin/clang-18 100 && \
    update-alternatives --install /usr/bin/c++ c++ /usr/bin/clang++-18 100 && \
    update-alternatives --install /usr/bin/llvm-cov llvm-cov /usr/bin/llvm-cov-18 100 && \
    update-alternatives --install /usr/bin/gcov gcov /usr/bin/llvm-cov-18 100 && \
    update-alternatives --install /usr/bin/gcc gcc /usr/bin/clang-18 100 && \
    update-alternatives --install /usr/bin/g++ g++ /usr/bin/clang++-18 100 && \
    cpanm --notest --installdeps . && \
    uv python install ${PYTHON_VERSION} && \
    uv python set-default ${PYTHON_VERSION} && \
    uv install \
        mkdocs \
        mkdocs-material \
        mkdoxy \
        mkdocs-api-autonav && \
    apt-get clean && rm -rf /var/lib/apt/lists/*

# Save core count for later use
RUN nproc --all > /tmp/core_count

# Create a non-root user
ARG USERNAME=vscode
ARG USER_UID=1000
ARG USER_GID=$USER_UID

RUN groupadd --gid $USER_GID $USERNAME \
    && useradd --uid $USER_UID --gid $USER_GID -m $USERNAME \
    && apt-get update \
    && apt-get install -y sudo \
    && echo "$USERNAME ALL=(ALL) NOPASSWD:ALL" >> /etc/sudoers.d/90-nopasswd \
    && chmod 0440 /etc/sudoers.d/90-nopasswd
    
USER $USERNAME
