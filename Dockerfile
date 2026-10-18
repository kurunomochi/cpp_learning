FROM ubuntu:24.04 AS build
RUN apt-get update && apt-get install -y --no-install-recommends g++ cmake ninja-build python3 ca-certificates && rm -rf /var/lib/apt/lists/*
WORKDIR /source
COPY . .
RUN cmake --preset release && cmake --build --preset release && ctest --preset release

FROM ubuntu:24.04 AS runtime
RUN apt-get update && apt-get install -y --no-install-recommends libstdc++6 libgcc-s1 ca-certificates && rm -rf /var/lib/apt/lists/* \
    && groupadd --gid 10001 app && useradd --uid 10001 --gid app --no-create-home app \
    && mkdir /data && chown app:app /data
COPY --from=build /source/build/release/task_server /usr/local/bin/task_server
USER app
WORKDIR /data
ENV APP_HOST=0.0.0.0 APP_PORT=8080 APP_DB=/data/tasks.db
EXPOSE 8080
ENTRYPOINT ["/usr/local/bin/task_server"]
