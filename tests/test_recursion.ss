fn fibonacci(int n) -> int {
    if (n <= 1) {
        return n;
    }

    return fibonacci(n - 1) + fibonacci(n - 2);
}

fn factorial(int n) -> int {
    if (n <= 1)
    {
        return 1;
    }

    return n * factorial(n - 1);
}

fn printArray(int arr[], int size) -> void {
    for (int i = 0; i < size; i++) {
        printf("%d ", arr[i]);
    }
    printf("\n");
}

fn main() -> int {    
    printf("Fibonacci(7): %d\n", fibonacci(7));
    
    int arr[] = {1, 2, 3, 4, 5};

    printArray(arr, 5);

    int fact = factorial(5);
    printf("Factorial: %d\n", fact);

    return 0;
}