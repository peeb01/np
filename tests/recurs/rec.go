package main

import (
	"fmt"
	"time"
)

func rec(n int) int {
	if n <= 1 {
		return n
	}
	return n + rec(n-1)
}
func main() {
	var n = 100000
	start := time.Now()
	result := rec(n)
	duration := time.Since(start)

	secs := duration.Seconds()
	ms := secs * 1000.0

	fmt.Printf("Result: %d\n", result)
	fmt.Printf("Duration (seconds): %f\n", secs)
	fmt.Printf("Duration (ms): %f\n", ms)
}