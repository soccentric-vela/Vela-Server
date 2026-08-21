# Vela-Server Build and Deployment

## Purpose and scope

Vela-Server is the C++ central orchestration backend. It exposes gRPC for
embedded clients and REST/HTTP for operators, uses PostgreSQL and migrations,
and includes Docker development workflows. The root Makefile owns native and
ARM64/server Docker builds; `CMakePresets.json` defines debug, release, test,
and ASan configurations.

## 1. Prerequisites

- CMake, a C++20 compiler, Make/Ninja as selected by the environment, and
  vcpkg with `VCPKG_ROOT` configured for native builds.
- PostgreSQL 15+, matching protobuf/gRPC tools, and values from `.env.example`.
- Docker/buildx for the recommended path. `docker/setup.sh` and
  `docker/check-prereqs.sh` validate/setup the development environment.
- Credentials/secrets for `VELA_DB_PASSWORD`, `VELA_SESSION_SECRET` (at least
  32 characters), and any production storage or auth integration.

## 2. Setup

Recommended: `cd docker && ./setup.sh`, then enter the `vela-dev` container.
Alternatively copy `.env.example` to `.env`, start PostgreSQL with `docker
compose -f docker/docker-compose.yml up postgres -d`, and use `scripts/run_dev.sh`.
The Makefile also provides `make -C docker check`, `setup`, `up`, `shell`, and
`logs` wrappers. On Ubuntu 22.04/24.04, `./local_install.sh` installs apt
dependencies and, unless skipped, builds yaml-cpp, Drogon, and
`grpc_cpp_plugin` from source; use `--skip-apt`, `--skip-postgres`, or
`--skip-build-from-src` when those steps are already satisfied. It creates
`.env.local`, which should be sourced before a native build.

## 3. Build and compile

- Native preset flow: `cmake --preset debug && cmake --build build/debug`.
- Release flow: `cmake --preset release && cmake --build build/release`.
- Test flow: `cmake --preset test && cmake --build build/test`; ASan uses the
  analogous `asan` preset.
- The Docker root targets include `make -C docker build`, `build-arm64`, and
  `build-amd64`; the root `make docker-build` builds the ARM64 client-side
  environment, while `make docker-build-server` builds the x86 server image.

## 4. Tests and lint

Run `ctest --test-dir build/test --output-on-failure` after the test preset, or
`make -C docker docker-test`/`make docker-test-server` for the corresponding
container path. The repository Makefiles expose `format`, `cppcheck`, and `doc`
for the native project; install clang-format, cppcheck, and Doxygen where those
targets are used.

## 5. Packaging and artifacts

The primary native binary is `build/<preset>/vela-server`. CMake build trees,
compile commands, test output, and optional sanitizer results remain under
`build/`. No root CPack/package target is defined; Docker images are the
deployment artifacts produced by `docker/build.sh` and its platform-specific
variants.

## 6. Deployment, install, and run

Run a local binary with `./build/vela-server --config etc/vela/server.toml
--log-level debug` after configuring the database and environment. Full local
stack operation uses `docker compose -f docker/docker-compose.yml up --build`;
the API and gRPC defaults are ports 8080 and 9090. The root Makefile can build
and run the ARM64 client environment, build/test the x86 server image with
`make docker-build-server` and `make docker-test-server`, and open shells with
`make docker-shell-server`. Apply migrations using the server's startup path
and verify PostgreSQL is reachable before accepting traffic.

## 7. Cleanup

Use `make clean` for the root build directory where defined. Docker operations
include `make -C docker down`, `clean`, and `delete-images`; `delete-images` is
destructive and can remove all local Docker images after confirmation. Use the
compose `down` command to stop services while retaining volumes unless an
explicit volume removal is intended.

## 8. Hardware, cloud, and manual verification caveats

Build and CTest do not establish PostgreSQL migrations, gRPC streaming, REST
authorization, OTA storage, metrics, or device heartbeat behavior. Server and
client Docker targets use different architectures and images; select the one
matching the deployment. Production deployment requires non-default database
passwords, session secrets, TLS/network policy, storage, migrations, and
connected Vela clients. Verify health, authentication, streams, OTA artifacts,
and recovery behavior against the real environment.