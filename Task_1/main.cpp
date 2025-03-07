#include <iostream>
#include <vector>
#include <cmath>
#include <numeric>
using namespace std;

#ifndef USE_DOUBLE
#define USE_DOUBLE 1
#endif

#if USE_DOUBLE
using Real = double;
#else
using Real = float;
#endif

constexpr size_t N = 10000000;
constexpr Real PI = 3.14159265358979323846;

int main() {
    vector<Real> data(N);
    for (size_t i = 0; i < N; ++i)
        data[i] = sin(2 * PI * i / N);
    Real sum = accumulate(data.begin(), data.end(), Real(0));
    cout << "Sum: " << sum << '\n';
    return 0;
}
