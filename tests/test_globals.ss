int globalVar = 42;
static int staticVar = 10;

fn modifyGlobals() -> void {
    globalVar = globalVar + 1;
    staticVar = staticVar + 1;
}

fn main() -> int {
    printf("Initial global: %d, static: %d\n", globalVar, staticVar);
    modifyGlobals();
    printf("After modification global: %d, static: %d\n", globalVar, staticVar);
    
    return 0;
}