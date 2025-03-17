struct Test {
    int a;
    double b;
    char c[3];
};

fn main() -> int
{
    struct *Test example;
    struct Test example2 = {1, 2.0, {'a', 'b', 'c'}};
    example2.a = 6;
    example->b = 2.0;
    return 5;
}