# Lecture 10: Time

Time to spend time talking about time. Time time time.

## Agenda

+ Time of day
+ High resolution timers
+ Absolute time vs durations/spans
+ Fixed frame step vs variable frame step
+ Time transformed
+ Series of queues; latency
+ Class schedule update
+ Homework 3: Implementing a profiler

## Links

+ https://learn.microsoft.com/en-us/windows/win32/sysinfo/acquiring-high-resolution-time-stamps
+ https://gafferongames.com/post/fix_your_timestep/
+ https://docs.google.com/document/d/1CvAClvFfyA5R-PhYUmn5OOQtYMH4h6I0nSsKchNAySU/preview

## Fixed Time Step vs Variable Time Step

Fixed time waits each frame so that all frame times are >= a fixed time step.

```
dt = fixed_time_step;
wait(max(actual_frame_time, fixed_time_step));
```

Variable time scales the length of the frame based on the prior:

```
dt = now() - last_time;
last_time = now();
```
