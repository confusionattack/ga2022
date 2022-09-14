# Homework 1: Memory Leak Detection and Reporting

Due: September 19th 2022 at 11:59pm

Giving the following:

```
void heap_test()
{
   heap_t* heap = heap_create(4096);
   void* leak = heap_alloc(heap, 100, 8);
   void* not_leak = heap_alloc(heap, 200, 8);
   heap_free(heap, not_leak);
   heap_destroy(heap);
}
```

Calling `heap_destroy` should report a single memory leak:

```
Memory leak of size 100 bytes with callstack:
[0] heap_test
[1] main
```

The report should include the size of *each* leaked block, and the
symbolicated callstack used to allocate each block.

## Grading Criteria

Maximum total points is 15.

+ Coding standard compliance and clean code - 3
+ All leaked memory blocks, and only leaked blocks, are reported - 3
+ Size of each leaked block is reported correctly (within 8 bytes of true size) - 2
+ Callstack of the *allocation site* of each leaked block is correctly reported - 7

See the `homework1_test` function in `main.c` for the official test cases that
will be used to validate the correctness of your code.
