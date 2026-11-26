#!/data/data/com.termux/files/usr/bin/sh
set -eu
ROOT=$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)
echo "Windows x64 builds run in GitHub Actions: push this branch or dispatch windows-build.yml." >&2
printf '%s\n' "Repository: $ROOT" "Workflow: .github/workflows/windows-build.yml"
