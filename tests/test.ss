struct Test {
    int a;
    int c;
    int arr[3];
};

fn test(struct Test *ptr, int hey[]) -> int
{
    return 5;
}

fn main() -> int
{
    struct Test example;
    int arr[3];
    test(&example, arr);
    return 9;
}