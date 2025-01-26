fn main() -> int {
    int a = 5;
    int b = 3;
    if (b == 8)
    {
        int a = 3;
        b = a + b;
    }
    b = a + b;
    return b;
}