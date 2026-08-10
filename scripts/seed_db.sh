#!/usr/bin/env bash
# scripts/seed_db.sh — Create initial admin user and API token.
# Requires psql on PATH and VELA_DB_* env vars.
set -euo pipefail

DB_HOST="${VELA_DB_HOST:-localhost}"
DB_PORT="${VELA_DB_PORT:-5432}"
DB_NAME="${VELA_DB_NAME:-vela}"
DB_USER="${VELA_DB_USER:-vela}"

PSQL="psql -h ${DB_HOST} -p ${DB_PORT} -U ${DB_USER} -d ${DB_NAME}"

echo "Seeding database ${DB_NAME}@${DB_HOST}..."

# Insert a default admin user with a placeholder password hash.
# Replace the hash with a real argon2id hash in production.
${PSQL} <<'SQL'
INSERT INTO users (id, username, password_hash, role, created_at, updated_at)
VALUES (
    gen_random_uuid()::text,
    'admin',
    '$argon2id$v=19$m=65536,t=2,p=1$PLACEHOLDER',
    'admin',
    NOW(),
    NOW()
)
ON CONFLICT (username) DO NOTHING;
SQL

echo "Seed complete.  Change admin password before production use."
