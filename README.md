# Vela Server The **Vela Server** is the central orchestration and data-collection backend for the Vela IV&V (Independent Validation & Verification) tool suite. It exposes: - A **gRPC service** (`HalService`) for the embedded HAL Client App to submit test results and receive streaming commands.
- A **REST/HTTP API** (`/api/v1`) for the Web UI and external tooling.
- A **PostgreSQL** backend with versioned migrations.
- **OTA firmware upgrade** orchestration.
- **Prometheus metrics** (optional). --- ## 🚀 Quick Start (Docker — Recommended) **The easiest and most reliable way** to get started on any machine: ```bash
cd docker
./setup.sh
``` This will:
1. Check Docker prerequisites
2. Create `.env` from `.env.example`
3. Build the Docker image for your platform (auto-detected)
4. Start PostgreSQL and dev environment
5. You're ready to build! Then enter the container and build:
```bash
docker exec -it vela-dev bash
cmake --preset debug && cmake --build build/debug
``` ### Alternative: VS Code Dev Container Open this project in VS Code and:
1. Install the "Dev Containers" extension
2. Press `F1` → "Dev Containers: Reopen in Container"
3. Wait for build (first time takes ~10 min) Everything is configured automatically! See [docker/README.md](docker/README.md) for detailed Docker documentation. --- ## Architecture Overview ```
HAL Client (embedded) ──gRPC (port 9090)──► HalServiceImpl │
Web UI/ REST clients ──HTTP (port 8080)──► Drogon REST API │ ┌──────────┴──────────┐ │ Service Layer │ │ RunService │ │ LogService │ │ AuthService │ │ OtaService ... │ └──────────┬──────────┘ │ ┌──────────┴──────────┐ │ Repository Layer │ │ (Drogon ORM/ PG) │ └──────────┬──────────┘ │ PostgreSQL 15+
``` --- ## Prerequisites (Native Build) **Note:** Using Docker (see Quick Start above) avoids all manual dependency installation. For native builds without Docker: | Tool | Version |
|------|---------|
| CMake | 3.22+ |
| vcpkg | latest |
| C++ compiler | GCC 12+ or Clang 16+ |
| PostgreSQL | 15+ |
| protoc + grpc_cpp_plugin | matching vcpkg grpc version | Set `VCPKG_ROOT` in your environment before building. --- ## Quick Start (Local Dev) ```bash
cp .env.example .env
# Edit .env — at minimum set VELA_DB_* and VELA_SESSION_SECRET # Start Postgres
docker compose -f docker/docker-compose.yml up postgres -d # Build and run (loads .env automatically)
./scripts/run_dev.sh
``` Or with Docker Compose (full stack): ```bash
docker compose -f docker/docker-compose.yml up --build
``` --- ## Building ```bash
# Debug (default dev preset)
cmake --preset debug
cmake --build build/debug # Release
cmake --preset release
cmake --build build/release # Run tests
cmake --preset test
cmake --build build/test
ctest --test-dir build/test --output-on-failure # ASan
cmake --preset asan
cmake --build build/asan
``` --- ## Configuration All configuration is via environment variables. See `.env.example` for the full list. | Variable | Default | Description |
|----------|---------|-------------|
| `VELA_GRPC_LISTEN` | `0.0.0.0:9090` | gRPC bind address |
| `VELA_HTTP_LISTEN` | `0.0.0.0:8080` | HTTP bind address |
| `VELA_DB_HOST` | `localhost` | PostgreSQL host |
| `VELA_DB_NAME` | `vela` | Database name |
| `VELA_DB_USER` | `vela` | DB username |
| `VELA_DB_PASSWORD` | *(required)* | DB password |
| `VELA_SESSION_SECRET` | *(required, ≥32 chars)* | Session signing key |
| `VELA_REGISTRY_PATH` | `registry.yaml` | Module registry YAML |
| `VELA_OTA_STORAGE_PATH` | `/var/vela/ota` | OTA artifact directory |
| `VELA_LOG_LEVEL` | `info` | Log level (trace/debug/info/warn/error) | --- ## Module Registry Modules are declared in a YAML file (path set by `VELA_REGISTRY_PATH`): ```yaml
modules: - id: "com.example.temperature" name: "Temperature Monitor" description: "Reads temperature from sensor A" version: "1.2.0" result_type_url: "type.googleapis.com/vela.TempResult" config_type_url: "type.googleapis.com/vela.TempConfig" modes: ["single_shot", "monitoring"]
``` --- ## Database Migrations SQL migration files live in `migrations/`. They are applied automatically at startup in alphabetical order. Migrations are idempotent (tracked in `schema_migrations`). To add a new migration, create `migrations/NNNN_description.sql`. --- ## API All protected endpoints require either:
- A session cookie (from `POST/api/v1/auth/login`), or
- An `Authorization: Bearer <token>` header. ### Key endpoints | Method | Path | Description |
|--------|------|-------------|
| GET | `/api/v1/health` | Health check |
| POST | `/api/v1/auth/login` | Login → sets session |
| POST | `/api/v1/auth/tokens` | Create API token |
| GET | `/api/v1/modules` | List test modules |
| POST | `/api/v1/runs` | Start a test run |
| GET | `/api/v1/runs/{id}` | Get run details |
| GET | `/api/v1/runs/{id}/logs` | List run logs |
| GET | `/api/v1/runs/{id}/logs/tail` | SSE live log tail |
| GET | `/api/v1/target` | Get board info |
| POST | `/api/v1/target/heartbeat` | Board heartbeat |
| POST | `/api/v1/ota/upload` | Upload OTA artifact |
| POST | `/api/v1/ota/{id}/trigger` | Trigger OTA upgrade | --- ## Decisions Log | Decision | Rationale |
|----------|-----------|
| gRPC sync API for unary, streaming wait-loop | Simpler than async CQ for initial impl; async CQ adds complexity without benefit at this scale |
| Drogon for REST | Built-in PG ORM, async, WebSocket, SSE, session management |
| argon2id for passwords | OWASP recommended |
| SHA-256 for API tokens | Tokens are long random strings; SHA-256 is fast and sufficient |
| 1-minute minimum cron resolution | Full cron parser adds a dependency; document the limitation |
| Single `board` row (unique partial index) | The system targets exactly one physical test board |
| Migrations at startup | Ensures schema is always up to date; no separate migration tool needed |
| vcpkg manifest mode | Reproducible dependency pinning per repo | ## Build ```sh
cmake -B build -DCMAKE_BUILD_TYPE=RelWithDebInfo
cmake --build build -j$(nproc)
``` Binary: `build/vela-server` ## Run (dev) ```sh
./build/vela-server --config etc/vela/server.toml --log-level debug
``` Without a config file the binary starts with compiled-in defaults. ## Test ```sh
cmake --build build --target config-test
ctest --test-dir build --output-on-failure
``` ## Config Default path: `/etc/vela/server.toml`. See `etc/vela/server.toml` for all keys and defaults. CLI flags override file values: | Flag | Default |
|------|---------|
| `--config PATH` | `/etc/vela/server.toml` |
| `--log-level LEVEL` | `info` | ## Implementation Status | Step | Status |
|------|--------|
| 1 · Skeleton + config + logger | ✓ done |
| 2 · DB layer + migrations | — |
| 3 · DB CRUD (runs + batches) | — |
| 4 · Ingest queue + result writer | — |
| 5 · gRPC bidi server | — |
| … | — |

## Overview

## Project Summary

Vela-Server is the central C++ orchestration and data-collection backend for
Vela’s independent validation and verification tooling. Its CMake and Docker
project contains configuration, database access, migration execution, gRPC
service implementations, logging, and the application entry point. `HalService`
and client-service components provide the embedded boundary, while the existing
documentation describes a REST `/api/v1` surface, PostgreSQL persistence,
versioned migrations, OTA orchestration, and optional Prometheus metrics.

The repository includes Docker setup, local setup, dependency, audit, and
server-update documentation. Source and configuration establish these feature
surfaces, but a migration runner does not prove database reachability, a service
class does not prove streaming success, and a compose file does not prove a
healthy deployment. End-to-end behavior should be validated with PostgreSQL,
protocol clients, and the intended deployment environment.

The server’s source layout separates transport, persistence, configuration, and
logging, which is a useful ownership boundary for future tests. Docker compose
files provide a repeatable development environment, while CMake presets expose
debug, release, test, and sanitizer builds in the existing documentation. The
REST endpoint inventory includes health, authentication, modules, runs, logs,
target heartbeat, and OTA operations. These documented routes should be tested
with authorization, database failures, stream disconnects, migration replay,
and invalid artifacts. The implementation-status notes in the original README
also describe unfinished database and ingestion work, so source presence must
not be read as full product completion. This status table keeps the central
service contract visible while preserving those limitations.

## Feature Status

| # | Name / Description | Implemented? | Test Coverage? | Manual Test |
|---:|---|---|---|---|
| 1 | CMake/Make and Docker development workflows | Implemented | Build configuration present; execution not verified | [ ] Manual verification required |
| 2 | Settings/configuration component | Implemented | Coverage depth unknown | [ ] Manual verification required |
| 3 | PostgreSQL client and database abstraction | Implemented | Integration coverage unknown | [ ] Manual verification required |
| 4 | Versioned migration runner | Implemented | Migration execution not verified | [ ] Manual verification required |
| 5 | `HalService` gRPC implementation | Implemented | End-to-end stream coverage unknown | [ ] Manual verification required |
| 6 | Client service and command streaming | Implemented | Stream/backpressure coverage unknown | [ ] Manual verification required |
| 7 | gRPC runner and stream backpressure helper | Implemented | Runtime result not verified | [ ] Manual verification required |
| 8 | REST `/api/v1` orchestration API | Partial | Endpoint coverage not established | [ ] Manual verification required |
| 9 | OTA firmware upgrade orchestration | Partial | Workflow/hardware coverage unknown | [ ] Manual verification required |
| 10 | Optional Prometheus metrics | Partial | Metrics runtime coverage unknown | [ ] Manual verification required |
| 11 | Production Docker/PostgreSQL deployment | Unknown | Environment-dependent; not verified | [ ] Manual verification required |

Start with `LOCAL_SETUP.md` and `docker/README.md`; use the root and Docker
Makefiles. The documented CMake presets and `ctest` commands are the primary
build/test entry points.
