fn main() -> int
{
    int a = 5;
    int *ptr = &a;
    *ptr = 89;
    return a;
}