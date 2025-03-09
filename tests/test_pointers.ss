int main() {
    int x = 42;
    int *ptr = &x;
    int **ptr2 = &ptr;
    
    printf("Value: %d\n", *ptr);
    printf("Double pointer: %d\n", **ptr2);
    
    *ptr = 100;
    printf("Modified value: %d\n", x);
    
    int arr[5] = {1, 2, 3, 4, 5};
    int *arr_ptr = arr;
    printf("Array via pointer: %d\n", arr_ptr[2]);
    
    return 0;
}