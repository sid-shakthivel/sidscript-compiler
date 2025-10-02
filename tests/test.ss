struct Test {
    int a;
    int c;
    int arr[3];
};

fn main() -> int
{
    struct Test example = {18, 14, {1, 2, 3}};
    return example.arr[5435];
}

