# Lecture 7: Multicore Part 2

## Agenda

+ Introducing semaphores and events
+ Five ways to increment a counter with multiple threads
+ Terminology; MESI, store-acquire atomics, compiler reordering, false sharing
+ Implement a thread-safe queue
+ Note about merging changes with Git
+ Lecture videos and Discord channel available
+ Homework 1 is due at 11:59pm. Have your changes *on Github* by that time.

## Three Problems When Using Threads

1. Read-modify-write operations are data races. E.g. `counter++`.
2. Reading on one thread while reading on another; reader can see stale values in cache.
3. Compiler is free to reorder operations if the end result is the same for a non-parallel program.

## Load-Acquire & Store-Release

Thread 1:
```c
assert(*write_here, 99);
assert(*write_last, 99);
assert(*write_there, 99);
*write_here = 5;
*write_there = 6;
atomic_store(*write_last, 7);
```

Thread 2:
```c
int value = atomic_load(*write_last);
if (value == 7)
{
	assert(*write_here == 5);
	assert(*write_there == 6);
}
```

## Cache Lines and Threads

Given:

```c
struct foo
{
	int x;
	int y;
};
```

* Thread 1: `foo->x = 12`
* Thread 2: `foo->y = 45`

What is the value of `foo->x` and `foo->y` after execution completes?

* `x == 12` and `y == 45`
* `x == ?` and `y == 45`
* `x == 12` and `y == ?`

## Links

+ https://fgiesen.wordpress.com/2014/07/07/cache-coherency/
+ https://fgiesen.wordpress.com/2014/08/18/atomics-and-contention/
