fn main() -> int {    
    int arr[5] = {1, 2, 3, 4, 5};
    int *arrPtr = arr;
    int test = arrPtr[2];
    printf("Array via pointer: %d\n", test);
    
    return 0;
}