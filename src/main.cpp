#include <iostream>
#include <sstream>
#include <string>
#include "Scheduler.h"
#include "Utils.h"

namespace {

// `scheduler --json <algorithm|all> <quantum>` reads "pid arrival burst priority" lines from stdin
// and prints {"results":[...]}. The web server uses this mode.
int runJsonMode(const std::string& key, int quantum) {
    ProcessScheduler scheduler;
    int pid, arrival, burst, priority;
    while (std::cin >> pid >> arrival >> burst >> priority) scheduler.addProcess(pid, arrival, burst, priority);

    std::string error;
    if (!Utils::validateProcesses(scheduler.getProcesses(), error)) {
        std::cout << "{\"error\":\"" << error << "\"}\n";
        return 1;
    }

    std::vector<Algorithm> algorithms;
    Algorithm a;
    if (key == "all") algorithms = ProcessScheduler::allAlgorithms();
    else if (ProcessScheduler::parseAlgorithm(key, a)) algorithms.push_back(a);
    else {
        std::cout << "{\"error\":\"Unknown algorithm.\"}\n";
        return 1;
    }

    std::cout << "{\"results\":[";
    for (size_t i = 0; i < algorithms.size(); ++i)
        std::cout << (i ? "," : "") << Utils::resultToJson(scheduler.run(algorithms[i], quantum));
    std::cout << "]}\n";
    return 0;
}

void showMenu() {
    std::cout << "\n===== Process Scheduler Simulator =====\n"
                 "1. First Come First Serve (FCFS)\n"
                 "2. Shortest Job First (SJF)\n"
                 "3. Shortest Remaining Time (SRTF)\n"
                 "4. Priority (Non-preemptive)\n"
                 "5. Priority (Preemptive)\n"
                 "6. Round Robin\n"
                 "7. Run All Algorithms\n"
                 "8. Load Sample Processes\n"
                 "9. Input Custom Processes\n"
                 "0. Exit\n";
}

}  // namespace

int main(int argc, char* argv[]) {
    if (argc >= 2 && std::string(argv[1]) == "--json") {
        int quantum = argc >= 4 ? std::atoi(argv[3]) : 2;
        return runJsonMode(argc >= 3 ? argv[2] : "all", quantum);
    }

    ProcessScheduler scheduler;
    Utils::loadSampleProcesses(scheduler);
    const std::vector<Algorithm>& all = ProcessScheduler::allAlgorithms();

    while (true) {
        showMenu();
        int choice = Utils::readInt("Choose an option: ", 0, 9);
        if (choice == 0) break;

        if (choice >= 1 && choice <= 7) {
            int quantum = 2;
            if (choice == 6 || choice == 7) quantum = Utils::readInt("Time quantum for Round Robin (1-100): ", 1, 100);
            if (choice == 7) {
                std::vector<Result> results;
                for (Algorithm a : all) {
                    results.push_back(scheduler.run(a, quantum));
                    Utils::printResult(results.back());
                }
                Utils::printComparison(results);
            } else {
                Utils::printResult(scheduler.run(all[choice - 1], quantum));
            }
        } else if (choice == 8) {
            Utils::loadSampleProcesses(scheduler);
            std::cout << "Sample processes loaded.\n";
        } else {
            Utils::inputCustomProcesses(scheduler);
        }
    }
    return 0;
}
