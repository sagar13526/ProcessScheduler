#include "Utils.h"
#include <algorithm>
#include <cstdio>
#include <cstdlib>
#include <iomanip>
#include <iostream>
#include <set>
#include <sstream>

namespace Utils {

namespace {
std::string fmt(double v) {
    char buf[32];
    std::snprintf(buf, sizeof(buf), "%.2f", v);
    return buf;
}
const std::string kRule(100, '-');
}  // namespace

void printResult(const Result& r) {
    std::cout << "\n" << std::string(40, '=') << "\nAlgorithm: " << r.name << "\n"
              << std::string(40, '=') << "\n";
    std::cout << std::left << std::setw(8) << "PID" << std::setw(10) << "Arrival" << std::setw(8) << "Burst"
              << std::setw(10) << "Priority" << std::setw(8) << "Start" << std::setw(12) << "Completion"
              << std::setw(10) << "Waiting" << std::setw(12) << "Turnaround" << "Response\n"
              << kRule << "\n";
    for (const Process& p : r.processes) {
        std::cout << std::left << std::setw(8) << p.getPid() << std::setw(10) << p.getArrival() << std::setw(8)
                  << p.getBurst() << std::setw(10) << p.getPriority() << std::setw(8) << p.getStart()
                  << std::setw(12) << p.getCompletion() << std::setw(10) << p.getWaiting() << std::setw(12)
                  << p.getTurnaround() << p.getResponse() << "\n";
    }
    std::cout << kRule << "\n"
              << "Average Waiting Time:    " << fmt(r.avgWaiting) << "\n"
              << "Average Turnaround Time: " << fmt(r.avgTurnaround) << "\n"
              << "Average Response Time:   " << fmt(r.avgResponse) << "\n"
              << "CPU Utilization:         " << fmt(r.cpuUtilization) << "%\n"
              << "Throughput:              " << fmt(r.throughput) << " processes/unit\n"
              << "Context Switches:        " << r.contextSwitches << "\n";
    printGantt(r);
}

void printGantt(const Result& r) {
    if (r.gantt.empty()) return;
    std::string bar = "|", times = std::to_string(r.gantt.front().start);
    for (const Segment& s : r.gantt) {
        size_t width = std::max<size_t>(4, std::min<size_t>((s.end - s.start) * 2, 14));
        std::string label = s.pid < 0 ? "idle" : "P" + std::to_string(s.pid);
        size_t pad = width - 1 > label.size() ? width - 1 - label.size() : 0;
        bar += std::string(pad / 2, ' ') + label + std::string(pad - pad / 2, ' ') + "|";
        // pad the time line so the next number sits under the next '|'
        size_t target = bar.size() - 1;
        if (times.size() < target) times += std::string(target - times.size(), ' ');
        times += std::to_string(s.end);
    }
    std::cout << "\nGantt chart\n" << bar << "\n" << times << "\n";
}

void printComparison(const std::vector<Result>& results) {
    std::cout << "\n" << std::left << std::setw(40) << "Algorithm" << std::setw(12) << "Avg wait" << std::setw(12)
              << "Avg TAT" << std::setw(12) << "Avg resp" << std::setw(10) << "CPU %" << "Switches\n"
              << std::string(94, '-') << "\n";
    for (const Result& r : results) {
        std::cout << std::left << std::setw(40) << r.name << std::setw(12) << fmt(r.avgWaiting) << std::setw(12)
                  << fmt(r.avgTurnaround) << std::setw(12) << fmt(r.avgResponse) << std::setw(10)
                  << fmt(r.cpuUtilization) << r.contextSwitches << "\n";
    }
}

int readInt(const std::string& prompt, int min, int max) {
    std::string line;
    while (true) {
        std::cout << prompt;
        if (!std::getline(std::cin, line)) return min;  // EOF: caller decides what to do
        std::istringstream in(line);
        long v;
        char extra;
        if ((in >> v) && !(in >> extra) && v >= min && v <= max) return static_cast<int>(v);
        std::cout << "Please enter a whole number from " << min << " to " << max << ".\n";
    }
}

void loadSampleProcesses(ProcessScheduler& s) {
    s.clear();
    // (PID, Arrival, Burst, Priority)
    s.addProcess(1, 0, 5, 2);
    s.addProcess(2, 1, 3, 1);
    s.addProcess(3, 2, 8, 3);
    s.addProcess(4, 3, 6, 2);
    s.addProcess(5, 4, 4, 4);
}

void loadRandomProcesses(ProcessScheduler& s, int count) {
    s.clear();
    for (int i = 1; i <= count; ++i) s.addProcess(i, std::rand() % 10, 1 + std::rand() % 9, 1 + std::rand() % 5);
}

void inputCustomProcesses(ProcessScheduler& s) {
    s.clear();
    int n = readInt("How many processes (1-20)? ", 1, 20);
    for (int i = 1; i <= n; ++i) {
        std::cout << "Process " << i << ":\n";
        int arrival = readInt("  Arrival time (>= 0): ", 0, 100000);
        int burst = readInt("  Burst time (>= 1):   ", 1, 100000);
        int priority = readInt("  Priority (lower = higher): ", 0, 100000);
        s.addProcess(i, arrival, burst, priority);
    }
}

bool validateProcesses(const std::vector<Process>& ps, std::string& error) {
    if (ps.empty()) { error = "Add at least one process."; return false; }
    std::set<int> seen;
    for (const Process& p : ps) {
        std::string id = "P" + std::to_string(p.getPid());
        if (!seen.insert(p.getPid()).second) { error = id + " appears more than once."; return false; }
        if (p.getArrival() < 0) { error = "Arrival time of " + id + " can't be negative."; return false; }
        if (p.getBurst() < 1) { error = "Burst time of " + id + " must be at least 1."; return false; }
    }
    return true;
}

std::string resultToJson(const Result& r) {
    std::ostringstream o;
    o << "{\"algorithm\":\"" << ProcessScheduler::algorithmKey(r.algorithm) << "\",\"name\":\"" << r.name
      << "\",\"startTime\":" << r.startTime << ",\"endTime\":" << r.endTime << ",\"processes\":[";
    for (size_t i = 0; i < r.processes.size(); ++i) {
        const Process& p = r.processes[i];
        o << (i ? "," : "") << "{\"pid\":" << p.getPid() << ",\"arrival\":" << p.getArrival()
          << ",\"burst\":" << p.getBurst() << ",\"priority\":" << p.getPriority() << ",\"start\":" << p.getStart()
          << ",\"completion\":" << p.getCompletion() << ",\"waiting\":" << p.getWaiting()
          << ",\"turnaround\":" << p.getTurnaround() << ",\"response\":" << p.getResponse() << "}";
    }
    o << "],\"gantt\":[";
    for (size_t i = 0; i < r.gantt.size(); ++i) {
        o << (i ? "," : "") << "{\"pid\":" << r.gantt[i].pid << ",\"start\":" << r.gantt[i].start
          << ",\"end\":" << r.gantt[i].end << "}";
    }
    o << "],\"metrics\":{\"avgWaiting\":" << fmt(r.avgWaiting) << ",\"avgTurnaround\":" << fmt(r.avgTurnaround)
      << ",\"avgResponse\":" << fmt(r.avgResponse) << ",\"cpuUtilization\":" << fmt(r.cpuUtilization)
      << ",\"throughput\":" << fmt(r.throughput) << ",\"contextSwitches\":" << r.contextSwitches << "}}";
    return o.str();
}

}  // namespace Utils
