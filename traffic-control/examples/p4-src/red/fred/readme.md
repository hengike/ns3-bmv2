# This folder contains an example implementation of the fred algorithm

## Details:
https://pdos.csail.mit.edu/~rtm/papers/fred.pdf

### The motivation behind fred(flaws of red):
* RED has a bias against low band width flows
* RED can exhibit unfair behavior among identical flows
* RED cannot reduce the transmission rate of UDP flows

The main difference between the two is that fred follows per flow metrics rather then judging only by the avarage data.

## Implementation

### Parameters(gen_commands.py):
* minthreshold (queue threshold to start random drop)
* maxthreshold (queue threshold to start deterministic drop)
* maxp (max. random drop probability)
* wq (expontial averaging factor)

### How it works:
* counts the number of active flows
* counts the number of packets in each flow (i.e., per flow accounting)
* compute the average number of packets sent per flow (= avgcq)
* tries to make sure that he number of packets in each flow in the router buffer between min and max q
* will penalize flows that has average flow queue size > avgcq 
* remembers persistently misbehaving flows 

### Variables:
qlen -> queue length per flow
packet_counter -> packet counter per flow
strike -> strike per flow
flow_hash ->Loaded from the standard_metadata
