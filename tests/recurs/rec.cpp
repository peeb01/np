#include <stdio.h>
#include <time.h>

long long rec(long long n) {
    if (n <= 1) {
        return n;
    }
    return n + rec(n - 1);
}

double time_now(void) {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return ts.tv_sec + ts.tv_nsec / 1e9;
}

int main(void) {
    long long n = 500000;

    double t_start = time_now();
    long long result = rec(n);
    double t_end = time_now();

    double duration = t_end - t_start;
    double duration_ms = duration * 1000.0;

    printf("Result: %lld\n", result);
    printf("Duration (seconds): %f\n", duration);
    printf("Duration (ms): %f\n", duration_ms);

    return 0;
}