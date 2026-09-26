## Overview
The `Queue` class is a core component designed to optimize the computational efficiency of your agent-based simulations. In large-scale epidemiological models, where thousands or millions of agents may be able to interact, iterating over all agents at every simulation step can be computationally prohibitive ($O(n^2)$). The `Queue` class addresses this challenge by selectively activating agents for processing. Specifically, it ensures that only agents in relevant states (such as those infected or those with infected neighbors) are processed during each simulation step. This selective queuing mechanism reduces unnecessary computations, focusing resources on agents that are likely to change state or influence others. This allows for larger simulations, and contributes greatly to the overall speed of epiworld.

The `Queue` class is tightly integrated with the `Model` class, which provides the broader context for agent-based simulations. The queue interacts with the model to access agent states, network structures, and other simulation parameters. This integration allows the queue to dynamically adjust its contents based on the evolving state of the simulation. For example, when an agent transitions to an infected state, the queue can add the agent and its neighbors for processing. Similarly, when an agent recovers or is removed from the simulation, the queue can remove the agent and its neighbors.

The queue operates on the principle of dynamic activation. Agents are added to the queue when their state or the state of their neighbors changes in a way that makes them relevant to the simulation. For example, an agent transitioning to an infected state would be added to the queue, along with its neighbors, as they are now at risk of infection. Conversely, agents are removed from the queue when their state or the state of their neighbors no longer requires processing. This dynamic management of the queue ensures that the simulation remains efficient while maintaining accuracy.

However, it is worth noting that the performance gains depend on the sparsity of the network and the frequency of state transitions. In densely connected networks or scenarios with high state transition rates, the overhead of managing the queue may offset the benefits. In such cases, alternative queuing strategies or optimizations may be required.

## Implementation
The `Queue` class is implemented as a template and encapsulates several key data members and methods that govern its behavior.

### Members
- `std::vector<epiworld_fast_int> active`: tracks the activation status of agents and their neighbors. Each element corresponds to an agent, with the value indicating the number of times the agent has been activated.
- `std::vector<epiworld_fast_int> everyone`: the number of outstanding `Everyone` registrations of each agent. `active[i]` is `everyone[i]` plus the sum of `everyone[j]` over the agents `j` whose neighbor lists contain `i` (`operator+=` credits the agents in the registered agent's own list). In an undirected network those are exactly `i`'s neighbors; in a directed one they are the agents that list `i`, not the ones `i` lists. Either way, `everyone[j]` is what a tie from `j` is worth to the agent at the other end.
- `std::vector<uint64_t> bits`: one bit per agent, set when its `active` count is non-zero. It is kept in step with `active` and lets the model visit only the queued agents, in ascending id order, without scanning the whole population. Walking it costs $O(N/64 + \text{queued agents})$ instead of $O(N)$.
- `Model<TSeq> * model`: A pointer to the associated `Model` instance. This allows the queue to interact with the broader simulation framework, accessing agent states and network structures as needed.
- `int n_in_queue`: The number of agents currently in the queue. This counter provides a quick way to determine the queue's size without iterating over the `active` vector.

### Methods
- `void operator+=(Agent<TSeq> * p)`: Adds an agent and its neighbors to the queue. This method increments the activation counters for the agent and its neighbors, ensuring they are processed in subsequent simulation steps.
- `void operator-=(Agent<TSeq> * p)`: Removes an agent and its neighbors from the queue. This method decrements the activation counters, removing agents from the queue when their counters reach zero.
- `notify_edge_added(a, b)`, `notify_edge_removed(a, b)`, and `notify_edges_swapped(a, b, c, d, directed)`: keep the counts exact when the network changes during a run, in constant time per tie. `+=` and `-=` walk an agent's neighbors as they are at that moment, so a tie added, removed, or rewired in between would otherwise make the counts drift, and an agent whose count drifted to zero would be skipped. `Model::add_edge()`, `Model::rm_edge()`, and `Agent::swap_neighbors()` (used by `rewire_degseq()`) call them. They do nothing before a run has sized the queue.
- `epiworld_fast_int operator[](epiworld_fast_uint i) const`: Returns the activation count of a specific agent. It is read-only (since 0.16): counts change only through `+=`, `-=`, and the network-edit notifications, which keep the ordered set of queued agents in step.
- `void for_each_nonzero(F && f)`: Calls `f(i)` for every agent with a non-zero count, in ascending id order. `Model::update_state()` and `Model::mutate_virus()` use it to visit the queued agents in the same order a full scan would, so the random number stream is unchanged. A count is read when the walk reaches the agent, so changes made by `f` to agents further ahead are seen, as in a plain loop.
- `void reset()`: Resets the queue, clearing all activation statuses. The model calls it when a simulation starts.
- `bool operator==(const Queue<TSeq> & other) const`: Compares two queues for equality. This method checks whether the `active` vectors of the two queues are identical, providing a way to verify the consistency of the queue's state.
- `bool operator!=(const Queue<TSeq> & other) const`: Compares two queues for inequality. This method is implemented as the negation of the equality operator.

## Usage Example
The following example demonstrates how to interact with the `Queue` class in the context of a simulation. This includes adding agents to the queue, removing them, and resetting the queue at the start of a new simulation step.

```cpp
#include "epiworld/queue-bones.hpp"
#include "epiworld/agent-bones.hpp"
#include "epiworld/model-bones.hpp"

void simulate_step(Queue<int> &queue, Model<int> &model) {
    // Example: Adding an agent to the queue
    Agent<int> *agent = model.get_agent(0); // Retrieve the first agent
    queue += agent; // Add the agent and its neighbors to the queue

    // Example: Processing agents in the queue (ascending id order,
    // without scanning the whole population)
    queue.for_each_nonzero([&](size_t i) {
        // Check if the agent is active
        if (queue[i] > 0) {
            // Perform some operation on the active agent
            std::cout << "Processing agent " << i << std::endl;
        }
    });

    // Example: Removing an agent from the queue
    queue -= agent; // Remove the agent and its neighbors from the queue

    // Example: Resetting the queue at the start of a new step
    queue.reset();
}
```

In this example:
- The `+=` operator is used to add an agent and its neighbors to the queue. This ensures that the agent and its neighbors are processed in the current simulation step.
- The `for_each_nonzero` method visits the queued agents, and `queue[i]` reads the activation status of an agent. Only active agents are processed.
- The `-=` operator is used to remove an agent and its neighbors from the queue. This is useful for deactivating agents that no longer need to be processed.
- The `reset` method clears the queue; the model calls it when a simulation starts.

The queue only decides *which* agents are updated. How susceptible agents acquire a virus -- by pulling from their neighbors or by having their infectious neighbors push to them -- is covered in [Push and Pull Transmission](transmission-sampling.md). That choice never depends on the queue, so a run gives the same results with queuing on or off.

### Directed networks

A directed tie `i -> j` is kept by its source only: `j` is one of `i`'s neighbors, so `i` is exposed to `j`. The queue marks the neighbors of an agent that becomes infectious, and in a directed network those are the agents it is exposed to. The agents it can infect are the ones that list *it*, which the queue never sees. So in a directed network, `Model::update_state()` and `Model::mutate_virus()` visit every agent at each step, as if queuing were off. The results are the same either way.

## See Also

- [Library Architecture](library-architecture.md) — overview of the `Model` class that the `Queue` integrates with.
- [Performance Optimization](performance-optimization.md) — broader strategies for improving simulation throughput.
- [Extending the Library](extending-the-library.md) — custom agent state update functions that interact with the queue.
- [Push and Pull Transmission](transmission-sampling.md) — how transmission is sampled in network models.
