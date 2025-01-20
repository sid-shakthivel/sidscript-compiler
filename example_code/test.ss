fn foo(int a) -> int {
    return a + 1;
}

fn bar(int b) -> int {
    return foo(b) + b;
}

fn main() -> int {
    int a = bar(5);
    return a;
}
