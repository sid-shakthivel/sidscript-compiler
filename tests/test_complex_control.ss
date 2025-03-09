int main() {
    for (int i = 0; i < 5; i = i + 1) {
        if (i == 2) {
            continue;
        }
        
        for (int j = 0; j < 3; j = j + 1) {
            if (i * j > 5) {
                break;
            }
            printf("i=%d, j=%d\n", i, j);
        }
    }
    
    int x = 0;
    while (x < 5) {
        if (x == 3) {
            break;
        }
        printf("while: %d\n", x);
        x = x + 1;
    }
    
    return 0;
}