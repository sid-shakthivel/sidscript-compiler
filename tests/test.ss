struct Test {
    int a;
    int c;
    int arr[3];
};

fn main() -> int
{
    struct Test example = {18, 14, {3, 6, 10}};
    example.arr[0] = 42;
    return example.arr[0];
}