# Homework 2: Asynchronous file decompression

Due: September 29th 2022 at 11:59pm

Extend our file system (`fs.c`) to support reading and writing compressed files.
Use LZ4 (already in the tree) to compress/decompress the data. Compression and
decompression operations should be asynchronous -- use a *separate* queue.

The user will pass a flag to `fs_read` and `fs_write` indicating whether or not
they want compression. For example, writing a compressed file:

```
const char* data = "Hello world!";
fs_work_t* work = fs_write(fs, "foo.txt", data, strlen(data), true); // True means write compressed
fs_work_destroy(work);
```

And reading a compressed file:

```
fs_work_t* work = fs_read(fs, "foo.txt", heap, true, true); // Second true means read compressed
const char* data = fs_work_get_buffer(work);
assert(data && strcmp(data, "Hello world!") == 0);
fs_work_destroy(work);
```

Look for `// HOMEWORK 2` comments in `fs.c` for places where code is needed.

## Grading Criteria

Maximum total points is 15.

+ Coding standard compliance and clean code - 5
+ Test cases pass (e.g. compressed read/write works correctly) - 8
+ No discernable thread safety issues (data races, etc.) - 2

See the `homework2_test` function in `main.c` for the official test cases that
will be used to validate the correctness of your code.
