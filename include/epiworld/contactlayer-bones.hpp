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
 * - `update()` once per step, right after the agents' update functions have
 *   run and their changes are applied (unless `set_auto_update(false)`, in
 *   which case the model calls it where it needs to).
 * - `clone()` whenever the model is copied (`run_multiple()` copies models),
 *   so replicates never share layer state.
 *
 * ### Edits and the dirty flag
 *
 * Every edit of a layer made through its own methods, as opposed to the
 * baseline it was configured with, sets the *dirty* flag. `reset()` restores
 * the baseline of a dirty layer and clears the flag, and leaves a clean layer
 * as it is. An edit made between runs, with the "baseline" option of the
 * setter, becomes the new baseline.
 *
 * @ingroup contact_layers
 */
template<typename TSeq = EPI_DEFAULT_TSEQ>
class ContactLayer {

    friend class Model<TSeq>;

private:

    std::string name;
    size_t id = 0u;          ///< Position in the model's list, set by the model
    bool dirty = false;      ///< Edited since the baseline was set
    bool auto_update = true; ///< Updated by `Model::run()` once per step

protected:

    /// Call it from every method that edits the layer after its baseline.
    void mark_dirty() { dirty = true; }

    /// Call it from `reset()` once the baseline has been restored.
    void mark_clean() { dirty = false; }

public:

    explicit ContactLayer(std::string name_) : name(std::move(name_)) {}

    virtual ~ContactLayer() = default;

    const std::string & get_name() const { return name; }
    size_t get_id() const { return id; }
    bool is_dirty() const { return dirty; }

    bool get_auto_update() const { return auto_update; }

    /**
     * @brief Whether `Model::run()` calls `update()` every step.
     * @details On by default. Turn it off if the model has to update the layer
     * at a different point of the step (e.g., after its own global events).
     */
    void set_auto_update(bool value) { auto_update = value; }

    /// Restore the baseline if dirty and size the buffers for a new run.
    virtual void reset(Model<TSeq> & model) = 0;

    /// Rebuild what depends on the state of the population (once per step).
    virtual void update(Model<TSeq> & model) = 0;

    /// Deep copy (the model's copy constructor calls it).
    virtual std::unique_ptr< ContactLayer<TSeq> > clone() const = 0;

};

#endif
