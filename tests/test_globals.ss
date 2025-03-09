int global_var = 42;
static int static_var = 10;

void modify_globals() {
    global_var = global_var + 1;
    static_var = static_var + 1;
}

int main() {
    printf("Initial global: %d, static: %d\n", global_var, static_var);
    modify_globals();
    printf("After modification global: %d, static: %d\n", global_var, static_var);
    
    return 0;
}