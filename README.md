# Process Scheduler Simulator

A C++ CPU scheduling simulator with a web interface. The C++ engine runs six algorithms and produces Gantt charts and metrics; a small Python server connects it to a browser UI.

![Web UI](screenshot.jpeg)

## Features

- **Algorithms:** FCFS, SJF, SRTF, Priority (non-preemptive and preemptive), Round Robin
- **Metrics:** waiting, turnaround, response, completion and start time, CPU utilization, throughput, context switches
- **Web UI:** editable process table, Gantt chart with idle time, per-process metrics, and an all-algorithm comparison
- **Console UI:** menu-driven simulator with ASCII Gantt charts
- **Tests:** 50+ checks against hand-worked textbook examples, run in CI

## Quick start

Requires a C++11 compiler, Python 3 (standard library only), and optionally `make`.

**Linux / macOS / WSL**

```bash
make            # build bin/scheduler
make test       # run the test suite
make web        # start the web UI at http://localhost:8000
make run        # or use the console menu
```

**Windows (MinGW, Command Prompt)**

```bat
mkdir bin
g++ -std=gnu++11 -I./include src/Process.cpp src/Scheduler.cpp src/Utils.cpp src/main.cpp -o bin/scheduler.exe
python server.py
```

Then open http://localhost:8000.

To run the tests on Windows:

```bat
g++ -std=gnu++11 -I./include src/Process.cpp src/Scheduler.cpp src/Utils.cpp tests/test_scheduler.cpp -o bin/run_tests.exe
bin\run_tests.exe
```

## Project structure

```
ProcessScheduler/
├── include/          Process.h, Scheduler.h, Utils.h
├── src/              Process.cpp, Scheduler.cpp, Utils.cpp, main.cpp
├── tests/            test_scheduler.cpp
├── web/              index.html, style.css, app.js
├── server.py         Serves web/ and calls the C++ binary
├── Makefile
└── .github/workflows/ci.yml
```

## How it fits together

```
Browser (web/app.js) --POST /api/simulate--> server.py --stdin--> bin/scheduler --json --> JSON back to the browser
```

`server.py` validates the request, then runs `bin/scheduler --json <algorithm|all> <quantum>` and passes one process per line (`pid arrival burst priority`) on stdin. The engine prints `{"results": [...]}`. You can call it directly:

```bash
printf '1 0 5 2\n2 1 3 1\n' | ./bin/scheduler --json rr 2
```

Algorithm keys: `fcfs`, `sjf`, `srtf`, `priority`, `priority_p`, `rr`, `all`.

## Algorithms

| Algorithm | Preemptive | Starvation risk | Notes |
|---|---|---|---|
| FCFS | No | No | Convoy effect with long jobs first |
| SJF | No | Yes | Needs burst times known in advance |
| SRTF | Yes | Yes | Minimizes average waiting time |
| Priority | No | Yes | Lower number = higher priority |
| Priority (preemptive) | Yes | Yes | Higher-priority arrival interrupts |
| Round Robin | Yes | No | Fixed time quantum |

Ties are broken by earlier arrival, then lower PID. SRTF and preemptive priority keep the running process on an exact tie, so equal jobs don't cause needless context switches. If the CPU has nothing ready, it idles until the next arrival, and idle time before the first arrival is not counted against utilization.

## Metrics

- **Turnaround** = completion - arrival
- **Waiting** = turnaround - burst
- **Response** = first start - arrival
- **CPU utilization** = busy time / (end - first arrival)
- **Context switches** = changes between two different processes with no idle gap

## Extending

1. Add a value to `Algorithm` in `include/Scheduler.h`, plus its name and key in `src/Scheduler.cpp`.
2. Implement it (most algorithms fit `nonPreemptive`, `preemptive` or `roundRobin` with a different `keyOf`).
3. Add it to `ALGOS` in `web/app.js` and add a test case.

Ideas: Multilevel Feedback Queue, aging for priority scheduling, CSV import, random workload benchmarks.