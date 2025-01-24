fn main() -> int {
    int a = 4;
    int b = 3;
    if (b == 3)
    {
        int a = 5;
        b = b + a;
    }
    b = b + a;
    return b;
}