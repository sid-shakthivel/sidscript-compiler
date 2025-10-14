struct Test {
    int a;
    int b;
};

fn main() -> int
{
    struct Test example = {45, 67};
    struct Test *ptr = &example;
    ptr->b = 34;
    return example.b;
}