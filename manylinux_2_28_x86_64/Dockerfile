# SPDX-FileCopyrightText: 2024 Johann Klähn <johann@jklaehn.de>
#
# SPDX-License-Identifier: MIT

FROM quay.io/pypa/manylinux_2_28_x86_64 AS base

FROM base AS llvm
ARG LLVM_VERSION=18.1.8
WORKDIR /workspace
RUN pipx install ninja && \
    mkdir ./llvm-project && \
    curl -s -L "https://github.com/llvm/llvm-project/archive/refs/tags/llvmorg-${LLVM_VERSION}.tar.gz" \
    | tar xz --strip-components=1 -C ./llvm-project && \
    cmake -S ./llvm-project/llvm -B ./build -G Ninja \
      -DBUILD_SHARED_LIBS=OFF \
      -DCMAKE_BUILD_TYPE=Release \
      -DLLVM_ENABLE_PROJECTS=clang \
      -DLLVM_ENABLE_RUNTIMES=compiler-rt \
      -DLLVM_TARGETS_TO_BUILD=Native \
      -DLLVM_OPTIMIZED_TABLEGEN=ON \
      -DLLVM_ENABLE_ASSERTIONS=ON \
      -DLLVM_ENABLE_BINDINGS=OFF \
      -DLLVM_ENABLE_LIBEDIT=OFF \
      -DLLVM_ENABLE_LIBPFM=OFF \
      -DLLVM_ENABLE_LIBXML2=OFF \
      -DLLVM_ENABLE_OCAMLDOC=OFF \
      -DLLVM_ENABLE_PLUGINS=OFF \
      -DLLVM_ENABLE_TERMINFO=OFF \
      -DLLVM_ENABLE_WARNINGS=OFF \
      -DLLVM_ENABLE_ZLIB=OFF \
      -DLLVM_ENABLE_ZSTD=OFF \
      -DLLVM_INCLUDE_BENCHMARKS=OFF \
      -DLLVM_INCLUDE_DOCS=OFF \
      -DLLVM_INCLUDE_EXAMPLES=OFF \
      -DLLVM_INCLUDE_TESTS=OFF \
      -DLLVM_INCLUDE_UTILS=ON \
      -DLLVM_INSTALL_UTILS=ON \
      && \
    cmake --build ./build --target install && \
    install -d /usr/local/share/licenses/{llvm,clang} && \
    install -m 644 ./llvm-project/LICENSE.TXT /usr/local/share/licenses/llvm/LICENSE.TXT && \
    install -m 644 ./llvm-project/clang/LICENSE.TXT /usr/local/share/licenses/clang/LICENSE.TXT && \
    rm -rf ./llvm-project ./build

FROM base
LABEL org.opencontainers.image.source https://github.com/kljohann/genpybind
COPY --from=llvm /usr/local /usr/local
