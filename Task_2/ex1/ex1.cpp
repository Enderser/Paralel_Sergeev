#include <iostream>
#include <fstream>
#include <vector>
#include <omp.h>
using namespace std;

double matrix_multiply(int N, int num_threads) {
    vector<double> A(N * N);
    vector<double> x(N, 1);
    vector<double> res(N, 0);

    for (int i = 0; i < N; i++)
        for (int j = 0; j < N; j++)
            A[i * N + j] = (i == j) ? 2 : 1;

    omp_set_num_threads(num_threads);

    double start = omp_get_wtime();

    #pragma omp parallel for
    for (int i = 0; i < N; i++) {
        double sum = 0;
        for (int j = 0; j < N; j++)
            sum += A[i * N + j] * x[j];
        res[i] = sum;
    }

    double end = omp_get_wtime();
    return end - start;
}

int main() {
    double t_etalon;
    cout << "Threads,Time,Sp\n";
    for (int matrix_size : {20000, 40000}) {
        for (int threads : {1, 2, 4, 8, 16, 20, 40}) {
            double t = matrix_multiply(matrix_size, threads);
            if (threads == 1) t_etalon = t;
            cout << matrix_size << ',' << threads << ',' << t << ',' << t_etalon / t << "\n";
            
            ofstream file("results.csv", ios::app);
            if (file.is_open()) {
                file << matrix_size << ',' << threads << ',' << t  << ',' << t_etalon / t << "\n";
                file.close();
            } else {
                cerr << "Error opening results.csv\n";
            }
        }
    }
    return 0;
}