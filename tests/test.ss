struct Test {
    int a;
    double b;
};

fn main() -> int
{
    struct Test example2 = {1, 2.0};   
    example2.a = 9;
    return 8;
}