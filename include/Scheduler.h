#ifndef SCHEDULER_H
#define SCHEDULER_H

#include <string>
#include <vector>
#include "Process.h"

enum class Algorithm { FCFS, SJF, SRTF, PRIORITY_NP, PRIORITY_P, ROUND_ROBIN };

// One contiguous slice of CPU time. pid == -1 means the CPU was idle.
struct Segment {
    int pid;
    int start;
    int end;
};

struct Result {
    Algorithm algorithm = Algorithm::FCFS;
    std::string name;
    std::vector<Process> processes;  // with computed metrics, in input order
    std::vector<Segment> gantt;
    int startTime = 0;
    int endTime = 0;
    int contextSwitches = 0;
    double avgWaiting = 0, avgTurnaround = 0, avgResponse = 0;
    double cpuUtilization = 0;  // percent
    double throughput = 0;      // processes per time unit
};

class ProcessScheduler {
public:
    void addProcess(int pid, int arrival, int burst, int priority);
    void clear() { processes_.clear(); }
    std::vector<Process>& getProcesses() { return processes_; }
    const std::vector<Process>& getProcesses() const { return processes_; }

    // Runs one algorithm on a fresh copy of the processes. quantum is used by Round Robin only.
    Result run(Algorithm algorithm, int quantum = 2) const;

    static std::string algorithmName(Algorithm algorithm);
    static std::string algorithmKey(Algorithm algorithm);
    static bool parseAlgorithm(const std::string& key, Algorithm& out);
    static const std::vector<Algorithm>& allAlgorithms();

private:
    Result nonPreemptive(Algorithm algorithm) const;
    Result preemptive(Algorithm algorithm) const;
    Result roundRobin(int quantum) const;

    std::vector<Process> processes_;
};

#endif
