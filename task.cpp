#include <algorithm>
#include <ctime>
#include <functional>
#include <iostream>
#include <map>
#include <queue>
#include <set>
#include <sstream>
#include <string>
#include <vector>

// --- 1. Task Data Structure ---

/**
 * @brief Represents a single task with priority, deadline, and dependencies.
 */
class Task {
public:
  int id;
  std::string name;
  int priority; // Lower number = Higher priority
  std::time_t deadline;
  bool is_completed;
  std::vector<int> dependencies; // IDs of tasks that must be completed first

  // Constructor: Uses initializer lists for default argument (C++11/17)
  Task(int i, const std::string &n, int p, std::time_t d,
       const std::vector<int> &deps = {})
      : id(i), name(n), priority(p), deadline(d), is_completed(false),
        dependencies(deps) {}

  // Comparison for Min-Heap
  bool operator>(const Task &other) const {
    if (priority != other.priority) {
      return priority > other.priority;
    }
    return deadline > other.deadline;
  }

  std::string toString() const {
    std::stringstream ss;
    ss << "ID: " << id << ", Name: " << name << ", Priority: " << priority
       << ", Deadline: " << std::asctime(std::localtime(&deadline))
       << " | Status: " << (is_completed ? "COMPLETED" : "PENDING");

    // Show dependencies if pending
    if (!dependencies.empty() && !is_completed) {
      ss << " | Waiting on: ";
      for (size_t i = 0; i < dependencies.size(); ++i) {
        ss << dependencies[i] << (i < dependencies.size() - 1 ? ", " : "");
      }
    }
    return ss.str();
  }
};

// --- 2. Task Scheduler Class (Min-Heap and Graph Management) ---

class TaskScheduler {
private:
  std::priority_queue<Task, std::vector<Task>, std::greater<Task>> ready_queue;
  std::map<int, Task> all_tasks;
  int next_task_id = 1;
  std::map<int, std::vector<int>> dependency_graph;

  /**
   * @brief Checks for a circular dependency using Depth-First Search (DFS).
   */
  bool isCyclicUtil(int u, std::set<int> &visited,
                    std::set<int> &recursion_stack) {
    visited.insert(u);
    recursion_stack.insert(u);

    for (int v : dependency_graph[u]) {
      if (recursion_stack.count(v)) {
        return true;
      }
      if (!visited.count(v)) {
        if (isCyclicUtil(v, visited, recursion_stack)) {
          return true;
        }
      }
    }

    recursion_stack.erase(u);
    return false;
  }

public:
  // --- Public Interface ---

  /**
   * @brief Adds a new task, verifies dependencies exist, and checks for cycles.
   */
  void addTask(const std::string &name, int priority, std::time_t deadline,
               const std::vector<int> &deps) {
    int new_id = next_task_id++;
    Task new_task(new_id, name, priority, deadline, deps);

    // 1. Check if all dependency IDs currently exist
    for (int dep_id : deps) {
      if (!all_tasks.count(dep_id)) {
        std::cout << "Error: Dependency ID " << dep_id
                  << " not found. Task not added." << std::endl;
        next_task_id--;
        return;
      }
    }

    // 2. Add the new task to 'all_tasks' (master list) using insert to avoid
    // default constructor.
    all_tasks.insert({new_id, new_task});

    // 3. Build a temporary graph to check for cycles
    std::map<int, std::vector<int>> temp_graph;
    for (const auto &pair : all_tasks) {
      int task_id = pair.first;
      for (int dep_id : pair.second.dependencies) {
        temp_graph[dep_id].push_back(task_id);
      }
    }
    dependency_graph = temp_graph;

    // 4. Check for cycles
    if (hasCircularDependency()) {
      std::cout << "\nERROR: Circular dependency detected! Task '" << name
                << "' (ID " << new_id << ") rejected and rolled back."
                << std::endl;
      all_tasks.erase(new_id); // Rollback
      next_task_id--;
      return;
    }

    // 5. Only push to the ready_queue if the task has NO dependencies.
    if (deps.empty()) {
      ready_queue.push(new_task);
      std::cout << "Task '" << name << "' (ID " << new_id
                << ") added successfully to the READY QUEUE." << std::endl;
    } else {
      std::cout << "Task '" << name << "' (ID " << new_id
                << ") added successfully to the WAITING LIST." << std::endl;
    }
  }

  /**
   * @brief Determines if there is a circular dependency in the current task
   * list (graph).
   */
  bool hasCircularDependency() {
    std::set<int> visited;
    std::set<int> recursion_stack;

    for (const auto &pair : all_tasks) {
      int task_id = pair.first;
      if (!visited.count(task_id)) {
        if (isCyclicUtil(task_id, visited, recursion_stack)) {
          return true;
        }
      }
    }
    return false;
  }

  /**
   * @brief Executes the highest priority task and moves dependent tasks to the
   * ready queue.
   */
  Task executeNextTask() {
    if (ready_queue.empty()) {
      std::cout << "No tasks currently in the ready queue." << std::endl;
      return Task(-1, "N/A", 0, 0, {});
    }

    Task next_task = ready_queue.top();
    ready_queue.pop();

    std::cout << "\nEXECUTING TASK: " << next_task.toString() << std::endl;

    // 1. Mark the executed task as completed in the master map
    auto it = all_tasks.find(next_task.id);
    if (it != all_tasks.end()) {
      it->second.is_completed = true;
    }

    // 2. Check all waiting tasks for dependencies met
    std::vector<Task> newly_ready_tasks;

    for (auto &pair : all_tasks) {
      Task &dependent_task = pair.second;

      // Skip completed tasks and tasks already available (no dependencies left)
      if (dependent_task.is_completed || dependent_task.dependencies.empty()) {
        continue;
      }

      // Check if ALL dependencies are met
      bool all_met = true;
      std::vector<int> pending_deps;

      for (int dep_id : dependent_task.dependencies) {
        // FIX: Use find() to safely check existence and access the element
        // without triggering the default constructor requirement.
        auto dep_it = all_tasks.find(dep_id);

        // Dependency is unmet if:
        // a) The ID is missing from the map (dep_it == all_tasks.end())
        // b) The dependency task is found but is not completed.
        if (dep_it == all_tasks.end() || !dep_it->second.is_completed) {
          all_met = false;
          pending_deps.push_back(dep_id);
        }
      }

      // If all dependencies are met, move the task to the ready_queue
      if (all_met) {
        newly_ready_tasks.push_back(dependent_task);
        // Clear the dependencies list as they are no longer pending
        dependent_task.dependencies.clear();
      } else {
        // Update the task's dependency list to only show the *pending* ones
        dependent_task.dependencies = pending_deps;
      }
    }

    // 3. Add newly ready tasks to the ready queue
    for (const auto &task : newly_ready_tasks) {
      ready_queue.push(task);
      std::cout << "-> Dependency Met: Task '" << task.name << "' (ID "
                << task.id << ") moved to Ready Queue." << std::endl;
    }

    return next_task;
  }

  void displayReadyQueue() const {
    std::priority_queue<Task, std::vector<Task>, std::greater<Task>>
        temp_queue = ready_queue;
    std::cout << "\n--- Ready Task Queue (Min-Heap Order) ---" << std::endl;
    if (temp_queue.empty()) {
      std::cout << "Queue is empty." << std::endl;
      return;
    }
    while (!temp_queue.empty()) {
      std::cout << temp_queue.top().toString() << std::endl;
      temp_queue.pop();
    }
    std::cout << "------------------------------------------" << std::endl;
  }

  void displayAllTasks() const {
    std::cout << "\n--- All Tasks (Including Waiting/Completed) ---"
              << std::endl;
    if (all_tasks.empty()) {
      std::cout << "No tasks in the system." << std::endl;
      return;
    }
    for (const auto &pair : all_tasks) {
      std::cout << pair.second.toString() << std::endl;
    }
    std::cout << "-----------------------------------------------" << std::endl;
  }
};

// --- Main Execution ---

int main() {
  TaskScheduler scheduler;
  std::time_t now = std::time(nullptr);

  std::cout << "--- 1. Initial Task Addition & Priority Test ---" << std::endl;

  // T1: P=3, +1hr (Will be executed second due to priority)
  std::time_t d1 = now + 3600;
  scheduler.addTask("Project Alpha Finalize", 3, d1, {});

  // T2: P=5, +10hr (Lower priority)
  std::time_t d2 = now + 36000;
  scheduler.addTask("Review Documentation", 5, d2, {});

  // T3: P=1, +2hr (Highest Priority - runs first)
  std::time_t d3 = now + 7200;
  scheduler.addTask("Hotfix Deployment", 1, d3, {});

  scheduler.displayReadyQueue();

  std::cout << "\n--- 2. Dependency System Setup (T4 and T5 wait) ---"
            << std::endl;

  // T4: P=3, +5hr. WAITS for T3 (ID 3). ID 4. -> WAITING LIST
  std::time_t d4 = now + 18000;
  scheduler.addTask("T4: Compile Code", 3, d4, {3});

  // T5: P=5, +7hr. WAITS for T4 (ID 4). ID 5. -> WAITING LIST
  std::time_t d5 = now + 25200;
  scheduler.addTask("T5: Deploy System", 5, d5, {4});

  // T6: P=2, No deps. ID 6. -> READY QUEUE
  std::time_t d6 = now + 4000;
  scheduler.addTask("T6: Prepare Release Notes", 2, d6, {});

  scheduler.displayReadyQueue();
  scheduler.displayAllTasks();

  std::cout << "\n--- 3. Execution & Dependency Trigger Test ---" << std::endl;

  // Execute T3 (Highest Priority P=1). T4 depends on this.
  scheduler.executeNextTask();

  // T4 should now be in the ready queue. T6 is next highest (P=2).
  scheduler.displayReadyQueue();

  // Execute T6 (Next Highest Priority P=2). T4 is still waiting due to T3
  // execution time.
  scheduler.executeNextTask();

  // Execute T1 (Next Highest Priority P=3).
  scheduler.executeNextTask();

  // Execute T4 (P=3, deadline earlier than T5). T5 depends on this.
  scheduler.executeNextTask();

  // T5 should now be ready. T2 is the last original task.
  scheduler.displayReadyQueue();

  // --- 4. Final Cleanup ---
  scheduler.executeNextTask(); // T5
  scheduler.executeNextTask(); // T2

  std::cout << "\n--- Final State ---" << std::endl;
  scheduler.displayAllTasks();

  return 0;
}
