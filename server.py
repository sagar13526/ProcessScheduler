#!/usr/bin/env python3
"""Serves the web UI and runs simulations through the C++ binary. Standard library only."""
import json
import os
import subprocess
import sys
from http.server import SimpleHTTPRequestHandler, ThreadingHTTPServer

ROOT = os.path.dirname(os.path.abspath(__file__))
WEB_DIR = os.path.join(ROOT, "web")
BINARY = os.path.join(ROOT, "bin", "scheduler.exe" if os.name == "nt" else "scheduler")
ALGORITHMS = {"fcfs", "sjf", "srtf", "priority", "priority_p", "rr", "all"}
MAX_PROCESSES, MAX_VALUE = 30, 1000


def build_input(payload):
    """Validates the request and returns (algorithm, quantum, stdin text)."""
    algorithm = payload.get("algorithm")
    if algorithm not in ALGORITHMS:
        raise ValueError("Unknown algorithm.")
    quantum = payload.get("quantum", 2)
    if not isinstance(quantum, int) or not 1 <= quantum <= MAX_VALUE:
        raise ValueError("Time quantum must be a whole number from 1 to %d." % MAX_VALUE)
    processes = payload.get("processes")
    if not isinstance(processes, list) or not 1 <= len(processes) <= MAX_PROCESSES:
        raise ValueError("Provide between 1 and %d processes." % MAX_PROCESSES)
    lines = []
    for p in processes:
        values = [p.get(k) if isinstance(p, dict) else None for k in ("pid", "arrival", "burst", "priority")]
        if not all(isinstance(v, int) and not isinstance(v, bool) and 0 <= v <= MAX_VALUE for v in values):
            raise ValueError("Every process needs whole numbers from 0 to %d." % MAX_VALUE)
        lines.append(" ".join(map(str, values)))
    return algorithm, quantum, "\n".join(lines) + "\n"


class Handler(SimpleHTTPRequestHandler):
    def __init__(self, *args, **kwargs):
        super().__init__(*args, directory=WEB_DIR, **kwargs)

    def send_json(self, status, body):
        data = body.encode("utf-8")
        self.send_response(status)
        self.send_header("Content-Type", "application/json")
        self.send_header("Content-Length", str(len(data)))
        self.end_headers()
        self.wfile.write(data)

    def do_POST(self):
        if self.path != "/api/simulate":
            return self.send_json(404, json.dumps({"error": "Not found."}))
        try:
            length = int(self.headers.get("Content-Length", 0))
            if length > 100_000:
                raise ValueError("Request is too large.")
            algorithm, quantum, stdin = build_input(json.loads(self.rfile.read(length) or b"{}"))
        except (ValueError, json.JSONDecodeError) as err:
            return self.send_json(400, json.dumps({"error": str(err)}))
        if not os.path.exists(BINARY):
            return self.send_json(500, json.dumps({"error": "Simulator not built. Run `make` first."}))
        try:
            run = subprocess.run([BINARY, "--json", algorithm, str(quantum)], input=stdin,
                                 capture_output=True, text=True, timeout=5)
        except subprocess.TimeoutExpired:
            return self.send_json(500, json.dumps({"error": "Simulation timed out."}))
        self.send_json(200 if run.returncode == 0 else 400, run.stdout.strip() or json.dumps({"error": "Simulation failed."}))

    def log_message(self, fmt, *args):
        sys.stderr.write("%s\n" % (fmt % args))


if __name__ == "__main__":
    port = int(sys.argv[1]) if len(sys.argv) > 1 else 8000
    print("Process Scheduler Simulator running at http://localhost:%d  (Ctrl+C to stop)" % port)
    ThreadingHTTPServer(("127.0.0.1", port), Handler).serve_forever()
