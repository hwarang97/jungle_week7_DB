#!/usr/bin/env python3
"""Small HTTP bridge for the MiniSQL demo page.

The browser cannot run the C binaries directly, so this server exposes two
local endpoints:

- POST /query: run ./sqlparser with the submitted SQL.
- POST /benchmark: run one of the Makefile benchmark targets.
"""

from http.server import BaseHTTPRequestHandler, ThreadingHTTPServer
from pathlib import Path
import json
import os
import re
import subprocess
import sys
import tempfile
import time


ROOT = Path(__file__).resolve().parent
SQLPARSER_BIN = ROOT / ("sqlparser.exe" if os.name == "nt" else "sqlparser")
DB_PERF_BENCH_BIN = ROOT / ("db_benchmark.exe" if os.name == "nt" else "db_benchmark")
INDEX_HTML = ROOT / "index.html"
ANSI_RE = re.compile(r"\x1b\[[0-9;]*m")

BENCHMARKS = {
    "search": {
        "label": "B+Tree search skeleton",
        "make_target": "bench",
        "timeout": 30,
    },
    "bptree-scale": {
        "label": "B+Tree scale vs linear search",
        "make_target": "bench-bptree-scale",
        "timeout": 120,
    },
    "sql-index": {
        "label": "SQL WHERE id index vs name scan",
        "make_target": "bench-sql-index",
        "timeout": 120,
    },
    "index-graph": {
        "label": "Search/SELECT full scan vs B-tree vs B+Tree",
        "make_target": "bench-index-graph",
        "timeout": 240,
    },
}


def strip_ansi(text: str) -> str:
    return ANSI_RE.sub("", text)


def run_process(command: list[str], timeout: int) -> dict:
    started = time.perf_counter()
    try:
        proc = subprocess.run(
            command,
            cwd=ROOT,
            capture_output=True,
            text=True,
            timeout=timeout,
        )
    except FileNotFoundError as exc:
        return {
            "error": f"command not found: {command[0]}",
            "detail": str(exc),
            "command": " ".join(command),
            "elapsed_ms": round((time.perf_counter() - started) * 1000.0, 3),
        }
    except subprocess.TimeoutExpired:
        return {
            "error": f"command timed out after {timeout}s",
            "command": " ".join(command),
            "elapsed_ms": round((time.perf_counter() - started) * 1000.0, 3),
        }

    return {
        "command": " ".join(command),
        "returncode": proc.returncode,
        "stdout": strip_ansi(proc.stdout),
        "stderr": strip_ansi(proc.stderr),
        "elapsed_ms": round((time.perf_counter() - started) * 1000.0, 3),
    }


def ensure_db_performance_benchmark() -> dict | None:
    build = run_process(["make", "db_benchmark"], 120)
    if build.get("returncode") == 0 and DB_PERF_BENCH_BIN.exists():
        return None

    return {
        "error": "failed to build db_benchmark",
        "build": build,
    }


def clamp_int(value, default: int, min_value: int, max_value: int) -> int:
    try:
        parsed = int(value)
    except (TypeError, ValueError):
        return default

    if parsed < min_value:
        return min_value
    if parsed > max_value:
        return max_value
    return parsed


def parse_json_object(text: str) -> dict | None:
    start = text.find("{")
    end = text.rfind("}")
    if start < 0 or end < start:
        return None

    try:
        parsed = json.loads(text[start : end + 1])
    except json.JSONDecodeError:
        return None
    return parsed if isinstance(parsed, dict) else None


def run_benchmark(target: str, payload: dict | None = None) -> dict:
    spec = BENCHMARKS.get(target)
    if spec is None:
        return {
            "error": f"unknown benchmark target: {target}",
            "available": sorted(BENCHMARKS),
        }

    command = ["make", spec["make_target"]]
    if target == "index-graph":
        payload = payload or {}
        rows = clamp_int(payload.get("rows"), 10000, 1, 1000000)
        requests = clamp_int(payload.get("requests"), 5000, 1, 1000000)
        command.extend([f"ROWS={rows}", f"REQUESTS={requests}"])

    result = run_process(command, spec["timeout"])
    result.update(
        {
            "benchmark": target,
            "label": spec["label"],
            "make_target": spec["make_target"],
        }
    )
    if target == "index-graph":
        result["rows"] = rows
        result["requests"] = requests
        parsed = parse_json_object(result.get("stdout", ""))
        if parsed is not None:
            result["data"] = parsed
    return result


def run_db_performance_benchmark(size: int, queries: int, order: int, all_sizes: bool = False) -> dict:
    build_error = ensure_db_performance_benchmark()
    if build_error is not None:
        return build_error

    command = [
        str(DB_PERF_BENCH_BIN),
        "--json",
        "--queries", str(queries),
        "--order", str(order),
    ]
    if all_sizes:
        command.append("--all")
    else:
        command.extend(["--size", str(size)])

    result = run_process(command, 240)
    if result.get("returncode") != 0:
        result["error"] = "db_benchmark failed"
        return result

    parsed = parse_json_object(result.get("stdout", ""))
    if parsed is None:
        result["error"] = "db_benchmark did not return JSON"
        return result

    parsed["stderr"] = result.get("stderr", "")
    parsed["returncode"] = result.get("returncode", 0)
    parsed["elapsed_ms"] = result.get("elapsed_ms", 0)
    return parsed


def run_sqlparser(sql: str) -> dict:
    if not SQLPARSER_BIN.exists():
        return {
            "error": f"sqlparser binary not found at {SQLPARSER_BIN}. Run 'make sqlparser' first.",
            "statements": [],
            "stderr": "",
        }

    with tempfile.NamedTemporaryFile(
        mode="w", suffix=".sql", delete=False, encoding="utf-8"
    ) as tmp_file:
        tmp_file.write(sql)
        tmp_path = tmp_file.name

    try:
        proc = subprocess.run(
            [str(SQLPARSER_BIN), tmp_path, "--json"],
            cwd=ROOT,
            capture_output=True,
            text=True,
            timeout=5,
        )
    except subprocess.TimeoutExpired:
        return {"error": "sqlparser timeout after 5s", "statements": [], "stderr": ""}
    finally:
        try:
            os.unlink(tmp_path)
        except OSError:
            pass

    statements = []
    pending_result_lines = []

    def flush_result() -> None:
        if statements and pending_result_lines:
            text = "\n".join(pending_result_lines).rstrip()
            if text:
                statements[-1]["_result"] = text

    for line in proc.stdout.splitlines():
        stripped = line.strip()
        if not stripped:
            continue

        try:
            parsed = json.loads(stripped)
        except json.JSONDecodeError:
            parsed = None

        if isinstance(parsed, dict) and "type" in parsed:
            flush_result()
            pending_result_lines = []
            statements.append(parsed)
        else:
            pending_result_lines.append(line)

    flush_result()

    return {
        "statements": statements,
        "stderr": proc.stderr,
        "returncode": proc.returncode,
    }


class Handler(BaseHTTPRequestHandler):
    def _send(self, status: int, body: bytes, content_type: str) -> None:
        self.send_response(status)
        self.send_header("Content-Type", content_type)
        self.send_header("Content-Length", str(len(body)))
        self.send_header("Access-Control-Allow-Origin", "*")
        self.end_headers()
        self.wfile.write(body)

    def _send_json(self, status: int, payload: dict) -> None:
        body = json.dumps(payload, ensure_ascii=False).encode("utf-8")
        self._send(status, body, "application/json; charset=utf-8")

    def do_OPTIONS(self) -> None:
        self.send_response(204)
        self.send_header("Access-Control-Allow-Origin", "*")
        self.send_header("Access-Control-Allow-Methods", "GET, POST, OPTIONS")
        self.send_header("Access-Control-Allow-Headers", "Content-Type")
        self.end_headers()

    def do_GET(self) -> None:
        if self.path in ("/", "/index.html"):
            if not INDEX_HTML.exists():
                self._send_json(404, {"error": "index.html not found"})
                return
            self._send(200, INDEX_HTML.read_bytes(), "text/html; charset=utf-8")
            return

        if self.path == "/health":
            self._send_json(
                200,
                {
                    "ok": True,
                    "sqlparser_exists": SQLPARSER_BIN.exists(),
                    "db_benchmark_exists": DB_PERF_BENCH_BIN.exists(),
                    "benchmarks": sorted(BENCHMARKS),
                },
            )
            return

        self._send_json(404, {"error": "not found"})

    def do_POST(self) -> None:
        try:
            length = int(self.headers.get("Content-Length", "0"))
        except ValueError:
            self._send_json(400, {"error": "invalid Content-Length"})
            return

        if length <= 0:
            self._send_json(400, {"error": "empty body"})
            return

        try:
            payload = json.loads(self.rfile.read(length).decode("utf-8"))
        except (UnicodeDecodeError, json.JSONDecodeError) as exc:
            self._send_json(400, {"error": f"invalid JSON: {exc}"})
            return

        if self.path == "/query":
            sql = str(payload.get("sql", "")).strip()
            if not sql:
                self._send_json(400, {"error": "missing 'sql' field"})
                return
            self._send_json(200, run_sqlparser(sql))
            return

        if self.path == "/benchmark":
            if "target" not in payload and (
                "size" in payload or "queries" in payload or "all" in payload
            ):
                all_sizes = bool(payload.get("all", False))
                allowed_sizes = {10000, 100000, 1000000}
                size = clamp_int(payload.get("size"), 100000, 1, 1000000)
                queries = clamp_int(payload.get("queries"), 5000, 1, 100000)
                order = clamp_int(payload.get("order"), 4, 4, 1024)

                if not all_sizes and size not in allowed_sizes:
                    self._send_json(400, {"error": "size must be 10000, 100000, or 1000000"})
                    return

                result = run_db_performance_benchmark(
                    size=size,
                    queries=queries,
                    order=order,
                    all_sizes=all_sizes,
                )
                self._send_json(200, result)
                return

            target = str(payload.get("target", "sql-index")).strip()
            result = run_benchmark(target, payload)
            self._send_json(400 if "available" in result else 200, result)
            return

        self._send_json(404, {"error": "not found"})

    def log_message(self, fmt, *args) -> None:
        sys.stderr.write(f"[server] {self.address_string()} {fmt % args}\n")


def main() -> None:
    port = int(sys.argv[1]) if len(sys.argv) > 1 else 8000
    server = ThreadingHTTPServer(("0.0.0.0", port), Handler)
    print(f"[server] MiniSQL viewer listening on http://0.0.0.0:{port}")
    print(f"[server] sqlparser binary: {SQLPARSER_BIN}")
    print(f"[server] db_benchmark binary: {DB_PERF_BENCH_BIN}")
    try:
        server.serve_forever()
    except KeyboardInterrupt:
        print("\n[server] bye")


if __name__ == "__main__":
    main()
