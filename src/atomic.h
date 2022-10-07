#pragma once

// Atomic operations on 32-bit integers.

// Increment a number atomically.
// Returns the old value of the number.
// Performs the following operation atomically:
//   int old_value = *address; (*address)++; return old_value;
int atomic_increment(int* address);

// Decrement a number atomically.
// Returns the old value of the number.
// Performs the following operation atomically:
//   int old_value = *address; (*address)--; return old_value;
int atomic_decrement(int* address);

// Compare two numbers atomically and assign if equal.
// Returns the old value of the number.
// Performs the following operation atomically:
//   int old_value = *address; if (*address == compare) *address = exchange; return old_value;
int atomic_compare_and_exchange(int* dest, int compare, int exchange);

// Reads an integer from an address.
// All writes that occurred before the last atomic_store to this address are flushed.
int atomic_load(int* address);

// Writes an integer.
// Paired with an atomic_load, can guarantee ordering and visibility.
void atomic_store(int* address, int value);
