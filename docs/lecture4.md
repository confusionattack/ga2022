# Lecture 4: Memory Continued

## Agenda

+ Review: addresses, sizes, padding, alignment, virtual memory, caches
+ TLSF
+ Heap implementation
+ Homework: Leak detection and reporting
+ Next time: Debugging

## Size of All the Things

+ bool - 8 bits, 1 byte
+ char - 8 bits, 1 byte
+ short - 16 bits, 2 bytes
+ int - 32 bits, 4 bytes
+ long long - 64 bits, 8 bytes
+ float - 32 bits, 4 bytes
+ double - 64 bits, 8 bytes
+ void* - 64 bits, 8 bytes
+ wm_window_t* - 64 bits, 8 bytes
+ LONG_PTR (long*) - 64 bits, 8 bytes

## Homework 1: Leak Detection and Reporting

Assuming the following test case:

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

## Links

+ https://github.com/mattconte/tlsf
