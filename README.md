# workload_gen

This program is a proof of concept for a fast state transfer algorithm in a byzantine setting. 

## How to compile

You will need the libssl-dev package to use the hash functions.

To compile with low memory usage:
```
make local
```

To compile with a memory usage of 60 Go:
```
make
```

## Execution

The program works in the following way:

First you need to setup nodes.txt such that every nodes is correctly listed. Line 1 contains the info of the consensus node.

Then you need to open N terminals and write this command 

```
./workload n
```

Where n is the ID of the node (ID 0 is the consensus node). It needs to be coherent with what you wrote on the nodes.txt file.

If everything works correctly the nodes will connect with the consensus node and they should start executing batches.

It is then possible to force a node to recover by writing 1 then pressing "enter" in any terminal except for the consensus node. The recovery process will ask all the other nodes to send hashes and state chunks to the recovering node. Beware that it doesn't support multiple recoveries at the same time and the behaviour in this case is unexpected.

On a side note: As the program is expected to work in a specific environment and has only been tested this way, it is only guaranteed to function properly if the total number of node is equals to 5 + 3k (with k an arbitrary integer) including the consensus node.

## Additional files

local_consensus.sh can be used with an argument to launch N terminals with the program. 

requirements.sh contains useful packages for a fresh AWS node but it doesn't need to be installed locally.
