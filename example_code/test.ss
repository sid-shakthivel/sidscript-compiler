fn main() -> int {
    int a = 5;
    
    for (int b = 0; b < 5; ++b)
    {
        a = a + 1;
        if (a == 5)
        {
            continue;
        }
    }

    return a;
}