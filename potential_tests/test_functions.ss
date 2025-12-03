int add(int a, int b) {
    return a + b;
}

double multiply(double x, double y) {
    return x * y;
}

int factorial(int n) {
    if (n <= 1) return 1;
    return n * factorial(n - 1);
}

int main() {
    int sum = add(5, 3);
    double product = multiply(2.5, 3.0);
    int fact = factorial(5);
    
    printf("Sum: %d\n", sum);
    printf("Product: %f\n", product);
    printf("Factorial: %d\n", fact);
    
    return 0;
}