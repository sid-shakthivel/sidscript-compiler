int main() {
    int numbers[5] = {1, 2, 3, 4, 5};
    char str[] = "Hello";
    
    printf("Third number: %d\n", numbers[2]);
    printf("String: %s\n", str);
    
    int *ptr = numbers;
    printf("First number via pointer: %d\n", *ptr);
    ptr = ptr + 1;
    printf("Second number via pointer: %d\n", *ptr);
    
    return 0;
}