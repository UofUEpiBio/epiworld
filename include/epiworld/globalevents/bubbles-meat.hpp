#ifndef EPIWORLD_GLOBALEVENTS_BUBBLES_MEAT_HPP
#define EPIWORLD_GLOBALEVENTS_BUBBLES_MEAT_HPP

// Standard library headers are included at global scope by epiworld.hpp (this
// file is included from within `namespace epiworld`, so system headers must not
// be re-included here).
#include "bubbles-bones.hpp"

template<typename TSeq>
inline BubbleTool<TSeq>::BubbleTool(
    std::string name,
    std::string event_name
) : Tool<TSeq>(name), _event_name(std::move(event_name))
{
}

template<typename TSeq>
inline epiworld_double BubbleTool<TSeq>::get_susceptibility_reduction(
    VirusPtr<TSeq> & v,
    Model<TSeq> * model
)
{

    // Binding to the policy of *this* model, once. clone_ptr() clears the
    // pointer, so a copy of this tool -- in another agent, or in a copy of the
    // model -- resolves against its own model rather than inheriting ours.
    if (_policy == nullptr)
    {

        _policy = Bubbles<TSeq>::get_from(*model, _event_name);

        if (_policy == nullptr)
            throw std::logic_error(
                "BubbleTool: the intervention '" + _event_name +
                "' is not installed on this model."
            );

    }

    return _policy->susceptibility_reduction(
        this->get_agent(), v->get_agent(), model
    );

}

template<typename TSeq>
inline std::unique_ptr<Tool<TSeq>> BubbleTool<TSeq>::clone_ptr() const
{
    auto ans = std::make_unique<BubbleTool<TSeq>>(*this);
    ans->_policy = nullptr; // the copy resolves against its own model
    return ans;
}

template<typename TSeq>
inline Bubbles<TSeq>::Bubbles(
    std::vector< size_t > household_id,
    BubbleFlavor flavor,
    size_t group_size,
    epiworld_double transmission_factor,
    int start_day,
    int end_day,
    int rewire_every,
    std::string name,
    size_t max_households,
    std::string param_name,
    BubbleTies ties
) :
    household_id(std::move(household_id)),
    flavor(flavor),
    group_size(group_size),
    max_households(max_households),
    transmission_factor(transmission_factor),
    start_day(start_day),
    end_day(end_day),
    rewire_every(rewire_every),
    param_name(std::move(param_name)),
    ties(ties)
{

    this->set_name(name);
    this->set_day(-99); // runs at the end of every day

    if ((this->transmission_factor < 0.0) || (this->transmission_factor > 1.0))
        throw std::range_error(
            "Bubbles: transmission_factor must be in [0, 1]."
        );

    if ((flavor == BubbleFlavor::Household) && (group_size < 1u))
        throw std::range_error(
            "Bubbles: group_size (households per bubble) must be >= 1."
        );

    if ((flavor == BubbleFlavor::Peer) && (max_households < 2u))
        throw std::range_error(
            "Bubbles: max_households must be >= 2 for the Peer flavor."
        );

    if ((this->end_day >= 0) && (this->end_day <= this->start_day))
        throw std::range_error(
            "Bubbles: end_day must be greater than start_day (or negative)."
        );

}

template<typename TSeq>
inline void Bubbles<TSeq>::partition_household(Model<TSeq> * model)
{

    // Map household label -> compact index, and list unique households.
    std::unordered_map< size_t, size_t > hh_index;
    std::vector< size_t > hh_labels;
    for (size_t a = 0u; a < household_id.size(); ++a)
    {
        size_t h = household_id[a];
        if (hh_index.find(h) == hh_index.end())
        {
            hh_index[h] = hh_labels.size();
            hh_labels.push_back(h);
        }
    }

    size_t nh = hh_labels.size();

    // Build the household contact graph: households h1 and h2 are adjacent when
    // at least one member of h1 is connected to a member of h2 in the contact
    // network. Bubbles are grown along these ties.
    //
    // Grouping households that share NO tie would be a no-op: the intervention
    // can only suppress transmission along existing edges, never create new
    // ones, so bubbling two unconnected households changes nothing. Pairing at
    // random therefore degenerates to the household-only lockdown. It also
    // matches the policy being modelled: a household picks a bubble partner it
    // actually socialises with.
    std::vector< std::vector< size_t > > hh_adj(nh);
    auto & pop_all = model->get_agents();
    for (size_t a = 0u; a < household_id.size(); ++a)
    {
        size_t ha = hh_index[household_id[a]];
        for (auto * nb : pop_all[a].get_neighbors(*model))
        {
            size_t b = static_cast< size_t >(nb->get_id());
            if (household_id[a] == household_id[b])
                continue;
            hh_adj[ha].push_back(hh_index[household_id[b]]);
        }
    }

    // De-duplicate each adjacency list.
    for (auto & adj : hh_adj)
    {
        std::sort(adj.begin(), adj.end());
        adj.erase(std::unique(adj.begin(), adj.end()), adj.end());
    }

    // Shuffle household order (Fisher-Yates with the model RNG).
    std::vector< size_t > order(nh);
    for (size_t i = 0u; i < nh; ++i)
        order[i] = i;

    for (size_t i = nh; i > 1u; --i)
    {
        size_t j = static_cast< size_t >(model->runif_index(static_cast<uint32_t>(i)));
        std::swap(order[i - 1u], order[j]);
    }

    // Grow each bubble from a seed household by repeatedly absorbing a random
    // household that is *connected* to the bubble, up to `group_size`
    // households. A household with no unassigned connected candidates simply
    // ends up in a smaller bubble (possibly alone) -- you can only bubble with
    // someone you already have contact with.
    std::vector< int > hh_bubble(nh, -1);
    std::vector< size_t > candidates;
    int next_bubble = 0;

    for (size_t pos = 0u; pos < nh; ++pos)
    {

        size_t seed = order[pos];
        if (hh_bubble[seed] != -1)
            continue;

        int b = next_bubble++;
        hh_bubble[seed] = b;
        size_t members = 1u;

        // Frontier of households connected to the bubble. May contain stale
        // (already assigned) or repeated entries; repeats make a household that
        // is tied to several members proportionally more likely to be picked.
        candidates.clear();
        for (size_t x : hh_adj[seed])
            if (hh_bubble[x] == -1)
                candidates.push_back(x);

        while ((members < group_size) && !candidates.empty())
        {

            size_t idx = static_cast< size_t >(
                model->runif_index(static_cast<uint32_t>(candidates.size()))
            );
            size_t pick = candidates[idx];
            candidates[idx] = candidates.back();
            candidates.pop_back();

            if (hh_bubble[pick] != -1) // stale entry
                continue;

            hh_bubble[pick] = b;
            ++members;

            for (size_t x : hh_adj[pick])
                if (hh_bubble[x] == -1)
                    candidates.push_back(x);

        }

    }

    // Assign each agent the bubble of its household.
    for (size_t a = 0u; a < household_id.size(); ++a)
        bubble_id[a] = hh_bubble[hh_index[household_id[a]]];

}

template<typename TSeq>
inline void Bubbles<TSeq>::partition_peer(Model<TSeq> * model)
{

    size_t n = household_id.size();

    // Map household label -> compact index.
    std::unordered_map< size_t, size_t > hh_index;
    std::vector< size_t > hh_labels;
    for (size_t a = 0u; a < n; ++a)
    {
        size_t h = household_id[a];
        if (hh_index.find(h) == hh_index.end())
        {
            hh_index[h] = hh_labels.size();
            hh_labels.push_back(h);
        }
    }

    size_t nh = hh_labels.size();

    // Disjoint-set (union-find) over households, tracking the number of
    // households in each set so bubbles can be capped.
    std::vector< size_t > parent(nh), set_size(nh, 1u);
    for (size_t i = 0u; i < nh; ++i)
        parent[i] = i;

    auto find = [&parent](size_t x) -> size_t {
        while (parent[x] != x)
        {
            parent[x] = parent[parent[x]]; // path halving
            x = parent[x];
        }
        return x;
    };

    // Agents choose in random order, each drawing peers from the contacts that
    // are still available. A household whose bubble is full drops out of the
    // pool: any choice involving it is declined, and its own members stop
    // choosing. This cap is what makes the policy's exclusivity bite -- without
    // it the merges percolate, the household graph becomes connected, and every
    // household ends up in one giant bubble, imposing no restriction at all.
    std::vector< size_t > agent_order(n);
    for (size_t i = 0u; i < n; ++i)
        agent_order[i] = i;

    for (size_t i = n; i > 1u; --i)
    {
        size_t j = static_cast< size_t >(
            model->runif_index(static_cast<uint32_t>(i))
        );
        std::swap(agent_order[i - 1u], agent_order[j]);
    }

    auto & pop = model->get_agents();
    std::vector< size_t > ext;

    for (size_t oi = 0u; oi < n; ++oi)
    {

        size_t a  = agent_order[oi];
        size_t ha = hh_index[household_id[a]];

        // This agent's household is already in a full bubble: it is out of the
        // pool and cannot take anyone else in.
        if (set_size[find(ha)] >= max_households)
            continue;

        // Households of this agent's contacts outside its own household.
        ext.clear();
        for (auto * nb : pop[a].get_neighbors(*model))
        {
            size_t nid = static_cast< size_t >(nb->get_id());
            if (household_id[nid] != household_id[a])
                ext.push_back(hh_index[household_id[nid]]);
        }

        // Keep drawing until the agent has made `group_size` choices or no
        // contact is left that its bubble can still take in.
        size_t chosen = 0u;
        while ((chosen < group_size) && !ext.empty())
        {

            size_t idx = static_cast< size_t >(
                model->runif_index(static_cast<uint32_t>(ext.size()))
            );
            size_t hb = ext[idx];
            ext[idx] = ext.back();
            ext.pop_back();

            size_t ra = find(ha);
            size_t rb = find(hb);

            if (ra == rb) // already sharing a bubble
                continue;

            if ((set_size[ra] + set_size[rb]) > max_households)
                continue; // that bubble is full: not available

            parent[ra] = rb;
            set_size[rb] += set_size[ra];
            ++chosen;

            if (set_size[rb] >= max_households)
                break; // this bubble is now full

        }

    }

    // Compact the component roots to 0..K-1 and label agents.
    std::unordered_map< size_t, int > root_label;
    int next_label = 0;
    for (size_t a = 0u; a < n; ++a)
    {
        size_t root = find(hh_index[household_id[a]]);
        auto it = root_label.find(root);
        if (it == root_label.end())
        {
            root_label[root] = next_label;
            bubble_id[a] = next_label;
            ++next_label;
        }
        else
        {
            bubble_id[a] = it->second;
        }
    }

}

template<typename TSeq>
inline bool Bubbles<TSeq>::wants_ties(Model<TSeq> * model) const
{

    if (ties != BubbleTies::Complete)
        return false;

    // Global events run *after* the day's transitions, so the network this
    // leaves behind is the one the next step will use -- which is also when the
    // tool starts (or stops) damping. Asking about `today() + 1` keeps the ties
    // and the damping switching on the same day.
    int next = model->today() + 1;

    // ... and when there is no next step, nothing needs ties. Withdrawing them
    // on the last day is what keeps the intervention from outliving its run:
    // Model::run() takes no population backup, so ties left in the network
    // would still be there when the model is run again, or would be captured by
    // the backup run_multiple() takes. `ndays == 0` means the day loop is being
    // driven by hand and there is no known end, so the question does not apply.
    size_t ndays = static_cast< size_t >(model->get_ndays());
    if ((ndays > 0u) && (next > static_cast< int >(ndays)))
        return false;

    return is_active(next);

}

template<typename TSeq>
inline void Bubbles<TSeq>::build_ties(Model<TSeq> * model)
{

    // Agents grouped by bubble. The labels compute_partition() hands out are
    // consecutive from zero, so this is an indexed bucket rather than a hash
    // map: the order ties are created in decides the order they take in each
    // agent's neighbor list, and that decides which transmitter roulette()
    // picks. A container with unspecified iteration order would make runs
    // depend on the standard library rather than on the seed.
    int n_bubbles = 0;
    for (int b : bubble_id)
        if (b >= n_bubbles)
            n_bubbles = b + 1;

    if (n_bubbles == 0)
        return;

    std::vector< std::vector< size_t > > members(
        static_cast< size_t >(n_bubbles)
    );

    for (size_t a = 0u; a < bubble_id.size(); ++a)
        if (bubble_id[a] >= 0)
            members[static_cast< size_t >(bubble_id[a])].push_back(a);

    // What the sampler can take is a *degree*, not a bubble size: roulette()
    // uses two slots per candidate in the fixed scratch array, so an agent may
    // have at most `array_double_tmp.size() / 2` infectious neighbors before it
    // throws. Completing a bubble adds ties on top of the ones an agent already
    // has outside it, so the number that matters is what each member's degree
    // will be afterwards -- a bubble small enough to look harmless can still
    // push a well-connected member over.
    //
    // Checked for every member before a single tie is created, so a bubble that
    // is too large fails without leaving the network half-rewritten.
    size_t max_degree = model->array_double_tmp.size() / 2u;

    for (auto & who : members)
    {

        if (who.size() < 2u)
            continue;

        for (size_t x : who)
        {

            // Ties to bubble-mates the agent already has are not added twice.
            size_t already = 0u;
            for (auto * nb : model->get_agent(x).neighbors_view(*model))
                if (bubble_id[static_cast< size_t >(nb->get_id())] ==
                    bubble_id[x])
                    ++already;

            size_t projected =
                model->get_agent(x).get_n_neighbors() +
                (who.size() - 1u) - already;

            if (projected > max_degree)
                throw std::length_error(
                    "Bubbles: completing a bubble of " +
                    std::to_string(who.size()) + " would give agent " +
                    std::to_string(x) + " a degree of " +
                    std::to_string(projected) + ", above the " +
                    std::to_string(max_degree) + " neighbors the virus sampler "
                    "can weigh. Reduce group_size or max_households."
                );

        }

    }

    for (auto & who : members)
    {

        for (size_t i = 0u; i < who.size(); ++i)
        {
            for (size_t j = i + 1u; j < who.size(); ++j)
            {

                // Only ties this actually created are recorded: the ones the
                // network already had are not the intervention's to withdraw.
                if (model->add_edge(who[i], who[j]))
                    created_ties.emplace_back(who[i], who[j]);

            }
        }

    }

    ties_epoch = last_epoch;

    // Snapshot taken after the clique is up, so the next day can tell in
    // constant time whether anything has disturbed it.
    ties_version = model->get_network_version();

}

template<typename TSeq>
inline Bubbles<TSeq> * Bubbles<TSeq>::heir_of(
    Model<TSeq> * model,
    size_t i,
    size_t j
)
{

    // Nothing to hand over unless somebody else is holding a bubble open.
    for (size_t e = 0u; e < model->get_n_globalevents(); ++e)
    {

        auto * other =
            dynamic_cast< Bubbles<TSeq> * >(&model->get_globalevent(e));

        if ((other == nullptr) || (other == this))
            continue;

        if (other->ties != BubbleTies::Complete)
            continue;

        if (!other->wants_ties(model))
            continue;

        const auto & other_id = other->bubble_id;
        if ((i >= other_id.size()) || (j >= other_id.size()))
            continue;

        if ((other_id[i] < 0) || (other_id[i] != other_id[j]))
            continue;

        return other;

    }

    return nullptr;

}

template<typename TSeq>
inline void Bubbles<TSeq>::withdraw_ties(
    Model<TSeq> * model,
    bool hand_over
)
{

    for (auto & tie : created_ties)
    {

        // Two policies can want the same tie -- household bubbles and school
        // bubbles, say -- but only the one that happened to create it has it on
        // its books. Dropping it here would take it away from a bubble that is
        // still open, so it is handed to that policy instead of being removed.
        // Without this the tie would vanish for a day and come back when the
        // other policy next re-asserts its clique.
        if (hand_over)
        {

            Bubbles<TSeq> * heir = heir_of(model, tie.first, tie.second);

            if (heir != nullptr)
            {
                heir->created_ties.push_back(tie);
                continue;
            }

        }

        // A tie that has gone missing is skipped rather than treated as an
        // error: the model is free to have removed it in the meantime, and this
        // only ever takes back what it put in.
        model->rm_edge(tie.first, tie.second);

    }

    created_ties.clear();
    ties_epoch = -1;

}

template<typename TSeq>
inline void Bubbles<TSeq>::restore_network(Model<TSeq> * model)
{
    withdraw_ties(model, false);
}

template<typename TSeq>
inline void Bubbles<TSeq>::sync_ties(Model<TSeq> * model)
{

    bool up = (ties_epoch >= 0);

    if (!wants_ties(model))
    {

        if (up)
            withdraw_ties(model, true);

        return;

    }

    // The standing clique belongs to a partition that has since moved on.
    if (up && (ties_epoch != last_epoch))
        withdraw_ties(model, true);

    // Nothing has touched the network since the clique went up, so it is still
    // exactly as it was left. This is the ordinary case, and skipping it is
    // what keeps the policy from walking every pair of every bubble on every
    // day of the run.
    else if (up && (model->get_network_version() == ties_version))
        return;

    // Otherwise the clique is re-asserted rather than assumed to still be
    // standing: something changed the network, and it may well have been a tie
    // this bubble wants. Another policy may own a tie this one also wants and
    // take it down when its own window closes; the model itself is free to
    // remove one too, and nothing tells this policy that happened.
    // build_ties() is idempotent -- Model::add_edge() is a no-op on a tie that
    // is already there -- so the same call both raises the clique the first
    // time and repairs it afterwards.
    build_ties(model);

}

template<typename TSeq>
inline void Bubbles<TSeq>::compute_partition(Model<TSeq> * model)
{

    bubble_id.assign(household_id.size(), -1);

    if (flavor == BubbleFlavor::Household)
        partition_household(model);
    else
        partition_peer(model);

}

template<typename TSeq>
inline bool Bubbles<TSeq>::is_active(int today) const
{
    return (today >= start_day) && ((end_day < 0) || (today < end_day));
}

template<typename TSeq>
inline epiworld_double Bubbles<TSeq>::susceptibility_reduction(
    const Agent<TSeq> * p,
    const Agent<TSeq> * transmitter,
    Model<TSeq> * model
) const
{

    if (!is_active(static_cast< int >(model->today())))
        return 0.0;

    if (bubble_id.empty() || (p == nullptr) || (transmitter == nullptr))
        return 0.0;

    int bp = bubble_id[static_cast< size_t >(p->get_id())];
    int bt = bubble_id[static_cast< size_t >(transmitter->get_id())];

    if ((bp < 0) || (bt < 0))
        return 0.0;

    // Contacts inside the bubble are exactly what the policy keeps: they are
    // left alone.
    if (bp == bt)
        return 0.0;

    // Contacts outside the bubble are scaled by the transmission factor:
    // 0 = perfectly observed bubble (contact cut), 1 = the bubble imposes
    // nothing.
    epiworld_double factor = model->par(param_name);
    if (factor <= 0.0)
        return 1.0;
    if (factor >= 1.0)
        return 0.0;

    return static_cast<epiworld_double>(1.0) - factor;

}

template<typename TSeq>
inline void Bubbles<TSeq>::_setup(Model<TSeq> * model)
{

    if (household_id.size() != model->size())
        throw std::length_error(
            "Bubbles: household_id length (" +
            std::to_string(household_id.size()) +
            ") must equal the number of agents (" +
            std::to_string(model->size()) + ")."
        );

    if (ties == BubbleTies::Complete)
    {

        // Completing a bubble means editing ties at both ends, which is not a
        // meaningful thing to do to a directed network.
        if (model->is_directed())
            throw std::logic_error(
                "Bubbles: BubbleTies::Complete needs an undirected model."
            );

        // Degree-sequence rewiring swaps neighbors *between* agents, so a tie
        // this created can be swapped out from under it and would never be
        // withdrawn. The two cannot be combined.
        if (model->get_rewire_prop() > 0.0)
            throw std::logic_error(
                "Bubbles: BubbleTies::Complete cannot be combined with "
                "degree-sequence rewiring; call set_rewire_prop(0)."
            );

    }

    // Whatever a previous run left in the network goes first. Normally there is
    // nothing to do -- a run withdraws its own ties on its last day (see
    // wants_ties) -- but a hand-driven day loop can stop early.
    restore_network(model);

    // ---- The transmission factor lives in the model -------------------------
    // The tool reads it on every exposure rather than holding a copy, so the
    // strictness of the policy can be inspected, calibrated, or switched
    // mid-run through the model's parameters. The value passed to the
    // constructor is only a default: a value already in the model (set by the
    // user, or read from a parameter file) is what governs the run.
    if (!model->has_param(param_name))
        model->add_param(transmission_factor, param_name);

    // ---- The partition ------------------------------------------------------
    // Drawn with the model's RNG, which the run has already seeded, so each
    // replicate of run_multiple() gets its own partition from its own seed and
    // never inherits one from a previous run.
    compute_partition(model);
    last_epoch = 0;

    // ---- The bubble as ties -------------------------------------------------
    // Under BubbleTies::Complete the bubble is not only a transmission rule: it
    // is a clique, so households are completed and merged households actually
    // meet. No queue bookkeeping is needed here -- Model::reset() has cleared
    // the queue and the dist_virus() events are still pending, so the
    // events_run() that follows this builds the counts from the finished
    // network.
    sync_ties(model);

    // ---- The tool: dampens out-of-bubble transmission -----------------------
    // It carries no state of its own: it finds the model's intervention by name
    // the first time it is used. It is registered without a distribution
    // function on purpose -- handing it out here, on every run, keeps the first
    // run and the ones after it (where the tool is already registered)
    // identical.
    if (!model->has_tool(this->get_name()))
    {
        BubbleTool<TSeq> bubble_tool(this->get_name(), this->get_name());
        model->add_tool(bubble_tool);
    }

    auto & bubble_tool = model->get_tool(this->get_name());
    for (size_t i = 0u; i < model->size(); ++i)
        model->get_agent(i).add_tool(*model, bubble_tool);

}

template<typename TSeq>
inline void Bubbles<TSeq>::reset(Model<TSeq> * model)
{

    // Model::reset() runs this once per run -- and once per replicate of
    // run_multiple(), on that replicate's own copy of the model -- just before
    // day 1, which is why the user has nothing to call: adding the intervention
    // to the model is the whole installation.
    this->model_id = static_cast< int >(model->get_sim_id());
    this->_setup(model);

}

template<typename TSeq>
inline void Bubbles<TSeq>::operator()(Model<TSeq> * model, int day)
{

    // Under Model::run() this never fires: reset() has already set us up for
    // this run. It is here for a model whose day loop is driven by hand, where
    // installing the policy a day late still beats running without it. The
    // simulation id is what tells one run from the next, so a copy of the model
    // (another replicate, another thread) sets itself up on its own.
    if (static_cast< int >(model->get_sim_id()) != this->model_id)
    {
        this->model_id = static_cast< int >(model->get_sim_id());
        this->_setup(model);
    }

    // Past setup, the daily event has two jobs: move the partition on at a
    // rewiring epoch, and keep the network matching the policy. The tool gates
    // itself by day. Because global events run after update_state(), both take
    // effect the following simulation step.
    if ((rewire_every > 0) && is_active(day))
    {

        int epoch = (day - start_day) / rewire_every;

        if (last_epoch != epoch)
        {

            // The clique comes down *before* the new partition is drawn. Both
            // rules read the contact network to decide which households may
            // bubble together, and the ties this put there are not contacts
            // anybody has -- leaving them up would let the last bubble's
            // members look connected to each other and pull the next bubble
            // towards the same membership. Grouping has to see the network the
            // model actually has, which is what keeps BubbleTies independent of
            // BubbleFlavor.
            //
            // Handing over rather than plainly removing: another policy may
            // want some of these ties for a bubble of its own that is still
            // open, and its window has nothing to do with this one's epochs.
            withdraw_ties(model, true);

            compute_partition(model);
            last_epoch = epoch;

        }

    }

    // Cheap and immediate unless the policy is realized as ties: builds the
    // clique on the day the policy starts, redraws it when the partition moves
    // to a new epoch, and withdraws it when the policy lifts or the run ends.
    // Model::add_edge()/rm_edge() keep the queueing system in step, so this is
    // safe in the middle of a run.
    sync_ties(model);

}

template<typename TSeq>
inline Bubbles<TSeq> * Bubbles<TSeq>::get_from(
    Model<TSeq> & model,
    const std::string & name
)
{

    if (!model.has_globalevent(name))
        return nullptr;

    return dynamic_cast< Bubbles<TSeq> * >(&model.get_globalevent(name));

}

template<typename TSeq>
inline const std::vector< int > & Bubbles<TSeq>::get_bubble_id() const
{
    return bubble_id;
}

template<typename TSeq>
inline int Bubbles<TSeq>::get_last_epoch() const
{
    return last_epoch;
}

template<typename TSeq>
inline BubbleFlavor Bubbles<TSeq>::get_flavor() const
{
    return flavor;
}

template<typename TSeq>
inline BubbleTies Bubbles<TSeq>::get_ties() const
{
    return ties;
}

template<typename TSeq>
inline void Bubbles<TSeq>::set_ties(BubbleTies ties)
{
    this->ties = ties;
}

template<typename TSeq>
inline const std::vector< std::pair< size_t, size_t > > &
Bubbles<TSeq>::get_created_ties() const
{
    return created_ties;
}

template<typename TSeq>
inline const std::string & Bubbles<TSeq>::get_param_name() const
{
    return param_name;
}

template<typename TSeq>
inline std::unique_ptr< GlobalEvent<TSeq> > Bubbles<TSeq>::clone_ptr() const
{
    return std::make_unique< Bubbles<TSeq> >(*this);
}

#endif
