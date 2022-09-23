#include <stdbool.h>

// Event thread synchronization

// Handle to an event.
typedef struct event_t event_t;

// Creates a new event.
event_t* event_create();

// Destroys a previously created event.
void event_destroy(event_t* event);

// Signals an event.
// All threads waiting on this event will resume.
void event_signal(event_t* event);

// Waits for an event to be signaled.
void event_wait(event_t* event);

// Determines if an event is signaled.
bool event_is_raised(event_t* event);
