#include <iostream>
#include <chrono>

using namespace std;

long long compute(long long n) {
    long long total = 0;
    long long val = 1;

    for (long long i = 0; i < n; i++) {
        total += val;
        val = (val * 3) / 2;
        if (val > 100000) {
            val = 1;
        }
    }

    return total;
}

int main() {
    cout << "--- C++ Math Loop Benchmark ---" << endl;

    long long n = 500000000LL;

    auto start = chrono::high_resolution_clock::now();
    long long result = compute(n);
    auto end = chrono::high_resolution_clock::now();

    chrono::duration<double> elapsed = end - start;

    cout << "Result: " << result << endl;
    cout << "Time taken: " << elapsed.count() << " seconds" << endl;

    return 0;
}