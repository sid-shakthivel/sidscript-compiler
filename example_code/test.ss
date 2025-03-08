fn main() -> int {
    int arr[3] = {5, 10, 15};
    int* ptr = arr;      
    
    ptr = ptr + 2;      
    int value = *ptr;    
    
    return value;        
}