#!/usr/bin/env bash
# Prefer the released npm compiler only when it proves it can emit DSL v5.
set -euo pipefail
HERE="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
if [[ -n "${LMSCRIPT_BIN:-}" ]]; then
  printf '%s\n' "$LMSCRIPT_BIN"
  exit 0
fi
WORK="$(mktemp -d "${TMPDIR:-/tmp}/lmcad-compiler-XXXXXX")"
trap 'rm -rf "$WORK"' EXIT
CANDIDATE="$(command -v lmscript || true)"
python3 - "$HERE/schema/lmcad.dsl.json" "$WORK" <<'PY'
import json, pathlib, sys
work = pathlib.Path(sys.argv[2])
(work / 'probe.lms').write_text(
    'p = _dsl_ schema=' + json.dumps(sys.argv[1]) + ';\nwhile (false) {}\nend _dsl_\n'
    + 'savedsl(p, ' + json.dumps(str(work / 'probe.dslb')) + ');\n')
PY
if [[ -n "$CANDIDATE" ]] &&
   "$CANDIDATE" --dsl-signatures "$HERE/schema/lmcad.dsl.json" >/dev/null 2>&1 &&
   "$CANDIDATE" "$WORK/probe.lms" "$WORK/probe.dslb" >/dev/null 2>&1 &&
   python3 - "$WORK/probe.dslb" <<'PY'
import pathlib, sys
p = pathlib.Path(sys.argv[1])
sys.exit(0 if p.is_file() and p.read_bytes()[:6] == b'DSLB\x00\x05' else 1)
PY
then
  printf '%s\n' "$CANDIDATE"
else
  printf '%s\n' "$HERE/scripts/lmscript-mono.sh"
fi
