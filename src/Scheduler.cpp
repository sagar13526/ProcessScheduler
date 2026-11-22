#include "Scheduler.h"
#include <algorithm>
#include <climits>
#include <deque>

namespace {

void pushSegment(std::vector<Segment>& gantt, int pid, int start, int end) {
    if (start >= end) return;
    if (!gantt.empty() && gantt.back().pid == pid && gantt.back().end == start) {
        gantt.back().end = end;  // merge back-to-back slices of the same process
        return;
    }
    gantt.push_back({pid, start, end});
}

// The value an algorithm minimises when choosing the next process.
int keyOf(const Process& p, Algorithm a) {
    switch (a) {
        case Algorithm::SJF: return p.getBurst();
        case Algorithm::SRTF: return p.getRemaining();
        case Algorithm::PRIORITY_NP:
        case Algorithm::PRIORITY_P: return p.getPriority();
        default: return p.getArrival();
    }
}

// Ties fall back to earlier arrival, then lower PID, so results are deterministic.
bool better(const Process& a, const Process& b, Algorithm alg) {
    int ka = keyOf(a, alg), kb = keyOf(b, alg);
    if (ka != kb) return ka < kb;
    if (a.getArrival() != b.getArrival()) return a.getArrival() < b.getArrival();
    return a.getPid() < b.getPid();
}

int earliestArrival(const std::vector<Process>& p) {
    int t = INT_MAX;
    for (const Process& x : p) t = std::min(t, x.getArrival());
    return p.empty() ? 0 : t;
}

int nextArrival(const std::vector<Process>& p) {
    int t = INT_MAX;
    for (const Process& x : p)
        if (!x.isFinished()) t = std::min(t, x.getArrival());
    return t;
}

Result finalize(Algorithm alg, std::vector<Process> procs, std::vector<Segment> gantt, int startTime) {
    Result r;
    r.algorithm = alg;
    r.name = ProcessScheduler::algorithmName(alg);
    r.startTime = startTime;
    r.endTime = gantt.empty() ? startTime : gantt.back().end;

    double sumW = 0, sumT = 0, sumR = 0;
    for (Process& p : procs) {
        p.setTurnaround(p.getCompletion() - p.getArrival());
        p.setWaiting(p.getTurnaround() - p.getBurst());
        p.setResponse(p.getStart() - p.getArrival());
        sumW += p.getWaiting();
        sumT += p.getTurnaround();
        sumR += p.getResponse();
    }

    int busy = 0, prevPid = -1;
    for (const Segment& s : gantt) {
        if (s.pid == -1) { prevPid = -1; continue; }
        busy += s.end - s.start;
        if (prevPid != -1 && prevPid != s.pid) r.contextSwitches++;
        prevPid = s.pid;
    }

    double n = static_cast<double>(procs.size());
    double span = r.endTime - r.startTime;
    if (n > 0) {
        r.avgWaiting = sumW / n;
        r.avgTurnaround = sumT / n;
        r.avgResponse = sumR / n;
    }
    if (span > 0) {
        r.cpuUtilization = 100.0 * busy / span;
        r.throughput = n / span;
    }
    r.processes = procs;
    r.gantt = gantt;
    return r;
}

}  // namespace

void ProcessScheduler::addProcess(int pid, int arrival, int burst, int priority) {
    processes_.push_back(Process(pid, arrival, burst, priority));
}

Result ProcessScheduler::run(Algorithm algorithm, int quantum) const {
    switch (algorithm) {
        case Algorithm::FCFS:
        case Algorithm::SJF:
        case Algorithm::PRIORITY_NP: return nonPreemptive(algorithm);
        case Algorithm::SRTF:
        case Algorithm::PRIORITY_P: return preemptive(algorithm);
        case Algorithm::ROUND_ROBIN: return roundRobin(quantum);
    }
    return Result();
}

Result ProcessScheduler::nonPreemptive(Algorithm alg) const {
    std::vector<Process> p = processes_;
    for (Process& x : p) x.reset();
    std::vector<Segment> gantt;
    int time = earliestArrival(p), startTime = time;
    size_t done = 0;

    while (done < p.size()) {
        int best = -1;
        for (size_t i = 0; i < p.size(); ++i) {
            if (p[i].isFinished() || p[i].getArrival() > time) continue;
            if (best < 0 || better(p[i], p[best], alg)) best = static_cast<int>(i);
        }
        if (best < 0) {  // nothing ready: CPU idles until the next arrival
            int next = nextArrival(p);
            pushSegment(gantt, -1, time, next);
            time = next;
            continue;
        }
        Process& c = p[best];
        c.setStart(time);
        pushSegment(gantt, c.getPid(), time, time + c.getBurst());
        time += c.getBurst();
        c.setRemaining(0);
        c.setCompletion(time);
        ++done;
    }
    return finalize(alg, p, gantt, startTime);
}

Result ProcessScheduler::preemptive(Algorithm alg) const {
    std::vector<Process> p = processes_;
    for (Process& x : p) x.reset();
    std::vector<Segment> gantt;
    int time = earliestArrival(p), startTime = time, cur = -1;
    size_t done = 0;

    while (done < p.size()) {
        int best = -1;
        for (size_t i = 0; i < p.size(); ++i) {
            if (p[i].isFinished() || p[i].getArrival() > time) continue;
            if (best < 0 || better(p[i], p[best], alg)) best = static_cast<int>(i);
        }
        if (best < 0) {
            int next = nextArrival(p);
            pushSegment(gantt, -1, time, next);
            time = next;
            cur = -1;
            continue;
        }
        // Only preempt the running process when another is strictly better.
        if (cur >= 0 && keyOf(p[cur], alg) == keyOf(p[best], alg)) best = cur;
        cur = best;

        Process& c = p[cur];
        if (c.getStart() < 0) c.setStart(time);
        pushSegment(gantt, c.getPid(), time, time + 1);
        c.setRemaining(c.getRemaining() - 1);
        ++time;
        if (c.isFinished()) {
            c.setCompletion(time);
            ++done;
            cur = -1;
        }
    }
    return finalize(alg, p, gantt, startTime);
}

Result ProcessScheduler::roundRobin(int quantum) const {
    if (quantum < 1) quantum = 1;
    std::vector<Process> p = processes_;
    for (Process& x : p) x.reset();

    std::vector<int> order(p.size());
    for (size_t i = 0; i < p.size(); ++i) order[i] = static_cast<int>(i);
    std::sort(order.begin(), order.end(), [&p](int a, int b) {
        if (p[a].getArrival() != p[b].getArrival()) return p[a].getArrival() < p[b].getArrival();
        return p[a].getPid() < p[b].getPid();
    });

    std::vector<Segment> gantt;
    std::deque<int> ready;
    size_t next = 0, done = 0;
    int time = earliestArrival(p), startTime = time;

    auto admit = [&]() {
        while (next < order.size() && p[order[next]].getArrival() <= time) ready.push_back(order[next++]);
    };
    admit();

    while (done < p.size()) {
        if (ready.empty()) {
            int t = p[order[next]].getArrival();
            pushSegment(gantt, -1, time, t);
            time = t;
            admit();
            continue;
        }
        int i = ready.front();
        ready.pop_front();
        Process& c = p[i];
        if (c.getStart() < 0) c.setStart(time);

        int slice = std::min(quantum, c.getRemaining());
        pushSegment(gantt, c.getPid(), time, time + slice);
        time += slice;
        c.setRemaining(c.getRemaining() - slice);
        admit();  // arrivals during the slice queue ahead of the preempted process
        if (c.isFinished()) {
            c.setCompletion(time);
            ++done;
        } else {
            ready.push_back(i);
        }
    }
    return finalize(Algorithm::ROUND_ROBIN, p, gantt, startTime);
}

std::string ProcessScheduler::algorithmName(Algorithm a) {
    switch (a) {
        case Algorithm::FCFS: return "First Come First Serve (FCFS)";
        case Algorithm::SJF: return "Shortest Job First (SJF)";
        case Algorithm::SRTF: return "Shortest Remaining Time First (SRTF)";
        case Algorithm::PRIORITY_NP: return "Priority (Non-preemptive)";
        case Algorithm::PRIORITY_P: return "Priority (Preemptive)";
        case Algorithm::ROUND_ROBIN: return "Round Robin";
    }
    return "";
}

std::string ProcessScheduler::algorithmKey(Algorithm a) {
    switch (a) {
        case Algorithm::FCFS: return "fcfs";
        case Algorithm::SJF: return "sjf";
        case Algorithm::SRTF: return "srtf";
        case Algorithm::PRIORITY_NP: return "priority";
        case Algorithm::PRIORITY_P: return "priority_p";
        case Algorithm::ROUND_ROBIN: return "rr";
    }
    return "";
}

bool ProcessScheduler::parseAlgorithm(const std::string& key, Algorithm& out) {
    for (Algorithm a : allAlgorithms()) {
        if (algorithmKey(a) == key) { out = a; return true; }
    }
    return false;
}

const std::vector<Algorithm>& ProcessScheduler::allAlgorithms() {
    static const std::vector<Algorithm> all = {
        Algorithm::FCFS, Algorithm::SJF, Algorithm::SRTF,
        Algorithm::PRIORITY_NP, Algorithm::PRIORITY_P, Algorithm::ROUND_ROBIN};
    return all;
}
