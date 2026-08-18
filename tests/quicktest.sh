#!/bin/bash

set -e

HOST="${HOST:-127.0.0.1}"
PORT="${PORT:-6670}"
PASSWORD="${PASSWORD:-pass}"
ROOT_DIR="$(cd "$(dirname "$0")/.." && pwd)"

cd "$ROOT_DIR"
make

if nc -z "$HOST" "$PORT" 2>/dev/null; then
	echo "quicktest: $HOST:$PORT is already in use; set PORT=<free-port> to override" >&2
	exit 1
fi

./ircserv "$PORT" "$PASSWORD" &
SERVER_PID=$!

cleanup()
{
	kill "$SERVER_PID" 2>/dev/null || true
	wait "$SERVER_PID" 2>/dev/null || true
}
trap cleanup EXIT INT TERM

for _ in $(seq 1 50); do
	if ! kill -0 "$SERVER_PID" 2>/dev/null; then
		echo "quicktest: ircserv exited before accepting connections" >&2
		exit 1
	fi
	if nc -z "$HOST" "$PORT" 2>/dev/null; then
		break
	fi
	sleep 0.1
done

if ! nc -z "$HOST" "$PORT" 2>/dev/null; then
	echo "quicktest: timed out waiting for ircserv on $HOST:$PORT" >&2
	exit 1
fi

python3 tests/harness.py "$HOST" "$PORT" "$PASSWORD"
