# syntax=docker/dockerfile:1

# Run 'docker build -f Docker/Dockerfile -t iris-restful:latest .' from the root of the repository to build the image.
# See https://docs.docker.com/engine/reference/builder/ for more information on the Dockerfile syntax.

FROM alpine:3.22.0 AS base

FROM base AS build_env
WORKDIR /server_build/
RUN apk update && apk upgrade && apk add clang ninja cmake git boost-dev openssl-dev

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

LABEL org.opencontainers.image.source=https://github.com/IrisDigitalPathology/Iris-RESTful-Server.git
LABEL org.opencontainers.image.description="Iris File Extension RESTful API Server"
LABEL org.opencontainers.image.licenses=MIT

ENTRYPOINT [ "IrisRESTful"]
CMD ["-p", "3000", "-d", "/slides"]
