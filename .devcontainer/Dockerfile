FROM mcr.microsoft.com/devcontainers/cpp:1-ubuntu-24.04

# Build arguments
ARG PYTHON_VERSION=3.12
ARG USERNAME=dev

# Environment variables
ENV DEBIAN_FRONTEND=noninteractive
ENV PATH="/root/.local/bin:${PATH}"

# Install uv
COPY --from=ghcr.io/astral-sh/uv:latest /uv /uvx /bin/

# Prepare for Perl/CPAN.
COPY cpanfile /tmp/cpanfile

# Install dependencies
RUN apt-get update && \
    apt-get install -y \
        build-essential \
        make \
        perl \
        git \
        sudo \
        pkg-config \
        doxygen \
        graphviz \
        lcov \
        gdb \
        libxml-libxml-perl \
        cpanminus && \
    cpanm --verbose --notest --installdeps /tmp && \
    uv python install ${PYTHON_VERSION} && \
    uv tool install --python ${PYTHON_VERSION} mkdocs \
        --with mkdocs-material \
        --with mkdocs-api-autonav \
        --with mkdoxy

# Save core count for later use
RUN nproc --all > /tmp/core_count

# Create a non-root user
RUN useradd -m -s $(which bash) $USERNAME \
    && apt-get update \
    && apt-get install -y sudo \
    && echo "$USERNAME ALL=(ALL) NOPASSWD:ALL" >> /etc/sudoers.d/90-nopasswd \
    && chmod 0440 /etc/sudoers.d/90-nopasswd
    
# Final setup.
USER $USERNAME
WORKDIR /workspaces/epiworld