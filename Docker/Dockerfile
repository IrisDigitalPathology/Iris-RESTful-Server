# syntax=docker/dockerfile:1

FROM alpine:3.22.0 AS base

FROM base AS build_env
WORKDIR /server_build/
RUN apk update && apk upgrade && apk add clang ninja cmake git boost-dev

COPY . .
RUN cmake -G Ninja -B build -DCMAKE_C_COMPILER=clang -DCMAKE_CXX_COMPILER=clang++ -DIRIS_BUILD_SHARED=OFF -DIRIS_BUILD_STATIC=OFF &&\
cmake --build build &&\
cmake --install build


FROM base AS deploy
RUN apk update && apk upgrade && apk add libstdc++

# See https://docs.docker.com/go/dockerfile-user-best-practices/
ARG UID=10001
RUN adduser \
    --disabled-password \
    --gecos "" \
    --home "/nonexistent" \
    --shell "/sbin/nologin" \
    --no-create-home \
    --uid "${UID}" \
    SlideServer
USER SlideServer

COPY --from=build_env /usr/local/bin/IrisRESTful /usr/local/bin/

ENTRYPOINT [ "IrisRESTful", "3000", "/slides" ]
