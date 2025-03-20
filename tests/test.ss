struct Test {
    int a;
    double b;
    char c[3];
};

fn main() -> int
{
    struct Test example2 = {1, 2.0, {'a', 'b', 'c'}};
    struct *Test example;
    example2.a = 9;
    return 5;
}