#include <iostream>
#include <fstream>
#include <vector>
#include <omp.h>
#include <cmath>
using namespace std;

const double a = -4.0;
const double b = 4.0;
const int nsteps = 40000000;

double func(double x){
    return exp(-x * x);
}

double integrate_omp(double (*func)(double), double a, double b, int n, int num_threads){
    double h = (b - a) / n;
    double sum = 0.0;
    omp_set_num_threads(num_threads);

    double start = omp_get_wtime();
    #pragma omp parallel
    {
        int nthreads = omp_get_num_threads();
        int threadid = omp_get_thread_num();
        int items_per_thread = n / nthreads;
        int lb = threadid * items_per_thread;
        int ub = (threadid == nthreads - 1) ? (n - 1) : (lb + items_per_thread - 1);
        double sumloc = 0.0;

        for (int i = lb; i <= ub; i++)
            sumloc += func(a + h * (i + 0.5));
        
        #pragma omp atomic
        sum += sumloc;
    }
    sum *= h;
    double end = omp_get_wtime();
    return end - start;
}

int main(){
    ofstream file("results_ex2.csv");
    if (file.is_open()) {
        file << "Nsteps,Threads,Time\n";
        file.close();
    } else {
        cerr << "Error opening results_ex2.csv\n";
        return 1;
    }

    cout << "Threads,Time\n";
    for (int threads : {1, 2, 4, 7, 8, 16, 20, 40}){
        double t = integrate_omp(func, a, b, nsteps, threads);
        cout << nsteps << ',' << threads << ',' << t << "\n";
        
        ofstream file("results_ex2.csv", ios::app);
        if (file.is_open()) {
            file << nsteps << ',' << threads << ',' << t << "\n";
            file.close();
        } else {
            cerr << "Error opening results.csv\n";
        }
    }
    return 0;
}