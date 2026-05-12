# Dictionary

## Library Usage

```C
int app_main()
{
    dictionary_t *dictionary = create_dictionary();
    uint32_t *value = NULL;

    set_value(dictionary, "foo", 42);
    set_value(dictionary, "bar", 13);

    get_value(dictionary, "foo", value);
    printf("%ld\n", *value);
    get_value(dictionary, "bar", value);
    printf("%ld\n", *value);
    get_value(dictionary, "baz", value);
    printf("%ld\n", *value);
    free_dictionary(dictionary);
    return 0;
}
```