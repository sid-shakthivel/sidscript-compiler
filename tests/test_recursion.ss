int fibonacci(int n) {
    if (n <= 1) return n;
    return fibonacci(n - 1) + fibonacci(n - 2);
}

double calculate(int a, double b, long c) {
    return a * b + c;
}

void print_array(int arr[], int size) {
    for (int i = 0; i < size; i = i + 1) {
        printf("%d ", arr[i]);
    }
    printf("\n");
}

int main() {
    printf("Fibonacci(7): %d\n", fibonacci(7));
    
    double result = calculate(5, 3.14, 1000L);
    printf("Calculate: %f\n", result);
    
    int arr[] = {1, 2, 3, 4, 5};
    print_array(arr, 5);
    
    return 0;
}