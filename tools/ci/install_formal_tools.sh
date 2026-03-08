#!/usr/bin/env bash
set -euo pipefail

REQUIRE_FORMAL_TOOLS="${REQUIRE_FORMAL_TOOLS:-0}"

sudo apt-get update -qq
sudo apt-get install -y software-properties-common
sudo add-apt-repository -y universe >/dev/null 2>&1 || true
sudo apt-get update -qq

missing=()
for pkg in frama-c alt-ergo; do
  candidate="$(apt-cache policy "$pkg" 2>/dev/null | awk '/Candidate:/ {print $2}')"
  if [[ -n "$candidate" && "$candidate" != "(none)" ]]; then
    echo "[ci] installing optional formal tool package: $pkg"
    sudo apt-get install -y "$pkg"
  else
    echo "[ci] package '$pkg' is not available on this runner image; skipping"
    missing+=("$pkg")
  fi
done

if [[ "$REQUIRE_FORMAL_TOOLS" == "1" && ${#missing[@]} -gt 0 ]]; then
  echo "[ci] required formal tools missing: ${missing[*]}"
  exit 1
fi
