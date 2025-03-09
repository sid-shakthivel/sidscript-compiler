int main() {
    int i = 0;
    
    while (i < 5) {
        printf("While: %d\n", i);
        i = i + 1;
    }
    
    for (int j = 0; j < 3; j = j + 1) {
        if (j == 1) {
            continue;
        }
        printf("For: %d\n", j);
    }
    
    int x = 10;
    if (x > 5) {
        printf("x is greater than 5\n");
    } else {
        printf("x is less than or equal to 5\n");
    }
    
    return 0;
}