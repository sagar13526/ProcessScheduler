#ifndef PROCESS_H
#define PROCESS_H

// A single process and the values the scheduler computes for it.
class Process {
public:
    Process(int pid, int arrival, int burst, int priority);

    int getPid() const { return pid_; }
    int getArrival() const { return arrival_; }
    int getBurst() const { return burst_; }
    int getPriority() const { return priority_; }
    int getRemaining() const { return remaining_; }
    int getStart() const { return start_; }
    int getCompletion() const { return completion_; }
    int getWaiting() const { return waiting_; }
    int getTurnaround() const { return turnaround_; }
    int getResponse() const { return response_; }
    bool isFinished() const { return remaining_ == 0; }

    void setRemaining(int v) { remaining_ = v; }
    void setStart(int v) { start_ = v; }
    void setCompletion(int v) { completion_ = v; }
    void setWaiting(int v) { waiting_ = v; }
    void setTurnaround(int v) { turnaround_ = v; }
    void setResponse(int v) { response_ = v; }

    // Clears everything the simulation computed so a process can be re-run.
    void reset();

private:
    int pid_, arrival_, burst_, priority_;
    int remaining_, start_, completion_, waiting_, turnaround_, response_;
};

#endif
