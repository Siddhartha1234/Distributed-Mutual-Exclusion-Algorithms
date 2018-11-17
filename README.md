# Distributed-Mutual-Exclusion-Algorithms

Implementation and Simulation of the following Mutual Exclusion Algorithms:

- A tree quorum based fault tolerant mutual exclusion algorithm.

ref:
Agrawal, Divyakant, and Amr El Abbadi. "An efficient and fault-tolerant solution for distributed mutual exclusion." ACM Transactions on Computer Systems (TOCS) 9.1 (1991): 1-20.
										

- An optimal path-reversal based dynamic mutual exclusion algorithm. 	

ref:
Naimi, Mohamed, Michel Trehel, and André Arnold. "A log (N) distributed mutual exclusion algorithm based on path reversal." Journal of parallel and distributed computing 34.1 (1996): 1-13.


Application Design:
1. A common simulation application to analyze the performance comparison of a static quorom vs a dynamic path reversal based algorithm will be designed.
2. It will compare based on average message complexity per request and synchronization delay at heavy load and light load which would be simulated by changing parameters of probabilistic distributions which randomly generate/simulate internal and application messages.
