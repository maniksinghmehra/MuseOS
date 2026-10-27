#pragma once

#include <algorithm>
#include <iostream>
#include <limits>
#include <queue>
#include <string>
#include <vector>

namespace museos_os {

struct Process {
    int id;
    int arrivalTime;
    int burstTime;
    int remainingTime;
    int startTime = -1;
    int completionTime = 0;
};

struct CpuSegment {
    int processId;
    int start;
    int end;
};

struct Schedule {
    std::string name;
    std::vector<Process> processes;
    std::vector<CpuSegment> timeline;
};

bool readNumber(const char* prompt, int& value, int minimum) {
    std::cout << prompt;

    if (!(std::cin >> value)) {
        std::cin.clear();
        std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
        std::cout << "Please enter a number.\n";
        return false;
    }

    if (value < minimum) {
        std::cout << "Please enter a value of at least " << minimum << ".\n";
        return false;
    }

    return true;
}

std::vector<Process> readProcesses() {
    int count;

    if (!readNumber("How many processes? ", count, 1)) {
        return {};
    }

    std::vector<Process> processes;

    for (int i = 0; i < count; ++i) {
        int arrival;
        int burst;

        std::cout << "\nProcess P" << i + 1 << '\n';

        if (!readNumber("Arrival time (0 or more): ", arrival, 0)) {
            return {};
        }

        if (!readNumber("CPU burst time (1 or more): ", burst, 1)) {
            return {};
        }

        processes.push_back({i + 1, arrival, burst, burst});
    }

    return processes;
}

Schedule runFcfs(std::vector<Process> processes) {
    Schedule result{"FCFS", processes, {}};

    std::sort(result.processes.begin(), result.processes.end(),
        [](const Process& a, const Process& b) {
            if (a.arrivalTime != b.arrivalTime) {
                return a.arrivalTime < b.arrivalTime;
            }
            return a.id < b.id;
        });

    int time = 0;

    for (Process& process : result.processes) {
        if (time < process.arrivalTime) {
            time = process.arrivalTime;
        }

        process.startTime = time;
        int start = time;
        time += process.burstTime;
        process.completionTime = time;
        result.timeline.push_back({process.id, start, time});
    }

    return result;
}

Schedule runSjf(std::vector<Process> processes) {
    Schedule result{"SJF", processes, {}};
    std::vector<bool> completed(result.processes.size(), false);

    int finishedCount = 0;
    int time = 0;

    while (finishedCount < static_cast<int>(result.processes.size())) {
        int selected = -1;

        for (int i = 0; i < static_cast<int>(result.processes.size()); ++i) {
            const Process& candidate = result.processes[i];

            if (completed[i] || candidate.arrivalTime > time) {
                continue;
            }

            if (selected == -1 ||
                candidate.burstTime < result.processes[selected].burstTime ||
                (candidate.burstTime == result.processes[selected].burstTime &&
                 candidate.arrivalTime < result.processes[selected].arrivalTime)) {
                selected = i;
            }
        }

        if (selected == -1) {
            int nextArrival = std::numeric_limits<int>::max();

            for (int i = 0; i < static_cast<int>(result.processes.size()); ++i) {
                if (!completed[i]) {
                    nextArrival = std::min(
                        nextArrival, result.processes[i].arrivalTime);
                }
            }

            time = nextArrival;
            continue;
        }

        Process& process = result.processes[selected];
        process.startTime = time;
        int start = time;
        time += process.burstTime;
        process.completionTime = time;
        completed[selected] = true;
        ++finishedCount;

        result.timeline.push_back({process.id, start, time});
    }

    return result;
}

Schedule runRoundRobin(std::vector<Process> processes, int quantum) {
    Schedule result{"Round Robin", processes, {}};

    std::vector<int> order(result.processes.size());

    for (int i = 0; i < static_cast<int>(order.size()); ++i) {
        order[i] = i;
    }

    std::sort(order.begin(), order.end(),
        [&result](int left, int right) {
            if (result.processes[left].arrivalTime !=
                result.processes[right].arrivalTime) {
                return result.processes[left].arrivalTime <
                       result.processes[right].arrivalTime;
            }
            return result.processes[left].id < result.processes[right].id;
        });

    std::queue<int> ready;
    int nextProcess = 0;
    int time = 0;
    int finishedCount = 0;

    while (finishedCount < static_cast<int>(result.processes.size())) {
        if (ready.empty() && nextProcess < static_cast<int>(order.size()) &&
            time < result.processes[order[nextProcess]].arrivalTime) {
            time = result.processes[order[nextProcess]].arrivalTime;
        }

        while (nextProcess < static_cast<int>(order.size()) &&
               result.processes[order[nextProcess]].arrivalTime <= time) {
            ready.push(order[nextProcess]);
            ++nextProcess;
        }

        if (ready.empty()) {
            continue;
        }

        int index = ready.front();
        ready.pop();

        Process& process = result.processes[index];

        if (process.startTime == -1) {
            process.startTime = time;
        }

        int runTime = std::min(quantum, process.remainingTime);
        int start = time;
        time += runTime;
        process.remainingTime -= runTime;
        result.timeline.push_back({process.id, start, time});

        while (nextProcess < static_cast<int>(order.size()) &&
               result.processes[order[nextProcess]].arrivalTime <= time) {
            ready.push(order[nextProcess]);
            ++nextProcess;
        }

        if (process.remainingTime > 0) {
            ready.push(index);
        } else {
            process.completionTime = time;
            ++finishedCount;
        }
    }

    return result;
}

void printSchedule(const Schedule& schedule) {
    double totalWaiting = 0;
    double totalTurnaround = 0;
    double totalResponse = 0;

    std::cout << "\n--- " << schedule.name << " ---\n";
    std::cout << "CPU timeline: ";

    for (const CpuSegment& segment : schedule.timeline) {
        std::cout << "P" << segment.processId
                  << "[" << segment.start << "-" << segment.end << "] ";
    }

    std::cout << "\n\nProcess results:\n";

    for (const Process& process : schedule.processes) {
        int turnaround = process.completionTime - process.arrivalTime;
        int waiting = turnaround - process.burstTime;
        int response = process.startTime - process.arrivalTime;

        totalWaiting += waiting;
        totalTurnaround += turnaround;
        totalResponse += response;

        std::cout << "P" << process.id
                  << ": start=" << process.startTime
                  << ", finish=" << process.completionTime
                  << ", waiting=" << waiting
                  << ", turnaround=" << turnaround
                  << ", response=" << response << '\n';
    }

    double count = schedule.processes.size();

    std::cout << "Average waiting time: " << totalWaiting / count << '\n'
              << "Average turnaround time: " << totalTurnaround / count << '\n'
              << "Average response time: " << totalResponse / count << '\n';
}

void runCpuScheduling() {
    std::cout << "\nEnter the same workload for all three algorithms.\n";
    std::vector<Process> processes = readProcesses();

    if (processes.empty()) {
        return;
    }

    int quantum;
    if (!readNumber("Round Robin time quantum (1 or more): ", quantum, 1)) {
        return;
    }

    printSchedule(runFcfs(processes));
    printSchedule(runSjf(processes));
    printSchedule(runRoundRobin(processes, quantum));
}

enum class FitMethod {
    First,
    Best,
    Worst
};

void runMemoryMethod(
    const std::string& name,
    const std::vector<int>& originalBlocks,
    const std::vector<int>& requests,
    FitMethod method) {

    std::vector<int> freeBlocks = originalBlocks;
    int allocatedCount = 0;

    std::cout << "\n--- " << name << " ---\n";

    for (int p = 0; p < static_cast<int>(requests.size()); ++p) {
        int selected = -1;

        for (int b = 0; b < static_cast<int>(freeBlocks.size()); ++b) {
            if (freeBlocks[b] < requests[p]) {
                continue;
            }

            if (method == FitMethod::First) {
                selected = b;
                break;
            }

            if (selected == -1 ||
                (method == FitMethod::Best &&
                 freeBlocks[b] < freeBlocks[selected]) ||
                (method == FitMethod::Worst &&
                 freeBlocks[b] > freeBlocks[selected])) {
                selected = b;
            }
        }

        if (selected == -1) {
            std::cout << "Process P" << p + 1 << " request "
                      << requests[p] << ": not allocated\n";
        } else {
            freeBlocks[selected] -= requests[p];
            ++allocatedCount;

            std::cout << "Process P" << p + 1 << " request "
                      << requests[p] << ": placed in block B" << selected + 1
                      << ", block space left=" << freeBlocks[selected] << '\n';
        }
    }

    int totalFree = 0;
    int largestFreeBlock = 0;

    for (int space : freeBlocks) {
        totalFree += space;
        largestFreeBlock = std::max(largestFreeBlock, space);
    }

    std::cout << "Processes allocated: " << allocatedCount
              << " of " << requests.size() << '\n'
              << "Total free memory left: " << totalFree << '\n'
              << "Largest free block: " << largestFreeBlock << '\n'
              << "Free space outside the largest block: "
              << totalFree - largestFreeBlock << '\n';
}

void runMemoryAllocation() {
    int blockCount;
    if (!readNumber("Number of free memory blocks: ", blockCount, 1)) {
        return;
    }

    std::vector<int> blocks;

    for (int i = 0; i < blockCount; ++i) {
        int size;
        std::string prompt =
            "Size of block B" + std::to_string(i + 1) + ": ";

        if (!readNumber(prompt.c_str(), size, 1)) {
            return;
        }

        blocks.push_back(size);
    }

    int processCount;
    if (!readNumber("Number of process requests: ", processCount, 1)) {
        return;
    }

    std::vector<int> requests;

    for (int i = 0; i < processCount; ++i) {
        int size;
        std::string prompt =
            "Memory request for process P" + std::to_string(i + 1) + ": ";

        if (!readNumber(prompt.c_str(), size, 1)) {
            return;
        }

        requests.push_back(size);
    }

    runMemoryMethod("First Fit", blocks, requests, FitMethod::First);
    runMemoryMethod("Best Fit", blocks, requests, FitMethod::Best);
    runMemoryMethod("Worst Fit", blocks, requests, FitMethod::Worst);
}

void runOsSimulator() {
    int choice = -1;

    while (choice != 0) {
        std::cout << "\n=== MuseOS: OS Simulator ===\n"
                  << "1. Compare CPU scheduling algorithms\n"
                  << "2. Compare memory allocation algorithms\n"
                  << "0. Return to the main menu\n";

        if (!readNumber("Choose an option: ", choice, 0)) {
            continue;
        }

        if (choice == 1) {
            runCpuScheduling();
        } else if (choice == 2) {
            runMemoryAllocation();
        } else if (choice != 0) {
            std::cout << "Choose 1, 2, or 0.\n";
        }
    }
}

} // namespace museos_os