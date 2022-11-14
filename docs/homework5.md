# Homework 5: Final Project RFC

Due: November 17th 2022 *in* *class*
If possible, email ahead of time.

Over the next several weeks we will be working on our final projects. A final
extends our game engine in a new way. We all get to choose our own final
project.

But before we start, we *need* an approved final project proposal. We call this
document an RFC, or Request For Comment. Homework 5 is to create an RFC for
your desired final project. The RFC should include the following information:

+ A title
+ Your name and email address
+ A summary of the current state in that area of the engine
+ A high-level description of proposed changes/additions
+ If you have considered multiple approaches, discuss pros/cons
+ A detailed breakdown of the tasks involved
+ A description of any challenges you expect to encounter

## Sample Final Project RFC

Title: Replicating Entities over the Network

Name/email: Chris McEvoy, mcevoc3

Summary: The engine supports sending and receiving UDP packets, but currently
has no way to replicate entity state over the network.

High-level Description: We will add support for replicating entity snapshots,
see https://www.jfedor.org/quake3/, using a method that ensures enventual
consistency and seeks to minimize bandwidth utilized by unchanging entity state.
With this approach the server is the authoritative source of all entity state.
Clients do *not* replicate entity state to the server. Using sequence numbers
servers will only send the entity data that has changed since it last received
an acknowlegement from a client. Finally compression will be used to further
reduce bandwidth.

Alternate Approaches: Our method requires that all modified entity state fit
into a single UDP packet snapshot. We considered a priority scheme where the
top N entities would be placed in the packet, but that leads to possibility
that the client only has a partial view into the authoritative state of the
world. We wanted to avoid that case with our system.

Tasks:

+ Give each entity a type. Associate a replicated component mask with each
entity type. Create functions to register a type and an entity with the net
system.

+ Have the server write an entity snapshot each frame. Put all replicated
component state for all replicated entities into a snapshot buffer no larger
than the MTU.

+ Allow the server to compare to past snapshots, and extract only the entity
data that has changed since the client last acknowledged a snapshot. Send
that changed data to the client.

+ Receive and process an entity snapshot on the client. This may involve
adding and removing entities, as well as updating component state.

Challenges:

+ Comparing two snapshots will be challenging code to write. Plan on adding
support for that last as it is not required to get the system functional.

+ Support for removing entities on the client will require that the server
send information to the client about entities that no longer exist.

## An Incomplete List of Final Project Ideas

The following is a list of some potential final projects. Feel free to propose
something not on this list!

+ Render/gpu improvements: load and render textured models, add lighting, etc.
+ Input improvements: support a range of different input devices.
+ Networking improvements: support for events, quantization, many entities, etc.
+ Audio support: adapt open source audio system to our engine.
+ Scripting support: support manipulating entity-components with Lua script.
+ UI support: integrate support Dear IMGUI into our engine.
+ Save game support: implement a way read/write entities from/to files.
+ Particle system: add support for particle systems in our engine.
+ Physics support: integrate an open source physics system into our engine.
+ Automated testing: implement a system for unit tests and unit test for the engine.
+ Documentation improvements: implement Doxygen (or similar), plus other docs for our engine.

## A Few Notes on Final Projects

Final projects should be completed by Thursday, December 8th. A final project
should be composed of at least two parts:

+ The work itself accessible in Github. Likely code, maybe data and/or documentation.
+ Inspired by Siggraph paper videos, a 2-minute video showing/describing your project.

A schedule of the remaining classes this semester:

| Date  | Description                        |
|-------|------------------------------------|
| 11/17 | Homework 5: Final project proposal |
| 11/21 | Open Studio                        |
| 11/28 | Activision CTO Talk                |
| 12/1  | Open Studio                        |
| 12/5  | Open Studio                        |
| 12/8  | Final Project Show                 |

Open Studio classes are simply another opportunity to attend office hours (in
our classroom or online). Please use this time if you have final project
questions.

Lastly, please plan on attending class on December 8th, ideally with a computer
so you can demo your final project to anyone interested.
