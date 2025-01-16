fn main() -> int {
    int b = 0;

    while (b < 5)
    {
        b = b + 1;

        if (b == 4)
        {
            break;
        }
    }

    return b;
}