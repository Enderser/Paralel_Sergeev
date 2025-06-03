#include <iostream>
#include <queue>
#include <unordered_map>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <future>
#include <fstream>
#include <random>
#include <chrono>
#include <iomanip> 

template<typename T>
class Server {
private:
    std::queue<std::pair<size_t, std::packaged_task<T()>>> tasks;
    std::unordered_map<size_t, T> results;
    std::mutex mtx;
    std::condition_variable cond_var;
    std::jthread server_thread;
    bool running = false;

    void process_tasks(std::stop_token stoken) {
        std::unique_lock lock(mtx, std::defer_lock);
        while (!stoken.stop_requested()) {
            lock.lock();
            cond_var.wait(lock, [this, &stoken] { return !tasks.empty() || stoken.stop_requested(); });
            if (stoken.stop_requested()) break;
            auto [id, task] = std::move(tasks.front());
            tasks.pop();
            lock.unlock();
            task();
            lock.lock();
            results[id] = task.get_future().get();
            lock.unlock();
            cond_var.notify_all();
        }
        std::cout << "Server stopped\n";
    }

public:
    void start() {
        running = true;
        server_thread = std::jthread([this](std::stop_token stoken) { this->process_tasks(stoken); });
    }

    void stop() {
        running = false;
        server_thread.request_stop();
        cond_var.notify_all();
    }

    size_t add_task(std::packaged_task<T()> task) {
        std::lock_guard lock(mtx);
        static size_t id = 0;
        tasks.push({++id, std::move(task)});
        cond_var.notify_one();
        return id;
    }

    T request_result(size_t id) {
        std::unique_lock lock(mtx);
        cond_var.wait(lock, [this, id] { return results.find(id) != results.end(); });
        T result = results[id];
        results.erase(id);
        return result;
    }
};

void client_thread(Server<double>& server, std::string task_type, size_t N, const std::string& filename) {
    std::ofstream out(filename);
    out << "Task ID,Task Type,Argument,Result\n";

    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_real_distribution<double> dis(0.0, 10.0);

    for (size_t i = 0; i < N; ++i) {
        double arg = dis(gen);
        std::packaged_task<double()> task;
        if (task_type == "sin") {
            task = std::packaged_task<double()>([arg]() { return std::sin(arg); });
        } else if (task_type == "sqrt") {
            task = std::packaged_task<double()>([arg]() { return std::sqrt(arg); });
        } else if (task_type == "pow") {
            task = std::packaged_task<double()>([arg]() { return std::pow(arg, 2); });
        }
        size_t id = server.add_task(std::move(task));
        double result = server.request_result(id);
        out << id << "," << task_type << "," << std::fixed << std::setprecision(6) << arg << "," << result << "\n";
    }
    out.close();
}

void test_results(const std::string& filename, const std::string& task_type) {
    std::ifstream in(filename);
    std::string line;
    std::getline(in, line); // Пропускаем заголовок
    while (std::getline(in, line)) {
        if (line.find(task_type) == std::string::npos) {
            std::cerr << "Error in " << filename << ": unexpected task type\n";
        }
    }
    in.close();
}

int main() {
    Server<double> server;
    server.start();

    size_t N = 1000;
    std::vector<std::jthread> clients;
    clients.emplace_back(client_thread, std::ref(server), "sin", N, "sin_results.csv");
    clients.emplace_back(client_thread, std::ref(server), "sqrt", N, "sqrt_results.csv");
    clients.emplace_back(client_thread, std::ref(server), "pow", N, "pow_results.csv");

    for (auto& t : clients) t.join();
    server.stop();

    test_results("sin_results.csv", "sin");
    test_results("sqrt_results.csv", "sqrt");
    test_results("pow_results.csv", "pow");

    return 0;
}