#ifndef EPIWORLD_ASSIGN_MEAT_HPP
#define EPIWORLD_ASSIGN_MEAT_HPP

// The forms of `Assign` (see assign.hpp)

inline std::vector< int > Assign::draw(
    Model & model,
    const std::vector< char > & eligible
) const
{
    if (!fun)
        return std::vector< int >(model.size(), -1);
    return fun(model, eligible);
}

namespace assign {

namespace detail {

    inline void check_share(epiworld_double p, const char * form)
    {
        if (!std::isfinite(p) || (p < 0.0) || (p > 1.0))
            throw std::range_error(
                std::string("assign::") + form + "(): a share must be within "
                "[0, 1]; got " + std::to_string(p) + "."
            );
    }

    inline void check_count(epiworld_double n, const char * form)
    {
        if (!std::isfinite(n) || (n < 0.0) || (n != std::floor(n)))
            throw std::range_error(
                std::string("assign::") + form + "(): a count must be a "
                "non-negative whole number; got " + std::to_string(n) + "."
            );
    }

    /**
     * Label by label, in order, draws `count` agents at random from the
     * eligible ones not yet labelled (taken in ascending id order). A count
     * above what is left is an error.
     */
    inline std::vector< int > draw_counts(
        Model & model,
        const std::vector< char > & eligible,
        const std::vector< std::pair< int, size_t > > & counts
    )
    {

        std::vector< int > labels(model.size(), -1);
        std::vector< size_t > idx;

        for (const auto & c : counts)
        {

            idx.clear();
            for (size_t i = 0u; i < labels.size(); ++i)
                if (eligible[i] && (labels[i] == -1))
                    idx.push_back(i);

            size_t n_left = idx.size();
            if (c.second > n_left)
                throw std::range_error(
                    "Cannot assign " + std::to_string(c.second) + " agents: "
                    "only " + std::to_string(n_left) + " are eligible."
                );

            for (size_t k = 0u; k < c.second; ++k)
            {
                const size_t loc = model.runif_index(
                    static_cast< uint32_t >(n_left--)
                );
                labels[idx[loc]] = c.first;
                std::swap(idx[loc], idx[n_left]);
            }

        }

        return labels;

    }

    inline size_t n_eligible(const std::vector< char > & eligible)
    {
        return static_cast< size_t >(
            std::count(eligible.begin(), eligible.end(), char(1))
        );
    }

    /// The members of each group of the mixing layer `layer`, in id order.
    inline std::vector< std::vector< size_t > > groups_of(
        Model & model,
        const std::string & layer,
        size_t n_values
    )
    {

        auto * mix = dynamic_cast< layer::Mixing * >(&model.get_layer(layer));
        if (mix == nullptr)
            throw std::logic_error(
                "The layer '" + layer + "' is not a mixing layer: it has no "
                "groups."
            );

        if (n_values != mix->get_n_groups())
            throw std::range_error(
                "There are " + std::to_string(n_values) + " values, but the "
                "layer '" + layer + "' has " +
                std::to_string(mix->get_n_groups()) + " groups."
            );

        std::vector< std::vector< size_t > > out(n_values);
        for (size_t g = 0u; g < n_values; ++g)
        {
            out[g] = mix->members_of(g);
            std::sort(out[g].begin(), out[g].end());
        }

        return out;

    }

    /// In each group, draws `want[g]` of its eligible members (capped).
    inline std::vector< int > draw_groups(
        Model & model,
        const std::vector< char > & eligible,
        const std::string & layer,
        const std::vector< epiworld_double > & values,
        bool as_share
    )
    {

        std::vector< int > labels(model.size(), -1);
        const auto members = groups_of(model, layer, values.size());

        std::vector< size_t > idx;
        for (size_t g = 0u; g < values.size(); ++g)
        {

            idx.clear();
            for (size_t i : members[g])
                if (eligible[i])
                    idx.push_back(i);

            size_t n = idx.size();
            const epiworld_double want = as_share ?
                std::floor(values[g] * static_cast< epiworld_double >(n)) :
                values[g];
            const size_t n_draw = (want >= static_cast< epiworld_double >(n)) ?
                n : static_cast< size_t >(want);

            for (size_t k = 0u; k < n_draw; ++k)
            {
                const size_t loc = model.runif_index(static_cast< uint32_t >(n--));
                labels[idx[loc]] = ANY;
                std::swap(idx[loc], idx[n]);
            }

        }

        return labels;

    }

} // namespace detail

/// A random share of the eligible agents (`floor(p x eligible)` of them).
inline Assign share(epiworld_double p)
{
    detail::check_share(p, "share");
    return Assign([p](Model & m, const std::vector< char > & e) {
        const size_t n = static_cast< size_t >(std::floor(
            p * static_cast< epiworld_double >(detail::n_eligible(e))
        ));
        return detail::draw_counts(m, e, {{ANY, n}});
    });
}

/// `n` eligible agents, at random.
inline Assign count(size_t n)
{
    return Assign([n](Model & m, const std::vector< char > & e) {
        return detail::draw_counts(m, e, {{ANY, n}});
    });
}

/**
 * @brief Several labels: label `l` goes to `floor(p x eligible)` agents,
 * drawn in order from those not yet labelled.
 */
inline Assign shares(std::vector< std::pair< int, epiworld_double > > s)
{
    epiworld_double total = 0.0;
    for (auto & x : s)
    {
        detail::check_share(x.second, "shares");
        total += x.second;
    }
    if (total > 1.0 + 1e-9)
        throw std::range_error("assign::shares(): the shares add up to more than 1.");

    return Assign([s](Model & m, const std::vector< char > & e) {
        const epiworld_double n = static_cast< epiworld_double >(
            detail::n_eligible(e)
        );
        std::vector< std::pair< int, size_t > > c;
        for (auto & x : s)
            c.emplace_back(x.first, static_cast< size_t >(std::floor(x.second * n)));
        return detail::draw_counts(m, e, c);
    });
}

/// Several labels: label `l` goes to `n` agents, drawn in order.
inline Assign counts(std::vector< std::pair< int, size_t > > c)
{
    return Assign([c](Model & m, const std::vector< char > & e) {
        return detail::draw_counts(m, e, c);
    });
}

/**
 * @brief In each group `g` of the mixing layer `layer`, a random share `p[g]`
 * of its eligible members (`floor(p[g] x members)`).
 */
inline Assign group_shares(std::string layer, std::vector< epiworld_double > p)
{
    if (p.empty())
        throw std::range_error("assign::group_shares(): no shares.");
    for (auto x : p)
        detail::check_share(x, "group_shares");

    return Assign([layer, p](Model & m, const std::vector< char > & e) {
        return detail::draw_groups(m, e, layer, p, true);
    });
}

/// In each group `g` of the mixing layer `layer`, `n[g]` of its eligible
/// members at random (all of them, if fewer).
inline Assign group_counts(std::string layer, std::vector< epiworld_double > n)
{
    if (n.empty())
        throw std::range_error("assign::group_counts(): no counts.");
    for (auto x : n)
        detail::check_count(x, "group_counts");

    return Assign([layer, n](Model & m, const std::vector< char > & e) {
        return detail::draw_groups(m, e, layer, n, false);
    });
}

/// The given agents (each must be eligible).
inline Assign agents(std::vector< size_t > ids)
{
    auto ids_ptr = std::make_shared< const std::vector< size_t > >(std::move(ids));
    return Assign([ids_ptr](Model & m, const std::vector< char > & e) {
        std::vector< int > labels(m.size(), -1);
        for (size_t i : *ids_ptr)
        {
            if (i >= m.size())
                throw std::range_error(
                    "assign::agents(): agent " + std::to_string(i) +
                    " is out of range."
                );
            if (!e[i])
                throw std::logic_error(
                    "assign::agents(): agent " + std::to_string(i) +
                    " cannot be given this (it is not eligible)."
                );
            labels[i] = ANY;
        }
        return labels;
    });
}

/// `v[i]` for agent `i` (-1: none); the same every run.
inline Assign labels(std::vector< int > v)
{
    auto v_ptr = std::make_shared< const std::vector< int > >(std::move(v));
    return Assign([v_ptr](Model & m, const std::vector< char > & e) {
        if (v_ptr->size() != m.size())
            throw std::range_error(
                "assign::labels(): " + std::to_string(v_ptr->size()) +
                " labels for " + std::to_string(m.size()) + " agents."
            );
        for (size_t i = 0u; i < v_ptr->size(); ++i)
            if (((*v_ptr)[i] != -1) && !e[i])
                throw std::logic_error(
                    "assign::labels(): agent " + std::to_string(i) +
                    " cannot be given this (it is not eligible)."
                );
        return *v_ptr;
    }, v_ptr);
}

/**
 * @brief The same, from whole numbers of another type (e.g., the groups of a
 * mixing layer as `size_t`); values that do not fit in an `int` (such as
 * `layer::Mixing::NO_GROUP`) are none.
 */
template<
    typename T,
    typename = std::enable_if_t< std::is_integral_v< T > && !std::is_same_v< T, int > >
>
inline Assign labels(const std::vector< T > & v)
{
    std::vector< int > l(v.size(), -1);
    for (size_t i = 0u; i < v.size(); ++i)
        if (
            (v[i] >= 0) &&
            (static_cast< unsigned long long >(v[i]) <=
                static_cast< unsigned long long >(std::numeric_limits< int >::max()))
        )
            l[i] = static_cast< int >(v[i]);
    return labels(std::move(l));
}

/// `fun(agent, model)` for each eligible agent (-1: none).
inline Assign by(AssignFun fun)
{
    return Assign([fun](Model & m, const std::vector< char > & e) {
        std::vector< int > labels(m.size(), -1);
        auto & pop = m.get_agents();
        for (size_t i = 0u; i < pop.size(); ++i)
            if (e[i])
                labels[i] = fun(pop[i], m);
        return labels;
    });
}

} // namespace assign

#endif
