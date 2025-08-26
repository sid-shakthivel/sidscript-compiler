struct Test {
    int a;
    int b;
};

fn main() -> int
{
    // Make a new struct 
    struct Test example;
    example = {1, 3};
    // Return it now

    /*
        This is a multiline comment
        And it's written in the style of how i write comments
    */
    return example.b;
}

