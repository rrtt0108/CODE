//2-2까지 했습니다.

#include <iostream>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <queue>
#include <list>
#include <atomic>
#include <vector>
#include <cstring>

using namespace std;

// Process structure
struct Process {
    int id;
    string command;
    bool isForeground;
    bool isPromoted;
    int wakeUpTime; // for wait queue
};

// Dynamic Queue structure
struct DynamicQueue {
    list<Process*> queue;
    mutex mtx;
    condition_variable cv;
    atomic<int> size;
    int threshold;

    DynamicQueue(int threshold) : threshold(threshold) {}

    void enqueue(Process* process) {
        unique_lock<mutex> lock(mtx);
        if (process->isForeground) {
            queue.push_front(process);
        } else {
            queue.push_back(process);
        }
        size++;
        cv.notify_one();
    }

    Process* dequeue() {
        unique_lock<mutex> lock(mtx);
        while (queue.empty()) {
            cv.wait(lock);
        }
        Process* process = queue.front();
        queue.pop_front();
        size--;
        return process;
    }

    void promote() {
        unique_lock<mutex> lock(mtx);
        if (!queue.empty()) {
            Process* process = queue.front();
            queue.pop_front();
            queue.push_back(process);
            process->isPromoted = true;
        }
    }

    void split_n_merge() {
        unique_lock<mutex> lock(mtx);
        if (size > threshold) {
            list<Process*> tempList;
            for (int i = 0; i < threshold / 2; i++) {
                tempList.push_back(queue.front());
                queue.pop_front();
            }
            queue.splice(queue.end(), tempList);
        }
    }
};

// Wait Queue structure
struct WaitQueue {
    list<Process*> queue;
    mutex mtx;

    void enqueue(Process* process) {
        unique_lock<mutex> lock(mtx);
        queue.push_back(process);
        queue.sort([](Process* a, Process* b) {
            return a->wakeUpTime < b->wakeUpTime;
        });
    }

    Process* dequeue() {
        unique_lock<mutex> lock(mtx);
        while (queue.empty()) {
            // wait for wake up time
        }
        Process* process = queue.front();
        queue.pop_front();
        return process;
    }
};

// parse function
char** parse(const char* command) {
    vector<char*> tokens;
    char* token = strtok(const_cast<char*>(command), " ");
    while (token != nullptr) {
        tokens.push_back(token);
        token = strtok(nullptr, " ");
    }
    tokens.push_back(nullptr); // end of tokens

    char** args = new char*[tokens.size()];
    for (int i = 0; i < tokens.size(); i++) {
        args[i] = tokens[i];
    }
    return args;
}

// exec function
void exec(char** args) {
    // execute command
    cout << "Executing command: ";
    for (int i = 0; args[i] != nullptr; i++) {
        cout << args[i] << " ";
    }
    cout << endl;

    // free memory
    delete[] args;
}

// shell process function
void shellProcess(DynamicQueue* dq, WaitQueue* wq) {
    int pid = 0;
    while (true) {
        char command[256];
        cin.getline(command, 256);
        char** args = parse(command);
        Process* process = new Process();
        process->id = pid++;
        process->command = command;
        process->isForeground = true;
        dq->enqueue(process);
        exec(args);
        this_thread::sleep_for(chrono::seconds(2)); // Y seconds
    }
}

// monitor process function
void monitorProcess(DynamicQueue* dq, WaitQueue* wq) {
    while (true) {
        cout << "System state:" << endl;
        cout << "Dynamic Queue: ";
        for (auto it = dq->queue.begin(); it != dq->queue.end(); ++it) {
            Process* process = *it;
            cout << process->id << (process->isForeground ? "F" : "B") << (process->isPromoted ? "*" : "") << " ";
        }
        cout << endl;
        cout << "Wait Queue: ";
        for (auto it = wq->queue.begin(); it != wq->queue.end(); ++it) {
            Process* process = *it;
            cout << process->id << " (wake up in " << process->wakeUpTime << " seconds) ";
        }
        cout << endl;
        this_thread::sleep_for(chrono::seconds(5)); // X seconds
    }
}

int main() {
    DynamicQueue dq(5);
    WaitQueue wq;

    thread shellThread(shellProcess, &dq, &wq);
    thread monitorThread(monitorProcess, &dq, &wq);

    shellThread.join();
    monitorThread.join();

    return 0;
}
