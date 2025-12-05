fn main() -> int {
    int numbers[10];
    for (int i = 0; i < 10; i++) {
        numbers[i] = i * i;
    }

    printf("Numbers[5]: %d\n", numbers[5]);

    char str[] = "Hello, World!";
    char str2[20] = "Test";
    
    printf("String: %s\n", str);

    for (int j = 9; j >= 0; j--) {
        printf("%d ", numbers[j]);
    }
    printf("\n");
    
    return 0;
}