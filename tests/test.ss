struct Test {
    int a;
    double b;
};

fn main() -> int
{
    struct Test example = {1, 2.0};   
    example.a = 9;
    return example.a;
}