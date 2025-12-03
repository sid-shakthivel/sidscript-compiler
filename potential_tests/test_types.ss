int main() {
    int i = 42;
    long l = 1234567890L;
    double d = 3.14159;
    
    double result1 = i + d;
    long result2 = l + i;
    int result3 = d;  
    
    printf("Int + Double: %f\n", result1);
    printf("Long + Int: %ld\n", result2);
    printf("Double to Int: %d\n", result3);
    
    return 0;
}