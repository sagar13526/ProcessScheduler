// Checks every algorithm against hand-worked textbook examples. No test framework required.
#include <cmath>
#include <iostream>
#include "Scheduler.h"
#include "Utils.h"

static int failures = 0, checks = 0;

static void expectNear(const std::string& label, double actual, double expected) {
    ++checks;
    if (std::fabs(actual - expected) > 0.005) {
        ++failures;
        std::cout << "FAIL  " << label << ": expected " << expected << ", got " << actual << "\n";
    }
}

static ProcessScheduler make(std::initializer_list<std::vector<int>> rows) {
    ProcessScheduler s;
    for (const auto& r : rows) s.addProcess(r[0], r[1], r[2], r[3]);
    return s;
}

int main() {
    // README sample set
    ProcessScheduler sample = make({{1, 0, 5, 2}, {2, 1, 3, 1}, {3, 2, 8, 3}, {4, 3, 6, 2}, {5, 4, 4, 4}});
    Result r = sample.run(Algorithm::FCFS);
    expectNear("FCFS sample avg wait", r.avgWaiting, 8.2);
    expectNear("FCFS sample avg turnaround", r.avgTurnaround, 13.4);
    expectNear("FCFS sample end time", r.endTime, 26);

    // Classic SJF / SRTF example (arrivals 0,2,4,5; bursts 7,4,1,4)
    ProcessScheduler sjf = make({{1, 0, 7, 1}, {2, 2, 4, 1}, {3, 4, 1, 1}, {4, 5, 4, 1}});
    expectNear("SJF avg wait", sjf.run(Algorithm::SJF).avgWaiting, 4.0);
    expectNear("SRTF avg wait", sjf.run(Algorithm::SRTF).avgWaiting, 3.0);
    expectNear("SRTF context switches (P1,P2,P3,P2,P4,P1)", sjf.run(Algorithm::SRTF).contextSwitches, 5);

    // Round Robin, quantum 4, bursts 24/3/3 all at t=0
    ProcessScheduler rr = make({{1, 0, 24, 1}, {2, 0, 3, 1}, {3, 0, 3, 1}});
    Result rrRes = rr.run(Algorithm::ROUND_ROBIN, 4);
    expectNear("RR avg wait", rrRes.avgWaiting, 17.0 / 3.0);
    expectNear("RR P1 completion", rrRes.processes[0].getCompletion(), 30);
    expectNear("RR P2 response", rrRes.processes[1].getResponse(), 4);

    // Priority, bursts 10/1/2/1/5, priorities 3/1/4/5/2, all at t=0 (non-preemptive)
    ProcessScheduler pr = make({{1, 0, 10, 3}, {2, 0, 1, 1}, {3, 0, 2, 4}, {4, 0, 1, 5}, {5, 0, 5, 2}});
    expectNear("Priority NP avg wait", pr.run(Algorithm::PRIORITY_NP).avgWaiting, 8.2);

    // Preemptive priority: a high-priority arrival interrupts the running process
    ProcessScheduler pp = make({{1, 0, 6, 3}, {2, 2, 2, 1}});
    Result ppRes = pp.run(Algorithm::PRIORITY_P);
    expectNear("Priority P: P2 completes at 4", ppRes.processes[1].getCompletion(), 4);
    expectNear("Priority P: P1 completes at 8", ppRes.processes[0].getCompletion(), 8);

    // Idle gaps: CPU sits idle between processes
    ProcessScheduler idle = make({{1, 0, 2, 1}, {2, 5, 2, 1}});
    Result idleRes = idle.run(Algorithm::FCFS);
    expectNear("Idle: utilisation", idleRes.cpuUtilization, 4.0 / 7.0 * 100);
    expectNear("Idle: gantt has idle slice", idleRes.gantt[1].pid, -1);

    // A late first arrival shouldn't count as idle time
    ProcessScheduler late = make({{1, 3, 2, 1}});
    expectNear("Late start utilisation", late.run(Algorithm::FCFS).cpuUtilization, 100);

    // Invariants for every algorithm on the sample set
    for (Algorithm a : ProcessScheduler::allAlgorithms()) {
        Result x = sample.run(a, 3);
        int busy = 0;
        for (const Segment& s : x.gantt) if (s.pid != -1) busy += s.end - s.start;
        expectNear(ProcessScheduler::algorithmName(a) + ": busy time = total burst", busy, 26);
        for (const Process& p : x.processes) {
            expectNear("turnaround = waiting + burst", p.getTurnaround(), p.getWaiting() + p.getBurst());
            if (p.getWaiting() < 0) { ++failures; std::cout << "FAIL  negative waiting time\n"; }
        }
    }

    // Validation
    std::string err;
    ProcessScheduler bad = make({{1, 0, 0, 1}});
    ++checks;
    if (Utils::validateProcesses(bad.getProcesses(), err)) { ++failures; std::cout << "FAIL  zero burst accepted\n"; }

    std::cout << (checks - failures) << "/" << checks << " checks passed\n";
    return failures == 0 ? 0 : 1;
}
