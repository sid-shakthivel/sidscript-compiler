struct Test {
    int a;
    int b;
};

fn main() -> int
{
    struct Test example;
    example = {1, 3};
    return example.b;
}

