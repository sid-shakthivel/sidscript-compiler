int main() {
    int numbers[10];
    for (int i = 0; i < 10; i = i + 1) {
        numbers[i] = i * i;
    }
    
    char str[] = "Hello, World!";
    char str2[20] = "Test";
    
    printf("String: %s\n", str);
    printf("Numbers[5]: %d\n", numbers[5]);
    
    for (int i = 9; i >= 0; i = i - 1) {
        printf("%d ", numbers[i]);
    }
    printf("\n");
    
    return 0;
}