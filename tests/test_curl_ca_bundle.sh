#!/bin/sh
set -eu

cxx="${CXX:-c++}"
out="${TMPDIR:-/tmp}/curl_ca_bundle_test.$$"
work="$(mktemp -d)"
server_pid=""
trap 'test -z "$server_pid" || kill "$server_pid" 2>/dev/null || true; rm -rf "$work" "$out"' EXIT

openssl req -x509 -newkey rsa:2048 -nodes -days 1 \
	-keyout "$work/ca.key" -out "$work/ca.pem" -subj '/CN=Amiberry test CA' >/dev/null 2>&1
openssl req -newkey rsa:2048 -nodes \
	-keyout "$work/server.key" -out "$work/server.csr" -subj '/CN=localhost' >/dev/null 2>&1
printf '%s\n' 'subjectAltName=DNS:localhost' > "$work/server.ext"
openssl x509 -req -days 1 -sha256 \
	-in "$work/server.csr" -CA "$work/ca.pem" -CAkey "$work/ca.key" -CAcreateserial \
	-out "$work/server.pem" -extfile "$work/server.ext" >/dev/null 2>&1

port=$((30000 + ($$ % 20000)))
openssl s_server -accept "$port" -www -quiet \
	-cert "$work/server.pem" -key "$work/server.key" >"$work/server.log" 2>&1 &
server_pid=$!
attempt=0
until openssl s_client -connect "localhost:$port" -servername localhost </dev/null >/dev/null 2>&1; do
	if ! kill -0 "$server_pid" 2>/dev/null; then
		cat "$work/server.log" >&2
		exit 1
	fi
	attempt=$((attempt + 1))
	if [ "$attempt" -ge 100 ]; then
		echo "local HTTPS fixture did not become ready" >&2
		exit 1
	fi
done

curl_flags="$(pkg-config --cflags --libs libcurl)"
# pkg-config emits a whitespace-delimited compiler argument list.
# shellcheck disable=SC2086
"$cxx" -std=c++17 -Wall -Wextra -Werror -DAMIBERRY_HAS_CURL -Isrc/osdep \
	-o "$out" \
	tests/curl_ca_bundle_test.cpp \
	src/osdep/amiberry_curl.cpp \
	$curl_flags
"$out" "$work/ca.pem" "https://localhost:$port/"
