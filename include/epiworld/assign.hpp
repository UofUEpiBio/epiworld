#ifndef EPIWORLD_ASSIGN_HPP
#define EPIWORLD_ASSIGN_HPP

/**
 * @file assign.hpp
 * @brief How agents get something at the start of each run: a virus (and the
 * stage it puts them in), a tool, an initial stage, or a group of a mixing
 * layer. One vocabulary for all of them.
 *
 * @details An `Assign` gives each eligible agent a **label**, or none (-1),
 * drawn with the model's random numbers when a run starts:
 *
 * | Form | Labels |
 * |---|---|
 * | `assign::share(p)`, `assign::count(n)` | A random share (or number) of the eligible agents |
 * | `assign::shares({{l, p}, ...})`, `assign::counts({{l, n}, ...})` | Several labels: each takes its share (or number) of the eligible agents, in order, from those left |
 * | `assign::group_shares(layer, {p_g})`, `assign::group_counts(layer, {n_g})` | A share (or number) of each group of a mixing layer |
 * | `assign::agents(ids)` | The given agents |
 * | `assign::labels(v)` | `v[i]` for agent `i` (-1: none); the same every run |
 * | `assign::by(fun)` | `fun(agent, model)` for each eligible agent |
 *
 * What a label means, and who is eligible, depends on the use:
 *
 * | Use | Label | Eligible |
 * |---|---|---|
 * | `Model::add_virus(v, a)` | The stage the agent starts in (a carrier stage); a form without labels uses the infection's target | Agents without a virus whose stage can be infected |
 * | `Model::add_tool(t, a)` | -- (has the tool) | Everyone |
 * | `Model::set_initial(a)` | A stage without the virus | Agents left without a virus, in the first stage |
 * | `layer::Mixing::set_groups(a, n)` | A group, in `[0, n)` | Everyone |
 *
 * The placements are made in this order at every reset: the layers (so a
 * placement can use their groups), the viruses, the tools, the initial
 * stages, then the global events' set-up. The random forms are drawn again in
 * each run; `agents()` and `labels()` are fixed.
 */

class Assign;

namespace assign {
    /// The label of the forms that give none: the use's default (see above)
    constexpr int ANY = -2;
}

/// A function that gives an agent's label (-1: none).
using AssignFun = std::function< int(Agent &, Model &) >;

/**
 * @brief Gives each eligible agent a label, or none (see `assign.hpp`).
 * @details Default-constructed, it gives no one anything.
 */
class Assign {
public:
    /// The labels of the agents (-1: none), given who is eligible.
    using DrawFun = std::function<
        std::vector< int >(Model &, const std::vector< char > &)
    >;

private:
    DrawFun fun = nullptr;
    std::shared_ptr< const std::vector< int > > fixed_labels = nullptr;

public:
    Assign() = default;
    explicit Assign(
        DrawFun f,
        std::shared_ptr< const std::vector< int > > fixed_ = nullptr
    ) : fun(std::move(f)), fixed_labels(std::move(fixed_)) {}

    /// The labels, if they are the same every run (`assign::labels()`).
    const std::vector< int > * get_fixed() const { return fixed_labels.get(); }

    /// Whether it gives anyone anything.
    bool is_set() const { return static_cast< bool >(fun); }

    /**
     * @brief The labels of the agents of `model` (-1: none).
     * @param eligible One entry per agent: whether it can be given a label.
     * @throws std::logic_error if the form names an agent that is not
     * eligible.
     */
    std::vector< int > draw(Model & model, const std::vector< char > & eligible) const;
};

#endif
