 fn add(int a, int b) -> int {
    return a + b;
}

fn multiply(double x, double y) -> double {
    return x * y;
}

fn main() -> int {
    int sum = add(5, 3);
    double product = multiply(2.5, 3.0);
    
    printf("Sum: %d\n", sum);
    printf("Product: %f\n", product);
    
    return 0;
}