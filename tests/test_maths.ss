int main() {
    int a = 5;
    int b = 3;
    double c = 5.5;
    long d = 1000000;
    
    int result1 = a + b * (c + d);
    double result2 = c * a / b;
    
    double converted = a + c;
    int truncated = c;
    
    printf("Basic arithmetic: %d\n", result1);
    printf("Float arithmetic: %f\n", result2);
    printf("Conversion test: %f, %d\n", converted, truncated);
    
    return 0;
}