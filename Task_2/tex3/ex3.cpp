#include <iostream>
#include <fstream>
#include <vector>
#include <omp.h>
#include <cmath>

using namespace std;

const double EPSILON = 1e-5;
const double TAU = 0.0001;
const int MAX_ITERATIONS = 10000;


double compute_residual(const vector<double>& A, const vector<double>& b, const vector<double>& x, int N) {
    vector<double> Ax(N, 0.0);
    
    #pragma omp parallel for schedule(static)
    for (int i = 0; i < N; ++i) {
        for (int j = 0; j < N; ++j) {
            Ax[i] += A[i * N + j] * x[j];
        }
    }
    
    double residual = 0.0;
    #pragma omp parallel for reduction(+:residual)
    for (int i = 0; i < N; ++i) {
        double diff = Ax[i] - b[i];
        residual += diff * diff;
    }
    residual = sqrt(residual);
    
    double norm_b = 0.0;
    #pragma omp parallel for reduction(+:norm_b)
    for (int i = 0; i < N; ++i) {
        norm_b += b[i] * b[i];
    }
    norm_b = sqrt(norm_b);
    
    return residual / norm_b;
}


double simple_iteration_var1(const vector<double>& A, const vector<double>& b, vector<double>& x, int N) {
    vector<double> x_new(N);
    double start = omp_get_wtime();
    int iterations = 0;
    
    while (iterations < MAX_ITERATIONS) {
        #pragma omp parallel for schedule(static)
        for (int i = 0; i < N; ++i) {
            double ax = 0.0;
            for (int j = 0; j < N; ++j) {
                ax += A[i * N + j] * x[j];
            }
            x_new[i] = x[i] - TAU * (ax - b[i]);
        }
        
        double residual = compute_residual(A, b, x_new, N);
        if (residual < EPSILON) break;
        
        #pragma omp parallel for schedule(static)
        for (int i = 0; i < N; ++i) {
            x[i] = x_new[i];
        }
        iterations++;
    }
    return omp_get_wtime() - start;
}


double simple_iteration_var2(const vector<double>& A, const vector<double>& b, vector<double>& x, int N) {
    vector<double> x_new(N);
    double start = omp_get_wtime();
    int iterations = 0;
    
    #pragma omp parallel
    {
        int local_iterations = 0;
        while (true) {
            #pragma omp for schedule(static)
            for (int i = 0; i < N; ++i) {
                double ax = 0.0;
                for (int j = 0; j < N; ++j) {
                    ax += A[i * N + j] * x[j];
                }
                x_new[i] = x[i] - TAU * (ax - b[i]);
            }
            
            double residual;
            #pragma omp single
            {
                residual = compute_residual(A, b, x_new, N);
                if (residual < EPSILON || local_iterations >= MAX_ITERATIONS) {
                    iterations = local_iterations;
                }
            }
            if (residual < EPSILON || local_iterations >= MAX_ITERATIONS) break;
            
            #pragma omp for schedule(static)
            for (int i = 0; i < N; ++i) {
                x[i] = x_new[i];
            }
            #pragma omp single
            {
                local_iterations++;
            }
        }
        #pragma omp single
        {
            iterations = local_iterations;
        }
    }
    return omp_get_wtime() - start;
}

int main() {
    ofstream file("results_ex3.csv");
    if (!file.is_open()) {
        cerr << "Error opening results_ex3.csv\n";
        return 1;
    }
    file << "Nsteps,Threads,Time\n";
    file.close();

    int N = 10000;
    vector<double> A(N * N, 1.0);
    vector<double> b(N, N + 1.0);
    
    for (int i = 0; i < N; ++i) {
        A[i * N + i] = 2.0;
    }
    
    cout << "Variant,Threads,Time\n";
    for (int variant : {1, 2}) {
        for (int threads : {1, 2, 4, 7, 8, 16, 20, 40}) {
            vector<double> x(N, 0.0);
            omp_set_num_threads(threads);
            
            double t;
            if (variant == 1) {
                t = simple_iteration_var1(A, b, x, N);
            } else {
                t = simple_iteration_var2(A, b, x, N);
            }
            
            cout << variant << "," << threads << "," << t << "\n";
            ofstream file("results_ex3.csv", ios::app);
            if (file.is_open()) {
                file << variant << "," << threads << "," << t << "\n";
                file.close();
            } else {
                cerr << "Error opening results_ex3.csv\n";
            }
        }
    }
    return 0;
}