# Simulate JIT C

## How to build?

```
make
```

## How to use?

Execute the program.

```
./jit-c
```

Write an code block with end of "//0".

```c
> int ret_int(void)
> {
>     return 0;
> }
> //0
```

When you write the main function, it will auto execute the program.

```c
> #include <stdio.h>
>
> int main(void) {
>     printf("get: %d\n", ret_int());
>     return 0;
> }
> //0
----------
get: 0
----------
```

After finishing your program, enter "quit" to exit.
