#include "Process.h"

Process::Process(int pid, int arrival, int burst, int priority)
    : pid_(pid), arrival_(arrival), burst_(burst), priority_(priority) {
    reset();
}

void Process::reset() {
    remaining_ = burst_;
    start_ = -1;
    completion_ = 0;
    waiting_ = 0;
    turnaround_ = 0;
    response_ = 0;
}
