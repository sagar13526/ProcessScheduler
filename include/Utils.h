#ifndef UTILS_H
#define UTILS_H

#include <string>
#include <vector>
#include "Scheduler.h"

namespace Utils {

// Console output
void printResult(const Result& result);
void printGantt(const Result& result);
void printComparison(const std::vector<Result>& results);

// Input
int readInt(const std::string& prompt, int min, int max);
void loadSampleProcesses(ProcessScheduler& scheduler);
void loadRandomProcesses(ProcessScheduler& scheduler, int count);
void inputCustomProcesses(ProcessScheduler& scheduler);

// Returns false and fills `error` if the process list can't be simulated.
bool validateProcesses(const std::vector<Process>& processes, std::string& error);

// JSON used by the web frontend (see `scheduler --json`).
std::string resultToJson(const Result& result);

}  // namespace Utils

#endif
