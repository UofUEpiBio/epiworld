#ifndef EPIWORLD_CONTACTLAYER_BONES_HPP
#define EPIWORLD_CONTACTLAYER_BONES_HPP

/**
 * @file contactlayer-bones.hpp
 * @brief Base class of the contact layers of a model.
 *
 * @details A *contact layer* is one source of contacts of a model: it answers
 * "whom did agent `i` contact this step?". A model owns any number of them (see
 * `Model::add_layer()`), and none are implied: a model without layers has no
 * transmission through contacts.
 *
 * The model drives the life cycle of its layers:
 *
 * - `reset()` at the end of `Model::reset()`, once the population is in its
 *   initial state. A layer restores its baseline if it was edited during the
 *   previous run (see `is_dirty()`) and sizes its per-run buffers.
 * - `update()` at the end of every step, including day 0 (at the start of
 *   `Model::next()`): after the agents' update functions, the global events
 *   and rewiring, so that whatever changed during a step (states, edits of the
 *   layer) is in force for the next step's contacts.
 * - `clone()` whenever the model is copied (`run_multiple()` copies models),
 *   so replicates never share layer state.
 *
 * ### Edits and the dirty flag
 *
 * Whether an edit is permanent depends only on when it is made. An edit made
 * during a run (from an update function or a global event) applies to that run
 * only: it sets the *dirty* flag, and `reset()` restores the baseline of a
 * dirty layer and clears the flag. An edit made between runs becomes the new
 * baseline.
 *
 * @ingroup contact_layers
 */
template<typename TSeq = EPI_DEFAULT_TSEQ>
class ContactLayer {

    friend class Model<TSeq>;

private:

    std::string name;
    size_t id = 0u;          ///< Position in the model's list, set by the model
    bool dirty = false;  ///< Edited since the baseline was set
    bool in_run = false; ///< Set by the model while it runs

protected:

    /**
     * @brief Call it from every method that edits the layer.
     * @return `true` if the edit is to the baseline (made between runs);
     * otherwise it marks the layer dirty and returns `false`.
     */
    bool edits_baseline()
    {
        dirty = dirty || in_run;
        return !in_run;
    }

    /// Call it from `reset()` once the baseline has been restored.
    void mark_clean() { dirty = false; }

public:

    explicit ContactLayer(std::string name_) : name(std::move(name_)) {}

    virtual ~ContactLayer() = default;

    const std::string & get_name() const { return name; }
    size_t get_id() const { return id; }
    bool is_dirty() const { return dirty; }

    /// Restore the baseline if dirty and size the buffers for a new run.
    virtual void reset(Model<TSeq> & model) = 0;

    /// Rebuild what depends on the state of the population (end of each step).
    virtual void update(Model<TSeq> & model) = 0;

    /// Deep copy (the model's copy constructor calls it).
    virtual std::unique_ptr< ContactLayer<TSeq> > clone() const = 0;

};

#endif
