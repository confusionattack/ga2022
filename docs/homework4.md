# Homework 4: Frogger Game

Due: November 10th 2022 at 11:59pm

Make a game using our engine inspired by Frogger. The base feature set:

+ Traffic moves in lanes horizontally across the screen.
+ Player character moves vertically from bottom of the screen to the top.
+ There should be at least 3 lanes of traffic with space between each lane for a player character to wait.
+ If the player succesfully reaches the top of the screen, they are respawned at the bottom.
+ If the player touches traffic, they are respawned at the bottom of the screen.
+ The player character moves in response to arrow keys.
+ Traffic is composed of rectangles of variable length. The player is a square (or cube).
+ The player should be green. Traffic can be arbitrary colors.
+ The game should use an orthographic projection matrix for the camera (not a perspective projection).

Other implementation notes:

+ You are free to use multiple C files to implement the game, but one of them should be "frogger_game.c".
+ You should remove references to "simple_game" in main.c and replace with "frogger_game".
+ You are free to modify any other aspect of the engine source code to implement your game.
+ You are free to extend this game beyond the base feature set.

Our engine probably has bugs. Making a game is a good way to find them. If you think you found a bug, please fix it and let others know.

## Grading Criteria

Maximum total points is 15. Points are gained and lost based on the number of features implemented and the correctness of those features.
