# Container-Port-Problem
This is an example code utilizing pthread to simulate Container Port Problem, in which the routes of the containers(the ships) are represented as the threads to be run.

In the function "ship_thread", three stages will be processed in sequece. They are in turn loading, shipping, and unloading threads.

In the function "shipping_thread", a period of time sleep will be lauched to represent shipping period.

In the function "port_thread", a period of time sleep will be lauched to represent (un)loading period. Plus, there would have mutex and conditional variable for checking whether the port is totally occupied or not. If yes, the newly lauched port_thread would have to wait until the port resources are open again.

Thoughout the function "port_thread", conditional variable instead of semaphore is used for lock mechanism simulating the occupation of the loading/unloading port units. Since semaphore object can only be intitialized once, conditional variable design will be more fexible for simluating the problem under condition with respect to different numbers of port units.
