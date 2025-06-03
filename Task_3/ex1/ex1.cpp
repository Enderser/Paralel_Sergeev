#include <vector>
#include <thread>
#include <chrono>
#include <iostream>
#include <random>
#include <fstream>
using namespace std;

void initialize_matrix(vector<vector<double>>& matrix, size_t start, size_t end) {
    random_device rd;
    mt19937 gen(rd());
    uniform_real_distribution<> dis(0.0, 1.0);
    for (size_t i = start; i < end; ++i) {
        for (size_t j = 0; j < matrix[i].size(); ++j) {
            matrix[i][j] = dis(gen);
        }
    }
}

void initialize_vector(vector<double>& vec, size_t start, size_t end) {
    random_device rd;
    mt19937 gen(rd());
    uniform_real_distribution<> dis(0.0, 1.0);
    for (size_t i = start; i < end; ++i) {
        vec[i] = dis(gen);
    }
}

void matrix_vector_multiply(const vector<vector<double>>& matrix,
                            const vector<double>& vec,
                            vector<double>& result,
                            size_t start, size_t end) {
    for (size_t i = start; i < end; ++i) {
        double sum = 0.0;
        for (size_t j = 0; j < vec.size(); ++j) {
            sum += matrix[i][j] * vec[j];
        }
        result[i] = sum;
    }
}

double run_parallel(size_t N, int num_threads, ofstream& csv_file) {
    vector<vector<double>> matrix(N, vector<double>(N));
    vector<double> vec(N), result(N);
    vector<jthread> threads;

    // Параллельная инициализация
    size_t chunk_size = N / num_threads;
    auto start_time = chrono::high_resolution_clock::now();
    for (int i = 0; i < num_threads; ++i) {
        size_t start = i * chunk_size;
        size_t end = (i == num_threads - 1) ? N : start + chunk_size;
        threads.emplace_back(initialize_matrix, ref(matrix), start, end);
        threads.emplace_back(initialize_vector, ref(vec), start, end);
    }
    threads.clear(); 

    threads.clear();
    for (int i = 0; i < num_threads; ++i) {
        size_t start = i * chunk_size;
        size_t end = (i == num_threads - 1) ? N : start + chunk_size;
        threads.emplace_back(matrix_vector_multiply, ref(matrix), ref(vec), ref(result), start, end);
    }
    threads.clear(); 

    auto end_time = chrono::high_resolution_clock::now();
    auto duration = chrono::duration_cast<chrono::milliseconds>(end_time - start_time).count();
    cout << "MatrixSize: " << N << ", Threads: " << num_threads << ", Time: " << duration << " ms\n";
    return static_cast<double>(duration);
}

int main() {
    ofstream csv_file("results_ex1.csv");
    csv_file << "MatrixSize,Threads,T_p(ms),Speedup\n"; 

    vector<size_t> sizes = {20000, 40000};
    vector<int> thread_counts = {1, 2, 4, 7, 8, 16, 20, 40};
    vector<vector<double>> times(sizes.size(), vector<double>(thread_counts.size()));

    for (size_t i = 0; i < sizes.size(); ++i) {
        for (size_t j = 0; j < thread_counts.size(); ++j) {
            times[i][j] = run_parallel(sizes[i], thread_counts[j], csv_file);
        }
        for (size_t j = 0; j < thread_counts.size(); ++j) {
            double speedup = times[i][0] / times[i][j];
            csv_file << sizes[i] << "," << thread_counts[j] << "," << times[i][j] << "," << speedup << "\n";
            cout << "MatrixSize: " << sizes[i] << ", Threads: " << thread_counts[j] << ", Speedup: " << speedup << "\n";
        }
    }
    csv_file.close();
    return 0;
}