int add(int a, int b) {
    return a + b;
}

double multiply(double x, double y) {
    return x * y;
}

int main() {
    int sum = add(5, 3);
    double product = multiply(2.5, 3.0);
    
    printf("Sum: %d\n", sum);
    printf("Product: %f\n", product);
    
    return 0;
}