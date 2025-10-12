struct Test {
    int a;
    int b;
};

fn main() -> int
{
    const struct Test example = {1, 2};
    return example.b;
}