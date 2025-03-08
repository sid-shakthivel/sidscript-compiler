fn main() -> int {
    int arr[3] = {5, 10, 15};
    int* ptr = arr;      
    
    ptr = ptr + 1;      
    int value = *ptr;    
    
    return value;        
}