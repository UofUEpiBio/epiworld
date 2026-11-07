// Every standard header the library uses must be included here: the other
// headers are included inside `namespace epiworld`, so a standard header first
// included there would be declared in `epiworld::std`.
#include <array>
#include <vector>
#include <functional>
#include <memory>
#include <utility>
#include <stdexcept>
#include <random>
#include <cmath>
#include <fstream>
#include <string>
#include <string_view>
#include <map>
#include <unordered_map>
#include <chrono>
#include <climits>
#include <cstdint>
#include <cstddef>
#include <limits>
#include <numeric>
#include <algorithm>
#include <regex>
#include <sstream>
#include <iomanip>
#include <set>
#include <type_traits>
#include <cassert>
#include <atomic>
#include <variant>
#include <optional>

#ifndef EPIWORLD_HPP
#define EPIWORLD_HPP

/* Versioning */
#define EPIWORLD_VERSION_MAJOR 1
#define EPIWORLD_VERSION_MINOR 0
#define EPIWORLD_VERSION_PATCH 0

#define EPIWORLD_VERSION_PRERELEASE ""

static constexpr int epiworld_version_major = EPIWORLD_VERSION_MAJOR;
static constexpr int epiworld_version_minor = EPIWORLD_VERSION_MINOR;
static constexpr int epiworld_version_patch = EPIWORLD_VERSION_PATCH;
static constexpr std::string_view epiworld_version_prerelease =
    EPIWORLD_VERSION_PRERELEASE;

inline std::string epiworld_version() {
    std::string v =
        std::to_string(epiworld_version_major) + "." +
        std::to_string(epiworld_version_minor) + "." +
        std::to_string(epiworld_version_patch);

    if (!epiworld_version_prerelease.empty()) {
        v += "-";
        v += epiworld_version_prerelease;
    }

    return v;
}

namespace epiworld {

/*//////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

 Start of -./include/epiworld/config.hpp-

////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////*/


#ifndef EPIWORLD_CONFIG_HPP
#define EPIWORLD_CONFIG_HPP

#ifndef printf_epiworld
    #define printf_epiworld fflush(stdout);printf
#endif

// In case the user has a way to stop the program
// This is called during `run_multiple()` and it is
// passed the simulation number.
#ifndef EPI_CHECK_USER_INTERRUPT
    #define EPI_CHECK_USER_INTERRUPT(a)
#endif

#ifndef EPIWORLD_MAXNEIGHBORS
    #define EPIWORLD_MAXNEIGHBORS 1048576
#endif

#if defined(_OPENMP) || defined(__OPENMP)
    #include <omp.h>
// #else
//     #define omp_get_thread_num() 0
//     #define omp_set_num_threads() 1
#endif

#ifndef epiworld_double
    #define epiworld_double float
#endif

#ifndef epiworld_fast_int
    #define epiworld_fast_int int
#endif

#ifndef epiworld_fast_uint
    #define epiworld_fast_uint unsigned long long int
#endif

// By default, `Model::rbinom` will use a Poisson approximation in the
// rare-event regime where p <= 0.01 and n * p^2 <= 0.1. Advanced users can
// disable this by defining EPI_NO_FAST_BINOM before including epiworld.
#if !defined(EPI_NO_FAST_BINOM) && !defined(EPI_FAST_BINOM)
    #define EPI_FAST_BINOM
#endif

#ifndef EPI_MAX_TRACKING
    #define EPI_MAX_TRACKING 200
#endif

// Degree at which an agent starts keeping a hash index of its neighbors.
//
// Below the threshold, membership tests and tie removal scan the (contiguous,
// insertion-ordered) neighbor vector, which for the degrees these models run at
// -- smallworld networks of degree 5-8, households of 3-5 -- fits in a cache
// line and beats a hash lookup. Past it, the linear scan starts to dominate
// graph construction and tie surgery, so the network layer builds the agent's
// index once and maintains it from then on. See `layer::Network`.
#ifndef EPI_NEIGHBOR_INDEX_THRESHOLD
    #define EPI_NEIGHBOR_INDEX_THRESHOLD 32u
#endif

class Model;

class Agent;

class PersonTools;

class Virus;

class Tool;

class GlobalEvent;

class ContactLayer;

namespace layer {
class Network;
class Mixing;
}

using VirusPtr = std::shared_ptr< Virus >;

using ToolPtr = std::shared_ptr< Tool >;

using GlobalEventPtr = std::shared_ptr< GlobalEvent >;

using ToolFun = std::function<epiworld_double(Tool&,Agent*,VirusPtr&,Model*)>;

using MixerFun = std::function<epiworld_double(Agent*,VirusPtr&,Model*)>;

using MutFun = std::function<bool(Agent*,Virus&,Model*)>;

/// The engine's update of the agents in a state (see `nh_build_states()`)
using UpdateFun = std::function<void(Agent*,Model*)>;

using GlobalFun = std::function<void(Model*)>;

struct Event;

using EventFun = std::function<void(Event&,Model*)>;

enum class EventAction : uint8_t {
    AddVirus,
    AddTool,
    RemoveVirus,
    RemoveTool,
    ChangeState
};

/**
 * @brief How network transmission is sampled each step.
 *
 * @details Agents in a stage whose only drawn exit is a default infection
 * (no custom sampler) can acquire a virus in two ways that give the same
 * distribution of outcomes:
 *
 * - `pull`: each susceptible agent scans its neighbors and draws its infector
 *   with `roulette()` (the approach of epiworld <= 0.15).
 * - `push`: each agent carrying a virus adds its infection odds to its
 *   susceptible neighbors, and each of those then makes one draw. Its cost
 *   scales with the carriers' ties rather than with the susceptibles'.
 * - `automatic` (the default): picks whichever of the two is cheaper at each
 *   step.
 *
 * Only the stream of random numbers differs between the modes; `pull`
 * reproduces the streams of earlier versions. See
 * `Model::set_transmission_mode()`.
 */
enum class TransmissionMode : uint8_t {
    automatic,
    push,
    pull
};

/// Defined when `TransmissionMode` and `Model::set_transmission_mode()` exist.
#define EPIWORLD_HAS_TRANSMISSION_MODE

/**
 * @brief Default threshold of the `"auto"` transmission mode.
 * @details The model pushes when the cost of the push is at most this
 * fraction of the cost of the pull. See `Model::set_transmission_mode()`.
 */
#ifndef EPI_DEFAULT_TRANSMISSION_KAPPA
    #define EPI_DEFAULT_TRANSMISSION_KAPPA 0.5
#endif

/**
 * @brief Cost of visiting an agent, in ties, in the `"auto"` transmission mode.
 * @details Both a push (per carrier) and a pull (per susceptible) pay a fixed
 * price for each agent they visit on top of the price of each tie they scan.
 * This is that price in units of ties. It is a property of how agents are laid
 * out in memory, not of the network. See `Model::set_transmission_mode()`.
 */
#ifndef EPI_TRANSMISSION_AGENT_COST
    #define EPI_TRANSMISSION_AGENT_COST 4.0
#endif

/**
 * @brief Event data for update an agent
 *
 */
struct Event {
    Agent * agent;
    VirusPtr virus;
    ToolPtr tool;
    epiworld_fast_int new_state;
    epiworld_fast_int queue;
    EventAction action;
    int layer = -1; ///< Layer of the contact that transmitted the virus, if any
    int transition = -1; ///< Declared transition of the natural history (-1: none)
public:
/**
     * @brief Construct a new Event object
     *
     * All the parameters are rather optional.
     *
     * @param agent_ Agent over who the action will happen
     * @param virus_ Virus to add
     * @param tool_ Tool to add
     * @param virus_idx Index of virus to be removed (if needed)
     * @param tool_idx Index of tool to be removed (if needed)
     * @param new_state_ Next state
     * @param queue_ Efect on the queue
     * @param action_ The action to execute
     * @param layer_ Layer of the contact that transmitted the virus, if any
     */
    Event(
        Agent * agent_,
        VirusPtr & virus_,
        ToolPtr & tool_,
        epiworld_fast_int new_state_,
        epiworld_fast_int queue_,
        EventAction action_,
        int layer_ = -1
    ) : agent(agent_), virus(virus_), tool(tool_),
        new_state(new_state_),
        queue(queue_), action(action_), layer(layer_) {
            return;
        };
};

/**
 * @name Constants in epiworld
 *
 * @details The following are the default values some probabilities and
 * rates take when no value has been specified in the model.
 */
///@{
#ifndef DEFAULT_TOOL_CONTAGION_REDUCTION
    #define DEFAULT_TOOL_CONTAGION_REDUCTION    0.0
#endif

#ifndef DEFAULT_TOOL_TRANSMISSION_REDUCTION
    #define DEFAULT_TOOL_TRANSMISSION_REDUCTION 0.0
#endif

#ifndef DEFAULT_TOOL_RECOVERY_ENHANCER
    #define DEFAULT_TOOL_RECOVERY_ENHANCER      0.0
#endif

#ifndef DEFAULT_TOOL_DEATH_REDUCTION
    #define DEFAULT_TOOL_DEATH_REDUCTION        0.0
#endif

#ifndef EPI_DEFAULT_VIRUS_PROB_INFECTION
    #define EPI_DEFAULT_VIRUS_PROB_INFECTION    1.0
#endif

#ifndef EPI_DEFAULT_VIRUS_PROB_RECOVERY
    #define EPI_DEFAULT_VIRUS_PROB_RECOVERY     0.1428
#endif

#ifndef EPI_DEFAULT_VIRUS_PROB_DEATH
    #define EPI_DEFAULT_VIRUS_PROB_DEATH        0.0
#endif

#ifndef EPI_DEFAULT_INCUBATION_DAYS
    #define EPI_DEFAULT_INCUBATION_DAYS         7.0
#endif
///@}

#ifdef EPI_DEBUG
    #define EPI_DEBUG_PRINTF printf_epiworld

    #define EPI_DEBUG_ERROR(etype, msg) \
        (etype)("[[epi-debug]] (error) " + std::string(msg));

    #define EPI_DEBUG_NOTIFY_ACTIVE() \
        EPI_DEBUG_PRINTF("DEBUGGING ON (compiled with EPI_DEBUG defined)%s\n", "");

    #define EPI_DEBUG_ALL_NON_NEGATIVE(vect) \
        for (auto & v : vect) \
            if (static_cast<double>(v) < 0.0) \
                throw EPI_DEBUG_ERROR(std::logic_error, "A negative value not allowed.");

    #define EPI_DEBUG_SUM_DBL(vect, num) \
        double _epi_debug_sum = 0.0; \
        for (auto & v : vect) \
        {   \
            _epi_debug_sum += static_cast<double>(v);\
            if (_epi_debug_sum > static_cast<double>(num)) \
                throw EPI_DEBUG_ERROR(std::logic_error, "The sum of elements not reached."); \
        }

    #define EPI_DEBUG_SUM_INT(vect, num) \
        int _epi_debug_sum = 0; \
        for (auto & v : vect) \
        {   \
            _epi_debug_sum += static_cast<int>(v);\
            if (_epi_debug_sum > static_cast<int>(num)) \
                throw EPI_DEBUG_ERROR(std::logic_error, "The sum of elements not reached."); \
        }

    #define EPI_DEBUG_VECTOR_MATCH_INT(a, b, c) \
        if (a.size() != b.size())  {\
            EPI_DEBUG_PRINTF("In '%s'", std::string(c).c_str()); \
            EPI_DEBUG_PRINTF("Size of vector a: %lu\n", (a).size());\
            EPI_DEBUG_PRINTF("Size of vector b: %lu\n", (b).size());\
            throw EPI_DEBUG_ERROR(std::length_error, "The vectors do not match size."); \
        }\
        for (int _i = 0; _i < static_cast<int>(a.size()); ++_i) \
            if (a[_i] != b[_i]) {\
                EPI_DEBUG_PRINTF("In '%s'", std::string(c).c_str()); \
                EPI_DEBUG_PRINTF("Iterating the last 5 values%s:\n", ""); \
                for (int _j = std::max(0, static_cast<int>(_i) - 4); _j <= _i; ++_j) \
                { \
                    EPI_DEBUG_PRINTF( \
                        "a[%i]: %i; b[%i]: %i\n", \
                        _j, \
                        static_cast<int>(a[_j]), \
                        _j, static_cast<int>(b[_j])); \
                } \
                throw EPI_DEBUG_ERROR(std::logic_error, "The vectors do not match."); \
            }

    #define EPI_DEBUG_FAIL_AT_TRUE(a,b) \
        if (a) \
        {\
            throw EPI_DEBUG_ERROR(std::logic_error, b); \
        }

    #define epiexception(a) std::logic_error
#else
    #define EPI_DEBUG_PRINTF(fmt, ...)
    #define EPI_DEBUG_ERROR(fmt, ...)
    #define EPI_DEBUG_NOTIFY_ACTIVE()
    #define EPI_DEBUG_ALL_NON_NEGATIVE(vect)
    #define EPI_DEBUG_SUM_DBL(vect, num)
    #define EPI_DEBUG_SUM_INT(vect, num)
    #define EPI_DEBUG_VECTOR_MATCH_INT(a, b, c)
    #define EPI_DEBUG_FAIL_AT_TRUE(a, b) \
        if (a) \
            return false;
    #define epiexception(a) a
#endif

#if defined(EPI_DEBUG_NO_THREAD_ID) || (!defined(__OPENMP) && !defined(_OPENMP))
    #define EPI_GET_THREAD_ID() 0
#else
    #define EPI_GET_THREAD_ID() omp_get_thread_num()
#endif

#endif
/*//////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

 End of -./include/epiworld/config.hpp-

////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////*/


/*//////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

 Start of -./include/epiworld/epiworld-macros.hpp-

////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////*/


#ifndef EPIWORLD_MACROS_HPP
#define EPIWORLD_MACROS_HPP

/**
 * @brief Helper macro to define a new tool
 * 
 */
#define EPI_NEW_TOOL(fname) inline epiworld_double \
(fname)(\
    epiworld::Tool & t, \
    epiworld::Agent * p, \
    std::shared_ptr<epiworld::Virus> v, \
    epiworld::Model * m\
    )

/**
 * @brief Create a Tool within a function
 * 
 */
#define EPI_NEW_TOOL_LAMBDA(funname) \
    epiworld::ToolFun funname = \
    [](epiworld::Tool & t, \
    epiworld::Agent * p, \
    std::shared_ptr<epiworld::Virus> v, \
    epiworld::Model * m) -> epiworld_double

/**
 * @brief Helper macro for accessing model parameters
 * 
 */
#define EPI_PARAMS(i) m->operator()(i)

/**
 * @brief Value of the parameter named `pname` (a string literal) in `model`
 * (a pointer to a model).
 *
 * @details Same value as `model->par(pname)`, but the name is looked up once
 * per model layout and cached in a function-local `static ParamRef`, so
 * repeated calls read the value directly. Use it in update functions and
 * other code that runs for many agents every day.
 */
#define EPI_PAR(model, pname) \
    ([]() -> const ::epiworld::ParamRef & { \
        static const ::epiworld::ParamRef epi_param_ref_(pname); \
        return epi_param_ref_; \
    }()(*(model)))

/**
 * @brief Helper macro for defining Mutation Functions
 * 
 */
#define EPI_NEW_MUTFUN(funname) inline bool \
    (funname)(\
    epiworld::Agent * p, \
    epiworld::Virus & v, \
    epiworld::Model * m )

#define EPI_NEW_MUTFUN_LAMBDA(funname) \
    epiworld::MutFun funname = \
    [](epiworld::Agent * p, \
    epiworld::Virus & v, \
    epiworld::Model * m) -> bool

#define EPI_RUNIF() m->runif()

#define EPI_TOKENPASTE(a,b) a ## b
#define MPAR(num) *(m->EPI_TOKENPASTE(p,num))


#define EPI_NEW_GLOBALFUN(funname) inline void \
    (funname)(epiworld::Model* m)

#define EPI_NEW_GLOBALFUN_LAMBDA(funname) \
    epiworld::GlobalFun funname = \
    [](epiworld::Model* m) -> void


#define EPI_CHECK_COALESCE(proposed_, virus_tool_, alt_) \
    if (static_cast<int>(proposed_) == -99) {\
        if (static_cast<int>(virus_tool_) == -99) \
            (proposed_) = (alt_);\
        else (proposed_) = (virus_tool_);}

#endif
/*//////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

 End of -./include/epiworld/epiworld-macros.hpp-

////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////*/


/*//////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

 Start of -./include/epiworld/param-ref.hpp-

////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////*/


#ifndef EPIWORLD_PARAM_REF_HPP
#define EPIWORLD_PARAM_REF_HPP

/**
 * @brief Position of a parameter in a model's parameter table.
 *
 * @details Returned by `Model::get_param_id()` and read with
 * `Model::par_at()`. Parameters are never removed, so an id stays valid for
 * the model that returned it and for every copy of that model (including the
 * copies `run_multiple()` makes). Models built separately may place the same
 * name at different positions; use `ParamRef` when the same code runs on
 * different models.
 */
struct ParamId {
    size_t idx;
};

/**
 * @brief Identifies one layout of a model's parameter table.
 *
 * @details Every model starts with a fresh id, copies keep the id of the
 * model they copy, and adding a new parameter gives the model a fresh id. So
 * two models with the same id always map names to the same positions. Zero
 * is never returned; `ParamRef` uses it for "not resolved yet".
 */
inline uint32_t new_param_layout_id()
{
    static std::atomic< uint32_t > counter{0u};
    uint32_t id = ++counter;
    while (id == 0u)
        id = ++counter;
    return id;
}

/**
 * @brief A model parameter referenced by name, resolved to its position on
 * first use.
 *
 * @details Calling the object with a model returns the parameter's current
 * value. The first call on a model looks the name up in the parameter table
 * and caches its position together with the model's parameter layout (see
 * `new_param_layout_id()`); later calls on that model, or on any copy of it,
 * read the value directly. Using the object with a model of a different
 * layout looks the name up again, so a `ParamRef` is always safe to share
 * between models and threads.
 *
 * Values are never cached, only positions: changes made with `set_param()`
 * are seen immediately.
 *
 * `EPI_PAR(model, "name")` wraps a function-local `static ParamRef`, which is
 * the easiest way to use it inside update functions.
 */
class ParamRef {
private:
    std::string pname;

    // (layout id << 32) | position; 0 means not resolved.
    mutable std::atomic< uint64_t > cache{0u};

public:

    explicit ParamRef(std::string name) : pname(std::move(name)) {};

    ParamRef(const ParamRef & other) :
        pname(other.pname),
        cache(other.cache.load(std::memory_order_relaxed)) {};

    ParamRef & operator=(const ParamRef & other)
    {
        pname = other.pname;
        cache.store(
            other.cache.load(std::memory_order_relaxed),
            std::memory_order_relaxed
        );
        return *this;
    };

    const std::string & name() const { return pname; };

    /**
     * @brief Position of the parameter in `model`'s table.
     * @throws std::logic_error if the model has no parameter with this name.
     */
    template< typename TModel >
    ParamId id(const TModel & model) const
    {
        const uint32_t layout = model.get_param_layout_id();
        const uint64_t c = cache.load(std::memory_order_relaxed);
        if (static_cast< uint32_t >(c >> 32) == layout)
            return ParamId{static_cast< size_t >(c & 0xffffffffu)};

        ParamId res = model.get_param_id(pname);
        if (res.idx > 0xffffffffu)
            return res;

        cache.store(
            (static_cast< uint64_t >(layout) << 32) |
                static_cast< uint64_t >(res.idx),
            std::memory_order_relaxed
        );
        return res;
    };

    /**
     * @brief Current value of the parameter in `model`.
     * @throws std::logic_error if the model has no parameter with this name.
     */
    template< typename TModel >
    epiworld_double operator()(const TModel & model) const
    {
        return model.par_at(id(model));
    };

};

#endif
/*//////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

 End of -./include/epiworld/param-ref.hpp-

////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////*/



/*//////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

 Start of -./include/epiworld/epiassert-bones.hpp-

////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////*/


#ifndef EPIWORLD_EPIASSERT_HPP
#define EPIWORLD_EPIASSERT_HPP

/**
 * @defgroup epiassert Argument Assertion Utilities
 * @brief Runtime validation of function arguments.
 *
 * The `EpiAssert` class provides static template methods for validating
 * function arguments. Each method works with both scalar and iterable
 * (container) types via `if constexpr` dispatch. On failure the methods
 * throw `std::range_error` or `std::invalid_argument` with a descriptive
 * message that includes the variable name, the offending value, and an
 * optional caller location string.
 *
 * @par Example
 * @code
 * EpiAssert::check_bounds(pop_size, 0, 10000, "pop_size", "Model::init");
 * EpiAssert::check_probability(rate, "rate", "ModelSIR::ModelSIR");
 * EpiAssert::check_sum(probs, 1.0, "probabilities", "create_init_function");
 * @endcode
 *
 * @note Requires C++17 or later (`if constexpr`, `std::void_t`).
 * @{
 */

namespace epiassert_detail {

    /**
     * @brief Type trait that detects iterable types (types with `begin()`
     *        and `end()`).
     *
     * `std::string` is explicitly excluded so that it is treated as a
     * scalar value rather than a character container.
     */
    template<typename T, typename = void>
    struct is_iterable : std::false_type {};

    template<typename T>
    struct is_iterable<T, std::void_t<
        decltype(std::begin(std::declval<const T &>())),
        decltype(std::end(std::declval<const T &>()))
    >> : std::true_type {};

    template<>
    struct is_iterable<std::string> : std::false_type {};

    /**
     * @brief Convert a numeric value to its string representation.
     *
     * Uses `std::ostringstream` so that floating-point values are printed
     * with enough precision to be useful in error messages.
     */
    template<typename T>
    inline std::string to_str(const T & v)
    {
        std::ostringstream oss;
        oss << v;
        return oss.str();
    }

} // namespace epiassert_detail

/**
 * @brief Argument-assertion utilities for epiworld.
 *
 * All methods are `static` and accept an optional @p varname (for the
 * variable being checked) and @p caller (for the function or method that
 * called the check). Both are used only in the error message.
 */
class EpiAssert {
private:
    /** @brief Format the caller location for error messages. */
    static std::string fmt_location(const std::string & caller)
    {
        if (caller.empty())
            return "";
        return " (in '" + caller + "')";
    }

public:
    // -----------------------------------------------------------------
    //  check_bounds – value(s) in [lower, upper]
    // -----------------------------------------------------------------

    /**
     * @brief Assert that @p value is in `[lower, upper]`.
     *
     * For container types every element is checked individually.
     *
     * @throws std::range_error if any value violates the bounds.
     */
    template<typename T, typename BoundT>
    static void check_bounds(
        const T      value,
        const BoundT lower,
        const BoundT upper,
        const std::string varname = "value",
        const std::string caller  = ""
    )
    {
        if (lower > upper)
        {
            throw std::invalid_argument(
                "check_bounds: 'lower' (" +
                epiassert_detail::to_str(lower) +
                ") must be <= 'upper' (" +
                epiassert_detail::to_str(upper) + ")" +
                fmt_location(caller)
            );
        }

        if constexpr (epiassert_detail::is_iterable<T>::value)
        {
            size_t idx = 0;
            for (const auto & v : value)
            {
                if ((v < lower) || (v > upper))
                {
                    throw std::range_error(
                        "'" + varname + "[" +
                        std::to_string(idx) + "]' must be in [" +
                        epiassert_detail::to_str(lower) + ", " +
                        epiassert_detail::to_str(upper) + "], but got " +
                        epiassert_detail::to_str(v) +
                        fmt_location(caller)
                    );
                }
                ++idx;
            }
        }
        else
        {
            if ((value < lower) || (value > upper))
            {
                throw std::range_error(
                    "'" + varname + "' must be in [" +
                    epiassert_detail::to_str(lower) + ", " +
                    epiassert_detail::to_str(upper) + "], but got " +
                    epiassert_detail::to_str(value) +
                    fmt_location(caller)
                );
            }
        }
    }

    // -----------------------------------------------------------------
    //  check_non_negative – value(s) >= 0
    // -----------------------------------------------------------------

    /**
     * @brief Assert that @p value is non-negative.
     *
     * For container types every element is checked individually.
     *
     * @throws std::range_error if any value is negative.
     */
    template<typename T>
    static void check_non_negative(
        const T value,
        const std::string varname = "value",
        const std::string caller  = ""
    )
    {
        if constexpr (epiassert_detail::is_iterable<T>::value)
        {
            size_t idx = 0;
            for (const auto & v : value)
            {
                if (v < 0)
                {
                    throw std::range_error(
                        "'" + varname + "[" +
                        std::to_string(idx) + "]' must be non-negative"
                        ", but got " + epiassert_detail::to_str(v) +
                        fmt_location(caller)
                    );
                }
                ++idx;
            }
        }
        else
        {
            if (value < 0)
            {
                throw std::range_error(
                    "'" + varname + "' must be non-negative"
                    ", but got " + epiassert_detail::to_str(value) +
                    fmt_location(caller)
                );
            }
        }
    }

    // -----------------------------------------------------------------
    //  check_probability – value(s) in [0, 1]
    // -----------------------------------------------------------------

    /**
     * @brief Assert that @p value is a valid probability in `[0, 1]`.
     *
     * For container types every element is checked individually.
     *
     * @throws std::range_error if any value is outside `[0, 1]`.
     */
    template<typename T>
    static void check_probability(
        const T value,
        const std::string varname = "value",
        const std::string caller  = ""
    )
    {
        if constexpr (epiassert_detail::is_iterable<T>::value)
        {
            size_t idx = 0;
            for (const auto & v : value)
            {
                if (v < 0.0 || v > 1.0)
                {
                    throw std::range_error(
                        "'" + varname + "[" +
                        std::to_string(idx) +
                        "]' must be a probability in [0, 1]"
                        ", but got " + epiassert_detail::to_str(v) +
                        fmt_location(caller)
                    );
                }
                ++idx;
            }
        }
        else
        {
            if (value < 0.0 || value > 1.0)
            {
                throw std::range_error(
                    "'" + varname +
                    "' must be a probability in [0, 1]"
                    ", but got " + epiassert_detail::to_str(value) +
                    fmt_location(caller)
                );
            }
        }
    }

    // -----------------------------------------------------------------
    //  check_sum – container elements sum to target ± tolerance
    // -----------------------------------------------------------------

    /**
     * @brief Assert that the elements of @p values sum to @p target
     *        within ± @p tolerance.
     *
     * @throws std::invalid_argument if the sum is outside
     *         `[target − tolerance, target + tolerance]`.
     */
    template<typename T>
    static void check_sum(
        const T      values,
        double         target,
        const std::string varname   = "values",
        const std::string caller    = "",
        double         tolerance = 1e-8
    )
    {
        static_assert(
            epiassert_detail::is_iterable<T>::value,
            "check_sum requires an iterable type."
        );

        if (tolerance < 0.0)
        {
            throw std::invalid_argument(
                "check_sum: 'tolerance' must be non-negative, but got " +
                epiassert_detail::to_str(tolerance) +
                fmt_location(caller)
            );
        }

        double s = 0.0;
        for (const auto & v : values)
            s += static_cast<double>(v);

        if (std::abs(s - target) > tolerance)
        {
            throw std::invalid_argument(
                "'" + varname + "' elements must sum to " +
                epiassert_detail::to_str(target) +
                " (tolerance " + epiassert_detail::to_str(tolerance) +
                "), but got " + epiassert_detail::to_str(s) +
                fmt_location(caller)
            );
        }
    }

    // -----------------------------------------------------------------
    //  check_size – container has expected number of elements
    // -----------------------------------------------------------------

    /**
     * @brief Assert that @p values has exactly @p expected elements.
     *
     * @throws std::invalid_argument if the size differs.
     */
    template<typename T>
    static void check_size(
        const T values,
        size_t    expected,
        const std::string varname = "values",
        const std::string caller  = ""
    )
    {
        static_assert(
            epiassert_detail::is_iterable<T>::value,
            "check_size requires an iterable type."
        );

        const auto actual_size = static_cast<size_t>(
            std::distance(std::begin(values), std::end(values))
        );

        if (actual_size != expected)
        {
            throw std::invalid_argument(
                "'" + varname + "' must have " +
                std::to_string(expected) + " elements, but got " +
                std::to_string(actual_size) +
                fmt_location(caller)
            );
        }
    }

    // -----------------------------------------------------------------
    //  check – custom predicate validation
    // -----------------------------------------------------------------

    /**
     * @brief Assert that @p pred returns `true` for @p value.
     *
     * The predicate receives the value by `const` reference and should
     * return `true` when the value is valid.
     *
     * @throws std::invalid_argument with the caller-provided @p message
     *         when the predicate returns `false`.
     */
    template<typename T, typename Predicate>
    static void check(
        const T     value,
        Predicate     pred,
        const std::string message,
        const std::string caller = ""
    )
    {
        if (!pred(value))
        {
            throw std::invalid_argument(
                message + fmt_location(caller)
            );
        }
    }
};

/** @} */  // end of epiassert group

#endif
/*//////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

 End of -./include/epiworld/epiassert-bones.hpp-

////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////*/



/*//////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

 Start of -./include/epiworld/misc.hpp-

////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////*/


#ifndef EPIWORLD_MISC_HPP
#define EPIWORLD_MISC_HPP

class Model;

class Agent;

// Relevant for anything using vecHasher function ------------------------------
/**
 * @brief Vector hasher
 * @tparam T
 */
template <typename T>
struct vecHasher {
    std::size_t operator()(std::vector< T > const&  dat) const noexcept {

        std::hash< T > hasher;
        std::size_t hash = hasher(dat[0u]);

        // ^ makes bitwise XOR
        // 0x9e3779b9 is a 32 bit constant (comes from the golden ratio)
        // << is a shift operator, something like lhs * 2^(rhs)
        if (dat.size() > 1u)
            for (epiworld_fast_uint i = 1u; i < dat.size(); ++i)
                hash ^= hasher(dat[i]) + 0x9e3779b9 + (hash<<6) + (hash>>2);

        return hash;

    }
};

template<typename Ta = epiworld_double, typename Tb = epiworld_fast_uint>
using MapVec_type = std::unordered_map< std::vector< Ta >, Tb, vecHasher<Ta>>;

/**
 * @brief Check whether `a` is included in `b`
 *
 * @tparam Ta Type of `a`. Could be int, epiworld_double, etc.
 * @param a Scalar of class `Ta`.
 * @param b Vector `std::vector` of class `Ta`.
 * @return `true` if `a in b`, and `false` otherwise.
 */
template<typename Ta>
inline bool IN(const Ta & a, const std::vector< Ta > & b) noexcept
{
    for (const auto & i : b)
        if (a == i)
            return true;

    return false;
}

/**
 * @brief Read parameters from a yaml file
 *
 * @details
 * The file should have the following structure:
 * ```yaml
 * # Comment
 * [name of parameter 1]: [value in T]
 * [name of parameter 2]: [value in T]
 * ...
 * ```
 *
 * @tparam T Type of the parameter
 * @param fn Path to the file containing the parameters
 * @return std::map<std::string, T>
 */
template <typename T>
inline std::map< std::string, T > read_yaml(std::string fn)
{

    std::ifstream paramsfile(fn);

    if (!paramsfile)
        throw std::logic_error("The file " + fn + " was not found.");

    std::regex pattern("^([^:]+)\\s*[:]\\s*([-]?[0-9]+|[-]?[0-9]*\\.[0-9]+)?\\s*$");

    std::string line;
    std::smatch match;
    auto empty = std::sregex_iterator();

    // Making room
    std::map<std::string, T> parameters;

    while (std::getline(paramsfile, line))
    {

        // Is it a comment or an empty line?
        if (std::regex_match(line, std::regex("^([*].+|//.+|#.+|\\s*)$")))
            continue;

        // Finding the pattern, if it doesn't match, then error
        std::regex_match(line, match, pattern);

        if (match.empty())
            throw std::logic_error("Line has invalid format:\n" + line);

        // Capturing the number
        std::string anumber = match[2u].str() + match[3u].str();
        T tmp_num = static_cast<T>(
            std::strtod(anumber.c_str(), nullptr)
            );

        std::string pname = std::regex_replace(
            match[1u].str(),
            std::regex("^\\s+|\\s+$"),
            "");

        // Adding the parameter to the map
        parameters[pname] = tmp_num;

    }

    return parameters;

}



/**
 * @brief Cast a `Model` pointer to a specific model type
 */
#ifndef EPI_ASSUME
    #if defined(EPI_DEBUG)
        #include <cassert>
        #define EPI_ASSUME(cond) assert(cond)
    #elif defined(__clang__)
        #define EPI_ASSUME(cond) __builtin_assume(cond)
    #elif defined(_MSC_VER)
        #define EPI_ASSUME(cond) __assume(cond)
    #elif defined(__GNUC__)
        #define EPI_ASSUME(cond) do { if (!(cond)) __builtin_unreachable(); } while (0)
    #else
        #define EPI_ASSUME(cond) ((void)0)
    #endif
#endif

/**
 * @brief Index of the lowest set bit of a non-zero 64-bit word.
 *
 * @details Used to walk bitsets of agents (the queue's queued agents, the
 * carriers that push, the agents to update after a push) in ascending id
 * order: take the lowest set bit, visit that agent, clear the bit, repeat. It
 * costs one instruction per agent visited, and a whole word of 64 absent
 * agents is skipped at once.
 *
 * C++17 has no `std::countr_zero` (that is C++20). GCC and Clang get the
 * builtin; other compilers use a de Bruijn multiply, which needs no platform
 * header. `x` must not be zero.
 */
inline unsigned int epi_ctz64(uint64_t x)
{
#if defined(__GNUC__) || defined(__clang__)
    return static_cast< unsigned int >(__builtin_ctzll(x));
#else
    static const unsigned int table[64] = {
         0,  1, 48,  2, 57, 49, 28,  3, 61, 58, 50, 42, 38, 29, 17,  4,
        62, 55, 59, 36, 53, 51, 43, 22, 45, 39, 33, 30, 24, 18, 12,  5,
        63, 47, 56, 27, 60, 41, 37, 16, 54, 35, 52, 21, 44, 32, 23, 11,
        46, 26, 40, 15, 34, 20, 31, 10, 25, 14, 19,  9, 13,  8,  7,  6
    };
    return table[((x & (~x + 1u)) * 0x03f79d71b4cb0a89ull) >> 58];
#endif
}

template<class To>
inline To* model_cast(Model* m) {
#ifdef EPI_DEBUG
    auto* out = dynamic_cast<To*>(m);
    assert(out != nullptr);
    return out;
#else
    auto* out = static_cast<To*>(m);
    EPI_ASSUME(out != nullptr);
    return out;
#endif
}

#endif
/*//////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

 End of -./include/epiworld/misc.hpp-

////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////*/


/*//////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

 Start of -./include/epiworld/progress.hpp-

////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////*/


#ifndef EPIWORLD_PROGRESS_HPP
#define EPIWORLD_PROGRESS_HPP

#ifndef EPIWORLD_PROGRESS_BAR_WIDTH
#define EPIWORLD_PROGRESS_BAR_WIDTH 80
#endif

/**
 * @brief A simple progress bar
  */
class Progress {
private:
    int    width;     ///< Total width size (number of bars)
    int    n;         ///< Total number of iterations
    epiworld_double step_size; ///< Size of the step
    int last_loc;     ///< Last location of the bar
    int cur_loc;      ///< Last location of the bar
    int i;            ///< Current iteration step

public:
    Progress() {};
    Progress(int n_, int width_);
    ~Progress() {};

    void start();
    void next();
    

};

inline Progress::Progress(int n_, int width_) {


    if (n_ < 0)
        throw std::invalid_argument("n must be greater or equal than 0.");

    if (width_ <= 0)
        throw std::invalid_argument("width must be greater than 0");

    width     = std::max(7, width_ - 7);
    n         = n_;
    step_size = n == 0? width : static_cast<epiworld_double>(width)/
        static_cast<epiworld_double>(n);
    last_loc  = 0;
    i         = 0;

}

inline void Progress::start()
{

    #ifndef EPI_DEBUG
    for (int j = 0; j < (width); ++j)
    {
        printf_epiworld("_");
    }
    printf_epiworld("\n");
    #endif
}

inline void Progress::next() {

    if (i == 0)
        start();

    cur_loc = std::floor((++i) * step_size);

    #ifndef EPI_DEBUG
    for (int j = 0; j < (cur_loc - last_loc); ++j)
    { 
        printf_epiworld("|");
    }

    if (i == n)
    {
        printf_epiworld(" done.\n");
    }
    #endif

    last_loc = cur_loc;

}

#endif
/*//////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

 End of -./include/epiworld/progress.hpp-

////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////*/



/*//////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

 Start of -./include/epiworld/rng-utils.hpp-

////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////*/


#ifndef EPIWORLD_RNG_UTILS_HPP
#define EPIWORLD_RNG_UTILS_HPP

#include <cstdint>
#include <limits>
#include <random>
// (already included include/epiworld/config.hpp)

/**
 * @brief Fast 64-bit PRNG based on the xoshiro256** algorithm.
 *
 * xoshiro256** has 256-bit state, 64-bit output, and passes all known
 * statistical tests. It is significantly faster than std::mt19937.
 * State is seeded via SplitMix64 from a single 64-bit value.
 *
 * Reference: Blackman & Vigna, "Scrambled Linear Pseudorandom Number
 * Generators", ACM TOMS, 2021. https://prng.di.unimi.it/
 */
class epi_xoshiro256ss {
    uint64_t s[4];

    static uint64_t rotl(const uint64_t x, int k) noexcept {
        return (x << k) | (x >> (64 - k));
    }

    static uint64_t splitmix64(uint64_t& x) noexcept {
        x += 0x9e3779b97f4a7c15ULL;
        uint64_t z = x;
        z = (z ^ (z >> 30)) * 0xbf58476d1ce4e5b9ULL;
        z = (z ^ (z >> 27)) * 0x94d049bb133111ebULL;
        return z ^ (z >> 31);
    }

public:
    using result_type = uint64_t;

    explicit epi_xoshiro256ss(uint64_t seed_val = 0) noexcept {
        seed(seed_val);
    }

    static constexpr result_type min() noexcept { return 0; }
    static constexpr result_type max() noexcept {
        return std::numeric_limits<result_type>::max();
    }

    void seed(uint64_t seed_val) noexcept {
        s[0] = splitmix64(seed_val);
        s[1] = splitmix64(seed_val);
        s[2] = splitmix64(seed_val);
        s[3] = splitmix64(seed_val);
    }

    result_type operator()() noexcept {
        const uint64_t result = rotl(s[1] * 5, 7) * 9;
        const uint64_t t = s[1] << 17;
        s[2] ^= s[0];
        s[3] ^= s[1];
        s[1] ^= s[2];
        s[0] ^= s[3];
        s[2] ^= t;
        s[3] = rotl(s[3], 45);
        return result;
    }
};

/**
 * @brief Draw a uniform [0, 1) random number from an epi_xoshiro256ss engine.
 *
 * Uses the top N bits of the 64-bit output (where N =
 * std::numeric_limits<epiworld_double>::digits) to produce a value in [0, 1)
 * without any long-double arithmetic.
 *
 * @param engine An epi_xoshiro256ss engine.
 * @return epiworld_double in [0, 1).
 */
inline epiworld_double runif_epi(epi_xoshiro256ss & engine) {
    static_assert(
        std::numeric_limits<epiworld_double>::digits < 64,
        "epiworld_double must have fewer than 64 mantissa bits; "
        "the bit-extraction in runif_epi requires digits < 64 to avoid "
        "undefined behaviour in the shift and scale computation"
    );
    
    constexpr int bits  = std::numeric_limits<epiworld_double>::digits;
    constexpr int shift = 64 - bits;
    // scale = 2^{-bits}: the result is in [0, 1) by construction (no clamp needed)
    constexpr epiworld_double scale =
        epiworld_double(1) / epiworld_double(uint64_t(1) << bits);
    return static_cast<epiworld_double>(engine() >> shift) * scale;
}

/**
 * @brief Draw a uniform [0, 1) random number from a std::mt19937 engine.
 *
 * Kept for backward compatibility with code that still holds a raw mt19937.
 */
inline epiworld_double runif_mt19937(std::mt19937 & engine) {
    static constexpr epiworld_double factor =
        epiworld_double(1) / (epiworld_double(std::mt19937::max()) + epiworld_double(1));
    epiworld_double res = epiworld_double(engine()) * factor;
    if (res >= epiworld_double(1))
        res = std::nextafter(epiworld_double(1), epiworld_double(0));
    return res;
}

#endif
/*//////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

 End of -./include/epiworld/rng-utils.hpp-

////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////*/


/*//////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

 Start of -./include/epiworld/modeldiagram-meat.hpp-

////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////*/


#ifndef EPIWORLD_MODELDIAGRAM_MEAT_HPP
#define EPIWORLD_MODELDIAGRAM_MEAT_HPP

/*//////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

 Start of -include/epiworld/modeldiagram-bones.hpp-

////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////*/


#ifndef EPIWORLD_MODELDIAGRAM_HPP
#define EPIWORLD_MODELDIAGRAM_HPP

enum class DiagramType {
    Mermaid, DOT
};

/**
 * @brief Class to create model diagrams from transition data
 * 
 * This class reads transition data from epidemiological model simulations
 * and generates visual diagrams representing the state transitions.
 * It supports multiple diagram formats, including Mermaid and DOT.
 * 
 * @ingroup model_utilities
 */
class ModelDiagram {
private:

    std::map< std::pair< std::string, std::string >, int > data;
    std::vector< std::string > states;
    std::vector< epiworld_double > tprob;

    void draw_mermaid(std::string fn_output, bool self);
    void draw_dot(std::string fn_output, bool self);

    void draw(
        DiagramType diagram_type,
        std::string fn_output = "",
        bool self = false
    );


    int n_runs = 0; ///< The number of runs included in the diagram.

    /**
     * @brief Reads the transitions from a file.
     * @details
     * The file is assumed to come from the output of [Model::write_data()],
     * and should be in the following format:
     * ```
     * 0 "S" "I" 10
     * 1 "I" "R" 5
     * 2 "R" "S" 3
     * ...
     * ```
     * The first column is the step, the second column is the state from which
     * the transition is made, the third column is the state to which the
     * transition is made, and the fourth column is the number of transitions.
     * 
     * @param fn_transition The name of the file to read.
     * @return void
     * @throws std::runtime_error if the file cannot be opened.
     */
    void read_transitions(
        const std::string & fn_transition
    );

    /**
     * @brief In the case of multiple files (like from run_multiple)
     * @param fns_transition The names of the files to read.
     * @details
     * It will read the transitions from multiple files and concatenate them.
     * into the same object.
     */
    void read_transitions(
        const std::vector< std::string > & fns_transition
    );
    
    /**
     * @brief Computes the transition probability matrix.
     * @param normalize Whether to compute only the counts,
     * otherwise the probabilities will be computed (row-stochastic).
     * @return void
     */
    void transition_probability(bool normalize = true);

    void clear();

public:

    ModelDiagram() {};

    void draw_from_data(
        DiagramType diagram_type,
        const std::vector< std::string > & states,
        const std::vector< epiworld_double > & tprob,
        const std::string & fn_output = "",
        bool self = false
    );

    void draw_from_file(
        DiagramType diagram_type,
        const std::string & fn_transition,
        const std::string & fn_output = "",
        bool self = false
    );

    void draw_from_files(
        DiagramType diagram_type,
        const std::vector< std::string > & fns_transition,
        const std::string & fn_output = "",
        bool self = false
    );

};

#endif
/*//////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

 End of -include/epiworld/modeldiagram-bones.hpp-

////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////*/



inline void ModelDiagram::read_transitions(
    const std::string & fn_transition
) {

    // Checking if the file exists
    std::ifstream file(fn_transition);

    if (!file.is_open())
        throw std::runtime_error(
            "Could not open the file '" +
            fn_transition + "' for reading."
        );

    // Reading the data
    std::string line;
    int i = 0;
    while (std::getline(file, line))
    {

        // Skipping the first line
        if (i++ == 0)
            continue;

        std::istringstream iss(line);
        #ifdef EPI_DEBUG
        int t;
        iss >> t;
        #endif

        int i_;
        std::string from_, to_;
        int counts_;

        iss >> i_;

        // Read the quoted strings
        iss >> std::quoted(from_) >> std::quoted(to_);

        // Read the integer
        iss >> counts_;

        if (counts_ > 0)
        {
            auto idx = std::make_pair(from_, to_);
            if (data.find(idx) == data.end())
                data[idx] = counts_;
            else
                data[idx] += counts_;
        }

    }

    // Incrementing the number of runs
    this->n_runs++;

}

inline void ModelDiagram::read_transitions(
    const std::vector< std::string > & fns_transition
)
{

    for (const auto & fn: fns_transition)
        this->read_transitions(fn);

    return;

}

inline void ModelDiagram::transition_probability(
    bool normalize
)
{

    // Generating the map of states
    std::set< std::string > states_set;

    for (const auto & d: data)
    {
        states_set.insert(d.first.first);
        states_set.insert(d.first.second);
    }

    // Generating the transition matrix
    states = std::vector< std::string >(states_set.begin(), states_set.end());
    size_t n_states = states.size();
    tprob.assign(n_states * n_states, 0.0);

    std::vector< epiworld_double > rowsum(n_states, 0.0);
    for (size_t i = 0; i < n_states; ++i)
    {

        for (size_t j = 0; j < n_states; ++j)
        {

            auto key = std::make_pair(states[i], states[j]);
            if (data.find(key) != data.end())
                tprob[i + j * n_states] = static_cast<epiworld_double>(
                    data[key]
                );

            if (normalize)
                rowsum[i] += tprob[i + j * n_states];

        }

        if (normalize)
        {
            for (size_t j = 0; j < n_states; ++j)
                tprob[i + j * n_states] /= rowsum[i];
        }

    }

    return;


}

inline void ModelDiagram::clear()
{
    data.clear();
    states.clear();
    tprob.clear();
    n_runs = 0;
    return;
}

inline void ModelDiagram::draw_mermaid(
    std::string fn_output,
    bool self
) {
     // Getting a sorting vector of indices from the states
    // string vector
    std::vector< size_t > idx(states.size());
    std::iota(idx.begin(), idx.end(), 0u);

    std::sort(
        idx.begin(),
        idx.end(),
        [&states = this->states](size_t i, size_t j) {
            return states[i] < states[j];
        }
    );

    std::vector< std::string > states_ids;
    for (size_t i = 0u; i < states.size(); ++i)
        states_ids.push_back("s" + std::to_string(i));

    std::string graph = "flowchart TB\n";

    // Declaring the states
    for (size_t i = 0u; i < states.size(); ++i)
    {
        graph += "\t" + states_ids[i] + "[" + states[idx[i]] + "]\n";
    }

    // Adding the transitions
    size_t n_states = states.size();
    for (size_t i = 0u; i < states.size(); ++i)
    {
        for (size_t j = 0u; j < states.size(); ++j)
        {
            if (!self && i == j)
                continue;

            if (tprob[idx[i] + idx[j] * n_states] > 0.0)
            {
                graph += "\t" + states_ids[i] + " -->|" +
                    std::to_string(tprob[idx[i] + idx[j] * n_states]) +
                    "| " + states_ids[j] + "\n";
            }
        }
    }

    if (fn_output != "")
    {
        std::ofstream file(fn_output);

        if (!file.is_open())
            throw std::runtime_error(
                "Could not open the file " +
                fn_output +
                " for writing."
            );

        file << graph;
        file.close();

    } else {
        printf_epiworld("%s\n", graph.c_str());
    }

    return;
}

inline void ModelDiagram::draw_dot(std::string fn_output, bool self) {
    std::vector<size_t> idx(states.size());
    std::iota(idx.begin(), idx.end(), 0u);

    std::sort(
        idx.begin(),
        idx.end(),
        [&states = this->states](size_t i, size_t j) {
            return states[i] < states[j];
        }
    );

    std::vector<std::string> states_ids;
    for (size_t i = 0u; i < states.size(); ++i)
        states_ids.push_back("s" + std::to_string(i));

    std::string graph = "digraph G {\n\trankdir=LR;\n";

    // Declaring the states
    for (size_t i = 0u; i < states.size(); ++i)
    {
        graph += "\t" + states_ids[i] + " [label=\"" + states[idx[i]] + "\"];\n";
    }

    // Adding the transitions
    size_t n_states = states.size();
    for (size_t i = 0u; i < states.size(); ++i)
    {
        for (size_t j = 0u; j < states.size(); ++j)
        {
            if (!self && i == j)
                continue;

            if (tprob[idx[i] + idx[j] * n_states] > 0.0)
            {
                graph += "\t" + states_ids[i] + " -> " + states_ids[j] +
                         " [label=\"" +
                         std::to_string(tprob[idx[i] + idx[j] * n_states]) +
                         "\"];\n";
            }
        }
    }

    graph += "}\n";

    if (fn_output != "")
    {
        std::ofstream file(fn_output);

        if (!file.is_open())
            throw std::runtime_error(
                "Could not open the file " +
                fn_output +
                " for writing."
            );

        file << graph;
        file.close();

    } else {
        printf_epiworld("%s\n", graph.c_str());
    }

    return;
}

inline void ModelDiagram::draw(
    DiagramType diagram_type,
    std::string fn_output,
    bool self
)
{
    switch (diagram_type) {
    case DiagramType::Mermaid:
        return draw_mermaid(fn_output, self);
    case DiagramType::DOT:
        return draw_dot(fn_output, self);
    default:
  		throw std::logic_error("Unknown how to dispatch DiagramType.");
    }
}

inline void ModelDiagram::draw_from_file(
    DiagramType diagram_type,
    const std::string & fn_transition,
    const std::string & fn_output,
    bool self
) {

    this->clear();

    // Loading the transition file
    this->read_transitions(fn_transition);

    // Computing the transition probability
    this->transition_probability();

    // Actually drawing the diagram
    this->draw(diagram_type, fn_output, self);

    return;

}

inline void ModelDiagram::draw_from_files(
    DiagramType diagram_type,
    const std::vector< std::string > & fns_transition,
    const std::string & fn_output,
    bool self
) {

    this->clear();

    // Loading the transition files
    this->read_transitions(fns_transition);

    // Computing the transition probability
    this->transition_probability();

    // Actually drawing the diagram
    this->draw(diagram_type, fn_output, self);

    return;
}

inline void ModelDiagram::draw_from_data(
    DiagramType diagram_type,
    const std::vector< std::string > & states,
    const std::vector< epiworld_double > & tprob,
    const std::string & fn_output,
    bool self
) {

    this->clear();

    this->states = states;
    this->tprob = tprob;

    this->draw(diagram_type, fn_output, self);

    return;

}

#endif
/*//////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

 End of -./include/epiworld/modeldiagram-meat.hpp-

////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////*/



/*//////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

 Start of -./include/epiworld/math/distributions.hpp-

////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////*/


#ifndef EPIWORLD_MATH_DISTRIBUTIONS_HPP
#define EPIWORLD_MATH_DISTRIBUTIONS_HPP

// Implementing the factorial function
/**
 * @brief Compute the log of the factorial
 * 
 * @param n Number
 * 
 * @return The log of the factorial
 */
inline double log_factorial(int n)
{
    if (n == 0)
        return 0.0;
    return std::log(static_cast<double>(n)) + log_factorial(n-1);
}

/**
 * @brief Compute the Poisson probability
 * 
 * @param k Number of events
 * @param lambda Rate
 * @param max_n Maximum number of events
 * @param as_log Return the log of the probability
 * 
 * @return The Poisson probability
 */
inline double dpois(
    int k,
    double lambda,
    int max_n = 100,
    bool as_log = false
    )
{

    if (max_n < k)
        throw std::runtime_error("max_n must be greater than k");

    double res = k * std::log(lambda) - lambda - log_factorial(
        std::min(k, max_n)
        );
    
    return as_log ? res : std::exp(res);
}

/**
 * @brief Compute the probability of the generation interval
 * 
 * @details
 * If `p_0_approx` is negative, it will be computed using the Poisson
 * distribution. If `normalizing` is negative, it will be computed on the fly
 * 
 * @param g Generation interval
 * @param S Population size
 * @param p_c Probability of contact
 * @param p_i Probability of infection
 * @param p_r Probability of recovery
 * @param p_0_approx Approximation of the probability of not being infected
 * @param normalizing Normalizing constant
 * @param max_contacts Maximum number of contacts
 * @param max_days Maximum number of days
 * 
 * @return The probability of the generation interval
 * 
 */
inline double dgenint(
    int g,
    double S,
    double p_c,
    double p_i,
    double p_r,
    double & p_0_approx,
    double & normalizing,
    int max_contacts = 200,
    int max_days = 200
    ) {

    if ((g < 1) || (g > max_days))
        return 0.0;

    if (p_0_approx < 0.0)
    {

        p_0_approx = 0.0;
        for (int i = 0; i < max_contacts; ++i)
        {

            p_0_approx += std::exp(
                dpois(i, S * p_c, max_contacts, true) +
                std::log(1.0 - p_i) * static_cast<double>(i)
                );

        }
    }

    double g_dbl = static_cast<double>(g);

    if (normalizing < 0.0)
    {

        normalizing = 1.0;
        double log1_p_r = std::log(1.0 - p_r);
        double log_p_r = std::log(p_r);
        double log_p_0_approx = std::log(p_0_approx);
        for (int i = 1; i <= max_days; ++i)
        {

            double i_dbl = static_cast<double>(i);

            normalizing -= std::exp(
                log1_p_r * (i_dbl - 1.0) +
                log_p_r +
                log_p_0_approx * (i_dbl - 1.0)
                );
        }

    }


    return std::exp(
        std::log(1 - p_r) * (g_dbl)+
        std::log(p_0_approx) * (g_dbl - 1.0) +
        std::log(1.0 - p_0_approx) -
        std::log(normalizing)
        );

}

// Mean of the generation interval
/**
 * @brief Compute the mean of the generation interval
 * @param S Population size.
 * @param p_c Probability of contact.
 * @param p_i Probability of infection.
 * @param p_r Probability of recovery.
 * @param max_days Maximum number of days.
 * @param max_n Maximum number of contacts.
 * 
 * @return The mean of the generation interval
 */
inline double gen_int_mean(
    double S,
    double p_c,
    double p_i,
    double p_r,
    int max_days = 200,
    int max_n = 200
    ) {

    double mean = 0.0;
    double p_0_approx = -1.0;
    double normalizing = -1.0;
    for (int i = 1; i < max_days; ++i)
    {
        mean += 
            static_cast<double>(i) *
            dgenint(
                i, S, p_c, p_i, p_r, p_0_approx, normalizing, max_n, max_days
                );

    }

    return mean;

}

#endif
/*//////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

 End of -./include/epiworld/math/distributions.hpp-

////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////*/



/*//////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

 Start of -./include/epiworld/math/lfmcmc.hpp-

////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////*/


#ifndef EPIWORLD_LFMCMC_HPP
#define EPIWORLD_LFMCMC_HPP

/*//////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

 Start of -include/epiworld/math/lfmcmc/lfmcmc-bones.hpp-

////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////*/


#ifndef EPIWORLD_LFMCMC_BONES_HPP
#define EPIWORLD_LFMCMC_BONES_HPP

#ifndef epiworld_double
    #define epiworld_double float
#endif


template<typename TData>
class LFMCMC;

template<typename TData>
using LFMCMCSimFun = std::function<TData(const std::vector< epiworld_double >&,LFMCMC<TData>*)>;

template<typename TData>
using LFMCMCSummaryFun = std::function<void(std::vector< epiworld_double >&,const TData&,LFMCMC<TData>*)>;

template<typename TData>
using LFMCMCProposalFun = std::function<void(std::vector< epiworld_double >&,const std::vector< epiworld_double >&,LFMCMC<TData>*)>;

template<typename TData>
using LFMCMCKernelFun = std::function<epiworld_double(const std::vector< epiworld_double >&,const std::vector< epiworld_double >&,epiworld_double,LFMCMC<TData>*)>;

/**
 * @brief Proposal function
 * @param new_params Vector where to save the new parameters.
 * @param old_params Vector of reference parameters.
 * @param m LFMCMC model.
 * @tparam TData 
 */
template<typename TData>
inline void proposal_fun_normal(
    std::vector< epiworld_double >& new_params,
    const std::vector< epiworld_double >& old_params,
    LFMCMC<TData>* m
);

/**
 * @brief Factory for a reflective normal kernel
 * 
 * @details Reflective kernel corrects proposals by forcing them to be
 * within prespecified boundaries. 
 * 
 * @tparam TData 
 * @param scale Scale of the normal kernel
 * @param lb Lower bound (applies the same to all parameters)
 * @param ub Upper bound (applies the same to all parameters)
 * @return LFMCMCProposalFun<TData> 
 */
template<typename TData>
inline LFMCMCProposalFun<TData> make_proposal_norm_reflective(
    epiworld_double scale,
    epiworld_double lb = std::numeric_limits<epiworld_double>::min(),
    epiworld_double ub = std::numeric_limits<epiworld_double>::max()
);

/**
 * @brief Uniform proposal kernel
 * 
 * Proposals are made within a radious 1 of the current
 * state of the parameters.
 * 
 * @param new_params Where to write the new parameters
 * @param old_params Reference parameters
 * @tparam TData 
 * @param m LFMCMC model.
 */
template<typename TData>
inline void proposal_fun_unif(
    std::vector< epiworld_double >& new_params,
    const std::vector< epiworld_double >& old_params,
    LFMCMC<TData>* m
);

/**
 * @brief Uses the uniform kernel with euclidean distance
 * 
 * @param simulated_stats Vector of statistics based on 
 * simulated data
 * @param observed_stats Vector of observed statistics
 * @param epsilon Epsilon parameter
 * @param m LFMCMC model
 * @return epiworld_double 
 */
template<typename TData>
inline epiworld_double kernel_fun_uniform(
    const std::vector< epiworld_double >& simulated_stats,
    const std::vector< epiworld_double >& observed_stats,
    epiworld_double epsilon,
    LFMCMC<TData>* m
);

/**
 * @brief Gaussian kernel
 * 
 * @tparam TData 
 * @param simulated_stats Vector of statistics based on 
 * simulated data
 * @param observed_stats Vector of observed statistics
 * @param epsilon Epsilon parameter
 * @param m LFMCMC model
 * @return epiworld_double 
 */
template<typename TData>
inline epiworld_double kernel_fun_gaussian(
    const std::vector< epiworld_double >& simulated_stats,
    const std::vector< epiworld_double >& observed_stats,
    epiworld_double epsilon,
    LFMCMC<TData>* m
);

/**
 * @brief Likelihood-Free Markov Chain Monte Carlo
 * 
 * @tparam TData Type of data that is generated
 */
template<typename TData>
class LFMCMC {
private:

    // Random number sampling
    std::shared_ptr< epi_xoshiro256ss > m_engine = nullptr;
    
    std::shared_ptr< std::normal_distribution<> > rnormd =
        std::make_shared< std::normal_distribution<> >(0.0);

    std::shared_ptr< std::gamma_distribution<> > rgammad = 
        std::make_shared< std::gamma_distribution<> >();

    // Process data
    TData m_observed_data;
    std::vector< TData > * m_simulated_data = nullptr;

    // Information about the size of the process
    size_t m_n_samples;
    size_t m_n_stats;
    size_t m_n_params;

    epiworld_double m_epsilon;

    std::vector< epiworld_double > m_initial_params;             ///< Initial parameters
    std::vector< epiworld_double > m_current_proposed_params;    ///< Proposed parameters for the next sample
    std::vector< epiworld_double > m_current_accepted_params;    ///< Most recently accepted parameters (current state of MCMC)
    std::vector< epiworld_double > m_current_proposed_stats;     ///< Statistics from simulation run with proposed parameters
    std::vector< epiworld_double > m_current_accepted_stats;     ///< Statistics from simulation run with most recently accepted params

    std::vector< epiworld_double > m_observed_stats;             ///< Observed statistics

    std::vector< epiworld_double > m_all_sample_params;          ///< Parameter samples
    std::vector< epiworld_double > m_all_sample_stats;           ///< Statistic samples
    std::vector< bool >            m_all_sample_acceptance;      ///< Indicator if sample was accepted
    std::vector< epiworld_double > m_all_sample_drawn_prob;      ///< Drawn probabilities (runif()) for each sample
    std::vector< epiworld_double > m_all_sample_kernel_scores;   ///< Kernel scores for each sample

    std::vector< epiworld_double > m_all_accepted_params;        ///< Posterior distribution of parameters from accepted samples
    std::vector< epiworld_double > m_all_accepted_stats;         ///< Posterior distribution of statistics from accepted samples
    std::vector< epiworld_double > m_all_accepted_kernel_scores; ///< Kernel scores for each accepted sample

    // Functions
    LFMCMCSimFun<TData> m_simulation_fun;
    LFMCMCSummaryFun<TData> m_summary_fun;
    LFMCMCProposalFun<TData> m_proposal_fun = proposal_fun_normal<TData>;
    LFMCMCKernelFun<TData> m_kernel_fun     = kernel_fun_uniform<TData>;

    // Misc
    std::vector< std::string > m_param_names;
    std::vector< std::string > m_stat_names;

    std::chrono::time_point<std::chrono::steady_clock> m_start_time;
    std::chrono::time_point<std::chrono::steady_clock> m_end_time;

    // Timing
    // std::chrono::milliseconds
    std::chrono::duration<epiworld_double,std::micro> m_elapsed_time = 
        std::chrono::duration<epiworld_double,std::micro>::zero();

    inline void get_elapsed_time(
        std::string unit,
        epiworld_double * last_elapsed,
        std::string * unit_abbr,
        bool print
    ) const;

    void chrono_start();
    void chrono_end();

    // Progress
    bool verbose = true;
    Progress progress_bar;
    
public:

    void run(
        std::vector< epiworld_double > params_init_,
        size_t n_samples_,
        epiworld_double epsilon_,
        int seed = -1
        );

    LFMCMC() {};
    LFMCMC(const TData & observed_data_) : m_observed_data(observed_data_) {};
    ~LFMCMC() {};

    // Setting LFMCMC variables
    void set_observed_data(const TData & observed_data_) {m_observed_data = observed_data_;};
    
    void set_proposal_fun(LFMCMCProposalFun<TData> fun);
    void set_simulation_fun(LFMCMCSimFun<TData> fun);
    void set_summary_fun(LFMCMCSummaryFun<TData> fun);
    void set_kernel_fun(LFMCMCKernelFun<TData> fun);

    void set_params_names(std::vector< std::string > names);
    void set_stats_names(std::vector< std::string > names);
    
    /**
     * @name Random number generation
     * 
     * @param eng 
     */
    ///@{
    void set_rand_engine(std::shared_ptr< epi_xoshiro256ss > & eng);
    std::shared_ptr< epi_xoshiro256ss > & get_rand_endgine();
    void seed(epiworld_fast_uint s);
    void set_rand_gamma(epiworld_double alpha, epiworld_double beta);
    epiworld_double runif();
    epiworld_double rnorm();
    epiworld_double rgamma();
    epiworld_double runif(epiworld_double lb, epiworld_double ub);
    epiworld_double rnorm(epiworld_double mean, epiworld_double sd);
    epiworld_double rgamma(epiworld_double alpha, epiworld_double beta);
    ///@}

    // Accessing parameters of the function
    size_t get_n_samples() const {return m_n_samples;};
    size_t get_n_stats() const {return m_n_stats;};
    size_t get_n_params() const {return m_n_params;};
    epiworld_double get_epsilon() const {return m_epsilon;};

    const std::vector< epiworld_double > & get_initial_params() const {return m_initial_params;};
    const std::vector< epiworld_double > & get_current_proposed_params() const {return m_current_proposed_params;};
    const std::vector< epiworld_double > & get_current_accepted_params() const {return m_current_accepted_params;};
    const std::vector< epiworld_double > & get_current_proposed_stats() const {return m_current_proposed_stats;};
    const std::vector< epiworld_double > & get_current_accepted_stats() const {return m_current_accepted_stats;};

    const std::vector< epiworld_double > & get_observed_stats() const {return m_observed_stats;};

    const std::vector< epiworld_double > & get_all_sample_params() const {return m_all_sample_params;};
    const std::vector< epiworld_double > & get_all_sample_stats() const {return m_all_sample_stats;};
    const std::vector< bool >            & get_all_sample_acceptance() const {return m_all_sample_acceptance;};
    const std::vector< epiworld_double > & get_all_sample_drawn_prob() const {return m_all_sample_drawn_prob;};
    const std::vector< epiworld_double > & get_all_sample_kernel_scores() const {return m_all_sample_kernel_scores;};

    const std::vector< epiworld_double > & get_all_accepted_params() const {return m_all_accepted_params;};
    const std::vector< epiworld_double > & get_all_accepted_stats() const {return m_all_accepted_stats;};
    const std::vector< epiworld_double > & get_all_accepted_kernel_scores() const {return m_all_accepted_kernel_scores;};
    
    std::vector< TData > * get_simulated_data() const {return m_simulated_data;};

    std::vector< epiworld_double > get_mean_params();
    std::vector< epiworld_double > get_mean_stats();

    // Printing
    LFMCMC<TData> & verbose_off();
    LFMCMC<TData> & verbose_on();
    void print(size_t burnin = 0u) const;

};

#endif
/*//////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

 End of -include/epiworld/math/lfmcmc/lfmcmc-bones.hpp-

////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////*/


/*//////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

 Start of -include/epiworld/math/lfmcmc/lfmcmc-meat.hpp-

////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////*/


#ifndef EPIWORLD_LFMCMC_MEAT_HPP
#define EPIWORLD_LFMCMC_MEAT_HPP

// (already included include/epiworld/math/lfmcmc/lfmcmc-bones.hpp)

/**
 * @brief Proposal function
 * @param new_params Vector where to save the new parameters.
 * @param old_params Vector of reference parameters.
 * @param m LFMCMC model.
 * @tparam TData 
 */
template<typename TData>
inline void proposal_fun_normal(
    std::vector< epiworld_double >& new_params,
    const std::vector< epiworld_double >& old_params,
    LFMCMC<TData>* m
) {

    for (size_t p = 0u; p < m->get_n_params(); ++p)
        new_params[p] = old_params[p] + m->rnorm();

    return;
}

/**
 * @brief Factory for a reflective normal kernel
 * 
 * @details Reflective kernel corrects proposals by forcing them to be
 * within prespecified boundaries. 
 * 
 * @tparam TData 
 * @param scale Scale of the normal kernel
 * @param lb Lower bound (applies the same to all parameters)
 * @param ub Upper bound (applies the same to all parameters)
 * @return LFMCMCProposalFun<TData> 
 */
template<typename TData>
inline LFMCMCProposalFun<TData> make_proposal_norm_reflective(
    epiworld_double scale,
    epiworld_double lb,
    epiworld_double ub
) {

    LFMCMCProposalFun<TData> fun =
        [scale,lb,ub](
            std::vector< epiworld_double >& new_params,
            const std::vector< epiworld_double >& old_params,
            LFMCMC<TData>* m
        ) {

        // Making the proposal
        for (size_t p = 0u; p < m->get_n_params(); ++p)
            new_params[p] = old_params[p] + m->rnorm() * scale;

        // Checking boundaries
        epiworld_double d = ub - lb;
        int odd;
        epiworld_double d_above, d_below;
        for (auto & p : new_params)
        {

            // Correcting if parameter goes above the upper bound
            if (p > ub)
            {
                d_above = p - ub;
                odd     = static_cast<int>(std::floor(d_above / d)) % 2;
                d_above = d_above - std::floor(d_above / d) * d;

                p = (lb + d_above) * odd +
                    (ub - d_above) * (1 - odd);

            // Correcting if parameter goes below upper bound
            } else if (p < lb)
            {
                d_below = lb - p;
                int odd = static_cast<int>(std::floor(d_below / d)) % 2;
                d_below = d_below - std::floor(d_below / d) * d;

                p = (ub - d_below) * odd +
                    (lb + d_below) * (1 - odd);
            }

        }

        #ifdef EPI_DEBUG
        for (auto & p : new_params)
            if (p < lb || p > ub)
                throw std::range_error("The parameter is out of bounds.");
        #endif


        return;

    };

    return fun;
}

/**
 * @brief Uniform proposal kernel
 * 
 * Proposals are made within a radious 1 of the current
 * state of the parameters.
 * 
 * @param new_params Where to write the new parameters
 * @param old_params Reference parameters
 * @tparam TData 
 * @param m LFMCMC model.
 */
template<typename TData>
inline void proposal_fun_unif(
    std::vector< epiworld_double >& new_params,
    const std::vector< epiworld_double >& old_params,
    LFMCMC<TData>* m
) {

    for (size_t p = 0u; p < m->get_n_params(); ++p)
        new_params[p] = (old_params[p] + m->runif(-1.0, 1.0));

    return;
}

/**
 * @brief Uses the uniform kernel with euclidean distance
 * 
 * @param simulated_stats Vector of statistics based on 
 * simulated data
 * @param observed_stats Vector of observed statistics
 * @param epsilon Epsilon parameter
 * @param m LFMCMC model
 * @return epiworld_double 
 */
template<typename TData>
inline epiworld_double kernel_fun_uniform(
    const std::vector< epiworld_double >& simulated_stats,
    const std::vector< epiworld_double >& observed_stats,
    epiworld_double epsilon,
    LFMCMC<TData>* m
) {

    epiworld_double ans = 0.0;
    for (size_t p = 0u; p < m->get_n_params(); ++p)
        ans += std::pow(observed_stats[p] - simulated_stats[p], 2.0);

    return std::sqrt(ans) < epsilon ? 1.0 : 0.0;

}

#define sqrt2pi() 2.5066282746310002416

/**
 * @brief Gaussian kernel
 * 
 * @tparam TData 
 * @param simulated_stats Vector of statistics based on 
 * simulated data
 * @param observed_stats Vector of observed statistics
 * @param epsilon Epsilon parameter
 * @param m LFMCMC model
 * @return epiworld_double 
 */
template<typename TData>
inline epiworld_double kernel_fun_gaussian(
    const std::vector< epiworld_double >& simulated_stats,
    const std::vector< epiworld_double >& observed_stats,
    epiworld_double epsilon,
    LFMCMC<TData>* m
) {

    epiworld_double ans = 0.0;
    for (size_t p = 0u; p < m->get_n_params(); ++p)
        ans += std::pow(observed_stats[p] - simulated_stats[p], 2.0);

    return std::exp(
        -.5 * (ans/std::pow(1 + std::pow(epsilon, 2.0)/3.0, 2.0))
        ) / sqrt2pi() ;

}


template<typename TData>
inline void LFMCMC<TData>::set_proposal_fun(LFMCMCProposalFun<TData> fun)
{
    m_proposal_fun = fun;
}

template<typename TData>
inline void LFMCMC<TData>::set_simulation_fun(LFMCMCSimFun<TData> fun)
{
    m_simulation_fun = fun;
}

template<typename TData>
inline void LFMCMC<TData>::set_summary_fun(LFMCMCSummaryFun<TData> fun)
{
    m_summary_fun = fun;
}

template<typename TData>
inline void LFMCMC<TData>::set_kernel_fun(LFMCMCKernelFun<TData> fun)
{
    m_kernel_fun = fun;
}


template<typename TData>
inline void LFMCMC<TData>::run(
    std::vector< epiworld_double > params_init_,
    size_t n_samples_,
    epiworld_double epsilon_,
    int seed
    )
{

    // Starting timing
    chrono_start();

    // Setting the baseline parameters of the model
    m_n_samples    = n_samples_;
    m_epsilon      = epsilon_;
    m_initial_params  = params_init_;
    m_n_params = params_init_.size();

    if (seed >= 0)
        this->seed(seed);

    m_current_proposed_params.resize(m_n_params);
    m_current_accepted_params.resize(m_n_params);

    if (m_simulated_data != nullptr)
        m_simulated_data->resize(m_n_samples);

    m_current_accepted_params = m_initial_params;
    m_current_proposed_params  = m_initial_params;

    // Computing the baseline sufficient statistics
    m_summary_fun(m_observed_stats, m_observed_data, this);
    m_n_stats = m_observed_stats.size();

    // Reserving size
    m_current_proposed_stats.resize(m_n_stats);
    m_current_accepted_stats.resize(m_n_stats);
    m_all_sample_drawn_prob.resize(m_n_samples);
    m_all_sample_acceptance.resize(m_n_samples, false);
    m_all_sample_params.resize(m_n_samples * m_n_params);
    m_all_sample_stats.resize(m_n_samples * m_n_stats);
    m_all_sample_kernel_scores.resize(m_n_samples);

    m_all_accepted_params.resize(m_n_samples * m_n_params);
    m_all_accepted_stats.resize(m_n_samples * m_n_stats);
    m_all_accepted_kernel_scores.resize(m_n_samples);

    TData data_i = m_simulation_fun(m_initial_params, this);

    m_summary_fun(m_current_proposed_stats, data_i, this);
    m_all_accepted_kernel_scores[0u] = m_kernel_fun(
        m_current_proposed_stats, m_observed_stats, m_epsilon, this
        );

    // Recording statistics
    for (size_t i = 0u; i < m_n_stats; ++i)
        m_all_sample_stats[i] = m_current_proposed_stats[i];
    
    m_current_accepted_stats = m_current_proposed_stats;

    for (size_t k = 0u; k < m_n_params; ++k)
        m_all_accepted_params[k] = m_initial_params[k];
    
    for (size_t k = 0u; k < m_n_params; ++k)
        m_all_sample_params[k] = m_initial_params[k];
   
    // Init progress bar
    progress_bar = Progress(m_n_samples, 80);

    // Run LFMCMC
    for (size_t i = 1u; i < m_n_samples; ++i)
    {
        // Step 1: Generate a proposal and store it in m_current_proposed_params
        m_proposal_fun(m_current_proposed_params, m_current_accepted_params, this);

        // Step 2: Using m_current_proposed_params, simulate data
        TData data_i = m_simulation_fun(m_current_proposed_params, this);

        // Are we storing the data?
        if (m_simulated_data != nullptr)
            m_simulated_data->operator[](i) = data_i;

        // Step 3: Generate the summary statistics of the data
        m_summary_fun(m_current_proposed_stats, data_i, this);

        // Step 4: Compute the hastings ratio using the kernel function
        epiworld_double hr = m_kernel_fun(
            m_current_proposed_stats, m_observed_stats, m_epsilon, this
            );

        m_all_sample_kernel_scores[i] = hr;

        // Storing data
        for (size_t k = 0u; k < m_n_params; ++k)
            m_all_sample_params[i * m_n_params + k] = m_current_proposed_params[k];

        for (size_t k = 0u; k < m_n_stats; ++k)
            m_all_sample_stats[i * m_n_stats + k] = m_current_proposed_stats[k];
        
        // Running Hastings ratio
        epiworld_double r = runif();
        m_all_sample_drawn_prob[i] = r;

        // Step 5: Update if likely
        if (r < std::min(static_cast<epiworld_double>(1.0), hr / m_all_accepted_kernel_scores[i - 1u]))
        {
            m_all_accepted_kernel_scores[i] = hr;
            m_all_sample_acceptance[i]     = true;
            
            for (size_t k = 0u; k < m_n_stats; ++k)
                m_all_accepted_stats[i * m_n_stats + k] =
                    m_current_proposed_stats[k];

            m_current_accepted_params = m_current_proposed_params;
            m_current_accepted_stats = m_current_proposed_stats;
        } else
        {

            for (size_t k = 0u; k < m_n_stats; ++k)
                m_all_accepted_stats[i * m_n_stats + k] =
                    m_all_accepted_stats[(i - 1) * m_n_stats + k];

            m_all_accepted_kernel_scores[i] = m_all_accepted_kernel_scores[i - 1u];
        }
            

        for (size_t k = 0u; k < m_n_params; ++k)
            m_all_accepted_params[i * m_n_params + k] = m_current_accepted_params[k];

        if (verbose)
           progress_bar.next();

    }

    // End timing
    chrono_end();

}


template<typename TData>
inline epiworld_double LFMCMC<TData>::runif()
{
    return runif_epi(*m_engine);
}

template<typename TData>
inline epiworld_double LFMCMC<TData>::runif(
    epiworld_double lb,
    epiworld_double ub
)
{
    return runif_epi(*m_engine) * (ub - lb) + lb;
}

template<typename TData>
inline epiworld_double LFMCMC<TData>::rnorm()
{
    return rnormd->operator()(*m_engine);
}

template<typename TData>
inline epiworld_double LFMCMC<TData>::rnorm(
    epiworld_double mean,
    epiworld_double sd
    )
{
    return (rnormd->operator()(*m_engine)) * sd + mean;
}

template<typename TData>
inline epiworld_double LFMCMC<TData>::rgamma()
{
    return rgammad->operator()(*m_engine);
}

template<typename TData>
inline epiworld_double LFMCMC<TData>::rgamma(
    epiworld_double alpha,
    epiworld_double beta
    )
{

    auto old_param = rgammad->param();

    rgammad->param(std::gamma_distribution<>::param_type(alpha, beta));

    epiworld_double ans = rgammad->operator()(*m_engine);

    rgammad->param(old_param);

    return ans;

}

template<typename TData>
inline void LFMCMC<TData>::seed(epiworld_fast_uint s) {

    this->m_engine->seed(s);

}

template<typename TData>
inline void LFMCMC<TData>::set_rand_engine(std::shared_ptr< epi_xoshiro256ss > & eng)
{
    m_engine = eng;
}

template<typename TData>
inline void LFMCMC<TData>::set_rand_gamma(epiworld_double alpha, epiworld_double beta)
{
    rgammad = std::make_shared<std::gamma_distribution<>>(alpha,beta);
}

template<typename TData>
inline std::shared_ptr< epi_xoshiro256ss > & LFMCMC<TData>::get_rand_endgine()
{
    return m_engine;
}

// Step 1: Simulate data

// Step 2: Compute the sufficient statistics

// Step 3: Compute the hastings-ratio

// Step 4: Accept/reject, and go back to step 1

#define DURCAST(tunit,txtunit) {\
        elapsed       = std::chrono::duration_cast<std::chrono:: tunit>(\
            m_end_time - m_start_time).count(); \
        abbr_unit     = txtunit;}

template<typename TData>
inline void LFMCMC<TData>::get_elapsed_time(
    std::string unit,
    epiworld_double * last_elapsed,
    std::string * unit_abbr,
    bool print
) const {

    // Preparing the result
    epiworld_double elapsed;
    std::string abbr_unit;

    // Figuring out the length
    if (unit == "auto")
    {

        size_t tlength = std::to_string(
            static_cast<int>(floor(m_elapsed_time.count()))
            ).length();
        
        if (tlength <= 1)
            unit = "nanoseconds";
        else if (tlength <= 3)
            unit = "microseconds";
        else if (tlength <= 6)
            unit = "milliseconds";
        else if (tlength <= 8)
            unit = "seconds";
        else if (tlength <= 9)
            unit = "minutes";
        else 
            unit = "hours";

    }

    if (unit == "nanoseconds")       DURCAST(nanoseconds,"ns")
    else if (unit == "microseconds") DURCAST(microseconds,"\xC2\xB5s")
    else if (unit == "milliseconds") DURCAST(milliseconds,"ms")
    else if (unit == "seconds")      DURCAST(seconds,"s")
    else if (unit == "minutes")      DURCAST(minutes,"m")
    else if (unit == "hours")        DURCAST(hours,"h")
    else
        throw std::range_error("The time unit " + unit + " is not supported.");


    if (last_elapsed != nullptr)
        *last_elapsed = elapsed;
    if (unit_abbr != nullptr)
        *unit_abbr = abbr_unit;

    if (!print)
        return;

    printf_epiworld("Elapsed time : %.2f%s.\n", elapsed, abbr_unit.c_str());
}

#undef DURCAST

/*//////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

 Start of -include/epiworld/math/lfmcmc/lfmcmc-meat-print.hpp-

////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////*/


#ifndef LFMCMC_MEAT_PRINT_HPP
#define LFMCMC_MEAT_PRINT_HPP

template<typename TData>
inline void LFMCMC<TData>::print(size_t burnin) const
{

    // For each statistic or parameter in the model, we print three values: 
    // - mean, the 2.5% quantile, and the 97.5% quantile
    std::vector< epiworld_double > summ_params(m_n_params * 3, 0.0);
    std::vector< epiworld_double > summ_stats(m_n_stats * 3, 0.0);

    // Compute the number of samples to use based on burnin rate
    size_t n_samples_print = m_n_samples;
    if (burnin > 0)
    {
        if (burnin >= m_n_samples)
            throw std::length_error(
                "The burnin is greater than or equal to the number of samples."
                );

        n_samples_print = m_n_samples - burnin;

    }

    epiworld_double n_samples_dbl = static_cast< epiworld_double >(
        n_samples_print
        );

    // Compute parameter summary values
    for (size_t k = 0u; k < m_n_params; ++k)
    {

        // Retrieving the relevant parameter
        std::vector< epiworld_double > par_i(n_samples_print);
        for (size_t i = burnin; i < m_n_samples; ++i)
        {
            par_i[i-burnin] = m_all_accepted_params[i * m_n_params + k];
            summ_params[k * 3] += par_i[i-burnin]/n_samples_dbl;
        }

        // Computing the 95% Credible interval
        std::sort(par_i.begin(), par_i.end());

        summ_params[k * 3 + 1u] = 
            par_i[std::floor(.025 * n_samples_dbl)];
        summ_params[k * 3 + 2u] = 
            par_i[std::floor(.975 * n_samples_dbl)];

    }

    // Compute statistics summary values
    for (size_t k = 0u; k < m_n_stats; ++k)
    {

        // Retrieving the relevant parameter
        std::vector< epiworld_double > stat_k(n_samples_print);
        for (size_t i = burnin; i < m_n_samples; ++i)
        {
            stat_k[i-burnin] = m_all_accepted_stats[i * m_n_stats + k];
            summ_stats[k * 3] += stat_k[i-burnin]/n_samples_dbl;
        }

        // Computing the 95% Credible interval
        std::sort(stat_k.begin(), stat_k.end());
        summ_stats[k * 3 + 1u] = 
            stat_k[std::floor(.025 * n_samples_dbl)];
        summ_stats[k * 3 + 2u] = 
            stat_k[std::floor(.975 * n_samples_dbl)];

    }

    printf_epiworld("___________________________________________\n\n");
    printf_epiworld("LIKELIHOOD-FREE MARKOV CHAIN MONTE CARLO\n\n");

    printf_epiworld("N Samples (total) : %zu\n", m_n_samples);
    printf_epiworld("N Samples (after burn-in period) : %zu\n", m_n_samples - burnin);

    std::string abbr;
    epiworld_double elapsed;
    get_elapsed_time("auto", &elapsed, &abbr, false);
    printf_epiworld("Elapsed t : %.2f%s\n\n", elapsed, abbr.c_str());
    
    ////////////////////////////////////////////////////////////////////////////
    // PARAMETERS
    ////////////////////////////////////////////////////////////////////////////
    printf_epiworld("Parameters:\n");

    // Figuring out format
    std::string fmt_params;
    
    int nchar_par_num = 0;
    for (auto & n : summ_params)
    {
        
        int tmp_nchar;
        
        if (std::abs(n) < 1) {
            // std::log10(<1) will return negative number
            // std::log10(0) will return -inf and throw a runtime error
            tmp_nchar = 0;
        } else {
            tmp_nchar = std::floor(std::log10(std::abs(n)));
        }

        if (nchar_par_num < tmp_nchar)
            nchar_par_num = tmp_nchar;
    }
    nchar_par_num += 5; // 1 for neg padd, 2 for decimals, 1 the decimal point, and one b/c log(<10) < 1.
    std::string charlen = std::to_string(nchar_par_num);

    if (m_param_names.size() != 0u)
    {
        int nchar_par = 0;
        for (auto & n : m_param_names)
        {
            int tmp_nchar = n.length();
            if (nchar_par < tmp_nchar)
                nchar_par = tmp_nchar;
        }

        fmt_params = std::string("  -%-") +
            std::to_string(nchar_par) +
            std::string("s : % ") + charlen  + 
            std::string(".2f [% ") + charlen + 
            std::string(".2f, % ") + charlen +
            std::string(".2f] (initial : % ") +
            charlen + std::string(".2f)\n");

        for (size_t k = 0u; k < m_n_params; ++k)
        {
            printf_epiworld(
                fmt_params.c_str(),
                m_param_names[k].c_str(),
                summ_params[k * 3],
                summ_params[k * 3 + 1u],
                summ_params[k * 3 + 2u],
                m_initial_params[k]
                );
        }

        
    } else {

        fmt_params = std::string("  [%-2ld]: % ") + charlen + 
            std::string(".2f [% ") + charlen +
            std::string(".2f, % ") + charlen + 
            std::string(".2f] (initial : % ") + charlen +
            std::string(".2f)\n");

        for (size_t k = 0u; k < m_n_params; ++k)
        {
            
            printf_epiworld(
                fmt_params.c_str(),
                k,
                summ_params[k * 3],
                summ_params[k * 3 + 1u],
                summ_params[k * 3 + 2u],
                m_initial_params[k]
                );
        }

    }    

    ////////////////////////////////////////////////////////////////////////////
    // Statistics
    ////////////////////////////////////////////////////////////////////////////
    printf_epiworld("\nStatistics:\n");
    int nchar = 0;
    for (auto & s : summ_stats)
    {
        int tmp_nchar;
        if (std::abs(s) < 1) {
            // std::log10(<1) will return negative number
            // std::log10(0) will return -inf and throw a runtime error
            tmp_nchar = 0;
        } else {
            tmp_nchar = std::floor(std::log10(std::abs(s)));
        }
    
        if (nchar < tmp_nchar)
            nchar = tmp_nchar;
    }

    nchar += 5; // See above

    std::string nchar_char = std::to_string(nchar);

    // Figuring out format
    std::string fmt_stats;
    if (m_stat_names.size() != 0u)
    {
        int nchar_stats = 0;
        for (auto & n : m_stat_names)
        {
            int tmp_nchar = n.length();
            if (nchar_stats < tmp_nchar)
                nchar_stats = tmp_nchar;
        }

        fmt_stats = std::string("  -%-") +
            std::to_string(nchar_stats) +
            std::string("s : % ") + nchar_char +
            std::string(".2f [% ") + nchar_char +
            std::string(".2f, % ") + nchar_char +
            std::string(".2f] (Observed: % ") + nchar_char +
            std::string(".2f)\n");

        for (size_t k = 0u; k < m_n_stats; ++k)
        {
            printf_epiworld(
                fmt_stats.c_str(),
                m_stat_names[k].c_str(),
                summ_stats[k * 3],
                summ_stats[k * 3 + 1u],
                summ_stats[k * 3 + 2u],
                m_observed_stats[k]
                );
        }

        
    } else {

        fmt_stats = std::string("  [%-2ld] : % ") + 
            nchar_char +
            std::string(".2f [% ") + nchar_char +
            std::string(".2f, % ") + nchar_char +
            std::string(".2f] (Observed: % ") + nchar_char +
            std::string(".2f)\n");

        for (size_t k = 0u; k < m_n_stats; ++k)
        {
            printf_epiworld(
                fmt_stats.c_str(),
                k,
                summ_stats[k * 3],
                summ_stats[k * 3 + 1u],
                summ_stats[k * 3 + 2u],
                m_observed_stats[k]
                );
        }

    }

    printf_epiworld("___________________________________________\n\n");
}

#endif
/*//////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

 End of -include/epiworld/math/lfmcmc/lfmcmc-meat-print.hpp-

////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////*/



template<typename TData>
inline void LFMCMC<TData>::chrono_start() {
    m_start_time = std::chrono::steady_clock::now();
}

template<typename TData>
inline void LFMCMC<TData>::chrono_end() {
    m_end_time = std::chrono::steady_clock::now();
    m_elapsed_time += (m_end_time - m_start_time);
}

template<typename TData>
inline void LFMCMC<TData>::set_params_names(std::vector< std::string > names)
{

    if (names.size() != m_n_params)
        throw std::length_error("The number of names to add differs from the number of parameters in the model.");

    m_param_names = names;

}
template<typename TData>
inline void LFMCMC<TData>::set_stats_names(std::vector< std::string > names)
{

    if (names.size() != m_n_stats)
        throw std::length_error("The number of names to add differs from the number of statistics in the model.");

    m_stat_names = names;

}

template<typename TData>
inline std::vector< epiworld_double > LFMCMC<TData>::get_mean_params()
{
    std::vector< epiworld_double > res(this->m_n_params, 0.0);
    
    for (size_t k = 0u; k < m_n_params; ++k)
    {
        for (size_t i = 0u; i < m_n_samples; ++i)
            res[k] += (this->m_all_accepted_params[k + m_n_params * i])/
                static_cast< epiworld_double >(m_n_samples);
    }

    return res;

}

template<typename TData>
inline std::vector< epiworld_double > LFMCMC<TData>::get_mean_stats()
{
    std::vector< epiworld_double > res(this->m_n_stats, 0.0);
    
    for (size_t k = 0u; k < m_n_stats; ++k)
    {
        for (size_t i = 0u; i < m_n_samples; ++i)
            res[k] += (this->m_all_accepted_stats[k + m_n_stats * i])/
                static_cast< epiworld_double >(m_n_samples);
    }

    return res;

}

template<typename TData>
inline LFMCMC<TData> & LFMCMC<TData>::verbose_off()
{
    verbose = false;
    return *this;
}

template<typename TData>
inline LFMCMC<TData> & LFMCMC<TData>::verbose_on()
{
    verbose = true;
    return *this;
}

#endif
/*//////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

 End of -include/epiworld/math/lfmcmc/lfmcmc-meat.hpp-

////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////*/



#endif
/*//////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

 End of -./include/epiworld/math/lfmcmc.hpp-

////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////*/



/*//////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

 Start of -./include/epiworld/run-outputs.hpp-

////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////*/


#ifndef EPIWORLD_RUN_OUTPUTS_HPP
#define EPIWORLD_RUN_OUTPUTS_HPP

/**
 * @brief A column-oriented table, the in-memory form of one output CSV.
 *
 * Columns have the same names, order and rows as the matching CSV file
 * (see `run_output_names()`), so a binding can turn a table into a data
 * frame, a dict of arrays, etc. with a single converter.
 */
struct OutputTable
{
    using Column = std::variant<
        std::vector< int >,
        std::vector< double >,
        std::vector< std::string >
    >;

    std::vector< std::string > colnames;
    std::vector< Column > columns;

    template<typename T>
    void add(std::string name, std::vector< T > values)
    {
        colnames.push_back(std::move(name));
        columns.emplace_back(std::move(values));
    }

    size_t nrow() const
    {
        return columns.empty() ?
            0u :
            std::visit([](const auto & c) { return c.size(); }, columns[0u]);
    }
};

/// Tables of one simulation (or, in `SaverMemory::results()`, of all of
/// them), keyed by output name.
using RunOutputs = std::map< std::string, OutputTable >;

/// The names of the outputs, which are also the suffixes of the CSV files.
inline const std::vector< std::string > & run_output_names()
{
    static const std::vector< std::string > names = {
        "virus_info", "virus_hist", "tool_info", "tool_hist", "total_hist",
        "transmission", "transition", "reproductive", "generation",
        "active_cases", "outbreak_size", "hospitalizations"
    };
    return names;
}

/**
 * @brief Writes a table as a space-separated file with a header.
 *
 * Strings are double-quoted.
 *
 * In `EPI_DEBUG` builds, each line is prefixed with the id of the thread that
 * wrote it (a leading `thread` column), as the CSVs always were. It describes
 * the writer, not the simulation, so it is not part of `OutputTable`.
 */
inline void write_table(const std::string & fn, const OutputTable & table)
{

    std::ofstream file(fn, std::ios_base::out);
    if (!file)
        throw std::runtime_error("Could not open file \"" + fn + "\" for writing.");

    #ifdef EPI_DEBUG
    file << "thread ";
    #endif

    for (size_t j = 0u; j < table.colnames.size(); ++j)
        file << table.colnames[j] << (j + 1u < table.colnames.size() ? " " : "\n");

    // Quoting the string columns
    std::vector< bool > quote(table.columns.size());
    for (size_t j = 0u; j < quote.size(); ++j)
        quote[j] =
            std::holds_alternative< std::vector< std::string > >(table.columns[j]);

    for (size_t i = 0u; i < table.nrow(); ++i)
    {

        #ifdef EPI_DEBUG
        file << EPI_GET_THREAD_ID() << " ";
        #endif

        for (size_t j = 0u; j < table.columns.size(); ++j)
        {
            std::visit(
                [&](const auto & col) { file << (quote[j] ? "\"" : "") << col[i] << (quote[j] ? "\"" : ""); },
                table.columns[j]
            );
            file << (j + 1u < table.columns.size() ? " " : "\n");
        }

    }

}

/**
 * @brief Keeps the outputs of `run_multiple()` in memory.
 *
 * An object of this class is a valid `fun` for `Model::run_multiple()`. Each
 * simulation's tables are stored by `sim_id`, so results do not depend on
 * which thread finishes first. Copies share the same storage (which is what
 * lets `std::function` and OpenMP copy it); `run_multiple()` already
 * serializes calls to `fun`, but the object is not otherwise thread-safe.
 *
 * @code{.cpp}
 * SaverMemory saver({"total_hist", "transition"});
 * model.run_multiple(100, 50, 123, saver, true, true, 4);
 * RunOutputs all = saver.results();
 * @endcode
 */
class SaverMemory
{
private:

    struct State
    {
        std::vector< std::string > whats;
        std::vector< RunOutputs > runs; ///< Indexed by sim_id
    };

    std::shared_ptr< State > state = std::make_shared< State >();

public:

    /// @param whats Names of the outputs to keep (see `run_output_names()`).
    SaverMemory(std::vector< std::string > whats = {"total_hist"})
    {
        state->whats = std::move(whats);
    }

    void operator()(size_t sim_id, Model * model) const; ///< In database-meat.hpp.

    /// All simulations, concatenated, with a leading 0-based `sim_id` column.
    RunOutputs results() const
    {

        RunOutputs ans;

        for (const auto & w : state->whats)
        {

            OutputTable * all = nullptr;

            for (size_t s = 0u; s < state->runs.size(); ++s)
            {

                auto it = state->runs[s].find(w);
                if (it == state->runs[s].end())
                    continue;

                const OutputTable & t = it->second;

                if (all == nullptr)
                {
                    all = &(ans[w] = t);
                    all->colnames.insert(all->colnames.begin(), "sim_id");
                    all->columns.insert(
                        all->columns.begin(),
                        OutputTable::Column(std::vector< int >(t.nrow(), static_cast<int>(s)))
                    );
                    continue;
                }

                std::get< std::vector< int > >(all->columns[0u]).insert(
                    std::get< std::vector< int > >(all->columns[0u]).end(),
                    t.nrow(), static_cast<int>(s)
                );

                for (size_t j = 0u; j < t.columns.size(); ++j)
                    std::visit(
                        [&](auto & dst)
                        {
                            const auto & src = std::get< std::decay_t< decltype(dst) > >(t.columns[j]);
                            dst.insert(dst.end(), src.begin(), src.end());
                        },
                        all->columns[j + 1u]
                    );

            }

        }

        return ans;

    }

    /// Forgets the stored simulations (call it before reusing the saver).
    void clear() { state->runs.clear(); }

};

#endif
/*//////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

 End of -./include/epiworld/run-outputs.hpp-

////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////*/



    // The classes, then their member functions: with no templates, a
    // function body needs the classes it uses defined before it
/*//////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

 Start of -./include/epiworld/userdata-bones.hpp-

////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////*/


#ifndef EPIWORLD_USERDATA_BONES_HPP
#define EPIWORLD_USERDATA_BONES_HPP

class Model;

class DataBase;

/**
 * @brief Personalized data by the user
 *
 */
class UserData
{
    friend class Model;
    friend class DataBase;

private:
    Model * model = nullptr;

    std::vector< std::string > data_names;
    std::vector< int > data_dates;
    std::vector< epiworld_double > data_data;

    epiworld_fast_uint k = 0u;
    epiworld_fast_uint n = 0u;

    int last_day = -1;

public:

    UserData() = delete;
    UserData(Model & m) : model(&m) {};
    UserData(Model * m) : model(m) {};

    /**
     * @brief Construct a new User Data object
     *
     * @param names A vector of names. The length of the vector sets
     * the number of columns to record.
     */
    UserData(std::vector< std::string > names);

    /**
     * @name Append data
     *
     * @param x A vector of length `ncol()` (if vector), otherwise a `epiworld_double`.
     * @param j Index of the data point, from 0 to `ncol() - 1`.
     */
    ///@{
    void add(std::vector<epiworld_double> x);
    void add(
        epiworld_fast_uint j,
        epiworld_double x
        );
    ///@}

    /**
     * @name Access data
     *
     * @param i Row (0 through ndays - 1.)
     * @param j Column (0 through `ncols()`).
     * @return epiworld_double&
     */
    ///@{
    epiworld_double & operator()(
        epiworld_fast_uint i,
        epiworld_fast_uint j
        );

    epiworld_double & operator()(
        epiworld_fast_uint i,
        std::string name
        );
    ///@}

    std::vector< std::string > & get_names();

    std::vector< int > & get_dates();

    std::vector< epiworld_double > & get_data();

    void get_all(
        std::vector< std::string > * names    = nullptr,
        std::vector< int > * date             = nullptr,
        std::vector< epiworld_double > * data = nullptr
    );

    epiworld_fast_uint nrow() const;
    epiworld_fast_uint ncol() const;

    void write(std::string fn);
    void print() const;

};

#endif
/*//////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

 End of -./include/epiworld/userdata-bones.hpp-

////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////*/


/*//////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

 Start of -./include/epiworld/hospitalizationstracker-bones.hpp-

////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////*/


#ifndef EPIWORLD_HOSPITALIZATIONSTRACKER_BONES_HPP
#define EPIWORLD_HOSPITALIZATIONSTRACKER_BONES_HPP

class Agent;

class Model;

/**
 * @brief Class to track hospitalizations in an epidemiological model.
 * 
 * @details
 * This class keeps track of hospitalizations in a model. For each hospitalized
 * agent, it records the date, virus ID, and tool IDs with appropriate weights.
 * 
 * Since agents always have at most one virus but may have multiple tools,
 * if an agent has N tools, then N records are created, each with weight
 * equal to 1/N. If the agent has no tools, a single record is created with
 * tool_id = -1 and weight = 1.0.
 * 
 * The `get()` method returns a summary of the hospitalizations grouped by
 * date, virus_id, and tool_id, with weight summed across all matching
 * records.
 * 
 */
class HospitalizationsTracker {

private:

    std::vector<int> _date;           ///< Date of hospitalization
    std::vector<int> _virus_id;       ///< ID of the virus causing hospitalization
    std::vector<int> _tool_id;        ///< ID of the tool (-1 if no tools)
    std::vector<double> _weight;      ///< Weight for the tool (1/N where N is tool count)

public:

    HospitalizationsTracker() = default;

    /**
     * @brief Reset the tracker by clearing all data.
     */
    void reset();

    /**
     * @brief Record a hospitalization event for an agent.
     * 
     * @param agent Reference to the agent being hospitalized.
     * @param model Reference to the model.
     * 
     * @details
     * For each hospitalization, the method records:
     * - The current date from the model
     * - The virus ID from the agent's virus
     * - For each tool the agent has, a separate record with weight = 1/N
     *   where N is the number of tools
     * - If the agent has no tools, a single record with tool_id = -1 and
     *   weight = 1.0
     */
    void record(Agent & agent, Model & model);

    /**
     * @brief Get the full time series of hospitalizations.
     * 
     * @param ndays Number of days in the simulation (0 to ndays-1).
     * @param date Output vector for dates.
     * @param virus_id Output vector for virus IDs.
     * @param tool_id Output vector for tool IDs.
     * @param count Output vector for counts (number of hospitalized individuals).
     * @param weight Output vector for summed weights (fractional contribution 
     *   based on tool distribution).
     * 
     * @details
     * Returns the full time series of hospitalization data. For each unique 
     * (virus_id, tool_id) combination observed, returns an entry for every 
     * day from 0 to ndays-1.
     * 
     * The `count` vector contains the actual number of individuals hospitalized 
     * for that (date, virus_id, tool_id) combination. This is useful for 
     * answering questions like "how many total people were hospitalized?" 
     * regardless of their tools.
     * 
     * The `weight` vector contains fractional contributions: if an agent has N 
     * tools, each tool gets weight = 1/N. Summing weights across all tool_ids 
     * for a given date and virus_id gives the total number of hospitalizations.
     */
    void get(
        int ndays,
        std::vector<int> & date,
        std::vector<int> & virus_id,
        std::vector<int> & tool_id,
        std::vector<int> & count,
        std::vector<double> & weight
    ) const;

    /**
     * @brief Get the number of raw records in the tracker.
     * @return Number of records.
     */
    size_t size() const;

};

#endif
/*//////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

 End of -./include/epiworld/hospitalizationstracker-bones.hpp-

////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////*/


/*//////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

 Start of -./include/epiworld/adjlist-bones.hpp-

////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////*/


#ifndef EPIWORLD_ADJLIST_BONES_HPP
#define EPIWORLD_ADJLIST_BONES_HPP


/**
 * @brief A network, as the ties of each vertex
 *
 * @details Row `i` holds the neighbors of vertex `i` (in a directed network,
 * the targets of its ties) in ascending order, without repeats; an undirected
 * tie is in the rows of both ends. The rows are stored back to back
 * (compressed rows): row `i` is `get_ids()` from `get_start()[i]` to
 * `get_start()[i + 1]`. A graph cannot be edited once built; the network layer
 * that loads it can (see `layer::Network`).
 */
class AdjList {
private:

    std::vector< size_t > start; ///< Where each row starts in `ids` (`N + 1`)
    std::vector< int > ids;      ///< The rows, back to back
    bool directed = false;
    epiworld_fast_uint N = 0;
    epiworld_fast_uint E = 0;

public:

    AdjList() {};

    /**
     * @brief Construct a new Adj List object
     *
     * @details
     * Ids in the network are assume to range from `0` to `size - 1`. A tie
     * given more than once is kept once.
     *
     * @param source Unsigned int vector with the source
     * @param target Unsigned int vector with the target
     * @param size Number of vertices in the network.
     * @param directed Bool true if the network is directed
     */
    AdjList(
        const std::vector< int > & source,
        const std::vector< int > & target,
        int size,
        bool directed
        );

    /**
     * @brief Read an edgelist
     *
     * Ids in the network are assume to range from `0` to `size - 1`.
     *
     * @param fn Path to the file
     * @param skip Number of lines to skip (e.g., 1 if there's a header)
     * @param directed `true` if the network is directed
     * @param size Number of vertices in the network.
     */
    void read_edgelist(
        std::string fn,
        int size,
        int skip = 0,
        bool directed = true
        );

    /// The neighbors of vertex `i` (a copy of its row).
    std::vector< int > operator()(
        epiworld_fast_uint i
        ) const;

    void print(epiworld_fast_uint limit = 20u) const;
    size_t vcount() const; ///< Number of vertices/nodes in the network.
    size_t ecount() const; ///< Number of edges given, repeats included.

    /// Where each row starts in `get_ids()`, with the end as the last entry.
    const std::vector< size_t > & get_start() const { return start; }

    /// The rows, back to back (see the class description).
    const std::vector< int > & get_ids() const { return ids; }

    bool is_directed() const; ///< `true` if the network is directed.

};


#endif
/*//////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

 End of -./include/epiworld/adjlist-bones.hpp-

////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////*/


/*//////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

 Start of -./include/epiworld/contacttracing-bones.hpp-

////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////*/


#ifndef EPIWORLD_CONTACTTRACING_BONES_H
#define EPIWORLD_CONTACTTRACING_BONES_H

#include <vector>
#include <set>
#include <map>
#include <stdexcept>
// (already included include/epiworld/config.hpp)

/**
 * @brief Represents a single contact relationship for an agent.
 * @details
 * Each instance holds the id of the contacted agent and the set of
 * simulation days on which the contact occurred.  Instances are
 * returned by `ContactTracing::get_contacts()`.
 */
class ContactRecord
{
private:
    size_t m_contact_id;
    std::set<int> m_times;

public:
    ContactRecord(size_t contact_id, std::set<int> times)
        : m_contact_id(contact_id), m_times(std::move(times)) {}

    /**
     * @brief Return the id of the contacted agent.
     */
    size_t get_contact_id() const { return m_contact_id; }

    /**
     * @brief Return the set of days on which the contact was recorded.
     * @return A const reference to the set of simulation days.
     */
    const std::set<int> & get_times() const { return m_times; }
};

/** 
 * @brief Class for tracing contacts between agents
 * @details
 * The class assumes that contacts are stored in a matrix-like
 * structure, where rows are agents and columns are contacts in a 
 * column-major format. Each entry stores the id of the contact
 * and the day of the contact is stored in a parallel array.
 * 
 * The maximum number of contacts per agent is fixed at initialization
 * time. If more contacts are added, they will overwrite previous
 * contacts in a circular buffer fashion.
 * */
class ContactTracing
{
private:

    std::vector< size_t > contact_matrix;
    std::vector< size_t > contacts_per_agent;
    std::vector< size_t > contact_date;

    size_t n_agents;
    size_t max_contacts;

    // Cache: for each agent, the list of unique contacts with their dates.
    // up_to_date[a] is false whenever add_contact(a, ...) is called, and
    // set to true after get_contacts(a) rebuilds the cache.
    std::vector< std::vector<ContactRecord> > cached_contacts;
    std::vector< bool > up_to_date;

    size_t get_location(size_t row, size_t col) const;

public:

    ContactTracing();

    /**
     * @brief Construct a new Contact Tracing object
     * 
     * @param n_agents Agents in the system, usually `Model::size()`.
     * @param max_contacts Maximum number of contacts to track per agent.
     */
    ContactTracing(size_t n_agents, size_t max_contacts);

    /**
     * @brief Add a contact between two agents at a given day.
     * 
     * @param agent_a Agent id (usually infectious agent)
     * @param agent_b Agent id (usually susceptible agent)
     * @param day Day of the contact (usually Model::today()).
     */
    void add_contact(size_t agent_a, size_t agent_b, size_t day);

    /**
     * @brief Get the number of contacts for an agent
     * 
     * @param agent Agent id
     * @return size_t Number of contacts recorded for that agent (can be more than max_contacts)
     */
    size_t get_n_contacts(size_t agent); 

    size_t get_max_contacts() const;

    /// Number of agents the log covers (agent ids run from 0 to this - 1).
    size_t get_n_agents() const;
    
    /**
     * @brief Get the contact object
     * 
     * @param agent Source agent (id)
     * @param idx Index of the contact (0 to get_n_contacts(agent)-1)
     * @return std::pair<size_t, int> with (contact_id, contact_day)
     * @throws std::out_of_range if idx is out of range.
     */
    std::pair<size_t, int> get_contact(size_t agent, size_t idx);

    /**
     * @brief Reset the contact tracing data
     * 
     * Usually called by `Model::reset()`.
     * 
     * @param n_agents Number of agents
     * @param max_contacts Maximum number of contacts to track per agent
     */
    void reset(
        size_t n_agents,
        size_t max_contacts
    );

    /**
     * @brief Get all unique contacts of an agent with the days they occurred.
     * @details
     * Returns a vector of `ContactRecord` objects, one per unique contacted
     * agent.  Each record exposes:
     *
     * - `get_contact_id()` – the id of the other agent.
     * - `get_times()` – the set of simulation days on which the contact was
     *   recorded in the current (non-reset) window.
     *
     * The result is lazily computed on the first call after any new contact is
     * recorded for this agent and cached until the next `add_contact()` call
     * for the same agent.
     *
     * @param agent Source agent id.
     * @return Const reference to the cached vector of ContactRecord objects.
     */
    const std::vector<ContactRecord> & get_contacts(size_t agent);

    /**
     * @brief Print the contacts of an agent 
     * 
     * @param agent Agent id
     */
    void print(size_t agent);
};

#endif
/*//////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

 End of -./include/epiworld/contacttracing-bones.hpp-

////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////*/


/*//////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

 Start of -./include/epiworld/model-bones.hpp-

////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////*/


#ifndef EPIWORLD_MODEL_BONES_HPP
#define EPIWORLD_MODEL_BONES_HPP

#include <memory>
#include <vector>
#include <functional>
#include <string>
#include <map>
// (already included include/epiworld/config.hpp)

/*//////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

 Start of -include/epiworld/naturalhistory-bones.hpp-

////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////*/


#ifndef EPIWORLD_NATURALHISTORY_BONES_HPP
#define EPIWORLD_NATURALHISTORY_BONES_HPP

/**
 * @file naturalhistory-bones.hpp
 * @brief The natural history of a disease: its stages and the declared
 * transitions between them (see `natural-history.md`).
 *
 * @details A natural history is a small graph. Its nodes are **stages**
 * (susceptible, latent, infectious, recovered, ...) and its edges are
 * **transitions**, each of one kind:
 *
 * - `infection()`: by contact, drawn from the model's layers (or a custom
 *   sampler). Taking it gives the agent the virus.
 * - `rate()`: a daily probability. The rates out of a stage compete: one
 *   draw (`roulette()`) picks one of them, or none.
 * - `after()`: a duration, drawn when the agent enters the stage. It is a
 *   deadline: the agent leaves on the update of the day it falls, unless a
 *   rate fired first.
 * - `external()`: fired by code, through `Model::fire()`.
 *
 * A stage is a **carrier** (its agents hold the virus) and may be
 * **infectious** (they transmit; this implies carrier). Entering a
 * non-carrier stage releases the virus. There are no fixed roles: a stage is
 * susceptible because an infection transition leaves it, and a death is a
 * stage with contact weight 0 and no exits.
 *
 * Every number a transition uses is a `Value`: a number, a model parameter
 * (by name) or a function. A number given directly becomes a model parameter
 * when the natural history is set on a model.
 */

class Nodes;

class NaturalHistory;

class TransitionRef;

/// Flags of a stage (see `NaturalHistory::add_stage()`).
namespace stage {
    constexpr unsigned int carrier    = 1u;      ///< Agents in it hold the virus
    constexpr unsigned int infectious = 1u | 2u; ///< They transmit (implies carrier)
}

/// Which tool effect a rate takes (see `TransitionRef::effect()`).
enum class Effect : uint8_t {
    none,     ///< No tool effect
    recovery, ///< `p` becomes `1 - (1 - p) * (1 - recovery enhancer)`
    death     ///< `p` becomes `p * (1 - death reduction)`
};

/// One or more stages (an `int` or a list of them).
class Nodes {
public:
    std::vector< int > ids;
    Nodes(int id) : ids({id}) {}
    Nodes(std::initializer_list< int > l) : ids(l) {}
    Nodes(std::vector< int > v) : ids(std::move(v)) {}
};

using ValueFun = std::function< epiworld_double(Agent &, Model &) >;

using DurationFun = std::function< int(Agent &, Model &) >;

/// A custom infection sampler: the virus the agent catches, or `nullptr`.
using InfectionSampler = std::function< Virus * (Agent &, Model &) >;

/**
 * @brief A number a transition uses: a constant, a model parameter, or a
 * function.
 * @details
 * - `Value(0.1)`: a number. When the natural history is set on a model, it
 *   becomes a model parameter (named after the transition).
 * - `Value("Recovery rate")`: a model parameter, by name.
 * - `Value::inverse("Days")`, `Value::complement("p")`: `1 / p`, `1 - p`.
 * - `Value(fun)`: `fun(agent, model)`.
 *
 * A parameter is read through the agent's virus, so a variant can point it to
 * another parameter (`Virus::use_param()`).
 */
class Value {
public:
    enum class Kind : uint8_t { none, number, param, inverse, complement, fun };

private:
    Kind kind = Kind::none;
    epiworld_double number = 0.0;
    ParamRef param = ParamRef("");
    ValueFun fun = nullptr;

    friend class NaturalHistory;

public:
    Value() = default;
    /// A number (any arithmetic type, whatever `epiworld_double` is)
    template<
        typename T,
        typename std::enable_if< std::is_arithmetic< T >::value, int >::type = 0
    >
    Value(T x) : kind(Kind::number), number(static_cast< epiworld_double >(x)) {}
    Value(const char * name) : kind(Kind::param), param(std::string(name)) {}
    Value(std::string name) : kind(Kind::param), param(std::move(name)) {}
    Value(ValueFun f) : kind(Kind::fun), fun(std::move(f)) {}

    static Value inverse(std::string name);    ///< `1 / parameter`
    static Value complement(std::string name); ///< `1 - parameter`

    Kind get_kind() const { return kind; }
    bool is_set() const { return kind != Kind::none; }

    /// The parameter's name ("" for a function).
    const std::string & get_param_name() const { return param.name(); }

    /// The value for this agent (its virus may redirect the parameter).
    epiworld_double eval(Agent & agent, Model & model) const;

    /// The same, with the parameter redirected by `virus` (e.g., the
    /// infector's, for a probability of infection).
    epiworld_double eval(
        Agent & agent,
        const Virus * virus,
        Model & model
    ) const;
};

/**
 * @brief How many days a stage lasts, drawn when the agent enters it.
 * @details Always a whole number of days, at least one.
 * - `fixed(p)`: `p`, rounded.
 * - `geometric(mean)`: a geometric number of days with that mean (the same as
 *   leaving each day with probability `1 / mean`).
 * - `lognormal(mean, sd)`, `gamma(mean, sd)`: the mean and standard deviation
 *   of the duration, in days. A standard deviation of zero is fixed.
 * - `fun`: `fun(agent, model)`.
 *
 * `since(s)` counts the duration from the day the agent entered stage `s`
 * (in the same infection), instead of the day it entered this stage.
 */
class Duration {
public:
    enum class Kind : uint8_t { none, fixed, geometric, lognormal, gamma, fun };

private:
    Kind kind = Kind::none;
    Value mean;
    Value sd;
    DurationFun fun = nullptr;
    int anchor = -1;

    friend class NaturalHistory;

public:
    Duration() = default;
    Duration(DurationFun f) : kind(Kind::fun), fun(std::move(f)) {}

    static Duration fixed(Value days);
    static Duration geometric(Value mean);
    static Duration lognormal(Value mean, Value sd);
    static Duration gamma(Value mean, Value sd);

    /// Counts from the day the agent entered stage `s` in this infection.
    Duration since(int s) const;

    Kind get_kind() const { return kind; }
    int get_anchor() const { return anchor; }

    /// Draws a number of days (at least one).
    int draw(Agent & agent, Model & model) const;
};

/// The kind of a transition (see `naturalhistory-bones.hpp`).
enum class TransitionKind : uint8_t { infection, rate, after, external };

/**
 * @brief A declared transition: from a set of stages to a stage (or a branch
 * of stages), of one kind.
 */
struct Transition {
    std::string name;
    TransitionKind kind;
    std::vector< int > from;
    int to = -1;

    /// A branch: the target is drawn among these, with these probabilities.
    std::vector< int > branch_to;
    std::vector< Value > branch_prob;

    Value value;          ///< infection (per contact) and rate
    Duration duration;    ///< after
    Effect effect = Effect::none;
    InfectionSampler sampler = nullptr; ///< infection only (optional)

    /// A status transition: only for agents in one of these stages (empty: any)
    std::vector< int > when;
};

/**
 * @brief Refines a transition just declared (chainable); converts to its id.
 */
class TransitionRef {
private:
    NaturalHistory * nh;
    int id;

public:
    TransitionRef(NaturalHistory * nh_, int id_) : nh(nh_), id(id_) {}

    /// The transition's id for `Model::fire()` and reactions: a stage
    /// transition's own index, or, for a status transition, `-2 - index`.
    operator int() const;
    int get_id() const { return static_cast< int >(*this); }

    TransitionRef & name(std::string n);     ///< Default: "From -> To"
    TransitionRef & effect(Effect e);        ///< The tool effect a rate takes

    /// The target is drawn among `targets`, with their probabilities.
    TransitionRef & to(std::vector< std::pair< int, Value > > targets);

    /// A custom infection sampler (infection transitions only; C++ only).
    TransitionRef & sampler(InfectionSampler f);

    /// A status transition: only for agents in these stages (read at the
    /// start of the day).
    TransitionRef & when(Nodes stages);
};

/**
 * @brief What the engine needs about a stage, derived from the declarations.
 */
struct StageInfo {
    int infection = -1;           ///< The infection transition out of it
    std::vector< int > rates;     ///< Its rate transitions, in order
    int after = -1;               ///< Its duration transition
    bool needs_visit = false;     ///< Has a rate or a duration
    bool pushable = false;        ///< Its only drawn exit is a default infection
};

/**
 * @brief The natural history of a disease: stages and declared transitions.
 * @details It is built on the virus (`Virus::add_stage()` and the others),
 * and the model takes its stages from `Model::add_virus()`. The first stage
 * is where agents start. Example (SIR):
 *
 * @code{.cpp}
 * Virus flu("Flu");
 * int S = flu.add_stage("Susceptible");
 * int I = flu.add_stage("Infected", stage::infectious);
 * int R = flu.add_stage("Recovered");
 * flu.infection(S, I, "Transmission rate");
 * flu.rate(I, R, "Recovery rate").effect(Effect::recovery);
 * model.add_virus(flu, assign::share(0.01));
 * @endcode
 */
class NaturalHistory {
protected:
    std::string name;
    std::vector< std::string > stage_labels;
    std::vector< unsigned int > stage_flags;
    std::vector< epiworld_double > stage_contact;
    std::vector< Transition > transitions;

    // Derived by finalize()
    std::vector< StageInfo > stages;
    bool finalized = false;

    friend class TransitionRef;
    friend class Model;
    bool status_chart = false; ///< See `Statuses`

    int add_transition(Transition t);
    void check_stage(int s, const char * what) const;

    /// Checks the declarations and derives `stages`; turns numbers into model
    /// parameters, named after `owner` (the virus) and the transition.
    void finalize(Model & model, const std::string & owner);

public:
    explicit NaturalHistory(std::string name_);

    /// Adds a stage; returns its id. Flags: `stage::carrier`, `stage::infectious`.
    int add_stage(std::string label, unsigned int flags = 0u);

    /// Contact weight of a stage (default 1). 0 takes its agents out of every
    /// layer: they are not met and do not transmit.
    void set_contact(int s, epiworld_double weight);

    TransitionRef infection(Nodes from, int to, Value per_contact);
    TransitionRef rate(Nodes from, int to, Value daily_probability);
    TransitionRef after(int from, int to, Duration d);
    TransitionRef external(Nodes from, int to, std::string label);

    const std::string & get_name() const { return name; }
    size_t get_n_stages() const { return stage_labels.size(); }
    size_t get_n_transitions() const { return transitions.size(); }
    const std::vector< std::string > & get_stages() const { return stage_labels; }
    const Transition & get_transition(int id) const { return transitions.at(id); }

    bool is_carrier(int s) const { return (stage_flags.at(s) & stage::carrier) != 0u; }
    bool is_infectious(int s) const { return (stage_flags.at(s) & 2u) != 0u; }
    epiworld_double get_contact(int s) const { return stage_contact.at(s); }

    /// The stage's id, by label (throws if absent).
    int stage_id(std::string_view label) const;

    /// The transition's id, by name (throws if absent).
    int transition_id(std::string_view label) const;

    /// Whether `s` is one of the transition's sources.
    bool is_source(int transition, int s) const;

    /**
     * @brief Whether `other` has the same structure: the same stages (labels,
     * flags, contact weights) and the same transitions (names, kinds,
     * sources and targets). Their values may differ.
     */
    bool same_structure(const NaturalHistory & other) const;

    /// What the engine derived about stage `s` (once added to a model).
    const StageInfo & get_stage_info(int s) const { return stages.at(s); }

    /**
     * @brief The stages and transitions as the body of a mermaid flowchart.
     * @details One node per stage (id `prefix` + index; infectious, carrier
     * and contact-0 stages get the classes `infectious`, `carrier` and
     * `away`) and one edge per transition, labeled with its name (unless it
     * is the default "From -> To"), its kind and its value: `-->` for an
     * infection or a rate, `==>` for a duration, `-.->` for an external
     * transition. See `Model::draw_natural_history()`.
     * @param prefix Prefix of the node ids.
     * @param stages The natural history whose stages `when()` and `since()`
     * name (for statuses, the virus's; `nullptr`: this one).
     */
    std::string draw_mermaid(
        const std::string & prefix,
        const NaturalHistory * stages = nullptr
    ) const;
};

/**
 * @brief The statuses of a model: where agents are (free, quarantined,
 * isolated, hospitalized), and the transitions between them.
 * @details The same vocabulary as a natural history (`rate()`, `after()`,
 * `external()`), without infections or carriers. A status transition can be
 * restricted to agents in some stages (`TransitionRef::when()`); a duration
 * can count from a stage's entry (`Duration::since()`). A status with contact
 * weight 0 takes its agents out of every layer. The first status is where
 * agents start. Set on a model with `Model::set_statuses()`; a model without
 * statuses has one, "Free".
 */
class Statuses : public NaturalHistory {
public:
    Statuses() : NaturalHistory("Statuses") { this->status_chart = true; }

    /// Adds a status; returns its id.
    int add_status(std::string label) { return this->add_stage(std::move(label)); }

    TransitionRef infection(Nodes, int, Value) = delete;
};

/**
 * @brief What a reaction responds to (see `Model::react()`).
 * @details `Trigger::entered(s)`: an agent enters stage `s`;
 * `Trigger::fired(t)`: transition `t` is applied. `.was({statuses})`: only if
 * the agent's status at the start of the day was one of these.
 */
struct Trigger {
    enum class Kind : uint8_t { entered, fired } kind = Kind::entered;
    int target = -1;
    std::vector< int > was_status; ///< Empty: any status

    static Trigger entered(int stage) { return {Kind::entered, stage, {}}; }
    static Trigger fired(int transition) { return {Kind::fired, transition, {}}; }
    Trigger was(std::vector< int > statuses) const
    {
        Trigger t(*this);
        t.was_status = std::move(statuses);
        return t;
    }
};

#endif
/*//////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

 End of -include/epiworld/naturalhistory-bones.hpp-

////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////*/


/*//////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

 Start of -include/epiworld/agent-bones.hpp-

////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////*/


#ifndef EPIWORLD_PERSON_BONES_HPP
#define EPIWORLD_PERSON_BONES_HPP

class Model;

class Virus;

class Tool;


class Queue;

struct Event;

class AgentsSample;

/**
 * @brief Non-allocating range over an agent's neighbors in a network layer.
 *
 * @details It iterates the agent's neighbor ids in place and resolves each one
 * against the model's population as it goes, so a loop over the neighbors does
 * not allocate:
 *
 * ```cpp
 * for (auto * neighbor : net.neighbors_view(p->get_id()))
 *     ...
 * ```
 *
 * The order is the layer's tie order (see `layer::Network`). The view borrows
 * from the layer and the model, so it must not outlive either, and it is
 * invalidated by anything that changes the agent's ties.
 *
 */
class NeighborsView {
private:

    const size_t * first = nullptr;
    size_t n = 0u;
    std::vector< Agent > * pop = nullptr;

public:

    class iterator {

        friend class NeighborsView;

        const size_t * ptr = nullptr;
        std::vector< Agent > * pop = nullptr;

        iterator(const size_t * ptr, std::vector< Agent > * pop) :
            ptr(ptr), pop(pop) {}

    public:

        Agent * operator*() const { return &pop->operator[](*ptr); }
        iterator & operator++() { ++ptr; return *this; }
        bool operator!=(const iterator & other) const { return ptr != other.ptr; }
        bool operator==(const iterator & other) const { return ptr == other.ptr; }

    };

    NeighborsView() = default;

    NeighborsView(
        const size_t * first,
        size_t n,
        std::vector< Agent > * pop
    ) : first(first), n(n), pop(pop) {}

    // The size is carried rather than derived as `last - first`: an agent with
    // no ties has nothing to point at, and subtracting two null pointers is
    // undefined behaviour (they point into no array).
    iterator begin() const { return iterator(first, pop); }
    iterator end() const { return iterator(first + n, pop); }

    size_t size() const { return n; }
    bool empty() const { return n == 0u; }

    /// Id of the `k`-th neighbor, without touching the neighbor's `Agent`.
    /// @throws std::out_of_range if `k >= size()`.
    size_t id(size_t k) const
    {
        if (k >= n)
            throw std::out_of_range("NeighborsView::id(): index out of range.");
        return first[k];
    }

    /// The `k`-th neighbor.
    /// @throws std::out_of_range if `k >= size()`.
    Agent * agent(size_t k) const
    {
        if (k >= n)
            throw std::out_of_range("NeighborsView::agent(): index out of range.");
        return &pop->operator[](first[k]);
    }

    /// The ids of the `size()` neighbors, in order (like `std::vector::data()`:
    /// only valid for indices below `size()`; `nullptr` when there are none).
    /// For scans that must not touch the neighbors' `Agent`s.
    const size_t * ids() const { return first; }

};

/**
 * @brief Agent (agents)
 * 
 */
class Agent {
    friend class Model;
    friend class Virus;
    friend class Tool;
    friend class Queue;
    friend class AgentsSample;
    friend class DataBase;
    friend class layer::Network;
protected:

    unsigned int state = 0u;
    unsigned int state_prev = 0u; ///< For accounting, if need to undo a change.
    
    int state_last_changed = -1; ///< Last time the agent was updated.
    int id = -1;
    
    VirusPtr virus = nullptr;

    std::vector< ToolPtr > tools;

    void reset(); ///< Resets the agent to the initial state (no virus, no tools, state 0.)

    /**
     * @name The engine's changes (with the target state and queue)
     * @details -99 means the default: no change of state (tools), the
     * infection's target (a virus given from outside).
     */
    ///@{
    void add_tool(
        Model & model,
        const Tool & tool,
        epiworld_fast_int state_new,
        epiworld_fast_int queue
        );

    void set_virus(
        Model & model,
        const Virus & virus,
        epiworld_fast_int state_new,
        epiworld_fast_int queue
        );

    void rm_tool(
        Model & model,
        epiworld_fast_uint tool_idx,
        epiworld_fast_int state_new,
        epiworld_fast_int queue
    );

    void rm_tool(
        Model & model,
        ToolPtr & tool,
        epiworld_fast_int state_new,
        epiworld_fast_int queue
    );

    void rm_virus(
        Model & model,
        epiworld_fast_int state_new,
        epiworld_fast_int queue
    );

    void change_state(
        Model & model,
        epiworld_fast_uint new_state,
        epiworld_fast_int queue = 0
        );

    /// The engine's index of the agent's stage and status (see
    /// `Model::get_stage()` and `Model::get_status()`).
    unsigned int get_state() const;
    unsigned int get_state_prev() const;
    ///@}

public:

    Agent() = default;
    Agent(Agent && p);
    Agent(const Agent & p);
    Agent & operator=(const Agent & other_agent);
    ~Agent();

    /**
     * @name Add/Remove Virus/Tool
     *
     * Any of these is ultimately reflected at the end of the iteration. A
     * virus given to an agent takes it through the infection transition out
     * of its stage (see `NaturalHistory`); moving an agent otherwise is done
     * with `Model::fire()`.
     *
     * @param tool Tool to add
     * @param virus Virus to add
     */
    ///@{
    void add_tool(Model & model, const Tool & tool);
    void set_virus(Model & model, const Virus & virus);
    void rm_tool(Model & model, epiworld_fast_uint tool_idx);
    void rm_tool(Model & model, ToolPtr & tool);
    void rm_agent_by_virus(Model & model) = delete; ///< Agent removed by virus
    ///@}
    
    /**
     * @name Get the rates (multipliers) for the agent
     * 
     * @param v A pointer to a virus.
     * @return epiworld_double 
     */
    ///@{
    epiworld_double get_susceptibility_reduction(VirusPtr & v, Model & model);
    epiworld_double get_transmission_reduction(VirusPtr & v, Model & model);
    epiworld_double get_recovery_enhancer(VirusPtr & v, Model & model);
    epiworld_double get_death_reduction(VirusPtr & v, Model & model);
    ///@}

    int get_id() const; ///< Id of the individual

    VirusPtr & get_virus();
    const VirusPtr & get_virus() const;

    ToolPtr & get_tool(int i);
    ToolPtr & get_tool(std::string name);

    std::vector<ToolPtr> get_tools();
    const std::vector<ToolPtr> get_tools() const;
    size_t get_n_tools() const noexcept;

    /// @brief Applies the virus' mutation function (throws if the agent has no virus).
    void mutate_virus(Model & model);

    /// The last day the agent's stage or status changed.
    int get_state_last_changed() const;


    bool has_tool(epiworld_fast_uint t) const;
    bool has_tool(std::string_view name) const;
    bool has_tool(const Tool & t) const;
    bool has_virus(epiworld_fast_uint t) const;
    bool has_virus(std::string_view name) const;
    bool has_virus(const Virus & v) const;

    void print(Model & model, bool compressed = false) const;

    /**
     * @brief Access the j-th column of the agent
     * 
     * If an external array has been specified, then these two
     * functions can be used to access additional agent's features 
     * not included in the model.
     * 
     * @param j 
     * @param model Reference to the Model
     * @return double& 
     */
    ///@{
    double & operator()(size_t j, Model & model);
    double operator()(size_t j, const Model & model) const;
    ///@}

    bool operator==(const Agent & other) const;
    bool operator!=(const Agent & other) const {return !operator==(other);};

};



#endif
/*//////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

 End of -include/epiworld/agent-bones.hpp-

////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////*/


/*//////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

 Start of -include/epiworld/virus-bones.hpp-

////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////*/


#ifndef EPIWORLD_VIRUS_HPP
#define EPIWORLD_VIRUS_HPP

class Agent;

class Virus;

class Model;

class Tool;

/**
 * @brief Virus
 * 
 * @details
 * A virus is identified by its id, which the model assigns: `Model::add_virus()`
 * records a new variant (the founder of a lineage), and so does every
 * mutation (a child of the variant it changed). Tools can reduce what a
 * virus does, so its effects are a function of the agent.
 */
class Virus {
    friend class Agent;
    friend class Model;
    friend class DataBase;
    friend class Tool;
    friend class ContactLayer;
private:
    
    Agent * agent = nullptr;


    std::string virus_name = "unknown virus";
    int date = -99;
    int id   = -99;    
    int lineage_id = -99; ///< Id of the founding virus (kept across mutations).
    uint64_t lineage_bit = uint64_t(1) << 63; ///< Bit matched against Tool::target_mask.
    MutFun          mutation                 = nullptr;

    /// Parameters this variant reads in place of others (see `use_param()`)
    std::vector< std::pair< ParamRef, ParamRef > > param_redirects;

    /// Its natural history (shared by its copies; copied on the first edit)
    std::shared_ptr< NaturalHistory > nh_def = nullptr;
    NaturalHistory & nh_edit();
        
public:

    #ifdef EPI_DEBUG_VIRUS
    static std::atomic<int> counter_construct;          // Default and parameterized constructors
    static std::atomic<int> counter_copy_construct;     // Copy constructor
    static std::atomic<int> counter_move_construct;     // Move constructor
    static std::atomic<int> counter_copy_assign;        // Copy assignment
    static std::atomic<int> counter_move_assign;        // Move assignment
    static std::atomic<int> counter_destruct;           // Destructor
    #endif

    Virus();

    Virus(std::string name = "unknown virus");
    
    
    #ifdef EPI_DEBUG_VIRUS
    
    // Copy and move operations for debugging
    Virus(const Virus& other);                    // Copy constructor
    Virus(Virus&& other) noexcept;                // Move constructor
    Virus& operator=(const Virus& other);   // Copy assignment
    Virus& operator=(Virus&& other) noexcept; // Move assignment

    ~Virus();
    #endif
    
    /**
     * @brief Mutation
     * @details The function returns `true` when it changed the virus (it may
     * change its name or, with `use_param()`, the parameters it reads). The
     * model then records the virus as a new variant: a new id, whose parent is
     * the id it had, in the same lineage. Each mutation is a new variant, even
     * if two of them made the same change.
     */
    ///@{
    void mutate(Model * model);
    void set_mutation(MutFun fun);
    ///@}
    
    
    Agent * get_agent();
    void set_agent(Agent * p);
    
    void set_date(int d);
    int get_date() const;

    void set_id(int idx);
    int get_id() const;

    /**
     * @brief Id of the virus lineage
     * 
     * The id assigned to the founding virus when it was added to the model
     * (see `Model::add_virus()`). Mutations change the virus id but keep
     * the lineage id, so all variants of a virus share it. Tools target
     * viruses by lineage (see `Tool::add_target()`).
     * 
     * @return int The lineage id (-99 if the virus has not been recorded).
     */
    int get_lineage_id() const;

    void set_name(std::string name);
    std::string get_name() const;

    bool operator==(const Virus & other) const;
    bool operator!=(const Virus & other) const {return !operator==(other);};

    void print() const;


    /**
     * @name Natural history
     * @brief The disease's stages and the transitions between them, declared
     * on the virus (see `NaturalHistory` for each method).
     * @details `Model::add_virus()` gives the model the virus's stages (its
     * states). A copy of the virus (a strain, or a variant from a mutation)
     * keeps the natural history; it can read other parameters with
     * `use_param()`. The viruses of one model share their stages.
     */
    ///@{
    int add_stage(std::string label, unsigned int flags = 0u);
    void set_contact(int s, epiworld_double weight);
    TransitionRef infection(Nodes from, int to, Value per_contact);
    TransitionRef rate(Nodes from, int to, Value daily_probability);
    TransitionRef after(int from, int to, Duration d);
    TransitionRef external(Nodes from, int to, std::string label);

    /// The natural history (`nullptr` if the virus has none).
    const NaturalHistory * get_natural_history() const { return nh_def.get(); }
    ///@}

    /**
     * @brief This variant reads the model parameter `replacement` wherever a
     * transition of the natural history reads `param`.
     * @details Every parameter lives in the model; a variant changes a value
     * by pointing to another parameter (e.g., `"Recovery rate (Delta)"`).
     */
    void use_param(std::string param, std::string replacement);

    /// The parameter this variant reads in place of `id`.
    ParamId param_redirect(ParamId id, const Model & model) const;

    /// Throws if a parameter it reads (see `use_param()`) is not in `model`.
    void check_params(const Model & model) const;

    virtual ~Virus() = default;
    virtual std::unique_ptr<Virus> clone_ptr() const;

};

#endif
/*//////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

 End of -include/epiworld/virus-bones.hpp-

////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////*/


/*//////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

 Start of -include/epiworld/tool-bones.hpp-

////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////*/


#ifndef EPIWORLD_TOOL_BONES_HPP
#define EPIWORLD_TOOL_BONES_HPP

class Virus;

class Agent;

class Model;

class Tool;

/**
 * @brief Tools for defending the agent against the virus
 * 
 */
class Tool {
    friend class Agent;
    friend class Model;
protected:

    Agent * agent = nullptr;
    int pos_in_agent        = -99; ///< Location in the agent

    int date = -99;
    int id   = -99;
    std::string tool_name;
    

    ToolFun susceptibility_reduction = nullptr;
    ToolFun transmission_reduction   = nullptr;
    ToolFun recovery_enhancer        = nullptr;
    ToolFun death_reduction          = nullptr;


    /// Bitmask of targeted virus lineages (all ones: every virus).
    uint64_t target_mask = ~uint64_t(0);


    void set_agent(Agent * p, size_t idx);

public:

    /// Bit of a virus lineage in a target mask (contact layers use it too).
    static uint64_t target_bit(int lineage_id); ///< Throws for ids out of range.

    Tool();
    Tool(std::string name = "unknown tool");

    virtual ~Tool() = default;


    /**
     * @name Get and set the tool functions
     * 
     * @param v The virus over which to operate
     * @param fun the function to be used
     * 
     * @return epiworld_double 
     */
    ///@{
    virtual epiworld_double get_susceptibility_reduction(VirusPtr & v, Model * model);
    virtual epiworld_double get_transmission_reduction(VirusPtr & v, Model * model);
    virtual epiworld_double get_recovery_enhancer(VirusPtr & v, Model * model);
    virtual epiworld_double get_death_reduction(VirusPtr & v, Model * model);
    
    virtual void set_susceptibility_reduction_fun(ToolFun fun);
    virtual void set_transmission_reduction_fun(ToolFun fun);
    virtual void set_recovery_enhancer_fun(ToolFun fun);
    virtual void set_death_reduction_fun(ToolFun fun);

    virtual void set_susceptibility_reduction(std::string param);
    virtual void set_transmission_reduction(std::string param);
    virtual void set_recovery_enhancer(std::string param);
    virtual void set_death_reduction(std::string param);

    // Deleting pointer versions to avoid mistakes
    virtual void set_susceptibility_reduction(epiworld_double * prob) = delete;
    virtual void set_transmission_reduction(epiworld_double * prob) = delete;
    virtual void set_recovery_enhancer(epiworld_double * prob) = delete;
    virtual void set_death_reduction(epiworld_double * prob) = delete;

    virtual void set_susceptibility_reduction(epiworld_double prob);
    virtual void set_transmission_reduction(epiworld_double prob);
    virtual void set_recovery_enhancer(epiworld_double prob);
    virtual void set_death_reduction(epiworld_double prob);
    ///@}

    /**
     * @name Virus targets
     * 
     * @details
     * By default, a tool acts on every virus. Adding targets restricts the
     * tool (all four effects: susceptibility and transmission reduction,
     * recovery enhancer, and death reduction) to the listed virus lineages.
     * A lineage is a virus added with `Model::add_virus()` together with all
     * of its mutations, so a tool targeting a virus also acts on its
     * variants. Lineage ids are the virus ids assigned by
     * `Model::add_virus()`; only lineages 0 to 62 can be targeted.
     * 
     * @param lineage_id Id of the virus lineage (see
     * `Virus::get_lineage_id()`).
     * @param v A virus already added to the model.
     */
    ///@{
    void add_target(int lineage_id);
    void add_target(const Virus & v);
    void set_targets(const std::vector< int > & lineage_ids);
    std::vector< int > get_targets() const; ///< Empty if the tool acts on every virus.
    void clear_targets();
    bool targets(const Virus & v) const;
    ///@}

    void set_name(std::string name);
    virtual std::string get_name() const;

    Agent * get_agent();
    int get_id() const;
    void set_id(int id);
    void set_date(int d);
    int get_date() const;


    bool operator==(const Tool & other) const;
    bool operator!=(const Tool & other) const {return !operator==(other);};

    void print() const;


    virtual std::unique_ptr<Tool> clone_ptr() const; 

};

#endif
/*//////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

 End of -include/epiworld/tool-bones.hpp-

////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////*/


/*//////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

 Start of -include/epiworld/database-bones.hpp-

////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////*/


#ifndef EPIWORLD_DATABASE_BONES_HPP
#define EPIWORLD_DATABASE_BONES_HPP

class Model;

class Virus;

class UserData;

/**
 * @brief Statistical data about the process
 *
 * **Implementation details:**
 * <a href="../impl/data-collection-and-analysis.md">Data Collection and Analysis</a>,
 * <a href="../impl/generation-interval-and-reproductive-number.md">Generation Interval and Reproductive Number Calculation</a>
 *
 */
class DataBase {
    friend class Model;
private:
    Model * model;

    HospitalizationsTracker m_hospitalizations; ///< Tracker for hospitalization events

    // Variants, by id: the viruses added to the model first (founders, kept
    // even if removed, so ids stay put), then the run's mutations
    size_t n_founders = 0u;
    std::vector< std::string > virus_name;
    std::vector< int > virus_origin_date;
    std::vector< int > virus_parent_id;

    // Tools, by id
    std::vector< std::string > tool_name;
    std::vector< int > tool_origin_date;

    // {Variant 1: {state 1, state 2, etc.}, Variant 2: {...}, ...}
    std::vector< std::vector<int> > today_virus;

    // {Variant 1: {state 1, state 2, etc.}, Variant 2: {...}, ...}
    std::vector< std::vector<int> > today_tool;

    // {Susceptible, Infected, etc.}
    std::vector< int > today_total;

    // Totals
    int today_total_nviruses_active = 0;

    int sampling_freq = 1;

    // Variants history
    std::vector< int > hist_virus_date;
    std::vector< int > hist_virus_id;
    std::vector< epiworld_fast_uint > hist_virus_state;
    std::vector< int > hist_virus_counts;

    // Tools history
    std::vector< int > hist_tool_date;
    std::vector< int > hist_tool_id;
    std::vector< epiworld_fast_uint > hist_tool_state;
    std::vector< int > hist_tool_counts;

    // Overall hist
    std::vector< int > hist_total_date;
    std::vector< int > hist_total_nviruses_active;
    std::vector< epiworld_fast_uint > hist_total_state;
    std::vector< int > hist_total_counts;
    std::vector< int > hist_transition_matrix;

    // Transmission network
    std::vector< int > transmission_date;                 ///< Date of the transmission event
    std::vector< int > transmission_source;               ///< Id of the source
    std::vector< int > transmission_target;               ///< Id of the target
    std::vector< int > transmission_virus;              ///< Id of the variant
    std::vector< int > transmission_source_exposure_date; ///< Date when the source acquired the variant
    std::vector< int > transmission_layer;                ///< Id of the layer of the contact (-1: none, e.g., seeded)

    std::vector< int > transition_matrix;

    /**
     * @name Declared transitions (natural history)
     * @details How many agents took each declared transition, since the last
     * record (see `get_hist_transitions()`).
     */
    ///@{
    std::vector< int > transition_today;       ///< [transition]
    std::vector< int > hist_transition_date;
    std::vector< int > hist_transition_id;
    std::vector< int > hist_transition_counts;
    void record_declared_transition(int id) { ++transition_today[static_cast< size_t >(id)]; }
    ///@}

    UserData user_data;

    void update_state(
        epiworld_fast_uint prev_state,
        epiworld_fast_uint new_state,
        bool undo = false
    );

    void update_virus(
        epiworld_fast_uint virus_id,
        epiworld_fast_uint prev_state,
        epiworld_fast_uint new_state,
        bool undo = false
    );

    void update_tool(
        epiworld_fast_uint tool_id,
        epiworld_fast_uint prev_state,
        epiworld_fast_uint new_state,
        bool undo = false
    );

    void record_transition(epiworld_fast_uint from, epiworld_fast_uint to, bool undo);

    /**
     * @brief The status and the stage of a state of the engine (the one place
     * the pair is split; models without statuses have the status "Free").
     */
    void split_state(
        epiworld_fast_uint state,
        std::string & status,
        std::string & stage
    ) const;

    /// `split_state()` over a vector of states.
    void split_states(
        const std::vector< epiworld_fast_uint > & states,
        std::vector< std::string > * status,
        std::vector< std::string > * stage
    ) const;

    /// Today's total of a stage (`by_stage`) or of a status.
    int today_total_of(const std::string & name, bool by_stage) const;


public:

    #ifdef EPI_DEBUG
    int n_transmissions_potential = 0;
    int n_transmissions_today     = 0;
    #endif

    DataBase() = delete;
    DataBase(Model & m) : model(&m), m_hospitalizations(), user_data(m) {};
    DataBase(const DataBase & db);
    // DataBase & operator=(const DataBase & m);

    /**
     * @brief Records a new variant, giving the virus its id
     *
     * @param v The virus.
     * @param founder `true` when `Model::add_virus()` records the virus: it
     * starts its own lineage (and the variants of the last run are dropped).
     * Otherwise `v` has mutated: the new variant is a child of the id it had,
     * and its agent's counts move from the parent to it.
     */
    void record_virus(Virus & v, bool founder = false);
    void record_tool(Tool & t); ///< Gives the tool the next id.
    void reset();
    void drop_variants(); ///< Keeps the founders' records, dropping the run's variants.
    Model * get_model();
    void record();

    const std::vector< int > & get_nexposed() const;
    size_t size() const;

    /**
     * @name Get recorded information from the model
     *
     * @details The counts are by status and stage: one row per pair (and
     * day, and virus or tool, in the histories). Models without statuses
     * report the status "Free". A total by stage or by status is a sum over
     * the rows.
     *
     * @param stage A stage's name.
     * @param status A status's name.
     * @return In `get_today_total_stage(stage)` and
     * `get_today_total_status(status)`, how many agents are in that stage (in
     * any status) or in that status (in any stage) now. A stage and a status
     * may share a name; each function looks only among its own.
     * @return In `get_today_total`, the current counts of each pair.
     * @return In `get_today_virus`, the current counts of each pair for
     * each virus.
     * @return In `get_hist_total`, the time series of each pair.
     * @return In `get_hist_virus` and `get_hist_tool`, the time series of
     * each pair for each virus and tool.
     * @return In `get_active_cases`, the number of active cases (currently infected individuals)
     * for each virus at each point in time.
     */
    ///@{
    int get_today_total_stage(const std::string & stage) const;
    int get_today_total_status(const std::string & status) const;
    void get_today_total(
        std::vector< std::string > * status,
        std::vector< std::string > * stage,
        std::vector< int > * counts
    ) const;

    void get_today_virus(
        std::vector< std::string > & status,
        std::vector< std::string > & stage,
        std::vector< int > & id,
        std::vector< int > & counts
    ) const;

    void get_today_transition_matrix(
        std::vector< int > & counts
    ) const;

    void get_hist_total(
        std::vector< int > * date,
        std::vector< std::string > * status,
        std::vector< std::string > * stage,
        std::vector< int > * counts
    ) const;

    void get_hist_virus(
        std::vector< int > & date,
        std::vector< int > & id,
        std::vector< std::string > & status,
        std::vector< std::string > & stage,
        std::vector< int > & counts
    ) const;

    void get_hist_tool(
        std::vector< int > & date,
        std::vector< int > & id,
        std::vector< std::string > & status,
        std::vector< std::string > & stage,
        std::vector< int > & counts
    ) const;

    /**
     * @brief How many agents took each declared transition of the natural
     * history, by day.
     * @details One row per recorded day and transition, the transitions in
     * the order they were declared. Seeding and initial placements are not
     * transitions. Requires a natural history.
     */
    void get_hist_transitions(
        std::vector< int > & date,
        std::vector< std::string > & transition,
        std::vector< int > & counts
    ) const;

    void get_hist_transition_matrix(
        std::vector< std::string > & status_from,
        std::vector< std::string > & stage_from,
        std::vector< std::string > & status_to,
        std::vector< std::string > & stage_to,
        std::vector< int > & date,
        std::vector< int > & counts,
        bool skip_zeros
    ) const;

    void get_active_cases(
        std::vector< int > & date,
        std::vector< int > & virus_id,
        std::vector< int > & count
    ) const;

    void get_outbreak_size(
        std::vector< int > & date,
        std::vector< int > & virus_id,
        std::vector< int > & size
    ) const;
    ///@}

    /**
     * @brief Get the transmissions object
     *
     * @param date
     * @param source
     * @param target
     * @param virus
     * @param source_exposure_date
     * @param layer Id of the layer of the contact that transmitted the virus
     * (-1 for infections that did not come from a contact, e.g., seeds)
     */
    ///@{
    void get_transmissions(
        std::vector<int> & date,
        std::vector<int> & source,
        std::vector<int> & target,
        std::vector<int> & virus,
        std::vector<int> & source_exposure_date,
        std::vector<int> & layer
    ) const;

    void get_transmissions(
        int * date,
        int * source,
        int * target,
        int * virus,
        int * source_exposure_date,
        int * layer
    ) const;
    ///@}

    void write_data(
        std::string fn_virus_info,
        std::string fn_virus_hist,
        std::string fn_tool_info,
        std::string fn_tool_hist,
        std::string fn_total_hist,
        std::string fn_transmission,
        std::string fn_transition,
        std::string fn_reproductive_number,
        std::string fn_generation_time,
        std::string fn_active_cases,
        std::string fn_outbreak_size,
        std::string fn_hospitalizations
        ) const;

    /**
     * @brief In-memory tables of this simulation, one per requested output
     * @param whats Names of the outputs (see `run_output_names()`).
     * @details Each table has exactly the columns and rows of the CSV that
     * `write_data()` writes for it. Throws `std::invalid_argument` on an
     * unknown name.
     */
    RunOutputs get_run_outputs(const std::vector< std::string > & whats) const;

    /***
     * @brief Record a transmission event
     * @param i,j Integers. Id of the source and target agents.
     * @param virus Integer. Id of the virus.
     * @param i_expo_date Integer. Date when the source agent was infected.
     * @details
     * If i is -1, then it means that the agent was assigned a virus at the
     * beginning of the simulation.
     */
    void record_transmission(int i, int j, int virus, int i_expo_date, int layer = -1);

    size_t get_n_viruses() const; ///< Get the number of viruses
    size_t get_n_tools() const; ///< Get the number of tools

    void set_user_data(std::vector< std::string > names);
    void add_user_data(std::vector< epiworld_double > x);
    void add_user_data(epiworld_fast_uint j, epiworld_double x);
    UserData & get_user_data();


    /**
     * @brief Computes the reproductive number of each case
     *
     * @details By definition, whereas it computes R0 (basic reproductive number)
     * or Rt/R (the effective reproductive number) will depend on whether the
     * virus is allowed to circulate naïvely or not, respectively.
     *
     * @param fn File where to write out the reproductive number.
     * @details
     * In the case of `MapVec_type<int,int>`, the key is a vector of 3 integers:
     * - Virus id
     * - Source id
     * - Date when the source was infected
     * 
     * The seed cases are those with source id != -1 and date when the source
     * was infected == 0. Cases with source id == -1 is equivalent to the 
     * Model's reproductive number, this is, the initial number of cases
     * at the beginning of the simulation.
     */
    ///@{
    MapVec_type<int,int> get_reproductive_number() const;

    void get_reproductive_number(
        std::string fn
        ) const;
    ///@}

    /**
     * @brief Calculates the transition probabilities
     * @param print Print the transition matrix.
     * @param normalize Normalize the transition matrix. Otherwise,
     * it returns raw counts.
     * @details
     * The transition matrix is the matrix of the counts of transitions
     * from one state to another. So the ij-th element of the matrix is
     * the number of transitions from state i to state j (when not normalized),
     * or the probability of transitioning from state i to state j
     * (when normalized).
     * @return std::vector< epiworld_double >
     */
    std::vector< epiworld_double > get_transition_probability(
        bool print = true,
        bool normalize = true
    ) const;

    bool operator==(const DataBase & other) const;
    bool operator!=(const DataBase & other) const {return !operator==(other);};

    /**
     * Calculates the generating time
     * @param agent_id,virus_id,time,gentime vectors where to save the values
     *
     * @details
     * The generation time is the time between the infection of the source and
     * the infection of the target.
    */
   ///@{
    void get_generation_time(
        std::vector< int > & agent_id,
        std::vector< int > & virus_id,
        std::vector< int > & time,
        std::vector< int > & gentime
    ) const; ///< Get the generation time

    void get_generation_time(
        std::string fn
    ) const; ///< Write the generation time to a file
    ///@}

    /**
     * @brief Record a hospitalization event for an agent.
     * 
     * @param agent Reference to the agent being hospitalized.
     * 
     * @details
     * For each hospitalization, the method records:
     * - The current date from the model
     * - The virus ID from the agent's virus
     * - For each tool the agent has, a separate record with weight = 1/N
     *   where N is the number of tools
     * - If the agent has no tools, a single record with tool_id = -1 and
     *   weight = 1.0
     */
    void record_hospitalization(Agent & agent);

    /**
     * @brief Get the full time series of hospitalization data.
     * 
     * @param date Output vector for dates.
     * @param virus_id Output vector for virus IDs.
     * @param tool_id Output vector for tool IDs.
     * @param count Output vector for counts (number of hospitalized individuals).
     * @param weight Output vector for summed weights (fractional contribution
     *   based on tool distribution).
     * 
     * @details
     * Returns the full time series of hospitalization data. For each unique 
     * (virus_id, tool_id) combination observed, returns an entry for every 
     * day from 0 to ndays-1.
     * 
     * The `count` vector contains the actual number of individuals hospitalized 
     * for that (date, virus_id, tool_id) combination. This is useful for 
     * answering questions like "how many total people were hospitalized?" 
     * regardless of their tools.
     * 
     * The `weight` vector contains fractional contributions: if an agent has N 
     * tools, each tool gets weight = 1/N. Summing weights across all tool_ids 
     * for a given date and virus_id gives the total number of hospitalizations.
     */
    void get_hospitalizations(
        std::vector<int> & date,
        std::vector<int> & virus_id,
        std::vector<int> & tool_id,
        std::vector<int> & count,
        std::vector<double> & weight
    ) const;

};


#endif
/*//////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

 End of -include/epiworld/database-bones.hpp-

////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////*/


/*//////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

 Start of -include/epiworld/queue-bones.hpp-

////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////*/


#ifndef EPIWORLD_QUEUE_BONES_HPP
#define EPIWORLD_QUEUE_BONES_HPP

/**
 * @brief Controls which agents are verified at each step
 * 
 * @details The idea is that only agents who are either in
 * an infected state or have an infected neighbor should be
 * checked. Otherwise it makes no sense (no chance to recover
 * or capture the disease).
 * 
 * **Implementation details:**
 * <a href="../impl/queueing-system.md">Queueing System</a>
 * 
 */
class Queue
{
    friend class Model;

private:

    /**
     * @brief Count of ego's neighbors in queue (including ego)
     */
    std::vector< epiworld_fast_int > active;

    /**
     * @brief Outstanding `Everyone` registrations per agent.
     *
     * @details `active[i]` is `everyone[i]` plus the sum of `everyone[j]` over
     * the agents `j` whose neighbor lists contain `i`, in every network layer
     * of the model (`operator+=` credits the agents in the registered agent's
     * own lists). In an undirected network those are `i`'s neighbors; in a
     * directed one they are the agents that list `i`. So this is what a tie is worth to the far end of
     * it: adding an edge next to a registered agent is a `+everyone[]`
     * over there, and removing one is the mirror (see `notify_edge_added`).
     *
     * Keeping the count explicit -- rather than inferring "is this agent
     * contributing?" from whether it carries a virus -- means the bookkeeping
     * stays right no matter what registered the agent: a virus, a tool, or an
     * explicit queue argument to `Agent::change_state`.
     */
    std::vector< epiworld_fast_int > everyone;

    /**
     * @brief One bit per agent: bit `i` is set iff `active[i] != 0`.
     *
     * @details This is what lets `Model::update_state()` visit only the agents
     * in the queue, in ascending id order, without scanning the whole
     * population: it walks the set bits (see `for_each_nonzero`). Every change
     * to `active` goes through `shift()`, which keeps the two in step.
     */
    std::vector< uint64_t > bits;

    Model * model = nullptr;
    int n_in_queue = 0;

    /// @brief Adds `n` to `active[id]`, keeping `n_in_queue` and `bits` in step.
    void shift(size_t id, epiworld_fast_int n);

    /// @brief Whether the counters have been sized to cover these two agents.
    bool tracks(size_t a, size_t b) const;

    /// @brief Adds `n` to the counts of `id`'s neighbors in every network
    /// and of its group-mates in every within-group mixing layer.
    void shift_neighbors(size_t id, epiworld_fast_int n);

    // Auxiliary variable that checks how many steps
    // left are there
    // int n_steps_left;
    // bool queuing_started   = false;

public:

    void operator+=(Agent * p);
    void operator-=(Agent * p);

    /**
     * @brief How many registrations cover agent `i` (itself or a neighbor).
     *
     * @details Read-only: the count must only change through `+=`, `-=` and
     * the edge notifications, which keep the ordered set of queued agents in
     * step with it.
     */
    epiworld_fast_int operator[](epiworld_fast_uint i) const;

    /**
     * @brief Calls `f(i)` for every agent with a non-zero count, in ascending
     * id order.
     *
     * @details Costs O(N / 64 + number of queued agents), instead of a scan of
     * the whole population. The count of an agent is read when the walk
     * reaches it, so if `f` changes the queue (e.g., a state function that
     * adds a tie), agents further ahead see the change -- exactly what a plain
     * `for (i = 0; i < N; ++i) if (queue[i] ...)` loop would do.
     */
    template< typename F >
    void for_each_nonzero(F && f);

    /**
     * @name Keep the queue in step with a change to the contact network
     *
     * @details The queue counts, for every agent, how many of its neighbors are
     * registered as active. That count is built when an agent is registered
     * (`operator+=`) and unwound when it is deregistered (`operator-=`), both
     * walking the agent's neighbors *as they are at that moment*. Changing the
     * network in between would leave the two walks disagreeing, and an agent
     * whose count drifted to zero is silently skipped by
     * `Model::update_state()`.
     *
     * This keeps the counts exact as the change happens, in constant time: an
     * agent that gains a tie to another hands it whatever it contributes, and
     * one that loses the tie takes it back.
     *
     * @param a,b The two ends of the tie that was just added or removed.
     * @param da,db How `a`'s list changed (`+1` gained `b`, `-1` lost it, `0`
     * unchanged), and `b`'s.
     */
    void notify_ties_changed(size_t a, int da, size_t b, int db);

    /**
     * @brief Keep the queue in step with `layer::Network::swap_ties()`.
     *
     * @details `a`'s tie to `b` and `c`'s tie to `d` traded ends: (a-b), (c-d)
     * became (a-d), (c-b). Nobody's degree changed, only whose registrations
     * reach whom: `b` now gets what `c` contributes instead of what `a` does,
     * and `d` the reverse. In an undirected network the other ends traded too,
     * so `a` gets what `d` contributes instead of `b`, and `c` the reverse.
     * Four shifts, whatever the degrees.
     *
     * In a directed network only `a`'s and `c`'s lists change, and the queue
     * follows each agent's own list (`operator+=`), so only `b`'s and `d`'s
     * counts change.
     *
     * @param a,c The agents whose neighbors were swapped.
     * @param b,d Their neighbors before the swap (`b` was `a`'s, `d` was `c`'s).
     * @param directed Whether only `a`'s and `c`'s lists changed.
     */
    void notify_edges_swapped(
        size_t a,
        size_t b,
        size_t c,
        size_t d,
        bool directed
    );

    /**
     * @brief Keep the queue in step with an agent changing groups in a
     * within-group mixing layer.
     *
     * @details A group works like a clique of ties for the queue: `a` leaving
     * `from` is a tie lost to each of its members, and joining `to` a tie
     * gained to each (`from` still lists `a`, `to` does not yet).
     *
     * @param a The agent that moved.
     * @param from,to Members of its old and new groups (`nullptr`: none).
     */
    void notify_group_changed(
        size_t a,
        const std::vector< size_t > * from,
        const std::vector< size_t > * to
    );

    // void initialize(Model * m, Agent * p);
    void reset();

    bool operator==(const Queue & other) const;
    bool operator!=(const Queue & other) const {return !operator==(other);};

    static const int NoOne    = 0;
    static const int OnlySelf = 1;
    static const int Everyone = 2;

};

#endif
/*//////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

 End of -include/epiworld/queue-bones.hpp-

////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////*/


/*//////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

 Start of -include/epiworld/globalevent-bones.hpp-

////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////*/


#ifndef EPIWORLD_GLOBALEVENT_BONES_HPP
#define EPIWORLD_GLOBALEVENT_BONES_HPP

#include <string>
// (already included include/epiworld/config.hpp)

/**
 * @brief Template for a Global Event
 * @details Global events are functions that Model executes
 * at the end of a day.
 * 
 */
class GlobalEvent
{
private:
    GlobalFun fun = nullptr;
    std::string name = "A global action";
    int day = -99;
public:

    GlobalEvent() {};

    /**
     * @brief Construct a new Global Event object
     * 
     * @param fun A function that takes a Model * as argument and returns void.
     * @param name A descriptive name for the action.
     * @param day The day when the action will be executed. If negative, it will be executed every day.
     */
    GlobalEvent(GlobalFun fun, std::string name, int day = -99);
    
    virtual ~GlobalEvent() = default;

    virtual void operator()(Model * m, int day);

    /**
     * @brief Prepare the event for a run.
     *
     * @details Called by `Model::reset()` at the end of every reset, i.e. once
     * per run and once per replicate of `Model::run_multiple()`, with the run's
     * RNG already seeded and the agents, tools, and viruses distributed. Events
     * that need to set themselves up on the model they are running in -- the
     * moment before day 1 -- do it here; the default does nothing.
     *
     * @param m The model the event belongs to.
     */
    virtual void reset(Model * m);

    void set_name(std::string name);
    std::string get_name() const;

    void set_day(int day);
    int get_day() const;
    
    void print() const;

    // Comparison operators
    bool operator==(const GlobalEvent & other) const;
    bool operator!=(const GlobalEvent & other) const;

    virtual std::unique_ptr<GlobalEvent> clone_ptr() const;

};



#endif
/*//////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

 End of -include/epiworld/globalevent-bones.hpp-

////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////*/


// (already included include/epiworld/contacttracing-bones.hpp)
/*//////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

 Start of -include/epiworld/postsampling-bones.hpp-

////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////*/


#ifndef EPIWORLD_POSTSAMPLING_BONES_HPP
#define EPIWORLD_POSTSAMPLING_BONES_HPP

#include <cstdint>
#include <cstddef>
#include <functional>
#include <vector>

class Agent;
class Model;

/**
 * @brief Read-only view of the agents one infectious agent was in contact with.
 *
 * @details Passed to a `PostSamplingFun`. It points into the model's scratch
 * storage, so it is valid only during the callback: do not keep it (copy the
 * ids if you need them later). The contacts are a multiset: repeated contacts
 * appear repeatedly, and contacts that did not transmit are included. The
 * order of the contacts within the view is unspecified.
 */
class SampledContactsView {
private:
    const uint32_t * first = nullptr;
    size_t n = 0u;
public:
    SampledContactsView() = default;
    SampledContactsView(const uint32_t * first_, size_t n_) : first(first_), n(n_) {}
    const uint32_t * begin() const { return first; }
    const uint32_t * end() const { return first + n; }
    size_t size() const { return n; }
    bool empty() const { return n == 0u; }
    size_t operator[](size_t i) const { return first[i]; }
};

/**
 * @brief Callback run after the contacts of a step were sampled.
 *
 * @details Called once per infectious agent that had at least one sampled
 * contact in the step, in ascending order of the agent's id, after all the
 * susceptible agents were updated and right before the events are applied.
 * The agents still have the state they had when the contacts were sampled;
 * events the callback queues are applied in the same step.
 *
 * Arguments: the infectious agent, the ids of the agents it was in contact
 * with (see `SampledContactsView`), and the model.
 */
using PostSamplingFun = std::function<
    void(Agent *, const SampledContactsView &, Model *)
>;

/**
 * @brief Model-local storage used to batch sampled contacts.
 *
 * @details Pairs `(infectious, contacted)` are appended while sampling and
 * grouped by infectious agent once, at the end of the sampling phase. Lengths
 * are cleared, not released, between steps. Copies of a model start with empty
 * scratch (nothing is shared or copied); the model sizes `counts` for its
 * population the first time it needs it (see `Model::post_sampling_dispatch()`).
 */
struct PostSamplingScratch {

    std::vector< uint32_t > pairs;    ///< Flat: infectious, contacted, ...
    std::vector< uint32_t > grouped;  ///< Contacted ids, by infectious agent
    std::vector< uint32_t > touched;  ///< Distinct infectious ids (sorted)
    std::vector< size_t > starts;     ///< [touched] Where the batch starts
    std::vector< uint32_t > counts;   ///< [agent] Zero between steps

    PostSamplingScratch() = default;
    PostSamplingScratch(const PostSamplingScratch &) {}
    PostSamplingScratch & operator=(const PostSamplingScratch &) { reset(0u); return *this; }

    /// Empties the batch and restores the invariant of `counts` (all zero).
    void clear()
    {
        for (auto id : touched)
            counts[id] = 0u;
        pairs.clear();
        grouped.clear();
        touched.clear();
        starts.clear();
    }

    /// Sizes the storage for a population (call at reset).
    void reset(size_t n_agents)
    {
        pairs.clear();
        grouped.clear();
        touched.clear();
        starts.clear();
        counts.assign(n_agents, 0u);
    }

};

#endif
/*//////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

 End of -include/epiworld/postsampling-bones.hpp-

////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////*/


/*//////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

 Start of -include/epiworld/assign.hpp-

////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////*/


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
/*//////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

 End of -include/epiworld/assign.hpp-

////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////*/



class AgentsSample;

class AdjList;

inline std::function<void(size_t,Model*)> make_save_run(
    std::string fmt = "%03lu-episimulation.csv",
    bool total_hist = true,
    bool virus_info = false,
    bool virus_hist = false,
    bool tool_info = false,
    bool tool_hist = false,
    bool transmission = false,
    bool transition = false,
    bool reproductive = false,
    bool generation = false,
    bool active_cases = false,
    bool outbreak_size = false,
    bool hospitalizations = false
    );

/**
 * @brief Read-only view of a contiguous run of agent ids.
 *
 * @details Returned by `Model::get_agents_in_state()`. It points into the
 * model's index, so it is invalidated by the next step (or anything else that
 * moves agents between states).
 */
class AgentIdsView {
private:
    const size_t * first = nullptr;
    size_t n = 0u;
public:
    AgentIdsView() = default;
    AgentIdsView(const size_t * first_, size_t n_) : first(first_), n(n_) {}
    const size_t * begin() const { return first; }
    const size_t * end() const { return first + n; }
    size_t size() const { return n; }
    bool empty() const { return n == 0u; }
    size_t operator[](size_t i) const { return first[i]; }
};

/**
 * @brief Core class of epiworld.
 *
 * The model class provides the wrapper that puts together `Agent`, `Virus`, and
 * `Tools`.
 *
 * virus and human sequence is represented as numeric vectors (if needed.)
 */
class Model {
    friend class Agent;
    friend class AgentsSample;
    friend class DataBase;
    friend class Queue;
    friend class layer::Network;
    friend class layer::Mixing;

protected:

    std::string name = ""; ///< Name of the model

    DataBase db = DataBase(*this);

    std::vector< Agent > population = {};

    /**
     * @brief Creates the population: `n` agents, in the baseline state.
     * @details The models' constructors call it; the population is set once,
     * when the model is built (a different size is a different model).
     */
    void set_population_size(size_t n);

    /**
     * @name Auxiliary variables for AgentsSample iterators
     *
     * @details These variables+objects are used by the AgentsSample
     * class for building efficient iterators over agents. The idea is to
     * reduce the memory allocation, so only during the first call of
     * AgentsSample::AgentsSample(Model) these vectors are allocated.
     */
    ///@{
    std::vector< Agent * > sampled_population;
    size_t sampled_population_n = 0u;
    std::vector< size_t > population_left;
    size_t population_left_n = 0u;
    ///@}

    /**
     * @name Agents features
     *
     * @details Optionally, a model can include an external data source
     * pointing to agents information. The data can then be access through
     * the `Agent::operator()` method.
     *
     */
    ///@{
    double * agents_data = nullptr;
    size_t agents_data_ncols = 0u;
    ///@}

    std::vector< VirusPtr > viruses = {};
    std::vector< ToolPtr > tools = {};
    std::vector< Assign > virus_assign = {}; ///< [virus] Who gets it at each reset
    std::vector< Assign > tool_assign = {};  ///< [tool] Who gets it at each reset

    std::shared_ptr< epi_xoshiro256ss > engine = std::make_shared< epi_xoshiro256ss >();

    epiworld_double runifd_a = 0.0;
    epiworld_double runifd_b = 1.0;
    std::normal_distribution<>       rnormd      =
        std::normal_distribution<>(0.0);
    std::gamma_distribution<>        rgammad     =
        std::gamma_distribution<>();
    std::lognormal_distribution<>    rlognormald =
        std::lognormal_distribution<>();
    std::exponential_distribution<>  rexpd       =
        std::exponential_distribution<>();
    std::binomial_distribution<> rbinomd         =
        std::binomial_distribution<>();
    int rbinomd_n = 1;
    epiworld_double rbinomd_fast_lambda = 0.0;
    bool rbinomd_use_poisson = false;
    std::negative_binomial_distribution<> rnbinomd =
        std::negative_binomial_distribution<>();
    std::geometric_distribution<> rgeomd          =
        std::geometric_distribution<>();
    std::poisson_distribution<> rpoissd           =
        std::poisson_distribution<>();

    // Parameter values, by position, and each name's position. Parameters are
    // never removed, so positions (ParamId) stay valid. The comparator is
    // transparent (std::less<>) so names can be looked up as
    // std::string_view without building a std::string.
    std::vector< epiworld_double > param_values;
    std::map<std::string, size_t, std::less<> > param_index;
    uint32_t param_layout_id = new_param_layout_id();
    epiworld_fast_uint ndays = 0;
    Progress pb;

    std::vector< UpdateFun >    state_fun = {};                  ///< Functions to update states
    std::vector< std::string >        states_labels = {};              ///< Labels of the states

    epiworld_fast_uint nstates = 0u;

    bool verbose     = true;
    int current_date = 0;

    // True while run() is driving the day loop, so get_ndays() is the run's
    // actual horizon. Not copied: a copy of the model is not in a run.
    bool running = false;

    void set_running(bool value); ///< Also tells the layers (see `ContactLayer`)

    void dist_tools();
    void dist_virus();

    std::chrono::time_point<std::chrono::steady_clock> time_start;
    std::chrono::time_point<std::chrono::steady_clock> time_end;

    // std::chrono::milliseconds
    std::chrono::duration<epiworld_double,std::micro> time_elapsed =
        std::chrono::duration<epiworld_double,std::micro>::zero();
    epiworld_fast_uint n_replicates = 0u;
    int last_seed = 0;
    void chrono_start();
    void chrono_end();

    std::vector<GlobalEventPtr> globalevents;

    /// Contact layers (see `add_layer()`), deep-copied with the model
    std::vector< std::shared_ptr< ContactLayer > > layers;

    /**
     * @name Network layers
     *
     * @details The network layers among `layers`, in order. Susceptible agents
     * pull from all of them, the queue and pushing follow all of their ties,
     * and the agents-by-state index counts an agent's ties across them (its
     * degree).
     */
    ///@{
    std::vector< layer::Network * > network_layers;
    std::vector< layer::Mixing * > group_layers; ///< Within-group mixing layers (the queue follows them)
    std::vector< size_t > layer_contacts; ///< Scratch of `sample_virus()`
    bool pull_only = false; ///< See `has_pull_only_layer()`
    bool queue_blind = false; ///< See `has_queue_blind_layer()`
    int sampled_layer = -1; ///< See `take_sampled_layer()`

    /// Adds the exposures of `contacts` to the arrays; returns their number.
    size_t add_exposures(
        Agent * agent,
        const ContactLayer * layer,
        const size_t * contacts,
        size_t n,
        size_t n_exposures
    );

    /// One draw over the first `n_exposures` exposures.
    Virus * draw_exposure(size_t n_exposures);
    void layers_attach(); ///< Points the layers at this model, lists the networks
    void layers_classify(); ///< Sets `pull_only`, `queue_blind` and `group_layers`
    void layers_detach(); ///< Leaves the layers without a model (they may outlive it)
    void add_layer_ptr(std::shared_ptr< ContactLayer > layer);
    size_t network_degree(size_t id) const; ///< Ties of agent `id`, all networks

    /**
     * Whether some layer's contacts are not seen from both ends or not by the
     * queue: a directed network (a tie is kept by its source only) or a layer
     * that is not a network. The model then pulls, and visits every agent
     * instead of using the queue.
     */
    bool has_pull_only_layer() const { return pull_only; }

    /**
     * Whether some layer's contacts are not seen by the queue (see
     * `ContactLayer::supports_queue()`): a directed network, or a mixing layer
     * with a contact matrix. The model then visits every agent.
     */
    bool has_queue_blind_layer() const { return queue_blind; }

    /// Agent `i` moved from `from` to `to` in a within-group mixing layer.
    void group_member_moved(
        size_t i,
        const std::vector< size_t > * from,
        const std::vector< size_t > * to
    );

    /// A network layer changed the ties of `i` and `j` by `di` and `dj`.
    void network_tie_changed(size_t i, int di, size_t j, int dj);

    /// A network layer swapped two ties (see `layer::Network::swap_ties()`).
    void network_ties_swapped(
        size_t i, size_t j, size_t k, size_t l, bool directed
    );
    ///@}

    Queue queue;
    bool use_queuing   = true;
    size_t sim_id = 0u;
    void set_sim_id(size_t id);

    PostSamplingFun post_sampling_fun = nullptr; ///< See set_post_sampling()
    bool post_sampling_on = false;                     ///< post_sampling_fun is set
    PostSamplingScratch post_sampling_scratch;         ///< Never copied
    void post_sampling_dispatch();
    void post_sampling_prepare_scratch(); ///< Sizes the scratch for the population

    /**
     * @name Agents by state
     *
     * @details All agent ids live in one array, `state_order`, grouped by
     * state: the agents in state `s` are `state_order[state_start[s]]` up to
     * (not including) `state_order[state_start[s + 1]]`, in no particular
     * order, and `state_member_pos[i]` is where agent `i` sits. One contiguous
     * block of N ids, whatever the number of states. Moving an agent from
     * state `a` to state `b` walks it across the blocks in between, one swap
     * per block boundary: O(|a - b|).
     *
     * Per state, the index also keeps the sum of the members' degrees, how
     * many of them carry a virus, and the sum of those carriers' degrees.
     *
     * Built in `reset()` and kept current in `events_run()` -- the only place
     * where an agent's state or virus changes during a run -- and by the
     * network layers' edits. The degree sums let the transmission step
     * compare the cost of pushing and pulling in O(number of states).
     */
    ///@{
    std::vector< size_t > state_order;       ///< [N] Agent ids, grouped by state
    std::vector< size_t > state_start;       ///< [nstates + 1] Where each state's block starts
    std::vector< size_t > state_member_pos;  ///< [agent] Position in state_order
    std::vector< unsigned int > agent_state;    ///< [agent] Copy of Agent::state, compact for scans
    std::vector< char > agent_carrier;          ///< [agent] 1 if Agent::virus is set, compact for scans
    std::vector< size_t > state_degree;
    std::vector< size_t > state_carriers;
    std::vector< size_t > state_carrier_degree;
    bool state_index_ready = false;

    void state_index_build();
    void state_index_update(Agent * p, unsigned int state_old, bool had_virus);
    void state_index_move(size_t id, unsigned int state_old, unsigned int state_new);
    AgentIdsView state_index_members(size_t state) const;
    void state_index_degree(size_t id, int delta);
    ///@}

    /**
     * @name Infectious and excluded states
     * @details Derived from the natural history and the statuses (see
     * `nh_build_states()`). The per-state masks are built at `reset()`.
     */
    ///@{
    std::vector< int > infectious_states;     ///< Empty: every state
    std::vector< int > excluded_states;
    std::vector< bool > state_not_infectious; ///< [state] Empty if every state is infectious
    std::vector< char > state_excluded;       ///< [state]
    void state_masks_build();
    ///@}

    /**
     * @name Network transmission
     *
     * @details See `set_transmission_mode()` and
     * `model-meat-transmission.hpp`. The push scratch space is per model and
     * is not copied: a copy sizes its own the first time it pushes.
     */
    ///@{
    TransmissionMode transmission_mode = TransmissionMode::automatic;
    TransmissionMode transmission_mode_last = TransmissionMode::pull;
    double transmission_kappa = EPI_DEFAULT_TRANSMISSION_KAPPA;

    struct PushTarget {
        size_t id;
        double odds;                // Sum of p / (1 - p) over the contacts
        unsigned int n_certain;     // Contacts with p >= 1
        Virus * candidate;    // Infector's virus drawn so far
        int layer;                  // Layer of the candidate's contact
    };

    std::vector< char > push_pushable;       ///< [state] Infected by a default infection only
    std::vector< char > push_source_ok;      ///< [state] Infectious, and some state is pushable
    std::vector< int >  push_slot;           ///< [agent] Index in push_targets, or -1
    std::vector< PushTarget > push_targets;
    std::vector< uint64_t > push_visit;      ///< [agent bit] To update after a push
    std::vector< uint64_t > push_sources;    ///< [agent bit] Carriers that push this step

    bool transmission_prepare();
    bool transmission_choose_push() const;
    void transmission_push();
    template<bool Record> void transmission_push_impl();
    void transmission_update_others();
    ///@}

    /**
     * @name Natural history
     *
     * @details See `naturalhistory-bones.hpp`. The first virus with a natural
     * history installs it (`add_virus()`); the model's copy of that virus
     * shares it. It is shared by the model's copies (it does not change once
     * installed); what it stores per agent is the model's, and is reset with
     * it.
     */
    ///@{
    std::shared_ptr< const NaturalHistory > nh = nullptr;
    std::vector< int > nh_until;    ///< [agent] Day the stage's duration ends
    std::vector< int > nh_entered;  ///< [agent * stage] Day entered, this infection (-1: not)
    std::vector< int > nh_episode;  ///< [agent] Infections so far
    std::vector< char > nh_queue;   ///< [stage] Queue mode: 0 none, 1 self, 2 everyone
    Assign nh_initial;        ///< See set_initial()
    int nh_pending = -1;            ///< Transition of the next event (see _add_event())

    /// Event: a seeded infection, and an initial placement (status transitions
    /// are `-2 - index`, see `TransitionRef`)
    static constexpr int NH_SEED = std::numeric_limits< int >::min();
    static constexpr int NH_INITIAL = std::numeric_limits< int >::min() + 1;

    /// The statuses (`nullptr`: one status, "Free")
    std::shared_ptr< const Statuses > st = nullptr;
    std::vector< int > nh_status_until; ///< [agent] Day the status's duration ends
    std::vector< int > nh_status_since; ///< [agent] Day the status was entered
    std::vector< int > nh_status_sod;   ///< [agent] Status at the start of the day it last changed

    struct Reaction {
        Trigger on;
        int transition;                          ///< Fired, or (-1) ...
        std::function< void(Agent &, Model &) > fun; ///< ... called
    };
    std::vector< Reaction > nh_reactions;

    size_t nh_nstatus() const { return st ? st->get_n_stages() : 1u; }
    int nh_stage_of(unsigned int cell) const { return static_cast< int >(cell / nh_nstatus()); }
    int nh_status_of(unsigned int cell) const { return static_cast< int >(cell % nh_nstatus()); }
    unsigned int nh_cell(int stage, int status) const
    {
        return static_cast< unsigned int >(stage) * static_cast< unsigned int >(nh_nstatus()) +
            static_cast< unsigned int >(status);
    }
    static bool nh_is_status_tr(int t) { return (t <= -2) && (t > NH_INITIAL); }
    static int nh_status_tr(int t) { return -2 - t; }
    int nh_db_id(int t) const
    {
        return nh_is_status_tr(t) ?
            static_cast< int >(nh->get_n_transitions()) + nh_status_tr(t) : t;
    }
    void nh_build_states();
    void nh_react(Agent & p, int transition, int stage_old);
    bool nh_removed(int stage) const;

    void nh_install(std::shared_ptr< NaturalHistory > def);
    void nh_reset();
    void nh_step(Agent & p);

    /// The natural history whose values apply to `p`: its virus's, or (no
    /// virus) the model's
    const NaturalHistory & nh_of(const Agent & p) const
    {
        const auto & v = p.get_virus();
        return (v && v->nh_def) ? *v->nh_def : *nh;
    }
    void nh_propose(int transition, Agent & p, Virus * virus = nullptr);
    void nh_enter(Agent & p, unsigned int state_old, int transition);
    void nh_queue_move(Agent & p, unsigned int state_old);
    void nh_initial_place();
    epiworld_double nh_infection_prob(Agent & target, const Virus & v);
    ///@}

    /**
     * @brief Variables used to keep track of the events
     * to be made regarding viruses.
     */
    std::vector< Event > events = {};
    epiworld_fast_uint nactions = 0u;

    /**
     * @brief Construct a new Event object
     *
     * @param agent_ Agent over which the action will be called
     * @param virus_ Virus pointer included in the action
     * @param tool_ Tool pointer included in the action
     * @param new_state_ New state of the agent
     * @param queue_ Change in the queue
      * @param action_ Action to execute when processing the event
     */
    void _add_event(
        Agent * agent_,
        VirusPtr virus_,
        ToolPtr tool_,
        epiworld_fast_int new_state_,
        epiworld_fast_int queue_,
        EventAction action_,
        int layer_ = -1
        );

    /**
     * @name Default event handlers
     *
     * These member functions implement the default behavior for each
     * event action (add/remove virus or tool, or change state).
     */
    ///@{
    void _event_add_virus(Event & a);
    void _event_add_tool(Event & a);
    void _event_rm_virus(Event & a);
    void _event_rm_tool(Event & a);
    void _event_change_state(Event & a);
    ///@}

    /**
     * @name Tool Mixers
     *
     * These functions combine the effects tools have to deliver
     * a single effect. For example, wearing a mask, been vaccinated,
     * and the immune system combine together to jointly reduce
     * the susceptibility for a given virus.
     *
     */
    virtual epiworld_double susceptibility_reduction_mixer(
        Agent * agent, VirusPtr & virus
    );
    virtual epiworld_double transmission_reduction_mixer(
        Agent * agent, VirusPtr & virus
    );
    virtual epiworld_double recovery_enhancer_mixer(
        Agent * agent, VirusPtr & virus
    );
    virtual epiworld_double death_reduction_mixer(
        Agent * agent, VirusPtr & virus
    );

    /**
     * @brief Advanced usage: Makes a copy of data and returns it as undeleted pointer
     *
     * @param copy
     */
    virtual std::unique_ptr<Model> clone_ptr();

    /// For the models built on this class, which set their population.
    Model();

public:

    /**
     * @name Scratch arrays for sampling exposures
     * @details Update functions store each exposure's probability in
     * `array_double_tmp` and its virus in `array_virus_tmp`, then draw with
     * `roulette()`, which uses two doubles per exposure. An agent can have any
     * number of exposures across its layers, so call `reserve_tmp()` before
     * storing them.
     */
    ///@{
    std::vector< epiworld_double > array_double_tmp =
        std::vector< epiworld_double >(1024u * 2u);
    std::vector< Virus * > array_virus_tmp =
        std::vector< Virus * >(1024u);
    std::vector< int > array_layer_tmp =
        std::vector< int >(1024u); ///< Layer of each exposure

    /// Makes room for `n` exposures in the scratch arrays.
    void reserve_tmp(size_t n);
    ///@}

    /// A model with `n` agents.
    explicit Model(size_t n);
    Model(const Model & m);
    Model(Model && m);
    Model & operator=(const Model & m);

    virtual ~Model() { layers_detach(); };

    DataBase & get_db();
    const DataBase & get_db() const;
    epiworld_double operator()(std::string_view pname) const;

    size_t size() const;

    /**
     * @name Random number generation
     *
     * @param eng Random number generator
     * @param s Seed
     */
    ///@{
    void set_rand_engine(std::shared_ptr< epi_xoshiro256ss > & eng);
    std::shared_ptr< epi_xoshiro256ss > & get_rand_endgine();
    void seed(size_t s);
    void set_rand_norm(epiworld_double mean, epiworld_double sd);
    void set_rand_unif(epiworld_double a, epiworld_double b);
    void set_rand_exp(epiworld_double lambda);
    void set_rand_gamma(epiworld_double alpha, epiworld_double beta);
    void set_rand_lognormal(epiworld_double mean, epiworld_double shape);
    void set_rand_binom(int n, epiworld_double p);
    void set_rand_nbinom(int n, epiworld_double p);
    void set_rand_geom(epiworld_double p);
    void set_rand_poiss(epiworld_double lambda);
    epiworld_double rnorm();
    epiworld_double rnorm(epiworld_double mean, epiworld_double sd);
    epiworld_double rgamma();
    epiworld_double rgamma(epiworld_double alpha, epiworld_double beta);
    epiworld_double rexp();
    epiworld_double rexp(epiworld_double lambda);
    epiworld_double rlognormal();
    epiworld_double rlognormal(epiworld_double mean, epiworld_double shape);

    /**
     * @brief Draw from the currently configured uniform distribution.
     * @return A random draw from the configured uniform distribution.
     * @details
     * These uniform draws make use of Lemire's algorithm for fast
     * uniform integer generation, which is both faster and more accurate than
     * the common `std::uniform_int_distribution` approach.
     * 
     * 
     * Lemire, D. (2019). Fast Random Integer Generation in an Interval.
     * ACM Trans. Model. Comput. Simul., 29(1), 3:1-3:12.
     * <https://doi.org/10.1145/3230636>
     */
    epiworld_double runif();
    epiworld_double runif(epiworld_double a, epiworld_double b);
    int runif_int(int a, int b);
    uint32_t runif_index(uint32_t n);
    /**
     * @brief Draw from the currently configured binomial distribution.
     * @details When `EPI_FAST_BINOM` is enabled (default), this uses
     * `rpoiss(lambda)` with `lambda = n * p` after `set_rand_binom(n, p)` in the
     * rare-event regime `p <= 0.01` and `n * p * p <= 0.1`. Define
     * `EPI_NO_FAST_BINOM` before including epiworld to disable this behavior.
     * @return A random draw from the configured binomial distribution, or from
     * its Poisson approximation when the fast path is active.
     */
    int rbinom();
    /**
     * @brief Draw from a binomial distribution with parameters `n` and `p`.
     * @details When `EPI_FAST_BINOM` is enabled (default), this uses
     * `rpoiss(lambda)` with `lambda = n * p` in the rare-event regime
     * `p <= 0.01` and `n * p * p <= 0.1`. This preserves the mean and is often
     * substantially faster in practice while remaining very accurate in that
     * region. Define `EPI_NO_FAST_BINOM` before including epiworld to disable
     * this behavior and force the exact binomial draw.
     * @param n Number of trials.
     * @param p Success probability.
     * @return A random draw from the binomial distribution, or from its
     * Poisson approximation when the fast path is active.
     */
    int rbinom(int n, epiworld_double p);
    int rnbinom();
    int rnbinom(int n, epiworld_double p);
    int rgeom();
    int rgeom(epiworld_double p);
    int rpoiss();
    int rpoiss(epiworld_double lambda);
    ///@}

    /**
     * @brief Sample from a set of probabilities stored in array_double_tmp.
     * @details Uses a cumulative probability approach: draws a uniform random
     * number and walks through array_double_tmp[0..n-1], accumulating
     * probabilities until the draw is exceeded. If no event fires, returns n
     * (meaning "none of the above").
     * @param n Number of probability entries in array_double_tmp to consider.
     * @return Index in [0, n] of the sampled event (n = no event).
     */
    size_t sample_from_probs(size_t n);

    /**
     * @name Add Virus/Tool to the model
     *
     * This is done before the model has been initialized.
     *
     * @param v Virus to be added
     * @param t Tool to be added
     * @param preval Initial prevalence (initial state.) It can be
     * specified as a proportion (between zero and one,) or an integer
     * indicating number of individuals.
     */
    ///@{
    /**
     * @brief Adds a virus, and who gets it at the start of each run.
     * @details `a` gives each agent without a virus whose stage can be
     * infected a label: the stage it starts in (a carrier stage), or, for the
     * forms without labels (e.g., `assign::share()`), the infection's target.
     * By default, no one gets it (see `assign.hpp`).
     *
     * Each call records a new variant, the founder of a lineage, whose id is
     * the number of viruses added before it (adding the same virus twice
     * records two; a removed virus keeps its record, so ids do not move).
     * Mutations during a run add variants after them, which the next run
     * (or the next `add_virus()`) drops.
     */
    void add_virus(const Virus & v, Assign a = Assign());

    /// Adds a tool, and who gets it at the start of each run (default: no one).
    void add_tool(const Tool & t, Assign a = Assign());

    /// Replaces who gets virus `id` (e.g., the virus of a built-in model).
    void assign_virus(size_t id, Assign a);

    /// Replaces who gets tool `id`.
    void assign_tool(size_t id, Assign a);

    void rm_virus(size_t virus_pos);
    void rm_tool(size_t tool_pos);
    ///@}

    /**
     * @name Accessing population of the model
     */
    ///@{
    std::vector< Agent > & get_agents(); ///< Returns a reference to the vector of agents.

    Agent & get_agent(size_t i);

    std::vector< epiworld_fast_uint > get_agents_states() const; ///< Returns a vector with the states of the agents.




    /**
     * @name Functions to run the model
     *
     * @param seed Seed to be used for Pseudo-RNG.
     * @param ndays Number of days (steps) of the simulation.
     * @param fun In the case of `run_multiple`, a function that is called
     * after each experiment.
     *
     */
    ///@{
    void update_state();
    void mutate_virus();
    virtual void next();
    virtual Model & run(
        epiworld_fast_uint ndays,
        int seed = -1
    ); ///< Runs the simulation (after initialization)
    Model & run_multiple( ///< Multiple runs of the simulation
        epiworld_fast_uint ndays,
        epiworld_fast_uint nexperiments,
        int seed_ = -1,
        std::function<void(size_t,Model*)> fun = make_save_run(),
        bool reset = true,
        bool verbose = true,
        int nthreads = 1
        );
    ///@}

    size_t get_n_viruses() const; ///< Number of viruses in the model
    size_t get_n_tools() const; ///< Number of tools in the model
    epiworld_fast_uint get_ndays() const;

    /**
     * @brief True while `run()` is driving the day loop.
     *
     * @details It is set from just before `reset()` until the last day is
     * done, so global events (including their `reset()`) can rely on
     * `get_ndays()` being the run's horizon -- even when it is zero. It is
     * false when the day loop is driven by hand (`reset()`, then the steps
     * called directly), where `get_ndays()` means nothing.
     */
    bool is_running() const;
    epiworld_fast_uint get_n_replicates() const;
    size_t get_sim_id() const;
    void set_ndays(epiworld_fast_uint ndays);
    bool get_verbose() const;
    Model & verbose_off();
    Model & verbose_on();
    int today() const; ///< The current time of the model


    /**
     * @brief Wrapper of `DataBase::write_data`
     *
     * @param fn_virus_info Filename. Information about the virus.
     * @param fn_virus_hist Filename. History of the virus.
     * @param fn_tool_info Filename. Information about the tool.
     * @param fn_tool_hist Filename. History of the tool.
     * @param fn_total_hist   Filename. Aggregated history (state)
     * @param fn_transmission Filename. Transmission history.
     * @param fn_transition   Filename. Markov transition history.
     * @param fn_reproductive_number Filename. Case by case reproductive number
     * @param fn_generation_time Filename. Generation time data.
     * @param fn_active_cases Filename. Active cases data.
     * @param fn_outbreak_size Filename. Outbreak size data.
     * @param fn_hospitalizations Filename. Hospitalization data.
     */
    void write_data(
        std::string fn_virus_info,
        std::string fn_virus_hist,
        std::string fn_tool_info,
        std::string fn_tool_hist,
        std::string fn_total_hist,
        std::string fn_transmission,
        std::string fn_transition,
        std::string fn_reproductive_number,
        std::string fn_generation_time,
        std::string fn_active_cases,
        std::string fn_outbreak_size,
        std::string fn_hospitalizations
        ) const;


    /**
     * @brief Copy of the model parameters, by name.
     * @details To change a parameter, use `set_param()`.
     */
    std::map<std::string, epiworld_double> params() const;

    /**
     * @brief Reset the model
     *
     * @details Resetting the model will:
     * - clear the database
     * - reset the contact layers (see `ContactLayer`)
     * - re-distribute tools
     * - re-distribute viruses
     * - set the date to 0
     *
     */
    virtual void reset();
    const Model & print(bool lite = false) const;

    /**
     * @brief The number of the engine's states: one per pair of stage (of the
     * virus's natural history) and status (of the model's statuses), stage
     * major (`state = stage * n_statuses + status`). Use `get_stage()` and
     * `get_status()` for an agent's.
     */
    size_t get_n_states() const;

    /**
     * @name Natural history
     * @brief The disease's stages and the declared transitions between them.
     */
    ///@{

    /// The natural history of the model's viruses (`nullptr` if none has one).
    const NaturalHistory * get_natural_history() const { return nh.get(); }

    /**
     * @brief Fires a declared transition on an agent, applied at the end of
     * the step (or of the global event).
     * @details Nothing happens if, when it is applied, the agent is not in one
     * of the transition's sources. An infection is not fired: seed instead.
     * @return Whether the agent was in a source when fired.
     */
    bool fire(int transition, Agent & agent);

    /**
     * @brief Places agents left without a virus in stages at the start of
     * each run (e.g., `assign::shares({{R, 0.1}})`: 10% Recovered).
     * @details The labels are stages without the virus; the eligible agents
     * are those left without a virus, in the first stage. Replaces what was
     * set before.
     */
    void set_initial(Assign a);

    /// Day agent `id` entered `stage` in its current (or last) infection (-1: not).
    int get_stage_entered(size_t id, int stage) const;

    /// Day agent `id`'s stage duration ends (`INT_MAX`: none).
    int get_stage_until(size_t id) const;

    /// Infections agent `id` has had in the current run.
    int get_episode(size_t id) const;

    /// The stage and the status of an agent.
    int get_stage(const Agent & agent) const;
    int get_status(const Agent & agent) const;

    /**
     * @brief Sets the model's statuses (see `Statuses`): where agents are,
     * and the transitions between them. Between runs only.
     */
    void set_statuses(Statuses s);

    /// The statuses (`nullptr` if the model has one, "Free").
    const Statuses * get_statuses() const { return st.get(); }

    /**
     * @brief When `on` happens to an agent, fire `transition` on it (or call
     * `fun`), in the same step.
     * @details Reactions link the axes: e.g., entering Rash fires
     * "Quarantined -> Isolated". Within a day, stage changes apply first, then
     * status changes, then reactions (which see the resulting status; `.was()`
     * reads the one at the start of the day).
     */
    void react(Trigger on, int transition);
    void react(Trigger on, std::function< void(Agent &, Model &) > fun);

    /// A transition's id (stage or status), by name.
    int transition_id(std::string_view name) const;
    ///@}

    /**
     * @brief Ids of the agents currently in a state.
     *
     * @details The list is kept up to date as the model runs, so looking up
     * who is in a state costs nothing (no scan of the population). The ids are
     * in no particular order. The index is built when a run starts, so this is
     * available once `run()` (or `run_multiple()`) has been called, and it
     * reflects the model at its current step.
     *
     * @param state The state code.
     * @return A view of the ids (iterable, with `size()` and `[]`); it is
     * invalidated by the next step.
     * @throws std::logic_error if the model has not been run yet (or its
     * population changed since).
     * @throws std::range_error if `state` is not a state of the model.
     */
    AgentIdsView get_agents_in_state(epiworld_fast_uint state) const;

    /**
     * @brief One flag per agent: 1 if the agent carries a virus.
     * @details A compact copy of "has a virus", one byte per agent, so a scan
     * of an agent's neighbors can rule most of them out without loading their
     * `Agent` (a cache miss each). It is kept current from the moment a run
     * starts. Returns `nullptr` before that (and after the population
     * changes): callers then have to look at the agents.
     */
    const char * get_agent_carrier() const
    {
        return state_index_ready ? agent_carrier.data() : nullptr;
    }

    /**
     * @brief Draws the virus, if any, that infects `agent` from its contacts.
     * @details This is where contacts become an infection. Every contact that
     * carries a virus is an exposure with probability
     * `(1 - susceptibility reduction of agent) * probability of infecting *
     * (1 - transmission reduction of the contact)`, and one draw
     * (`roulette()`) picks at most one infector. A contact listed twice is
     * two exposures.
     * With a `layer`, the probabilities follow its weight and the viruses it
     * admits (see `ContactLayer`), and the layer is recorded as the one of the
     * transmission.
     * @param agent Agent without a virus.
     * @param contacts Ids of the contacts.
     * @param n Number of contacts.
     * @param layer The layer the contacts came from (none: weight 1, any
     * virus, no layer recorded).
     * @return The virus transmitted, or `nullptr` if none is.
     */
    Virus * sample_virus(
        Agent * agent,
        const size_t * contacts,
        size_t n,
        const ContactLayer * layer = nullptr
    );

    /**
     * @brief The layer of the last transmission drawn by `sample_virus()`
     * (-1 if none), which the next `Agent::set_virus()` records and clears.
     */
    int take_sampled_layer()
    {
        const int layer = sampled_layer;
        sampled_layer = -1;
        return layer;
    }

    /**
     * @brief Draws the virus, if any, that `agent` catches from its contacts
     * in the model's layers.
     * @details The contacts that carry a virus, layer by layer in the order
     * the layers were added and each layer in its own order, are all
     * exposures of one draw (`sample_virus(agent, contacts, n, layer)` adds
     * them). Only contacts in an infectious stage (`stage::infectious`) are
     * sources. This is what a default infection does; a pushing step
     * (networks only) gives the same distribution.
     * @throws std::logic_error if `agent` carries a virus.
     */
    Virus * sample_virus(Agent * agent);

    /**
     * @name Network transmission mode
     *
     * @details Stages whose only drawn exit is a default infection (no
     * custom sampler) can be updated by pulling (each
     * susceptible agent scans its neighbors) or by pushing (each agent with a
     * virus adds its infection odds to its susceptible neighbors). Both give
     * the same distribution of who gets infected, and by whom; only the random
     * number stream differs. See `TransmissionMode`.
     *
     * With `"auto"` (the default) the model estimates the cost of each step,
     * as the ties to scan plus a fixed price per agent visited
     * (`EPI_TRANSMISSION_AGENT_COST` ties), for the carriers that can transmit
     * (push) and for the susceptible agents (pull). It pushes whenever the
     * push cost is no more than `kappa` times the pull cost, and pulls
     * otherwise; `kappa` only matters in this mode. The choice depends only on
     * the model's state, never on the queueing system, so turning queuing on or
     * off leaves results unchanged. A `kappa` below 1 makes pushing harder to
     * choose (it favors pulling); the default, 0.5, is below 1 because the
     * queue spares a pull the susceptibles with no infectious neighbor, which
     * the costs do not see. It was fitted together with the per-agent price
     * with `examples/20-transmission-benchmark`.
     *
     * Directed networks, and states with other update functions, always pull.
     * Set `"pull"` to reproduce the random streams of epiworld <= 0.15.
     *
     * @param mode `"auto"`, `"push"`, or `"pull"` (or the enum).
     * @param kappa Relative cost threshold used by `"auto"`: a finite,
     * non-negative number (default `EPI_DEFAULT_TRANSMISSION_KAPPA`, 0.5).
     * @throws std::invalid_argument for an unknown mode.
     * @throws std::range_error for a negative or infinite `kappa`.
     */
    ///@{
    Model & set_transmission_mode(
        TransmissionMode mode,
        double kappa = EPI_DEFAULT_TRANSMISSION_KAPPA
    );
    Model & set_transmission_mode(
        std::string_view mode,
        double kappa = EPI_DEFAULT_TRANSMISSION_KAPPA
    );
    TransmissionMode get_transmission_mode() const;
    /// The mode used in the most recent step (`push` or `pull`).
    TransmissionMode get_last_transmission_mode() const;
    /// The threshold used by `"auto"`.
    double get_transmission_kappa() const;

    /**
     * @brief The sums `"auto"` compares to choose between push and pull.
     * @details Totals over the states that can be sources of infection (the
     * carriers) and over the states updated by the default susceptible sampler
     * (the susceptibles), as they stand now. All zero before the model has run.
     */
    struct TransmissionSums {
        size_t carrier_degree;      ///< Sum of the carriers' degrees
        size_t susceptible_degree;  ///< Sum of the susceptibles' degrees
        size_t carriers;            ///< Number of carriers that can transmit
        size_t susceptibles;        ///< Number of susceptible agents
    };
    TransmissionSums get_transmission_sums() const;
    ///@}


    /**
     * @name Setting and accessing parameters from the model
     *
     * @details Parameters are stored in a vector, in the order they were
     * added, with a map from each name to its position. Lookups by name take
     * the name as a `std::string_view` (no allocation) and search the map once.
     * Unknown names throw.
     *
     * Code that reads parameters for many agents every day can skip the
     * search:
     *
     * - `get_param_id()` returns the position (`ParamId`) of a parameter, and
     *   `par_at()` / `set_param_at()` read and write by position. Positions
     *   are valid for the model and all its copies.
     * - `ParamRef` (and the `EPI_PAR(model, "name")` macro) resolves a name
     *   once per model layout and caches the position, so it is safe to use
     *   with any model.
     *
     * The `par()` function members are aliases for `get_param()`.
     *
     * In the case of the function `read_params`, users can pass a file
     * listing parameters to be included in the model. Each line in the
     * file should have the following structure:
     *
     * ```
     * [name of parameter 1]: [value in double]
     * [name of parameter 2]: [value in double]
     * ...
     * ```
     *
     * The only condition for parameter names is that these do not include
     * a colon.
     *
     *
     * @param initial_val
     * @param pname Name of the parameter to add or to fetch
     * @param fn Path to the file containing parameters
     * @return The current value of the parameter
     * in the model.
     *
     */
    ///@{
    epiworld_double add_param(
        epiworld_double initial_val, std::string pname, bool overwrite = false
    );
    Model & read_params(std::string fn, bool overwrite = false);
    epiworld_double get_param(std::string_view pname) const;
    bool has_param(std::string_view pname) const;
    void set_param(std::string_view pname, epiworld_double val);
    epiworld_double par(std::string_view pname) const;
    ParamId get_param_id(std::string_view pname) const;
    epiworld_double par_at(ParamId id) const;
    void set_param_at(ParamId id, epiworld_double val);
    size_t get_n_params() const { return param_values.size(); };
    uint32_t get_param_layout_id() const { return param_layout_id; };
    ///@}

    void get_elapsed(
        std::string unit = "auto",
        epiworld_double * last_elapsed = nullptr,
        epiworld_double * total_elapsed = nullptr,
        std::string * unit_abbr = nullptr,
        bool print = true
    ) const;

    /**
     * @name Set the user data object
     *
     * @param names string vector with the names of the variables.
     */
    ///[@
    void set_user_data(std::vector< std::string > names);
    void add_user_data(epiworld_fast_uint j, epiworld_double x);
    void add_user_data(std::vector< epiworld_double > x);
    UserData & get_user_data();
    ///@}

    /**
     * @brief Set a global action
     *
     * @param fun A function to be called on the prescribed date
     * @param name Name of the action.
     * @param date Integer indicating when the function is called (see details)
     *
     * @details When date is less than zero, then the function is called
     * at the end of every day. Otherwise, the function will be called only
     * at the end of the indicated date.
     */
    void add_globalevent(
        std::function<void(Model*)> fun,
        std::string name = "A global action",
        int date = -99
        );

    void add_globalevent(
        GlobalEvent & action
    );

    GlobalEvent & get_globalevent(std::string name); ///< Retrieve a global action by name
    GlobalEvent & get_globalevent(size_t i); ///< Retrieve a global action by index
    bool has_globalevent(std::string_view name) const; ///< Whether a global action by that name exists
    size_t get_n_globalevents() const; ///< Number of global actions registered

    void rm_globalevent(std::string name); ///< Remove a global action by name
    void rm_globalevent(size_t i); ///< Remove a global action by index

    void run_globalevents();

    /**
     * @name Contact layers
     * @brief Sources of contacts of the model (see `ContactLayer`).
     * @{
     */

    /**
     * @brief Adds a contact layer to the model.
     * @details The model shares the layer with the caller (an edit through
     * `layer` is an edit of the model's layer), resets it at the start of
     * `reset()`, updates it at the end of every step, and clones it when the
     * model is copied (see `ContactLayer`). Layers are added between runs only
     * (a layer that should act only part of the time is added up front and
     * edited, e.g., by a global event).
     * @param layer The layer (e.g., from `layer::network()`). Its name must not
     * be in use by another layer, and it must not belong to another model.
     * @return A reference to the layer, as its own type.
     */
    template<typename TLayer>
    TLayer & add_layer(std::shared_ptr< TLayer > layer)
    {
        TLayer * ptr = layer.get();
        add_layer_ptr(std::move(layer));
        return *ptr;
    }

    size_t get_n_layers() const; ///< Number of contact layers
    bool has_layer(std::string_view name) const; ///< Whether a layer by that name exists
    size_t get_layer_id(std::string_view name) const; ///< Id of a layer by name (throws if absent)
    ContactLayer & get_layer(size_t id); ///< Retrieve a layer by id
    ContactLayer & get_layer(std::string_view name); ///< Retrieve a layer by name

    /// Retrieve a layer as its own type (unchecked cast; ids are cheaper than names)
    template<typename TLayer>
    TLayer & get_layer_as(size_t id)
    {
        return static_cast< TLayer & >(get_layer(id));
    }

    template<typename TLayer>
    TLayer & get_layer_as(std::string_view name)
    {
        return static_cast< TLayer & >(get_layer(name));
    }

    /// Calls `update()` on every layer (done by `next()` at the end of each step)
    void update_layers();

    /// The network layers of the model, in the order they were added.
    const std::vector< layer::Network * > & get_network_layers() const
    {
        return network_layers;
    }

    /** @} */

    void clear_state_set();

    /**
     * @name Queuing system
     * @details When queueing is on, the model will keep track of which agents
     * are either in risk of exposure or exposed. This then is used at each
     * step to act only on the aforementioned agents.
     *
     */
    ////@{
    void queuing_on(); ///< Activates the queuing system (default.)
    Model & queuing_off(); ///< Deactivates the queuing system.
    bool is_queuing_on() const; ///< Query if the queuing system is on.
    Queue & get_queue(); ///< Retrieve the `Queue` object.
    ///@}

    /**
     * @name Post-sampling callback
     * @details A callback that receives, for each infectious agent, the agents
     * it was in contact with during the step (see `PostSamplingFun`). The
     * models whose samplers report contacts (network pull and push, the
     * mixing models) collect them only while a callback is installed;
     * otherwise the cost is one check per sampling operation (per step in a
     * push). Pull and push report the same contacts. The callback is kept by
     * copies of the model, including the ones `run_multiple()` makes.
     *
     * Installing a callback replaces the previous one. Contact tracing does
     * not use it: a layer keeps its own log (`ContactLayer::enable_tracing()`).
     */
    ///@{
    Model & set_post_sampling(PostSamplingFun fun); ///< Install a callback (an empty function clears it).
    Model & clear_post_sampling(); ///< Remove the callback.
    bool has_post_sampling() const { return post_sampling_on; } ///< Is a callback installed?

    /// Reports that `infectious_id` was in contact with `contacted_id`. Samplers
    /// call it, only if `has_post_sampling()`.
    void register_sampled_contact(size_t infectious_id, size_t contacted_id);

    /// Same, for several infectious agents in contact with `contacted_id`.
    void register_sampled_contacts(
        const size_t * infectious_ids, size_t n, size_t contacted_id
    );
    ///@}

    const std::vector< VirusPtr > & get_viruses() const;
    const std::vector< ToolPtr > & get_tools() const;
    Virus & get_virus(size_t id);
    Virus & get_virus(std::string_view name);
    Tool & get_tool(size_t id);
    Tool & get_tool(std::string_view name);

    bool has_virus(std::string_view name) const;
    bool has_tool(std::string_view name) const;

    /**
     * @brief Set the agents data object
     *
     * @details The data should be an array with the data stored in a
     * column major order, i.e., by column.
     *
     * @param data_ Pointer to the first element of an array of size
     * `size() * ncols_`.
     * @param ncols_ Number of features included in the data.
     *
     */
    void set_agents_data(double * data_, size_t ncols_);
    double * get_agents_data();
    size_t get_agents_data_ncols() const;

    /**
     * @brief Set the name object
     *
     * @param name
     */
    void set_name(std::string name);
    std::string get_name() const;

    bool operator==(const Model & other) const;
    bool operator!=(const Model & other) const {return !operator==(other);};

    /**
     * @brief Executes the stored action
     *
     * @param model_ Model over which it will be executed.
     */
    void events_run();

    /**
     * @brief Draws a mermaid diagram of the model.
     * @param model The model to draw.
     * @param fn_output The name of the file to write the diagram.
     * If empty, the diagram will be printed to the standard output.
     * @param self Whether to allow self-transitions.
     */
    void draw(
        DiagramType diagram_type = DiagramType::Mermaid,
        const std::string & fn_output = "",
        bool self = false
    );

    /**
     * @brief The model's natural history as a mermaid flowchart: the stages
     * and, if the model has statuses, the statuses, each in a subgraph (see
     * `NaturalHistory::draw_mermaid()`). Drawn from the declarations, so it
     * needs no run.
     * @throws std::logic_error if the model has no natural history.
     */
    std::string draw_natural_history() const;

    /**
     * @brief Record a hospitalization event for an agent.
     * 
     * @param agent Reference to the agent being hospitalized.
     * 
     * @details
     * This is a wrapper for `DataBase::record_hospitalization()`.
     * For each hospitalization, the method records:
     * - The current date from the model
     * - The virus ID from the agent's virus
     * - For each tool the agent has, a separate record with weight = 1/N
     *   where N is the number of tools
     * - If the agent has no tools, a single record with tool_id = -1 and
     *   weight = 1.0
     */
    void record_hospitalization(Agent & agent);

    /**
     * @brief Get the full time series of hospitalization data.
     * 
     * @param date Output vector for dates.
     * @param virus_id Output vector for virus IDs.
     * @param tool_id Output vector for tool IDs.
     * @param count Output vector for counts (number of hospitalized individuals).
     * @param weight Output vector for summed weights (fractional contribution
     *   based on tool distribution).
     * 
     * @details
     * This is a wrapper for `DataBase::get_hospitalizations()`.
     * Returns the full time series of hospitalization data. For each unique 
     * (virus_id, tool_id) combination observed, returns an entry for every 
     * day from 0 to ndays-1.
     * 
     * The `count` vector contains the actual number of individuals hospitalized 
     * for that (date, virus_id, tool_id) combination. This is useful for 
     * answering questions like "how many total people were hospitalized?" 
     * regardless of their tools.
     * 
     * The `weight` vector contains fractional contributions: if an agent has N 
     * tools, each tool gets weight = 1/N. Summing weights across all tool_ids 
     * for a given date and virus_id gives the total number of hospitalizations.
     */
    void get_hospitalizations(
        std::vector<int> & date,
        std::vector<int> & virus_id,
        std::vector<int> & tool_id,
        std::vector<int> & count,
        std::vector<double> & weight
    ) const;


};

#endif
/*//////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

 End of -./include/epiworld/model-bones.hpp-

////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////*/


/*//////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

 Start of -./include/epiworld/contactlayer-bones.hpp-

////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////*/


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
 * - `reset()` at the start of `Model::reset()`, before anything is distributed
 *   (viruses, tools, the global events' set-up), so that those see
 *   the run's contacts. A layer restores its baseline if it was edited during
 *   the previous run (see `is_dirty()`), or draws a new one, and sizes its
 *   per-run buffers.
 * - `update()` at the end of every step, including day 0 (at the start of
 *   `Model::next()`): after the agents' update functions, the global events
 *   and rewiring, so that whatever changed during a step (states, edits of the
 *   layer) is in force for the next step's contacts.
 * - `clone()` whenever the model is copied (`run_multiple()` copies models),
 *   so replicates never share layer state.
 *
 * ### Ownership
 *
 * Layers are created as shared pointers (`layer::network()`,
 * `layer::mixing()`; `std::make_shared` for a layer type of your own), and
 * `Model::add_layer()` shares the pointer: the model and whoever else holds it
 * see the same layer, so an edit made through either is an edit of the model.
 * A layer belongs to one model at most. A copy of the model clones its layers
 * (the pointers held outside keep pointing at the original's), and a layer
 * outlives its model only detached from it. A copy of a layer is detached too.
 *
 * ### Weight and viruses
 *
 * `set_weight()` makes the layer's contacts count more or less than the others':
 * the odds of infection through the layer are multiplied by the weight (a
 * model parameter, read at every step; 1 if none is set; 0 turns the layer
 * off). `add_virus_target()` restricts the layer to some viruses, as tools are
 * restricted; by default every virus spreads through every layer.
 *
 * ### Edits and the dirty flag
 *
 * Whether an edit is permanent depends only on when it is made. An edit made
 * during a run (from an update function or a global event) applies to that run
 * only: it sets the *dirty* flag, and `reset()` restores the baseline of a
 * dirty layer and clears the flag. An edit made between runs becomes the new
 * baseline.
 *
 * ### Contact tracing
 *
 * `enable_tracing()` makes the layer keep a log of its contacts (a
 * `ContactTracing`): every contact the layer samples, with a carrier, is
 * recorded under the carrier, with the day. The log starts empty at every run
 * and keeps the last `max_contacts` contacts of each agent. Tracing is off by
 * default, and it is a setting, not an edit: it lasts across runs whenever it
 * is turned on or off.
 *
 * @ingroup contact_layers
 */
class ContactLayer {

    friend class Model;

private:

    std::string name;
    size_t id = 0u;          ///< Position in the model's list, set by the model
    bool dirty = false;  ///< Edited since the baseline was set
    bool in_run = false; ///< Set by the model while it runs

    std::optional< ParamRef > weight_ref; ///< Empty: weight 1
    uint64_t virus_mask = ~uint64_t(0);   ///< Lineages that spread here
    size_t trace_max = 0u;   ///< Contacts kept per agent; 0: not traced
    ContactTracing trace_log; ///< This run's contacts, if traced

    /// An empty log for a run of `n` agents (the model calls it at reset).
    void reset_tracing(size_t n)
    {
        if (trace_max > 0u)
            trace_log.reset(n, trace_max);
    }

protected:

    /// The model the layer belongs to (set by the model), or `nullptr`.
    Model * model = nullptr;

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

    /// A copy belongs to no model (see `clone()`).
    ContactLayer(const ContactLayer & other) :
        name(other.name), dirty(other.dirty), weight_ref(other.weight_ref),
        virus_mask(other.virus_mask), trace_max(other.trace_max),
        trace_log(other.trace_log) {}

    ContactLayer & operator=(const ContactLayer &) = delete;

    virtual ~ContactLayer() = default;

    const std::string & get_name() const { return name; }
    size_t get_id() const { return id; }
    bool is_dirty() const { return dirty; }

    /// The weight is the model parameter `param` (see the class description).
    void set_weight(std::string param) { weight_ref.emplace(std::move(param)); }

    bool has_weight() const { return weight_ref.has_value(); }

    /// Current weight in `model`.
    epiworld_double get_weight(const Model & model) const
    {
        return weight_ref ? (*weight_ref)(model) : 1.0;
    }

    /**
     * @name Virus targets
     * @brief Restrict the layer to the listed virus lineages (see
     * `Tool::add_target()`, which these mirror). The first target turns the
     * default (every virus) off.
     */
    ///@{
    void add_virus_target(int lineage_id);
    void add_virus_target(const Virus & v);
    ///@}

    /**
     * @name Contact tracing
     * @brief Keep a log of the layer's contacts (see the class description).
     */
    ///@{
    /// Turn tracing on, keeping up to `max_contacts` contacts per agent.
    void enable_tracing(size_t max_contacts = EPI_MAX_TRACKING);
    void disable_tracing();
    bool is_tracing() const { return trace_max > 0u; }
    /// The log of the current run (throws if tracing is off).
    ContactTracing & get_tracing();

    /// Log that `source` (the carrier) met `contact` today, if traced.
    void trace(size_t source, size_t contact)
    {
        if (trace_max > 0u)
            trace_log.add_contact(
                source, contact, static_cast< size_t >(model->today())
            );
    }

    /// Log that each of the `n` `sources` met `contact` today, if traced.
    void trace(const size_t * sources, size_t n, size_t contact)
    {
        if (trace_max > 0u)
            for (size_t k = 0u; k < n; ++k)
                trace(sources[k], contact);
    }
    ///@}

    /// Whether the virus spreads through the layer.
    bool admits(const Virus & v) const
    {
        return (virus_mask & v.lineage_bit) != 0u;
    }

    /// Restore the baseline if dirty and size the buffers for a new run.
    virtual void reset(Model & model) = 0;

    /**
     * @brief Appends to `out` the contacts of agent `agent` this step that
     * carry a virus (repeats allowed), and reports them to the model's
     * post-sampling callback, if there is one.
     * @details This is the layer's part of a pull. Contacts in a state flagged
     * in `exclude` (if not null) are skipped. Whether the contacts are able to
     * infect (their viruses, the weight) is up to the model.
     * @return The number of contacts appended.
     */
    virtual size_t sample(
        size_t agent,
        Model & model,
        const std::vector< bool > * exclude,
        std::vector< size_t > & out
    ) = 0;

    /// Rebuild what depends on the state of the population (end of each step).
    virtual void update(Model & model) = 0;

    /// Deep copy (the model's copy constructor calls it).
    virtual std::shared_ptr< ContactLayer > clone() const = 0;

    /// The layer as a network layer, or `nullptr` if it is of another kind.
    virtual layer::Network * as_network() { return nullptr; }

    /// The layer as a mixing layer, or `nullptr` if it is of another kind.
    virtual layer::Mixing * as_mixing() { return nullptr; }

    /**
     * @brief Whether the model's queue sees this layer's contacts.
     * @details True when a carrier can reach only agents the layer can list
     * from the carrier's side: its ties in an undirected network, its group in
     * a within-group mixing layer. A model with any other layer visits every
     * agent instead of using the queue.
     */
    virtual bool supports_queue() const { return false; }

};

inline void ContactLayer::enable_tracing(size_t max_contacts)
{
    if (max_contacts == 0u)
        throw std::invalid_argument(
            "Contact tracing should keep at least one contact per agent."
        );

    trace_max = max_contacts;

    // Turned on in a model that has agents: logs from now on
    if (model != nullptr)
        reset_tracing(model->size());
}

inline void ContactLayer::disable_tracing()
{
    trace_max = 0u;
    trace_log = ContactTracing();
}

inline ContactTracing & ContactLayer::get_tracing()
{
    if (trace_max == 0u)
        throw std::logic_error(
            "The contact layer '" + name + "' is not traced; call "
            "enable_tracing() first."
        );

    return trace_log;
}

inline void ContactLayer::add_virus_target(int lineage_id)
{

    const uint64_t bit = Tool::target_bit(lineage_id);

    // The first target turns the default off
    if (virus_mask == ~uint64_t(0))
        virus_mask = 0u;

    virus_mask |= bit;

}

inline void ContactLayer::add_virus_target(const Virus & v)
{

    if (v.get_lineage_id() < 0)
        throw std::logic_error(
            "The virus \"" + v.get_name() + "\" has no lineage id. Add it to "
            "the model with Model::add_virus() before using it as a target."
        );

    add_virus_target(v.get_lineage_id());

}

#endif
/*//////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

 End of -./include/epiworld/contactlayer-bones.hpp-

////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////*/


/*//////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

 Start of -./include/epiworld/agentssample-bones.hpp-

////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////*/


#ifndef EPIWORLD_AGENTS_BONES_HPP
#define EPIWORLD_AGENTS_BONES_HPP

/**
 * @brief Sample of agents
 * 
 * A sample without replacement from the model's agents, optionally restricted
 * to some states.
 * 
 */
class AgentsSample {
private:

    size_t sample_size = 0u;

    std::vector< Agent* >* agents = nullptr; ///< Pointer to sample of agents
    size_t * agents_n = nullptr;                    ///< Size of sample of agents
    
    std::vector< size_t >* agents_left = nullptr;  ///< Pointer to agents left (iota)
    size_t * agents_left_n = nullptr;               ///< Size of agents left

    Model * model = nullptr; ///< Extracts runif() and the population.
    std::vector< size_t > states = {};

    void sample_n(size_t n); ///< Backbone function for sampling


public:

    // Not available (for now)
    AgentsSample() = delete;                             ///< Default constructor
    AgentsSample(const AgentsSample & a) = delete; ///< Copy constructor
    AgentsSample(AgentsSample && a) = delete;      ///< Move constructor

    AgentsSample(
        Model & model_, size_t n,
        std::vector< size_t > states_ = {},
        bool truncate = false
        );

    ~AgentsSample();

    typename std::vector< Agent * >::iterator begin();
    typename std::vector< Agent * >::iterator end();

    Agent * operator[](size_t n);
    Agent * operator()(size_t n);
    size_t size() const noexcept;

};

inline AgentsSample::AgentsSample(
    Model & model_,
    size_t n,
    std::vector< size_t > states_,
    bool truncate
    ) {

    states = states_;

    if (truncate)
    {
        
        if (n > model_.size())
            n = model_.size();

    } else if (n > model_.size())
        throw std::logic_error(
            "There are only " + std::to_string(model_.size()) + " agents. You cannot " +
            "sample " + std::to_string(n));

    sample_size   = n;
    model         = &model_;

    agents        = &model_.sampled_population;
    agents_n      = &model_.sampled_population_n;

    agents_left   = &model_.population_left;
    agents_left_n = &model_.population_left_n;

    sample_n(n);
    
    return; 

}

inline AgentsSample::~AgentsSample() {}

inline size_t AgentsSample::size() const noexcept
{ 
    return this->sample_size;
}

inline Agent * AgentsSample::operator[](size_t i)
{

    return agents->operator[](i);

}

inline Agent * AgentsSample::operator()(size_t i)
{

    if (i >= this->sample_size)
        throw std::range_error("The requested agent is out of range.");

    return agents->operator[](i);

}

inline typename std::vector< Agent * >::iterator AgentsSample::begin()
{
    // Check for null pointer
    if (agents == nullptr)
        return typename std::vector< Agent * >::iterator{};
    
    return agents->begin();
}

inline typename std::vector< Agent * >::iterator AgentsSample::end()
{
    // Check for null pointer
    if (agents == nullptr)
        return typename std::vector< Agent * >::iterator{};
    
    // Ensure we don't go beyond the actual vector size
    size_t actual_end = std::min(sample_size, agents->size());
    return agents->begin() + actual_end;
}

inline void AgentsSample::sample_n(size_t n)
{

    if (states.size())
    {

        // Only the agents in the selected states can be sampled
        agents_left->clear();
        agents_left->reserve(model->size());
        for (size_t a_i = 0u; a_i < model->population.size(); ++a_i)
        {
            size_t s = model->population[a_i].get_state();
            if (std::find(states.begin(), states.end(), s) != states.end())
                agents_left->push_back(a_i);
        }

    }
    else if (model->size() != agents_left->size())
    {

        // The population changed size (or this is the first sample)
        agents_left->resize(model->size(), 0u);
        std::iota(agents_left->begin(), agents_left->end(), 0u);

    }

    // Restart the counter of agents left
    *agents_left_n = agents_left->size();

    // Making sure we have enough room for the sample of agents
    if (agents->size() < sample_size)
        agents->resize(sample_size, nullptr);

    #ifdef EPI_DEBUG
    std::vector< bool > __sampled(model->size(), true);
    for (auto & a_i: *agents_left)
        __sampled[a_i] = false;
    #endif

    for (size_t i = 0u; i < n; ++i)
    {

        // Sampling from 0 to (agents_left_n - 1)
        size_t ith_ = static_cast<size_t>(model->runif() * ((*agents_left_n)--));

        // Getting the id of the agent and adding it to the list of agents
        size_t ith  = agents_left->operator[](ith_);
        agents->operator[](i) = &model->population[ith];

        #ifdef EPI_DEBUG
        if (__sampled[ith])
            throw std::logic_error("The same agent was sampled twice.");
        else
            __sampled[ith] = true;
        #endif

        // Updating list
        std::swap(
            agents_left->operator[](ith_),
            agents_left->operator[](*agents_left_n)
            );

    }

    return;

}

#endif
/*//////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

 End of -./include/epiworld/agentssample-bones.hpp-

////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////*/


/*//////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

 Start of -./include/epiworld/globalevents/quarantinetrigger-bones.hpp-

////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////*/


#ifndef QUARANTINETRIGGER_BONES_HPP
#define QUARANTINETRIGGER_BONES_HPP

#include <vector>
// (already included include/epiworld/globalevents/../config.hpp)

class QuarantineTrigger {
private:
    int _model_sim_id = -1;
    int _day = -1;
    std::vector< size_t > _agents_triggering_quarantine;
    std::vector< int > _date_infectious;
    
    void _setup(const Model & model);
public:
    QuarantineTrigger() = default;
    void add_triggering_agent(
        const Model & model,
        const Agent & agent,
        int date_infectious
    );

    std::vector< size_t > & get_triggering_agents();
    std::vector< int > & get_date_infectious();
};

#endif
/*//////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

 End of -./include/epiworld/globalevents/quarantinetrigger-bones.hpp-

////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////*/


/*//////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

 Start of -./include/epiworld/globalevents/bubbles-bones.hpp-

////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////*/


#ifndef EPIWORLD_GLOBALEVENTS_BUBBLES_BONES_HPP
#define EPIWORLD_GLOBALEVENTS_BUBBLES_BONES_HPP

// Standard library headers (vector, memory, string, unordered_map, utility,
// algorithm, stdexcept) are included at global scope by epiworld.hpp; this file
// is only ever included from within `namespace epiworld`, so it must not
// re-include system headers here.
// (already included include/epiworld/globalevents/../config.hpp)

/**
 * @brief Flavor of the social-bubble intervention (how bubbles are formed).
 * @ingroup globalevents
 *
 * @details Both flavors produce a partition of the population into disjoint
 * bubbles in which households are never split, and both form bubbles along
 * *existing* contacts (see Bubbles).
 */
enum class BubbleFlavor {
    /**
     * Household-level rule: whole households bubble together, up to
     * `group_size` households per bubble. A bubble is grown from a seed
     * household by repeatedly absorbing a random household that is connected
     * to it in the contact network. `group_size == 1` leaves every household
     * on its own, i.e. a strict household-only lockdown.
     *
     * Models e.g. the Belgian rule of 10 May 2020 ("a household may form one
     * fixed bubble with other households").
     */
    Household,
    /**
     * Individual-level rule: each agent nominates up to `group_size` peers
     * among its existing contacts outside its own household. A nomination
     * merges the two agents' households -- so a teenager choosing a partner
     * brings both households into one bubble.
     *
     * Nominations are only accepted while the bubble stays within
     * `max_households` households; this cap is what enforces the policy's
     * exclusivity. Without it the merges percolate: with several members per
     * household each nominating someone, the household graph becomes connected
     * and everyone ends up in a single bubble, imposing no restriction at all.
     *
     * Models e.g. the Belgian rule of 19 October 2020 (`group_size == 1`, "one
     * close contact per person").
     */
    Peer
};

/**
 * @brief How a bubble is *realized*: a transmission rule, or also contacts.
 * @ingroup globalevents
 *
 * @details Orthogonal to `BubbleFlavor`, which decides how households are
 * grouped. This decides what being in a bubble together actually does.
 */
enum class BubbleTies {
    /**
     * The contact network is never modified. Being in a bubble together only
     * means transmission between the two is left alone, while contact outside
     * the bubble is damped by the transmission factor.
     *
     * This makes the policy purely subtractive, which under-states it: two
     * households merged because one member of each happened to be tied keep
     * exactly that one contact between them, and everybody else in the two
     * households never meets. The same gap exists inside a household, where two
     * members with no edge between them never meet either.
     */
    Existing,
    /**
     * The bubble is a group of a within-group mixing layer, the household
     * layer (`Bubbles::make_layer()`). Outside the policy window each agent
     * is in its household's group, where each pair meets with the household
     * contact probability; while the policy is in force it is in its
     * bubble's, where each pair meets with the bubble contact probability
     * (households left on their own keep theirs), and it goes back when the
     * policy lifts.
     *
     * This is what the modelled policy actually says -- the household you have
     * bubbled with is a household you now see. The force of infection inside a
     * bubble grows with its size, since every bubblemate met is an independent
     * exposure. There is one probability per group, so housemates in a merged
     * bubble meet at the bubble's, like the other households.
     *
     * Contacts *outside* the bubble (the network layers) are untouched and
     * still damped by the transmission factor, which is what makes the bubble
     * imperfect. A pair of bubblemates that is also tied in a network layer
     * meets in both layers.
     */
    Complete
};

class Bubbles;

/**
 * @brief Tool carried by every agent under a Bubbles policy.
 * @ingroup globalevents
 *
 * @details The tool does not hold the bubble partition -- it holds a pointer to
 * the `Bubbles` intervention that owns it, resolved by name from the model the
 * first time the tool is used. `clone_ptr()` clears that pointer, exactly as
 * `ToolVaccine` clears its per-agent immunity, so a copy of the tool always
 * re-resolves against the model it ends up in. Copies of a model therefore
 * never reach into the model they were copied from, which is what lets bubble
 * models run in parallel replicates.
 *
 * Users do not instantiate this directly; the intervention hands it to every
 * agent when it sets itself up, at the start of each run.
 *
 */
class BubbleTool : public Tool {
private:

    std::string _event_name;                  ///< Name of the owning intervention.
    Bubbles * _policy = nullptr;        ///< Resolved lazily, per model.

public:

    BubbleTool(std::string name, std::string event_name);

    /**
     * @brief Reduction applied to an exposure of this tool's agent.
     *
     * `0.0` when the transmitter shares the agent's bubble (contacts inside the
     * bubble are what the policy preserves), and `1 - f` otherwise, where `f`
     * is the intervention's transmission factor. Outside the policy window, or
     * before a partition exists, the reduction is `0.0`.
     */
    epiworld_double get_susceptibility_reduction(
        VirusPtr & v,
        Model * model
    ) override;

    std::unique_ptr<Tool> clone_ptr() const override;

};

/**
 * @brief Social-bubble contact-restriction intervention for network models.
 * @ingroup globalevents
 *
 * @details A "social bubble" policy lets people keep seeing a small, fixed set
 * of others while cutting off the rest of their contacts. `Bubbles` implements
 * this for models with network layers (e.g. `ModelSEIR` or `ModelSIR` with a
 * `layer::Network`; it reads the contacts of every network layer), and is
 * designed for the household-based rules used during COVID-19.
 *
 * ## How it works
 *
 * The intervention is a global event, so installing it is a one-liner:
 *
 * ```cpp
 * model.add_globalevent(bubbles);
 * ```
 *
 * Everything else happens by itself. At the start of every run -- from
 * `Model::reset()`, so the policy is in force on day 1 -- the intervention sets
 * itself up on the model it is running in: it registers the
 * transmission-factor parameter, draws the bubble partition with that run's
 * RNG, and hands a `BubbleTool` ("Social bubble") to every agent. Each
 * replicate of `run_multiple()` -- each of which runs on its own copy of the
 * model, possibly on its own thread -- repeats this with its own seed.
 *
 * By default the contacts are **not modified** (`BubbleTies::Existing`); with
 * `BubbleTies::Complete` the bubble is also a group of the household layer,
 * which is described under *Two ways to be in a bubble* below. Either way, when a
 * susceptible agent `p` is exposed to an infectious neighbor, the tool
 * identifies the transmitter through the virus (`v->get_agent()`) and compares
 * the two agents' bubble labels:
 *
 * - same bubble  -> reduction `0.0`: contacts *inside* the bubble are what the
 *                   policy preserves, so they are left untouched;
 * - different bubbles -> reduction `1 - f`, i.e. transmission along contacts
 *                   *outside* the bubble is scaled by the transmission factor
 *                   `f`;
 * - outside the policy window -> reduction `0.0`, no effect.
 *
 * The transmission factor `f` is how strictly the bubble is observed:
 * `f == 0` is a perfectly efficient bubble (out-of-bubble contact is cut
 * entirely, which is equivalent to deleting those edges), `f == 1` disables the
 * intervention (out-of-bubble contact is as good as before), and intermediate
 * values model a soft contact reduction -- people still meet outside their
 * bubble, just less often or more carefully.
 *
 * `f` is **not** stored in the intervention: setup registers it as a model
 * parameter (`param_name`, "Bubble transmission factor" by default) and the
 * tool reads it from the model on every exposure. It can therefore be inspected
 * with `model.get_param()`, changed mid-run with `model.set_param()`, or swept
 * over in a calibration without rebuilding the intervention. The value given to
 * the constructor is only a default: if the model already carries that
 * parameter -- because it was set with `model.add_param()` or read from a
 * parameter file with `model.read_params()` before the run -- the model's value
 * is kept. Values outside `[0, 1]` are clamped.
 *
 * Since reductions combine as `1 - prod(1 - r_i)`, a reduction of `1.0`
 * (`f == 0`) zeroes the transmission probability regardless of any other tools
 * the agent carries. Contact weights are uniform in these models, so
 * suppressing transmission on out-of-bubble contacts is equivalent to deleting
 * those contacts, while keeping the network intact for other purposes (contact
 * tracing, output).
 *
 * ## Where the state lives
 *
 * `Bubbles` *is* the model's global event: `add_globalevent()` hands the model a
 * clone of it, and the bubble partition (`get_bubble_id()`) is a member of that
 * clone. Since `Model`'s copy constructor deep-copies global events and tools,
 * every copy of a model -- including the per-thread copies `run_multiple()`
 * makes -- owns its partition outright, and the tool resolves to the
 * intervention of whichever model it belongs to. Nothing is shared between
 * models, so replicates may run on as many threads as you like.
 *
 * The object you construct is a template: once added to a model it is no longer
 * connected to it, and its own `get_bubble_id()` stays empty. To look at the
 * partition of a model that has run, ask the model for its copy:
 *
 * ```cpp
 * const auto & bubble_id = Bubbles::get_from(model)->get_bubble_id();
 * ```
 *
 * ## Forming bubbles (why ties matter)
 *
 * Households are declared with a per-agent `household_id` vector (one entry per
 * agent, indexed by agent id). Bubbles are always:
 *
 * - **disjoint** -- every agent belongs to exactly one bubble;
 * - **household-preserving** -- a household is never split; and
 * - **connection-aware** -- bubbles only ever join households that are actually
 *   connected in the contact network.
 *
 * The last point is essential, and it holds for both realizations. Under
 * `BubbleTies::Existing` the intervention can only suppress transmission along
 * edges that exist, so putting two households that share no contact into one
 * bubble would change nothing at all and `group_size` would be inert. Under
 * `BubbleTies::Complete` they would meet, but bubbling two households that have
 * never met is not the policy being modelled: a household chooses a bubble
 * partner it already socialises with.
 *
 * ## Two ways to be in a bubble
 *
 * `BubbleFlavor` decides *how households are grouped*. `BubbleTies` decides
 * *what being grouped does*, and the two are independent.
 *
 * Under **`BubbleTies::Existing`** (the default, and the original behaviour) a
 * bubble is only a transmission rule. This under-states the policy in a way
 * worth being explicit about: if households `a` and `b` were merged because one
 * member of each happened to be tied, that single contact is the only one
 * preserved between them -- the rest of `a` never meets the rest of `b`, though
 * the policy says the two households have bubbled. The same gap exists inside a
 * household, whose members need not all be tied to each other.
 *
 * Under **`BubbleTies::Complete`** the bubble is a group of the household
 * layer, a within-group mixing layer that `make_layer()` builds from
 * `household_id`:
 *
 * ```cpp
 * // Housemates meet every day; two members of a merged bubble, 3 days in 10
 * bubbles.set_ties(BubbleTies::Complete, "household", 0.3);
 * model.add_layer(bubbles.make_layer(1.0));
 * model.add_globalevent(bubbles);
 * ```
 *
 * Outside the policy window everyone meets their household, each pair with
 * the household contact probability. While the policy is in force each agent
 * is in its bubble's group instead, where each pair meets with the bubble
 * contact probability; a household that bubbled with nobody keeps its own.
 * A within-group layer has one probability per group, so in a merged bubble
 * housemates meet at the bubble's rate too. When the bubbles are redrawn
 * agents move to the new ones, and when the policy lifts they go back to
 * their households (with the probabilities the layer had at reset). Moves are
 * a layer's ordinary edits: they take effect at the end of the step, a reset
 * puts everyone back, and a traced layer records them in its history
 * (`groupmates_of()`). No ties are stored, and the network layers are left
 * alone. The layer must start each run with one group per household, as
 * `make_layer()` builds it; setup checks it.
 *
 * Completing a bubble **raises the force of infection inside it**: every
 * bubblemate met is an independent exposure, so with the default (everyone,
 * every day) a bubble of `k` agents gives each member `k - 1` chances a day;
 * a bubble contact probability below 1 thins the meetings (the policy says
 * the households may meet, not that they meet daily). A pair of bubblemates
 * that is also tied in a network layer meets in both layers.
 *
 * Contacts *outside* the bubble are untouched by either setting: they stay in
 * the network and are damped by `f`, which is what makes the bubble imperfect.
 *
 * Grouping reads the network layers only, so `BubbleFlavor` draws the same
 * bubbles whatever `BubbleTies` says. An agent is in one group per layer, so
 * two `Complete` policies on one model (household bubbles and school bubbles,
 * say) each need their own layer; sharing one is an error. `Complete` runs
 * within `Model::run()` (or `run_multiple()`): a move outside a run would edit
 * the layer's baseline.
 *
 * ### Matching a policy
 *
 * These are the closest settings for two styles of rule used during
 * COVID-19, not exact matches:
 *
 * | Policy style                                       | Settings                                       |
 * |----------------------------------------------------|------------------------------------------------|
 * | Households join one or a few others (New Zealand;  | `Household`, `group_size` = households per     |
 * | UK support bubbles; Belgium, May 2020)             | bubble, `Complete`                             |
 * | One close contact per person (Belgium, Oct. 2020)  | `Peer`, `max_households = 2` (the default),    |
 * |                                                    | `Existing`                                     |
 *
 * The second is an approximation. `Peer` commits whole households, so with
 * `max_households = 2` one accepted choice pairs two households and the other
 * members of either can choose no one else (`group_size` then has no effect;
 * see *The algorithms*). Under `Existing` every tie between the two households
 * is spared, not only the chosen one. The model therefore gives a household
 * one partner household where the rule gives each person their own contact. A
 * larger `max_households` lets more members choose, at the cost of larger
 * bubbles. `Peer` with `Complete` has both households meet in full: more
 * contact than the rule allows, but still allowed, for the case where a
 * person brings their household along.
 *
 * ## The algorithms
 *
 * Both rules start from the **household contact graph**: one node per
 * household, with an edge between two households whenever at least one member
 * of the first is connected to a member of the second in the network layers.
 * All random draws use the model's RNG, so a run is reproducible from its
 * seed.
 *
 * **`BubbleFlavor::Household`** -- grow bubbles from seed households:
 *
 * 1. Visit households in random order.
 * 2. Skip a household if it already belongs to a bubble; otherwise open a new
 *    bubble containing it, and set the *frontier* to its unassigned neighbours
 *    in the household contact graph.
 * 3. While the bubble holds fewer than `group_size` households and the frontier
 *    is not empty, draw a household from the frontier at random, add it to the
 *    bubble, and extend the frontier with that household's unassigned
 *    neighbours. A household that is tied to several members of the bubble
 *    appears in the frontier more than once and is correspondingly more likely
 *    to be drawn, so stronger ties are favoured.
 * 4. Stop when no unassigned neighbour remains, even if the bubble is smaller
 *    than `group_size`.
 *
 * Each bubble is therefore a *connected* subgraph of the household contact
 * graph -- not necessarily a clique, so with `group_size > 2` two households in
 * one bubble need not be tied to each other directly. Because a household with
 * no available partner is left on its own, the number of bubbles is at least
 * `ceil(n_households / group_size)`. Cost is linear in the number of edges.
 *
 * **`BubbleFlavor::Peer`** -- agents choose peers from those still available:
 *
 * Households are kept in a disjoint-set (union-find) structure that also tracks
 * how many households each bubble holds, so a bubble that is full can be
 * recognised at once.
 *
 * 1. Visit the agents in random order.
 * 2. Skip an agent whose household is already in a full bubble -- it has left
 *    the pool and can neither choose nor be chosen.
 * 3. Otherwise, repeatedly draw one of the agent's contacts outside its own
 *    household, at random and without replacement, until the agent has made
 *    `group_size` successful choices or has no contact left that its bubble can
 *    still take in. A draw is accepted when the two households are in different
 *    bubbles **and** the merged bubble would hold at most `max_households`
 *    households; otherwise that contact is simply unavailable and the agent
 *    draws again. Accepting a draw merges the two households (household
 *    commitment), and the agent stops once its bubble is full.
 *
 * The cap is what makes the rule work. Without it the merges percolate: with a
 * few members per household each choosing someone, the household graph becomes
 * connected and every household lands in one giant bubble -- no restriction at
 * all.
 *
 * Note that `max_households` is the effective dial on bubble size, while
 * `group_size` rarely binds: because a choice by *any* member commits the whole
 * household, the members of one household together tend to fill its bubble
 * regardless of how many choices each of them is allowed. With
 * `max_households == 2`, in particular, one accepted choice fills the bubble and
 * `group_size` has no effect at all. Households whose every contact was taken
 * first remain on their own.
 *
 * ## Scheduling
 *
 * `start_day` is the first day the policy applies; `end_day` is the (exclusive)
 * day it lifts, or `< 0` to never lift. With `rewire_every > 0` the partition is
 * re-randomised every that-many days, modelling policies whose bubbles change
 * over time (e.g. contacts renewed weekly); `0` keeps a fixed bubble.
 *
 * The initial partition is drawn when the intervention sets itself up, at reset
 * time, so it is redrawn for each replicate of `run_multiple()` using that
 * replicate's seed and is already in force on day 1. Re-randomisations are
 * applied by the intervention itself (a daily global event) and, since global
 * events run after each day's transitions, take effect the following step.
 *
 * ## Example
 *
 * ```cpp
 * epimodels::ModelSEIR model("flu", 10000, 0.01, 0.1, 4.5, 1.0/7.0);
 * model.add_layer(layer::network(
 *     "contacts", graph::smallworld(8, 0.05)
 * ));
 *
 * std::vector< size_t > household_id(10000);
 * for (size_t i = 0u; i < 10000; ++i)
 *     household_id[i] = i / 3;             // households of three
 *
 * // Two households per bubble from day 10, halving out-of-bubble transmission.
 * Bubbles bubbles(
 *     household_id, BubbleFlavor::Household,
 *     2,      // group_size
 *     0.5,    // transmission_factor (default of the model parameter)
 *     10      // start_day
 * );
 * model.add_globalevent(bubbles);
 *
 * // The factor lives in the model, so it can be changed without touching the
 * // intervention. Setting it before the run overrides the default above.
 * model.add_param(0.25, "Bubble transmission factor", true);
 *
 * model.run(100, 1231);
 *
 * // The partition belongs to the model, not to `bubbles`.
 * const auto & bubble_id = Bubbles::get_from(model)->get_bubble_id();
 * ```
 *
 * To have those two households actually meet -- every member of one meeting
 * every member of the other, every day -- make the bubble a group of the
 * household layer:
 *
 * ```cpp
 * bubbles.set_ties(BubbleTies::Complete, "household");
 * model.add_layer(bubbles.make_layer());
 * model.add_globalevent(bubbles);
 * ```
 *
 * A household-only lockdown in which the household meets in full, and everyone
 * else is reachable only through an imperfectly observed bubble, is
 * `group_size == 1` with a non-zero factor:
 *
 * ```cpp
 * Bubbles household_only(
 *     household_id, BubbleFlavor::Household,
 *     1,      // group_size: nobody bubbles with another household
 *     0.15,   // but the bubble leaks: 15% of out-of-bubble transmission remains
 *     0, -1, 0, "Social bubble", 2u, "Bubble transmission factor",
 *     BubbleTies::Complete, "household"
 * );
 * ```
 *
 * @note Both rules produce *exclusive* bubbles, which is what the modelled
 * policies prescribe. A rule that instead grants each person a personal budget
 * of contacts that need be neither mutual nor exclusive (e.g. "up to ten
 * different people a week") is not a partition of the population and cannot be
 * expressed this way.
 *
 * @note Not yet modelled: fixed out-of-bubble "sport partners", travel between
 * regions, and caps on the number of individuals (as opposed to households) a
 * bubble may contain.
 *
 */
class Bubbles final : public GlobalEvent {
private:

    std::vector< size_t > household; ///< Household of each agent, 0..n_households - 1.
    size_t n_households;
    BubbleFlavor flavor;
    size_t group_size;               ///< households per bubble (Household) or max peers (Peer).
    size_t max_households;           ///< cap on households per bubble (Peer only).
    epiworld_double transmission_factor; ///< initial value of the model parameter, in [0, 1].
    int start_day;
    int end_day;
    int rewire_every;
    std::string param_name;          ///< model parameter holding the transmission factor.
    BubbleTies ties;                 ///< whether bubblemates meet in a layer.
    std::string layer;               ///< the household layer of `BubbleTies::Complete`.
    double bubble_contact_prob;      ///< daily meeting probability in a merged bubble.

    /// The household layer of `BubbleTies::Complete` (throws if there is none).
    layer::Mixing & bubble_layer(Model * model) const;

    // Per-model state. Copied with the intervention, so each model (including
    // each per-thread copy made by run_multiple) owns its own partition.
    std::vector< int > bubble_id;    ///< Per-agent bubble label; -1 = unassigned.

    std::vector< double > home_prob; ///< Contact probability of each group at reset.
    int placed_epoch = -1;           ///< Partition agents are in (-1: their homes).
    int model_id   = -1;             ///< Sim id this intervention was set up for.
    int last_epoch = -1;             ///< Rewiring epoch the current partition was computed for.

    /// @name The two grouping rules.
    ///@{
    void partition_household(Model * model);
    void partition_peer(Model * model);
    ///@}

    /**
     * @brief Install the intervention on the model it is running in.
     *
     * @details Called from `reset()`, i.e. once per run and once per replicate
     * of `run_multiple()`. It registers the transmission-factor parameter
     * (keeping any value the model already has), draws the epoch-0 partition
     * with the run's RNG, and gives the bubble tool to every agent.
     *
     * @param model Model the intervention belongs to.
     * @throws std::length_error if `household_id` does not have exactly one
     *         entry per agent.
     */
    void _setup(Model * model);

    /**
     * @brief Moves agents in the household layer to match the policy as of
     *        the next simulation step.
     *
     * @details Into their bubbles when the policy is about to apply (or the
     * partition was redrawn), back to their households when it is about to
     * lapse. Nothing under `BubbleTies::Existing`.
     */
    void sync_groups(Model * model);

public:

    /**
     * @brief Configure a social-bubble policy.
     *
     * @param household_id Household label of each agent, indexed by agent id.
     *        Its length must equal the number of agents in the model. Labels are
     *        arbitrary (they need not be consecutive); agents sharing a label
     *        form a household and are always placed in the same bubble.
     * @param flavor Whether bubbles are chosen by households or by individuals
     *        (see `BubbleFlavor`).
     * @param group_size For `Household`, the maximum number of households per
     *        bubble (`1` = strict household-only lockdown). For `Peer`, the
     *        maximum number of external peers each agent may pick.
     * @param transmission_factor Initial value of the model parameter
     *        `param_name`: the multiplier applied to transmission between
     *        agents of *different* bubbles, in `[0, 1]`. `0.0` (the default) is
     *        a perfectly efficient bubble -- contact outside it is cut
     *        entirely; `0.5` halves out-of-bubble transmission (a soft contact
     *        reduction); and `1.0` turns the intervention off. Transmission
     *        within a bubble is never altered. The value is stored in the model
     *        (see `deploy()`), not here, so it can be changed at any time with
     *        `model.set_param(param_name, ...)`.
     * @param start_day First day on which the policy applies.
     * @param end_day Day on which the policy is lifted (exclusive). Use a
     *        negative value for a policy that never ends. Must be greater than
     *        `start_day`.
     * @param rewire_every Re-randomise the bubbles every this-many days, for
     *        policies whose contacts change over time. `0` keeps the bubbles
     *        fixed for the whole intervention.
     * @param name Name given to the tool and to the intervention's global
     *        event; the tool finds its policy by this name, and `get_from()`
     *        looks it up on a model with it.
     * @param max_households **`Peer` flavor only**: the largest number of
     *        households a bubble may contain. A nomination that would exceed it
     *        is declined, which is what stops one household's choices from
     *        chaining into the next (see `BubbleFlavor::Peer`). The default of
     *        `2` represents two households joined by a close contact. Ignored by
     *        the `Household` flavor, where `group_size` already is the cap.
     * @param param_name Name of the model parameter that holds the transmission
     *        factor. Give two interventions deployed on the same model
     *        different names if they are to be dialled independently.
     * @param ties Whether the bubble is only a transmission rule
     *        (`BubbleTies::Existing`, the default) or also a group of the
     *        household layer, where bubblemates meet every day
     *        (`BubbleTies::Complete`). See `BubbleTies`.
     * @param layer Name of the household layer of `BubbleTies::Complete`
     *        (required then; see `make_layer()`).
     * @param bubble_contact_prob With `BubbleTies::Complete`, the probability
     *        that two members of a bubble that merged households meet on a
     *        given day, in `[0, 1]`.
     *
     * @throws std::range_error if `transmission_factor` or
     *         `bubble_contact_prob` is outside `[0, 1]`, if
     *         `group_size` is zero for the `Household` flavor, if
     *         `max_households` is less than 2 for the `Peer` flavor, or if
     *         `end_day` is non-negative and not greater than `start_day`.
     */
    Bubbles(
        std::vector< size_t > household_id,
        BubbleFlavor flavor,
        size_t group_size,
        epiworld_double transmission_factor = 0.0,
        int start_day = 0,
        int end_day = -1,
        int rewire_every = 0,
        std::string name = "Social bubble",
        size_t max_households = 2u,
        std::string param_name = "Bubble transmission factor",
        BubbleTies ties = BubbleTies::Existing,
        std::string layer = "",
        double bubble_contact_prob = 1.0
    );

    /**
     * @brief The model's copy of the intervention.
     *
     * @param model Model the intervention was added to.
     * @param name Name it was added under.
     * @return Pointer to the model's own intervention, or `nullptr` if the
     *         model has no global event by that name, or it is not a `Bubbles`.
     */
    static Bubbles * get_from(
        Model & model,
        const std::string & name = "Social bubble"
    );

    /**
     * @brief Recompute the partition using the model's RNG.
     *
     * Called when the intervention sets itself up (at reset) and at each
     * rewiring epoch. Rarely needed directly.
     */
    void compute_partition(Model * model);

    /**
     * @brief Current bubble label of every agent, indexed by agent id.
     *
     * Two agents in the same bubble transmit as they would without the policy;
     * transmission between labels is scaled by the transmission factor.
     * Populated at the start of each run, and only on the model's own copy of
     * the intervention -- see `get_from()`.
     */
    const std::vector< int > & get_bubble_id() const;

    /// @brief The rule used to form bubbles.
    BubbleFlavor get_flavor() const;

    /// @brief Whether bubblemates meet in the household layer.
    BubbleTies get_ties() const;

    /**
     * @brief Choose how the bubble is realized. See `BubbleTies`.
     *
     * Set it before the run; the intervention reads it when it sets itself up.
     * Remember that the model owns its own copy once the intervention has been
     * added, so set this on the object you are about to add, or on the model's
     * copy via `get_from()`.
     *
     * @param layer Name of the household layer of `BubbleTies::Complete`
     *        (required then; see `make_layer()`).
     * @param bubble_contact_prob The probability that two members of a bubble
     *        that merged households meet on a given day, in `[0, 1]`.
     */
    void set_ties(
        BubbleTies ties,
        std::string layer = "",
        double bubble_contact_prob = 1.0
    );

    /**
     * @brief The household layer of `BubbleTies::Complete`, to add to the
     *        model with `Model::add_layer()`.
     *
     * @details A within-group mixing layer named as in `set_ties()`, with one
     * group per household (from `household_id`), in which each pair meets
     * with probability `household_contact_prob` a day; the policy moves agents
     * to their bubble's group while it is in force (see `BubbleTies`). The
     * layer can be changed further before it is added (e.g.,
     * `enable_tracing()`).
     *
     * @param household_contact_prob The probability that two housemates meet
     *        on a given day (`1`: every day).
     * @throws std::logic_error if the layer has no name.
     */
    std::shared_ptr< layer::Mixing > make_layer(
        double household_contact_prob = 1.0
    ) const;

    /**
     * @brief Name of the model parameter holding the transmission factor.
     *
     * Use it with `model.get_param()` / `model.set_param()` to read or change
     * how leaky the bubbles are, including in the middle of a run.
     */
    const std::string & get_param_name() const;

    /// @brief True when the policy applies on the given day.
    bool is_active(int today) const;

    /**
     * @brief Rewiring epoch the current partition was computed for.
     *
     * `0` for the partition drawn at setup, incrementing every `rewire_every`
     * days while the policy is active. `-1` before the first partition.
     */
    int get_last_epoch() const;

    /**
     * @brief Reduction applied to an exposure between two agents.
     *
     * The tool's side of the intervention; see `BubbleTool`.
     */
    epiworld_double susceptibility_reduction(
        const Agent * p,
        const Agent * transmitter,
        Model * model
    ) const;

    /**
     * @brief Installs the intervention on the model, once per run.
     *
     * Called by `Model::reset()`; see `_setup()`. This is what makes adding the
     * intervention to a model the only step there is.
     */
    void reset(Model * model) override;

    /// @brief Re-randomises the partition at rewiring epochs, and keeps the
    /// household layer of a `Complete` bubble in step with the policy.
    void operator()(Model * model, int day) override;

    std::unique_ptr< GlobalEvent > clone_ptr() const override;

};

#endif
/*//////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

 End of -./include/epiworld/globalevents/bubbles-bones.hpp-

////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////*/



    // Classes with their member functions (they come before the other
    // member functions, which use them), and the graphs they build on
/*//////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

 Start of -./include/epiworld/randgraph.hpp-

////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////*/


#ifndef EPIWORLD_RANDGRA
#define EPIWORLD_RANDGRA

class Model;

class Agent;

class AdjList;


/**
 * @brief The first `i` with `prob <= cumulative[i]`, or the last index.
 *
 * @details The draw of an agent with probability proportional to its weight,
 * given the cumulative probabilities (non-decreasing); the same index as a
 * scan from the start, found by bisection.
 */
inline size_t pick_cumulative(
    const std::vector< epiworld_double > & cumulative,
    epiworld_double prob
)
{
    auto it = std::lower_bound(cumulative.begin(), cumulative.end(), prob);
    return (it == cumulative.end()) ?
        cumulative.size() - 1u :
        static_cast< size_t >(it - cumulative.begin());
}

/**
 * @brief Degree-preserving rewiring of a graph under construction (used by
 * `rgraph_smallworld()`).
 *
 * @details `rows[i]` holds the neighbors of `i` in ascending order, without
 * repeats (in an undirected graph, each tie in the rows of both ends), and is
 * kept that way. Picks two agents with probability proportional to their
 * degree and one tie of each, uniformly, and swaps them, skipping swaps that
 * would create self-ties or repeated ties. A network layer is rewired with
 * `graph::rewire_degseq()` instead.
 */
inline void rewire_degseq_rows(
    std::vector< std::vector< int > > & rows,
    bool directed,
    Model & model,
    epiworld_double proportion
    )
{

    #ifdef EPI_DEBUG
    std::vector< size_t > _degree0(rows.size(), 0u);
    for (size_t i = 0u; i < rows.size(); ++i)
        _degree0[i] = rows[i].size();
    #endif

    std::vector< int > non_isolates;
    non_isolates.reserve(rows.size());

    std::vector< epiworld_double > weights;
    weights.reserve(rows.size());

    epiworld_double nedges = 0.0;
    for (size_t i = 0u; i < rows.size(); ++i)
    {
        if (rows[i].empty())
            continue;

        non_isolates.push_back(static_cast< int >(i));
        const epiworld_double nties = static_cast< epiworld_double >(rows[i].size());
        if (directed)
        {
            weights.push_back(nties);
            nedges += nties;
        }
        else
        {
            weights.push_back(nties / 2.0);
            nedges += nties / 2.0;
        }
    }

    if (non_isolates.size() == 0u)
        throw std::logic_error("The graph is completely disconnected.");

    // Cumulative probs
    weights[0u] /= nedges;
    for (size_t i = 1u; i < non_isolates.size(); ++i)
    {
         weights[i] /= nedges;
         weights[i] += weights[i - 1u];
    }

    const int N = static_cast< int >(non_isolates.size());
    int nrewires = floor(proportion * nedges / (directed ? 1.0 : 2.0));

    auto has = [](const std::vector< int > & row, int j) {
        return std::binary_search(row.begin(), row.end(), j);
    };

    auto insert = [](std::vector< int > & row, int j) {
        row.insert(std::lower_bound(row.begin(), row.end(), j), j);
    };

    auto erase = [](std::vector< int > & row, int j) {
        row.erase(std::lower_bound(row.begin(), row.end(), j));
    };

    while (nrewires-- > 0)
    {

        // Picking egos
        int id0 = static_cast< int >(pick_cumulative(weights, model.runif()));
        int id1 = static_cast< int >(pick_cumulative(weights, model.runif()));

        // Correcting for under or overflow.
        if (id1 == id0)
            id1++;

        if (id1 >= N)
            id1 = 0;

        const int ego0 = non_isolates[id0];
        const int ego1 = non_isolates[id1];
        std::vector< int > & p0 = rows[ego0];
        std::vector< int > & p1 = rows[ego1];

        // Picking alters (relative location in their lists), uniformly
        int id01 = model.runif_index(p0.size());
        int id11 = model.runif_index(p1.size());

        // From positions to ids. The scan keeps comparing the count with the
        // id it found (so a later position can replace it), as the version on
        // std::map did; kept so that the same seed gives the same graph.
        int count = 0;
        for (int j : p0)
            if (count++ == id01)
                id01 = j;

        count = 0;
        for (int j : p1)
            if (count++ == id11)
                id11 = j;

        // After the swap: (ego0, id11) and (ego1, id01). Skip self-ties,
        // repeated ties, and swaps of the same neighbor.
        if (id01 == ego1 || id11 == ego0)
            continue;

        if (has(p0, id11) || has(p1, id01))
            continue;

        if (id01 == id11)
            continue;

        erase(p0, id01);
        erase(p1, id11);
        insert(p0, id11);
        insert(p1, id01);

        // For undirected graphs, also update from alter perspectives
        if (!directed)
        {
            erase(rows[id01], ego0);
            erase(rows[id11], ego1);
            insert(rows[id01], ego1);
            insert(rows[id11], ego0);
        }

    }

    #ifdef EPI_DEBUG
    for (size_t _i = 0u; _i < _degree0.size(); ++_i)
    {
        if (_degree0[_i] != rows[_i].size())
            throw std::logic_error(
                "[epi-debug] Degree does not match afted rewire_degseq. " +
                std::string("Expected: ") +
                std::to_string(_degree0[_i]) +
                std::string(", observed: ") +
                std::to_string(rows[_i].size())
                );
    }
    #endif

}

/**
 * Threshold for the hybrid row-advance in Batagelj-Brandes.
 *
 * When a geometric skip lands past the current row of the lower-triangle
 * enumeration, the flat index must be mapped back to (i, j). For small
 * excess (j - i <= threshold) the integer while-loop is fastest; for
 * large excess the O(1) inverse-triangular-number formula avoids
 * O(sqrt(excess)) iterations. 200 keeps the loop body to at most ~20
 * iterations (sqrt(2 * 200) ~ 20) before switching to FPU math.
 */
static constexpr long long BB_FPU_THRESHOLD = 200;

/**
 * @brief Generates an Erdős–Rényi random graph G(n, p) using the
 * Batagelj-Brandes algorithm.
 *
 * Uses geometric skipping to advance through the space of possible edges,
 * generating each edge independently with probability p. This produces
 * **unbiased** edge counts in O(n + m) expected time, where m is the
 * number of edges generated.
 *
 * Reference: V. Batagelj and U. Brandes, "Efficient generation of large
 * random networks," Physical Review E, vol. 71, no. 3, 2005.
 *
 * @param n Number of nodes.
 * @param p Edge probability, in [0, 1].
 * @param directed Whether the graph is directed.
 * @param model A reference to the Model, used for random number generation.
 * @return An AdjList representing the generated network.
 * @throws std::range_error if `p` is not in [0, 1].
 */
inline AdjList rgraph_bernoulli(
    epiworld_fast_uint n,
    epiworld_double p,
    bool directed,
    Model & model
) {

    // Written so that NaN fails too
    if (!((p >= 0.0) && (p <= 1.0)))
        throw std::range_error(
            "The edge probability must be in [0, 1]; got " +
            std::to_string(p) + "."
        );

    std::vector< int > source;
    std::vector< int > target;

    // No ties, and nothing to draw: with p = 0, log(1 - p) below is 0 and
    // the first skip would be infinite
    if (p == 0.0)
        return AdjList(source, target, static_cast<int>(n), directed);

    // Pre-allocate based on expected edge count
    double n_possible = directed
        ? static_cast<double>(n) * (n - 1.0)
        : static_cast<double>(n) * (n - 1.0) / 2.0;
    source.reserve(static_cast<size_t>(n_possible * p * 1.1));
    target.reserve(static_cast<size_t>(n_possible * p * 1.1));

    // Batagelj-Brandes: use geometric skipping to enumerate edges.
    // log(1 - p) is precomputed; each skip is 1 + floor(log(U) / lp).
    double lp = std::log(1.0 - p);

    if (!directed)
    {
        // Undirected: iterate over (i, j) with i > j
        long long i = 1;
        long long j = -1;

        while (i < static_cast<long long>(n))
        {
            // Geometric skip
            double r = model.runif();

            // Avoid log(0)
            if (r == 0.0)
                r = std::numeric_limits<double>::min();

            j += 1 + static_cast<long long>(
                std::floor(std::log(r) / lp)
            );

            // Advance to the next row if j >= i.
            // Hybrid approach: for large skips, use O(1) FPU
            // inverse-triangular-number formula instead of the
            // integer loop, avoiding O(sqrt(skip)) iterations.
            if (j >= i && i < static_cast<long long>(n))
            {
                if (j - i > BB_FPU_THRESHOLD)
                {
                    // O(1) FPU mapping: flat index → (i, j) via
                    // inverse triangular number. `flat` fits in
                    // long long since n ≤ INT_MAX (AdjList limit),
                    // so i*(i-1)/2 + j < n*(n-1)/2 ≪ 2^62.
                    long long flat = i * (i - 1) / 2 + j;
                    double df = static_cast<double>(flat);
                    i = static_cast<long long>(std::floor(
                        (1.0 + std::sqrt(1.0 + 8.0 * df)) / 2.0
                    ));
                    j = flat - i * (i - 1) / 2;
                    // Floating-point correction: sqrt may round i
                    // up or down by 1 for large flat values, so j
                    // may land outside [0, i). Each correction loop
                    // runs at most 1–2 iterations; only one of the
                    // two can execute for a given (i, j).
                    while (j >= i) { j -= i; i++; }
                    while (j < 0)  { i--; j += i; }
                }
                else
                {
                    while (j >= i && i < static_cast<long long>(n))
                    {
                        j -= i;
                        i++;
                    }
                }
            }

            if (i < static_cast<long long>(n))
            {
                source.push_back(static_cast<int>(i));
                target.push_back(static_cast<int>(j));
            }
        }
    }
    else
    {
        // Directed: iterate over (i, j) with i != j
        // We linearize pairs: for row i, columns [0..n-1] excluding i.
        // Position k in [0, n*(n-1)) maps to row = k/(n-1),
        // col = k%(n-1), if col >= row then col++.
        long long total = static_cast<long long>(n) * (n - 1);
        long long k = -1;

        while (true)
        {
            double r = model.runif();
            if (r == 0.0)
                r = std::numeric_limits<double>::min();

            k += 1 + static_cast<long long>(
                std::floor(std::log(r) / lp)
            );

            if (k >= total)
                break;

            long long row = k / static_cast<long long>(n - 1);
            long long col = k % static_cast<long long>(n - 1);
            if (col >= row)
                col++;

            source.push_back(static_cast<int>(row));
            target.push_back(static_cast<int>(col));
        }
    }

    AdjList al(source, target, static_cast<int>(n), directed);

    return al;

}

inline AdjList rgraph_ring_lattice(
    epiworld_fast_uint n,
    epiworld_fast_uint k,
    bool directed = false
) {

    if ((n - 1u) < k)
        throw std::logic_error("k can be at most n - 1.");

    std::vector< int > source;
    std::vector< int > target;

    if (!directed)
        if (k > 1u) k = static_cast< size_t >(floor(k / 2.0));

    for (size_t i = 0; i < n; ++i)
    {

        for (size_t j = 1u; j <= k; ++j)
        {

            // Next neighbor
            size_t l = i + j;
            if (l >= n) l = l - n;

            source.push_back(i);
            target.push_back(l);

        }

    }

    return AdjList(source, target, n, directed);

}

/**
 * @brief Smallworld network (Watts-Strogatz)
 *
 * @param n
 * @param k
 * @param p
 * @param directed
 * @param model
 * @return AdjList
 */
inline AdjList rgraph_smallworld(
    epiworld_fast_uint n,
    epiworld_fast_uint k,
    epiworld_double p,
    bool directed,
    Model & model
) {

    // Creating the ring lattice
    AdjList ring = rgraph_ring_lattice(n,k,directed);

    if (k == 0u)
        return ring;

    // Rewired as rows, which stay sorted, then loaded back
    const auto & start = ring.get_start();
    const auto & ids = ring.get_ids();
    std::vector< std::vector< int > > rows(n);
    for (size_t i = 0u; i < n; ++i)
        rows[i].assign(
            ids.begin() + static_cast< std::ptrdiff_t >(start[i]),
            ids.begin() + static_cast< std::ptrdiff_t >(start[i + 1u])
        );

    rewire_degseq_rows(rows, directed, model, p);

    // Each undirected tie once
    std::vector< int > source;
    std::vector< int > target;
    source.reserve(ids.size());
    target.reserve(ids.size());
    for (size_t i = 0u; i < n; ++i)
        for (int j : rows[i])
            if (directed || (static_cast< int >(i) <= j))
            {
                source.push_back(static_cast< int >(i));
                target.push_back(j);
            }

    return AdjList(source, target, static_cast< int >(n), directed);

}

/**
 * @brief Generates a blocked network
 *
 * Since block sizes and number of connections between blocks are fixed,
 * this routine is fully deterministic.
 *
 * @param n Size of the network
 * @param blocksize Size of the block.
 * @param ncons Number of connections between blocks
 * @param model A model
 * @return AdjList
 */
inline AdjList rgraph_blocked(
    epiworld_fast_uint n,
    epiworld_fast_uint blocksize,
    epiworld_fast_uint ncons,
    Model&
) {

    std::vector< int > source_;
    std::vector< int > target_;

    size_t i = 0u;
    size_t cum_node_count = 0u;
    while (i < n)
    {

        for (size_t j = 0; j < blocksize; ++j)
        {

            for (size_t k = 0; k < j; ++k)
            {
                // No loops
                if (k == j)
                    continue;

                // Exists the loop in case there are no more
                // nodes available
                if ((i + k) >= n)
                    break;

                source_.push_back(static_cast<int>(j + i));
                target_.push_back(static_cast<int>(k + i));
            }

            // No more nodes left to build connections
            if (++cum_node_count >= n)
                break;

        }

        // Connections between this and the previou sone
        if (i != 0)
        {

            size_t max_cons = std::min(ncons, n - cum_node_count);

            // Generating the connections
            for (size_t j = 0u; j < max_cons; ++j)
            {

                source_.push_back(static_cast<int>(i + j - blocksize));
                target_.push_back(static_cast<int>(i + j));

            }
        }

        i += blocksize;

    }

    return AdjList(source_, target_, n, false);

}

/**
 * @brief Generates a network using a Stochastic Block Model (SBM).
 *
 * This function creates a random undirected network where connections between
 * agents are determined by a mixing matrix and block (group) membership.
 * Each potential edge between agents in blocks \f$g\f$ and \f$h\f$ is included
 * independently with probability \f$M(g, h) / n_h\f$, where \f$M(g, h)\f$ is
 * the entry of the mixing matrix and \f$n_h\f$ is the number of agents in
 * block \f$h\f$.
 *
 * The mixing matrix is **not** row-stochastic. For undirected networks, its
 * row sums should be interpreted as the average expected degree for agents in
 * that group only when the balance condition
 * \f$M(g, h) \times n_g = M(h, g) \times n_h\f$ holds for all pairs
 * \f$(g, h)\f$. In that balanced case,
 * \f[
 *   \mathbb{E}[\text{degree of agent in group } g] = \sum_{h} M(g, h)
 * \f]
 *
 * For undirected networks, the implementation samples between-block edges only
 * once for each unordered pair of blocks (with \f$g \le h\f$) using
 * \f$M(g, h) / n_h\f$. Therefore, if the balance condition is violated, the
 * row sums generally do not match the expected degree from every group's
 * perspective.
 *
 * **Unbiased generation.** Edge generation uses the Batagelj-Brandes
 * algorithm (geometric skipping) independently for each block pair.
 * Each possible edge is included with exactly the correct probability,
 * with no duplicate edge collisions. The expected number of edges for a
 * block pair is exactly:
 * - Within-block: \f$\binom{n_g}{2} \times p_{gg}\f$
 * - Between-block: \f$n_g \times n_h \times p_{gh}\f$
 *
 * **Balance condition warning.** If the balance condition is violated for
 * any pair of blocks and the model's verbose mode is on, a warning is
 * printed indicating which entries are being silently ignored.
 *
 * Reference: V. Batagelj and U. Brandes, "Efficient generation of large
 * random networks," Physical Review E, vol. 71, no. 3, 2005.
 *
 * @param block_sizes A vector of size \f$K\f$ indicating the number of agents
 *   per block. The total number of agents is \f$\sum_k \text{block\_sizes}[k]\f$.
 * @param mixing_matrix A vector of size \f$K \times K\f$ representing the
 *   mixing matrix. The entry \f$M(g, h)\f$ controls the expected number of
 *   connections agents in group \f$g\f$ make with agents in group \f$h\f$.
 * @param row_major If `true`, the mixing matrix is stored in row-major order
 *   (C-style), i.e., \f$M(g, h) = \text{mixing\_matrix}[g \times K + h]\f$.
 *   If `false`, column-major order (Fortran-style) is assumed, i.e.,
 *   \f$M(g, h) = \text{mixing\_matrix}[h \times K + g]\f$.
 * @param model A reference to the Model, used for random number generation.
 * @return An AdjList representing the generated undirected network.
 *
 * @throws std::length_error If `block_sizes` is empty, `mixing_matrix` size
 *   does not equal \f$K^2\f$, or any block size is zero.
 * @throws std::range_error If any mixing matrix entry is negative or if
 *   \f$M(g, h) / n_h > 1\f$ for any pair of blocks.
 */
inline AdjList rgraph_sbm(
    const std::vector< size_t > & block_sizes,
    const std::vector< double > & mixing_matrix,
    bool row_major,
    Model & model
) {

    size_t n_blocks = block_sizes.size();

    if (n_blocks == 0u)
        throw std::length_error(
            "block_sizes must have at least one element."
        );

    if (mixing_matrix.size() != n_blocks * n_blocks)
        throw std::length_error(
            "mixing_matrix must have size K * K, where K = block_sizes.size(). "
            "Expected " + std::to_string(n_blocks * n_blocks) +
            " but got " + std::to_string(mixing_matrix.size()) + "."
        );

    // Total number of agents
    size_t n = 0u;
    const size_t max_adjlist_n = static_cast<size_t>(
        std::numeric_limits<int>::max()
    );
    for (auto bs : block_sizes)
    {
        if (bs == 0u)
            throw std::length_error(
                "All block sizes must be positive."
            );

        if (bs > max_adjlist_n - n)
            throw std::length_error(
                "The total number of agents implied by block_sizes exceeds "
                "the maximum supported graph size (" +
                std::to_string(std::numeric_limits<int>::max()) + ")."
            );

        n += bs;
    }

    // Compute the starting index for each block
    std::vector< size_t > block_start(n_blocks, 0u);
    for (size_t g = 1u; g < n_blocks; ++g)
        block_start[g] = block_start[g - 1u] + block_sizes[g - 1u];

    // Validate mixing matrix entries and compute probabilities.
    // p(g, h) = M(g, h) / n_h for the undirected case.
    // We validate that all entries are non-negative and p(g,h) <= 1.
    // Also estimate the total expected edge count for vector reservation.
    double estimated_edges = 0.0;
    for (size_t g = 0u; g < n_blocks; ++g)
    {
        for (size_t h = 0u; h < n_blocks; ++h)
        {
            double m_gh = row_major
                ? mixing_matrix[g * n_blocks + h]
                : mixing_matrix[h * n_blocks + g];

            if (m_gh < 0.0)
                throw std::range_error(
                    "Mixing matrix entries must be non-negative. "
                    "Entry (" + std::to_string(g) + ", " +
                    std::to_string(h) + ") = " + std::to_string(m_gh) + "."
                );

            double p_gh = m_gh / static_cast<double>(block_sizes[h]);
            if (p_gh > 1.0)
                throw std::range_error(
                    "Mixing matrix entry (" + std::to_string(g) + ", " +
                    std::to_string(h) + ") = " + std::to_string(m_gh) +
                    " implies probability " + std::to_string(p_gh) +
                    " > 1 (block size = " +
                    std::to_string(block_sizes[h]) + ")."
                );
        }

        // Accumulate expected edges for upper triangle (g <= h)
        for (size_t h = g; h < n_blocks; ++h)
        {
            double m_gh = row_major
                ? mixing_matrix[g * n_blocks + h]
                : mixing_matrix[h * n_blocks + g];

            double p_gh = m_gh / static_cast<double>(block_sizes[h]);
            if (p_gh <= 0.0)
                continue;

            double n_possible;
            if (g == h)
                n_possible = static_cast<double>(block_sizes[g]) *
                    static_cast<double>(block_sizes[g] - 1u) / 2.0;
            else
                n_possible = static_cast<double>(block_sizes[g]) *
                    static_cast<double>(block_sizes[h]);

            estimated_edges += n_possible * std::min(p_gh, 1.0);
        }
    }

    // Warn when the balance condition M(g,h)*n_g = M(h,g)*n_h is
    // violated for any pair. The upper triangle is used for sampling,
    // so the lower-triangle entries are silently ignored.
    if (model.get_verbose())
    {
        for (size_t g = 0u; g < n_blocks; ++g)
        {
            for (size_t h = g + 1u; h < n_blocks; ++h)
            {
                double m_gh = row_major
                    ? mixing_matrix[g * n_blocks + h]
                    : mixing_matrix[h * n_blocks + g];
                double m_hg = row_major
                    ? mixing_matrix[h * n_blocks + g]
                    : mixing_matrix[g * n_blocks + h];

                double lhs = m_gh * static_cast<double>(block_sizes[g]);
                double rhs = m_hg * static_cast<double>(block_sizes[h]);

                if (std::abs(lhs - rhs) > 1e-10 *
                    std::max(std::abs(lhs), std::abs(rhs)))
                {
                    printf_epiworld(
                        "Warning [rgraph_sbm]: balance condition violated "
                        "for blocks (%zu, %zu): M(%zu,%zu)*n_%zu = %.4f "
                        "!= M(%zu,%zu)*n_%zu = %.4f. Only M(%zu,%zu)/n_%zu "
                        "is used for edge sampling.\n",
                        g, h,
                        g, h, g, lhs,
                        h, g, h, rhs,
                        g, h, h
                    );
                }
            }
        }
    }

    std::vector< int > source;
    std::vector< int > target;

    // Pre-allocate based on estimated edge count to avoid
    // repeated reallocations during push_back.
    source.reserve(static_cast<size_t>(estimated_edges * 1.1));
    target.reserve(static_cast<size_t>(estimated_edges * 1.1));

    // Batagelj-Brandes geometric skipping for each block pair.
    // For each pair (g, h) with g <= h, we enumerate possible edges
    // and skip ahead using geometric random variables, producing each
    // edge independently with exactly probability p_gh. No duplicates,
    // no bias.
    for (size_t g = 0u; g < n_blocks; ++g)
    {
        for (size_t h = g; h < n_blocks; ++h)
        {

            double m_gh = row_major
                ? mixing_matrix[g * n_blocks + h]
                : mixing_matrix[h * n_blocks + g];

            // Probability of an edge between agents in blocks g and h
            double p_gh = m_gh / static_cast<double>(block_sizes[h]);

            if (p_gh <= 0.0)
                continue;

            // Fast path: p_gh >= 1.0 means all edges are present
            if (p_gh >= 1.0)
            {
                if (g == h)
                {
                    // All unique pairs within the block
                    for (size_t i = block_start[g];
                         i < block_start[g] + block_sizes[g]; ++i)
                    {
                        for (size_t j = i + 1u;
                             j < block_start[g] + block_sizes[g]; ++j)
                        {
                            source.push_back(static_cast<int>(i));
                            target.push_back(static_cast<int>(j));
                        }
                    }
                }
                else
                {
                    // All pairs between blocks g and h
                    for (size_t i = block_start[g];
                         i < block_start[g] + block_sizes[g]; ++i)
                    {
                        for (size_t j = block_start[h];
                             j < block_start[h] + block_sizes[h]; ++j)
                        {
                            source.push_back(static_cast<int>(i));
                            target.push_back(static_cast<int>(j));
                        }
                    }
                }
                continue;
            }

            double lp = std::log(1.0 - p_gh);

            if (g == h)
            {
                // Within-block: iterate (i, j) with i > j, both in
                // [0, n_g). Use BB geometric skipping.
                long long n_g = static_cast<long long>(block_sizes[g]);
                long long i = 1;
                long long j = -1;

                while (i < n_g)
                {
                    double r = model.runif();
                    if (r == 0.0)
                        r = std::numeric_limits<double>::min();

                    j += 1 + static_cast<long long>(
                        std::floor(std::log(r) / lp)
                    );

                    // Hybrid row-advance (see BB_FPU_THRESHOLD
                    // comment in rgraph_bernoulli above).
                    if (j >= i && i < n_g)
                    {
                        if (j - i > BB_FPU_THRESHOLD)
                        {
                            long long flat = i * (i - 1) / 2 + j;
                            double df = static_cast<double>(flat);
                            i = static_cast<long long>(std::floor(
                                (1.0 + std::sqrt(1.0 + 8.0 * df)) / 2.0
                            ));
                            j = flat - i * (i - 1) / 2;
                            while (j >= i) { j -= i; i++; }
                            while (j < 0)  { i--; j += i; }
                        }
                        else
                        {
                            while (j >= i && i < n_g)
                            {
                                j -= i;
                                i++;
                            }
                        }
                    }

                    if (i < n_g)
                    {
                        source.push_back(
                            static_cast<int>(block_start[g] +
                                static_cast<size_t>(i))
                        );
                        target.push_back(
                            static_cast<int>(block_start[g] +
                                static_cast<size_t>(j))
                        );
                    }
                }
            }
            else
            {
                // Between-block: iterate (i, j) with i in [0, n_g),
                // j in [0, n_h). Linearize as k = i * n_h + j.
                long long n_g = static_cast<long long>(block_sizes[g]);
                long long n_h = static_cast<long long>(block_sizes[h]);
                long long total = n_g * n_h;
                long long k = -1;

                while (true)
                {
                    double r = model.runif();
                    if (r == 0.0)
                        r = std::numeric_limits<double>::min();

                    k += 1 + static_cast<long long>(
                        std::floor(std::log(r) / lp)
                    );

                    if (k >= total)
                        break;

                    long long row = k / n_h;
                    long long col = k % n_h;

                    source.push_back(
                        static_cast<int>(block_start[g] +
                            static_cast<size_t>(row))
                    );
                    target.push_back(
                        static_cast<int>(block_start[h] +
                            static_cast<size_t>(col))
                    );
                }
            }

        }
    }

    return AdjList(source, target, static_cast<int>(n), false);

}

#endif
/*//////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

 End of -./include/epiworld/randgraph.hpp-

////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////*/


/*//////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

 Start of -./include/epiworld/layer-mixing.hpp-

////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////*/


#ifndef EPIWORLD_LAYER_MIXING_HPP
#define EPIWORLD_LAYER_MIXING_HPP

namespace layer {

class Mixing;

/**
 * @brief A mixing layer (see `Mixing`).
 * @param name Name of the layer (unique in a model).
 * @ingroup contact_layers
 */
std::shared_ptr< Mixing > mixing(std::string name);

/**
 * @brief Contact layer with mixing: contacts drawn fresh every step, by group.
 *
 * @details Created with `layer::mixing()`. Every agent of a group takes part
 * in the mixing (the contacts are spread over all of them), and the agents in
 * the model's infectious states transmit (`Model::set_infectious_states()`).
 * The quarantine model takes its isolated agents out of the mixing altogether
 * (see `Model::set_excluded_states()`).
 *
 * ### Groups
 *
 * The layer owns the grouping, set with `set_groups()` before the first run:
 * - `assign::labels(v)`, one group per agent (or -1, which leaves the agent
 *   out of the layer): the grouping is *fixed*;
 * - any other `Assign` (e.g., `assign::counts({{0, 300}, {1, 700}})`), drawn
 *   at every reset: each run (and each replicate of `run_multiple()`) then
 *   gets its own draw, from its own seed, as a network layer from a `graph::`
 *   generator does. See `assign.hpp`.
 *
 * ### Contacts: a matrix or a probability per pair
 *
 * The contacts are given in one of two ways:
 * - `set_contact_matrix()`: contacts across groups, for a few groups that meet
 *   each other (age groups in public spaces). Entry `(g, h)` is the expected
 *   number of contacts per day of an agent in `g` with agents in `h`. With
 *   `set_contact_rate(param)`, the contacts are the model parameter `param`
 *   times the matrix, so a global event or a calibration can change them
 *   (one group and a 1 x 1 matrix of 1: everyone meets "Contact rate" others
 *   a day, at random).
 * - `set_contact_prob()`: contacts within groups only, for many small groups
 *   (households, classrooms, transit trips). Each member meets each other
 *   member of its group with probability `q` per day, whatever the size of the
 *   group; `q = 1` makes every group a clique (everyone meets everyone, every
 *   day), with no ties stored. One number sets the same `q` for every group.
 *   It needs the grouping from `set_groups()`.
 *
 * Setting one between runs drops the other. A within-group layer is seen by
 * the model's queue (an infectious agent reaches only its own group), and
 * `members_of()` lists a group.
 *
 * ### Sampling
 *
 * Each step, `update()` rebuilds the pool of infectious agents of each group,
 * and the number of agents in the mixing per group, without scanning the
 * population: infectious agents are the carriers in infectious states, from
 * the model's per-state index (walked in ascending id order), and the counts
 * are the group sizes minus the few agents in excluded states.
 *
 * `sample()` then meets each infectious agent of a group `g` with a
 * probability `p` per pair, independently: it draws a binomial number of
 * contacts over the infectious agents of `g` and picks that many *distinct*
 * ones (a partial shuffle of the pool), skipping the agent itself. With
 * `p = 1` it takes them all, with no random draws. Within groups, the only
 * group is the agent's own and `p = q[g]`. With a matrix, `p = min(1,
 * rate * contact_matrix(group of agent, g) / others(g))`, where `rate` is the
 * contact-rate parameter (1 without one) and `others(g)` is the number of
 * agents of `g` in the mixing other than the agent itself: the contacts per
 * day stay as the matrix says when agents are excluded.
 *
 * ### Edits
 *
 * `set_group()`, `set_contact_matrix()` and `set_contact_prob()` called
 * during a run apply to that run only: they set the dirty flag, and the next
 * `reset()` restores the baseline. A move made during a step takes effect at
 * the end of the step, with the pools (the agents of the step sample from the
 * grouping it started with). Called between runs, they set the baseline (so a
 * calibration can reuse one model). `set_groups()` sets the baseline grouping
 * and can only be called between runs.
 *
 * ### History (traced within-group layers)
 *
 * A within-group layer with tracing on (`enable_tracing()`) keeps, for the
 * current run, the history of who was in which group and who was absent (in
 * an excluded state), day by day: `groupmates_of(agent, d1, d2, out)` lists
 * who shared a group with `agent`, both present, between days `d1` and `d2`, with
 * the days they shared. With `q = 1` that is exactly who the agent met; with
 * `q < 1`, who it could have met (contact tracing by membership: "notify the
 * household"). A move or an absence is recorded when it takes effect (the end
 * of the step, so the next day); leaving the layer (`NO_GROUP`) is a move like
 * any other. Agents that never change group or presence add nothing to the
 * history. Editing the grouping between runs drops it.
 *
 * @ingroup contact_layers
 */
class Mixing : public ContactLayer
{
public:

    /// Group of the agents that are not in the layer.
    static constexpr size_t NO_GROUP = std::numeric_limits< size_t >::max();

private:

    /// A grouping drawn at every reset (see `set_groups()`)
    struct Grouping {
        size_t n_groups;
        std::function< std::vector< size_t >(Model &) > draw;
    };

    std::optional< ParamRef > rate_ref; ///< Empty: rate 1

    // Grouping
    std::vector< size_t > groups;          ///< Group of each agent
    std::vector< size_t > groups_baseline; ///< Fixed, or this run's draw
    Grouping grouping{0u, nullptr};        ///< Empty draw: fixed groups
    size_t n_groups = 0u;
    bool explicit_groups = false; ///< Set by set_groups()
    bool layout_stale = true;     ///< Group sizes and pool offsets to rebuild
    std::vector< size_t > group_size; ///< Agents in each group

    std::vector< double > contact_matrix;          ///< Column-major
    std::vector< double > contact_matrix_baseline; ///< Set between runs
    std::vector< double > contact_prob;          ///< Within-group `q`, per group
    std::vector< double > contact_prob_baseline; ///< Set between runs (one entry: every group)

    std::vector< std::vector< size_t > > members; ///< Agents of each group

    /// Moves made during a step (agent, group), applied by `update()`
    std::vector< std::pair< size_t, size_t > > moves;

    // History of the run (traced within-group layers, see groupmates_of())

    /// From day `from` on, the agent is in `group`, present or absent.
    struct Spell {
        int from;
        bool present;
        size_t group;
    };

    /// An agent left a group on day `day` (its first day elsewhere).
    struct Departure {
        int day;
        size_t agent;
    };

    bool history = false; ///< Kept this run, and still valid
    /// Per agent, its spells in order (sized only with a history)
    std::vector< std::vector< Spell > > spells_of;
    /// Per group, the departures in order (sized only with a history)
    std::vector< std::vector< Departure > > departures;
    std::vector< bool > absent;       ///< Per agent, in an excluded state
    std::vector< size_t > absent_ids; ///< The agents flagged in `absent`
    /// The step's movers and their group before it (scratch of `update()`)
    std::vector< std::pair< size_t, size_t > > moved_from;

    /// Records the agent's group and presence from tomorrow on.
    void record(size_t agent_id, size_t old_group, int today);
    void clear_history();

    /// Calls `f(group, first, last)` for each span of days in `[a, b]` that
    /// the agent spent in a group, present.
    template<typename F>
    void spells(size_t agent_id, int a, int b, F f) const;

    /// The day the agent last left group `g` (it is no longer there).
    int last_departure(size_t agent_id, size_t g) const;

    std::vector< size_t > pool;      ///< Infectious agents, partitioned by group
    std::vector< size_t > offsets;   ///< Where each group starts in `pool`
    std::vector< size_t > counts;    ///< Infectious agents per group
    std::vector< size_t > n_available; ///< Agents in the mixing, per group

    std::vector< uint64_t > marks;   ///< Bitset used to order the pool
    std::vector< size_t > ordered;   ///< Infectious ids (ascending), scratch

    /**
     * @brief How many agents of a group an agent meets in a day: the draw of
     * `Model::rbinom(n, p)` (same distribution, same random numbers), with
     * its parameters set once a day instead of at every draw.
     */
    struct DailyDraw {
        int n = 0;      ///< Infectious agents of the group
        double p = 0.0; ///< Probability of meeting each
        bool poisson = false;
        std::poisson_distribution<>::param_type pois;
        std::binomial_distribution<>::param_type binom;
        void set(int n_, double p_);
        int operator()(Model & model) const;
    };

    /// With a contact matrix: entry `own * n_groups + g` is the draw of an
    /// agent of group `own` (in the mixing) from group `g`, set by `update()`.
    std::vector< DailyDraw > daily;
    void daily_build(const Model & model);

    void rebuild_layout();
    void rebuild_members();
    void move(size_t agent_id, size_t group, bool notify_queue);
    void check_agent(size_t agent_id) const;
    void check_group(size_t group) const;

    /// Meets `n_met` distinct infectious agents of group `g`.
    void draw(size_t g, size_t n_met, size_t agent, Model & model,
        std::vector< size_t > & out);

protected:

    // Created with layer::mixing()
    explicit Mixing(std::string name) : ContactLayer(std::move(name)) {}

    friend std::shared_ptr< Mixing > mixing(std::string);

    // Reads the groups unchecked (as the ties of a network layer)
    friend class epiworld::Queue;

public:

    /**
     * @brief Sets the grouping of the layer (between runs only).
     * @param a Who is in which group: each agent's label is its group, in
     * `[0, n_groups_)`, or none (out of the layer). With `assign::labels()`
     * the grouping is fixed (and `set_group()` between runs edits it);
     * otherwise it is drawn again at every reset, e.g.,
     * `assign::counts({{0, 300}, {1, 700}})`. See `assign.hpp`.
     * @param n_groups_ Number of groups (the contact matrix is
     * `n_groups x n_groups`).
     */
    void set_groups(Assign a, size_t n_groups_);

    /// Group of an agent (`NO_GROUP` if it is not in the layer).
    size_t get_group(size_t agent_id) const
    {
        check_agent(agent_id);
        return groups[agent_id];
    }

    /// Whether the agent takes part in the layer (valid after `reset()`).
    bool in_layer(size_t agent_id) const { return groups[agent_id] != NO_GROUP; }

    /**
     * @brief Moves an agent to another group (`NO_GROUP` takes it out of the
     * layer).
     * @details Like every change made during a step, it is in force for the
     * contacts of the next step: the move waits for the end of the step, when
     * the pools are rebuilt (until then, `get_group()` gives the old group).
     */
    void set_group(size_t agent_id, size_t group);

    size_t get_n_groups() const { return n_groups; }

    /**
     * @brief Sets the contact matrix.
     * @param cmat Column-major `n_groups x n_groups` matrix: entry `(i, j)`
     * is the expected number of contacts per day that an agent in group `i` has
     * with agents in group `j`.
     */
    void set_contact_matrix(std::vector< double > cmat);

    const std::vector< double > & get_contact_matrix() const
    {
        return contact_matrix;
    }

    /**
     * @brief The contacts are the model parameter `param` times the contact
     * matrix (read at the end of every step, after the global events, so a
     * change to the parameter applies from the next step).
     * @details For a layer with a contact matrix only; `reset()` throws for a
     * layer with contact probabilities.
     */
    void set_contact_rate(std::string param) { rate_ref.emplace(std::move(param)); }

    bool has_contact_rate() const { return rate_ref.has_value(); }

    /// The contact rate: the value of the `set_contact_rate()` parameter (1
    /// without one).
    double get_contact_rate(const Model & model) const
    {
        return rate_ref ? (*rate_ref)(model) : 1.0;
    }

    /**
     * @brief Sets the probability that two members of a group meet on a given
     * day (contacts within groups only).
     * @param q One probability per group, or a single one for every group;
     * `1` makes the groups cliques (everyone meets everyone, every day).
     */
    void set_contact_prob(std::vector< double > q);
    void set_contact_prob(double q)
    {
        set_contact_prob(std::vector< double >{q});
    }

    /// The within-group contact probabilities (one per group after `reset()`;
    /// empty with a contact matrix).
    const std::vector< double > & get_contact_prob() const
    {
        return contact_prob;
    }

    /// Whether the layer mixes within groups only (`set_contact_prob()`).
    bool within_groups() const { return !contact_prob_baseline.empty(); }

    /// The agents of group `g`, in no particular order (valid after `reset()`).
    const std::vector< size_t > & members_of(size_t g) const
    {
        check_group(g);
        return members[g];
    }

    /// A groupmate of an agent over a window of days (see `groupmates_of()`).
    struct Groupmate {
        size_t agent;
        int last_day; ///< The last day they shared a group, both present
        int n_days;   ///< The days they shared a group, both present
    };

    /**
     * @brief Who shared a group with `agent`, both present, between days `d1`
     * and `d2` (traced within-group layers; see the class description).
     * @details One entry per groupmate, however many times either moved or
     * was absent in between. Days before day 1 (when contacts start) and after
     * today (not known yet) are left out. The history is that of the current (or last) run, and covers it
     * whole: tracing turned on during a run starts it at the next one.
     * @throws std::logic_error Without a history (see the class description).
     * @param out Replaced by the groupmates, in ascending id order (a vector
     * reused across calls allocates nothing once it has grown).
     */
    void groupmates_of(
        size_t agent_id,
        int d1,
        int d2,
        std::vector< Groupmate > & out
    ) const;

    layer::Mixing * as_mixing() override { return this; }

    /// Within groups only: a carrier reaches only its own group.
    bool supports_queue() const override { return within_groups(); }

    /**
     * @brief Restores the baseline and sizes all buffers for the model (the
     * first `update()`, on day 0, builds the pools).
     */
    void reset(Model & model) override;

    /**
     * @brief Rebuilds the infectious pools and the available counts from the
     * current state of the model (once per step).
     */
    void update(Model & model) override;

    std::shared_ptr< ContactLayer > clone() const override
    {
        return std::make_shared< Mixing >(*this);
    }

    /**
     * @brief Samples the infectious contacts of `agent` (none if it is not in
     * the layer), appending them to `out`.
     * @details `exclude` is ignored: the pools hold the agents in the model's
     * infectious states only. See `ContactLayer::sample()`.
     */
    size_t sample(
        size_t agent,
        Model & model,
        const std::vector< bool > * exclude,
        std::vector< size_t > & out
    ) override;

    /// Number of infectious agents in a group.
    size_t get_n_infectious(size_t group) const { return counts[group]; }

};

inline std::shared_ptr< Mixing > mixing(std::string name)
{
    return std::shared_ptr< Mixing >(new Mixing(std::move(name)));
}

inline void Mixing::set_groups(Assign a, size_t n_groups_)
{

    if (!this->edits_baseline())
        throw std::logic_error(
            "Mixing layer '" + this->get_name() + "': set_groups() sets the "
            "baseline and cannot be called during a run; use set_group()."
        );

    if (!a.is_set())
        throw std::invalid_argument(
            "Mixing layer '" + this->get_name() + "': empty grouping."
        );

    // A label is a group, or none
    const std::string lname = this->get_name();
    auto to_groups = [lname, n_groups_](const std::vector< int > & labels) {
        std::vector< size_t > g(labels.size(), NO_GROUP);
        for (size_t i = 0u; i < labels.size(); ++i)
        {
            if (labels[i] == -1)
                continue;
            if ((labels[i] < 0) || (static_cast< size_t >(labels[i]) >= n_groups_))
                throw std::range_error(
                    "Mixing layer '" + lname + "': the label " +
                    std::to_string(labels[i]) + " is not a group (" +
                    std::to_string(n_groups_) + " groups; a grouping gives "
                    "each agent a group, e.g., assign::counts({{0, n}, ...}))."
                );
            g[i] = static_cast< size_t >(labels[i]);
        }
        return g;
    };

    n_groups        = n_groups_;
    explicit_groups = true;
    layout_stale    = true;

    if (const auto * fixed = a.get_fixed())
    {
        groups_baseline = to_groups(*fixed);
        groups          = groups_baseline;
        grouping        = {0u, nullptr};
        rebuild_members();
    }
    else
    {
        // Drawn at every reset; everyone can be in a group
        grouping = {
            n_groups_,
            [a, to_groups](Model & model) -> std::vector< size_t > {
                const std::vector< char > everyone(model.size(), 1);
                return to_groups(a.draw(model, everyone));
            }
        };
        groups_baseline.clear();
        groups.clear();
    }

    clear_history();

}

inline void Mixing::check_agent(size_t agent_id) const
{
    if (agent_id >= groups.size())
        throw std::range_error(
            "Mixing layer '" + this->get_name() + "': agent " +
            std::to_string(agent_id) + " is out of range (the layer has " +
            std::to_string(groups.size()) + " agents; was it reset?)."
        );
}

inline void Mixing::check_group(size_t group) const
{
    if (group >= n_groups)
        throw std::range_error(
            "Mixing layer '" + this->get_name() + "': group " +
            std::to_string(group) + " is out of range (" +
            std::to_string(n_groups) + " groups)."
        );
}

inline void Mixing::set_group(size_t agent_id, size_t group)
{

    check_agent(agent_id);
    if (group != NO_GROUP)
        check_group(group);

    // During a run, the move waits for the end of the step (see update())
    if (!this->edits_baseline())
    {
        moves.emplace_back(agent_id, group);
        return;
    }

    if (grouping.draw)
        throw std::logic_error(
            "Mixing layer '" + this->get_name() + "': its grouping is drawn "
            "at every reset, so a move between runs would be lost; move "
            "agents during a run, or set fixed groups with set_groups()."
        );

    groups_baseline[agent_id] = group;
    move(agent_id, group, false);
    clear_history();

}

inline void Mixing::move(
    size_t agent_id,
    size_t group,
    bool notify_queue
)
{

    const size_t old = groups[agent_id];
    if (old == group)
        return;

    // During a run, the queue follows the move (it sees within-group layers)
    if (notify_queue && (this->model != nullptr) && within_groups())
        this->model->group_member_moved(
            agent_id,
            old   == NO_GROUP ? nullptr : &members[old],
            group == NO_GROUP ? nullptr : &members[group]
        );

    if (old != NO_GROUP)
    {
        auto & m = members[old];
        auto it = std::find(m.begin(), m.end(), agent_id);
        *it = m.back();
        m.pop_back();
    }

    if (group != NO_GROUP)
        members[group].push_back(agent_id);

    groups[agent_id] = group;
    layout_stale = true;

}

inline void Mixing::set_contact_matrix(std::vector< double > cmat)
{

    for (double c : cmat)
        if (c < 0.0)
            throw std::range_error(
                "Mixing layer '" + this->get_name() + "': the contact matrix "
                "must be non-negative (" + std::to_string(c) + " < 0)."
            );

    // Between runs, the size is checked by reset() (the grouping may not be
    // set yet); during a run, the grouping and the form are fixed
    if (this->edits_baseline())
    {
        contact_matrix_baseline = cmat;
        contact_prob_baseline.clear();
    }
    else if (within_groups())
        throw std::logic_error(
            "Mixing layer '" + this->get_name() + "': the layer mixes within "
            "groups (set_contact_prob()); a contact matrix can only replace "
            "the contact probabilities between runs."
        );
    else if (cmat.size() != n_groups * n_groups)
        throw std::length_error(
            "Mixing layer '" + this->get_name() + "': the contact matrix has " +
            std::to_string(cmat.size()) + " entries, but the layer has " +
            std::to_string(n_groups) + " groups."
        );

    contact_matrix = std::move(cmat);

}

inline void Mixing::set_contact_prob(std::vector< double > q)
{

    if (q.empty())
        throw std::length_error(
            "Mixing layer '" + this->get_name() + "': set_contact_prob() needs "
            "at least one probability."
        );

    for (double x : q)
        if (!((x >= 0.0) && (x <= 1.0)))
            throw std::range_error(
                "Mixing layer '" + this->get_name() + "': contact probabilities "
                "must be in [0, 1] (" + std::to_string(x) + ")."
            );

    // Between runs, the size is checked by reset(); during a run, the
    // grouping and the form are fixed
    if (this->edits_baseline())
    {
        contact_prob_baseline = q;
        contact_matrix_baseline.clear();
    }
    else
    {

        if (!within_groups())
            throw std::logic_error(
                "Mixing layer '" + this->get_name() + "': the layer has a "
                "contact matrix; contact probabilities can only replace it "
                "between runs."
            );

        // A copy: assign() may free the element before reading it
        if (q.size() == 1u)
            q.assign(n_groups, double(q[0u]));
        else if (q.size() != n_groups)
            throw std::length_error(
                "Mixing layer '" + this->get_name() + "': " +
                std::to_string(q.size()) + " contact probabilities, but the "
                "layer has " + std::to_string(n_groups) + " groups."
            );

    }

    contact_prob = std::move(q);

}

inline void Mixing::rebuild_members()
{

    members.assign(n_groups, {});
    for (size_t i = 0u; i < groups.size(); ++i)
        if (groups[i] != NO_GROUP)
            members[groups[i]].push_back(i);

}

inline void Mixing::rebuild_layout()
{

    group_size.assign(n_groups, 0u);
    for (size_t g : groups)
        if (g != NO_GROUP)
            ++group_size[g];

    counts.assign(n_groups, 0u);
    offsets.assign(n_groups, 0u);
    n_available.assign(n_groups, 0u);

    // Groups start where the previous one ends (at most its size)
    for (size_t i = 1u; i < n_groups; ++i)
        offsets[i] = offsets[i - 1u] + group_size[i - 1u];

    layout_stale = false;

}

inline void Mixing::reset(Model & model)
{

    const size_t n = model.size();

    if (!explicit_groups)
        throw std::logic_error(
            "Mixing layer '" + this->get_name() + "': it has no groups; set "
            "them with set_groups()."
        );

    // This run's draw, or the fixed groups (edits of the last run dropped)
    if (grouping.draw)
    {
        groups_baseline = grouping.draw(model);
        for (size_t g : groups_baseline)
            if ((g != NO_GROUP) && (g >= n_groups))
                throw std::range_error(
                    "Mixing layer '" + this->get_name() + "': the grouping "
                    "drew group " + std::to_string(g) + ", out of range (" +
                    std::to_string(n_groups) + " groups)."
                );
    }

    groups = groups_baseline;
    if (groups.size() != n)
        throw std::length_error(
            "Mixing layer '" + this->get_name() + "': the grouping has " +
            std::to_string(groups.size()) + " agents, but the model has " +
            std::to_string(n) + "."
        );

    // Edits of the last run are dropped
    if (within_groups())
    {

        contact_prob = contact_prob_baseline;
        // A copy: assign() may free the element before reading it
        if (contact_prob.size() == 1u)
            contact_prob.assign(n_groups, double(contact_prob[0u]));
        else if (contact_prob.size() != n_groups)
            throw std::length_error(
                "Mixing layer '" + this->get_name() + "': " +
                std::to_string(contact_prob.size()) + " contact "
                "probabilities, but the layer has " +
                std::to_string(n_groups) + " groups."
            );

        contact_matrix.clear();

        if (has_contact_rate())
            throw std::logic_error(
                "Mixing layer '" + this->get_name() + "': a contact rate "
                "scales a contact matrix; this layer has contact "
                "probabilities (within groups)."
            );

    }
    else
    {

        contact_matrix = contact_matrix_baseline;
        if (contact_matrix.size() != n_groups * n_groups)
            throw std::length_error(
                "Mixing layer '" + this->get_name() + "': the contact matrix "
                "has " + std::to_string(contact_matrix.size()) + " entries, but "
                "the layer has " + std::to_string(n_groups) + " groups."
            );

        contact_prob.clear();

    }

    rebuild_members();
    moves.clear();

    clear_history();
    history = within_groups() && this->is_tracing();
    if (history)
    {
        spells_of.resize(n);
        departures.resize(n_groups);
        absent.assign(n, false);
    }

    pool.assign(n, 0u);
    ordered.clear();
    ordered.reserve(n);
    marks.assign((n + 63u) / 64u, 0u);

    rebuild_layout();
    this->mark_clean();

}

inline void Mixing::update(Model & model)
{

    // Moves made during the step; the history keeps each agent's net change
    // (several moves of one agent in a step are one, or none if it is back)
    moved_from.clear();
    for (const auto & [agent_id, group] : moves)
    {
        if (history)
            moved_from.emplace_back(agent_id, groups[agent_id]);

        move(agent_id, group, true);
    }
    moves.clear();

    // The first entry of each agent has its group before the step
    std::stable_sort(
        moved_from.begin(), moved_from.end(),
        [](const auto & x, const auto & y) { return x.first < y.first; }
    );
    for (size_t i = 0u; i < moved_from.size(); ++i)
    {
        const auto & [agent_id, old] = moved_from[i];
        if ((i > 0u) && (moved_from[i - 1u].first == agent_id))
            continue;

        if (groups[agent_id] != old)
            record(agent_id, old, model.today());
    }

    if (layout_stale)
        rebuild_layout();

    const size_t ns = static_cast< size_t >(model.nstates);
    const auto & not_infectious = model.state_not_infectious;

    std::fill(counts.begin(), counts.end(), 0u);

    // Marking the carriers in infectious states; walking the bitset gives
    // ascending ids. States without carriers (e.g., the susceptible) are
    // skipped without a scan.
    for (size_t s = 0u; s < ns; ++s)
    {
        if ((model.state_carriers[s] == 0u) ||
            (!not_infectious.empty() && not_infectious[s]))
            continue;

        for (size_t id : model.state_index_members(s))
            if (model.agent_carrier[id])
                marks[id >> 6] |= (uint64_t(1) << (id & 63u));
    }

    ordered.clear();
    for (size_t w = 0u; w < marks.size(); ++w)
    {
        uint64_t word = marks[w];
        marks[w] = 0u;
        while (word != 0u)
        {
            ordered.push_back((w << 6) + epi_ctz64(word));
            word &= (word - 1u);
        }
    }

    // Placing them in their group (agents out of the layer are not in the mixing)
    for (size_t id : ordered)
    {
        const size_t g = groups[id];
        if (g == NO_GROUP)
            continue;

        pool[offsets[g] + counts[g]++] = id;
    }

    // In the mixing = group size - agents in excluded states
    for (size_t g = 0u; g < n_groups; ++g)
        n_available[g] = group_size[g];

    // Back from an excluded state
    if (history)
    {
        size_t k = 0u;
        for (size_t id : absent_ids)
        {
            if (model.state_excluded[model.agent_state[id]])
                absent_ids[k++] = id;
            else
            {
                absent[id] = false;
                record(id, NO_GROUP, model.today());
            }
        }
        absent_ids.resize(k);
    }

    for (size_t s = 0u; s < ns; ++s)
    {
        if (!model.state_excluded[s])
            continue;

        for (size_t id : model.state_index_members(s))
        {
            if (history && !absent[id])
            {
                absent[id] = true;
                absent_ids.push_back(id);
                record(id, NO_GROUP, model.today());
            }

            const size_t g = groups[id];
            if (g == NO_GROUP)
                continue;

            if (n_available[g] > 0u)
                --n_available[g];
        }
    }

    if (!within_groups())
        daily_build(model);

}

inline void Mixing::record(size_t agent_id, size_t old_group, int today)
{

    const size_t g = groups[agent_id];
    spells_of[agent_id].push_back({today + 1, !absent[agent_id], g});

    // Absences do not change the members
    if ((old_group != NO_GROUP) && (old_group != g))
        departures[old_group].push_back({today + 1, agent_id});

}

inline void Mixing::clear_history()
{
    history = false;
    // Freed, not just emptied: an untraced layer holds nothing
    std::vector< std::vector< Spell > >().swap(spells_of);
    std::vector< std::vector< Departure > >().swap(departures);
    std::vector< bool >().swap(absent);
    absent_ids.clear();
}

template<typename F>
inline void Mixing::spells(size_t agent_id, int a, int b, F f) const
{

    // Walking back from the last spell, each one holding until the next
    int end = b + 1;
    const auto & sp = spells_of[agent_id];
    for (auto s = sp.rbegin(); (s != sp.rend()) && (end > a); ++s)
    {
        const int first = std::max(s->from, a);
        if (s->present && (s->group != NO_GROUP) && (first < end))
            f(s->group, first, end - 1);

        end = std::min(end, s->from);
    }

    // Before its first change, the agent is where the run started
    if ((end > a) && (groups_baseline[agent_id] != NO_GROUP))
        f(groups_baseline[agent_id], a, end - 1);

}

inline int Mixing::last_departure(size_t agent_id, size_t g) const
{

    // The spell after its last one in g (none: it left its starting group)
    const auto & sp = spells_of[agent_id];
    for (size_t k = sp.size() - 1u; k > 0u; --k)
        if (sp[k - 1u].group == g)
            return sp[k].from;

    return sp[0u].from;

}

inline void Mixing::groupmates_of(
    size_t agent_id,
    int d1,
    int d2,
    std::vector< Groupmate > & out
) const
{

    check_agent(agent_id);
    if (!history)
        throw std::logic_error(
            "Mixing layer '" + this->get_name() + "': " + (
                (this->is_tracing() && within_groups()) ?
                "no history. It covers whole runs: tracing was turned on, "
                "or the grouping edited, after the last run started; the "
                "next run keeps one." :
                "groupmates_of() needs a traced layer (enable_tracing() "
                "before the run) with contact probabilities (within groups)."
            )
        );

    // Contacts start on day 1 (day 0 is the initial state)
    d1 = std::max(d1, 1);
    d2 = std::min(d2, this->model->today());

    // One entry per shared span, merged per groupmate below
    out.clear();
    spells(agent_id, d1, d2, [&](size_t g, int a, int b) {

        auto shared = [&](size_t other) {
            spells(other, a, b, [&](size_t h, int first, int last) {
                if (h == g)
                    out.push_back({other, last, last - first + 1});
            });
        };

        // The members now
        for (size_t other : members[g])
            if (other != agent_id)
                shared(other);

        // Those who left after day a, each once: at its last departure
        const auto & left = departures[g];
        for (auto d = left.rbegin(); (d != left.rend()) && (d->day > a); ++d)
            if (
                (d->agent != agent_id) && (groups[d->agent] != g) &&
                (last_departure(d->agent, g) == d->day)
            )
                shared(d->agent);

    });

    std::sort(out.begin(), out.end(), [](const auto & x, const auto & y) {
        return x.agent < y.agent;
    });

    size_t k = 0u;
    for (size_t i = 0u; i < out.size(); ++i)
    {
        if ((k > 0u) && (out[k - 1u].agent == out[i].agent))
        {
            out[k - 1u].last_day = std::max(out[k - 1u].last_day, out[i].last_day);
            out[k - 1u].n_days  += out[i].n_days;
        }
        else
            out[k++] = out[i];
    }
    out.resize(k);

}

inline void Mixing::DailyDraw::set(int n_, double p_)
{

    n = n_;
    p = p_;
    poisson = false;

    if ((n == 0) || (p <= 0.0) || (p >= 1.0))
        return;

    // The same choice as Model::rbinom(n, p)
    #ifdef EPI_FAST_BINOM
    if ((p <= 0.01) && (static_cast< double >(n) * p * p <= 0.1))
    {
        poisson = true;
        pois = std::poisson_distribution<>::param_type(static_cast< double >(n) * p);
        return;
    }
    #endif

    binom = std::binomial_distribution<>::param_type(n, p);

}

inline int Mixing::DailyDraw::operator()(Model & model) const
{

    if ((n == 0) || (p <= 0.0))
        return 0;

    // Everyone, with no random draws
    if (p >= 1.0)
        return n;

    if (poisson)
        return std::min(model.rpoissd(*model.engine, pois), n);

    return model.rbinomd(*model.engine, binom);

}

inline void Mixing::daily_build(const Model & model)
{

    // The contact rate scales the whole matrix
    const double scale = get_contact_rate(model);

    daily.resize(n_groups * n_groups);
    for (size_t own = 0u; own < n_groups; ++own)
    {

        // Column of the group: entry (own, g) is rate[g * n_groups]
        const double * rate = contact_matrix.data() + own;
        for (size_t g = 0u; g < n_groups; ++g)
        {

            // In its own group, an agent's contacts are the others
            const size_t others = n_available[g] - ((g == own) ? 1u : 0u);
            const double p = (others == 0u || n_available[g] == 0u) ? 0.0 :
                std::min(
                    1.0,
                    scale * rate[g * n_groups] / static_cast< double >(others)
                );

            daily[own * n_groups + g].set(static_cast< int >(counts[g]), p);

        }

    }

}

inline void Mixing::draw(
    size_t g,
    size_t n_met,
    size_t agent,
    Model & model,
    std::vector< size_t > & out
)
{

    const size_t n_pool = counts[g];

    // A binomial number of distinct agents is the same as meeting each one
    // independently with probability p; with p = 1 (a clique) it is everyone,
    // with no draws. A partial shuffle of the pool puts the ones met first.
    size_t * first = pool.data() + offsets[g];
    for (size_t s = 0u; s < n_met; ++s)
    {

        if (n_met < n_pool)
        {
            size_t which = s + static_cast< size_t >(
                model.runif() * static_cast< double >(n_pool - s)
            );

            // Correcting overflow error
            if (which >= n_pool)
                which = n_pool - 1u;

            std::swap(first[s], first[which]);
        }

        const size_t id = first[s];

        #ifdef EPI_DEBUG
        if (model.get_agent(id).get_virus() == nullptr)
            throw std::logic_error(
                "The agent is not infected, but it should be."
            );
        #endif

        // Can't meet itself
        if (id != agent)
            out.push_back(id);

    }

}

inline size_t Mixing::sample(
    size_t agent,
    Model & model,
    const std::vector< bool > *,
    std::vector< size_t > & out
)
{

    if (!in_layer(agent))
        return 0u;

    const size_t first = out.size();

    const size_t own = groups[agent];

    if (within_groups())
    {
        // Only the agent's own group
        const double q = contact_prob[own];
        const size_t n_pool = counts[own];
        if ((n_pool > 0u) && (q > 0.0))
            draw(
                own,
                (q < 1.0) ?
                    static_cast< size_t >(model.rbinom(static_cast< int >(n_pool), q)) :
                    n_pool,
                agent, model, out
            );
    }
    else if (!model.state_excluded[model.agent_state[agent]])
    {

        // The day's draws of the agent's group (see update())
        const DailyDraw * day = daily.data() + own * n_groups;
        for (size_t g = 0u; g < n_groups; ++g)
        {
            const int n_met = day[g](model);
            if (n_met > 0)
                draw(g, static_cast< size_t >(n_met), agent, model, out);
        }

    }
    else
    {

        // An excluded agent is not one of the others of its group (it rarely
        // samples: its state usually has no sampler)
        const double scale = get_contact_rate(model);
        const double * rate = contact_matrix.data() + own;
        for (size_t g = 0u; g < n_groups; ++g)
        {
            const size_t n_pool = counts[g];
            const double p = (n_available[g] == 0u) ? 0.0 : std::min(
                1.0,
                scale * rate[g * n_groups] / static_cast< double >(n_available[g])
            );
            if ((n_pool == 0u) || (p <= 0.0))
                continue;

            draw(
                g,
                (p < 1.0) ?
                    static_cast< size_t >(model.rbinom(static_cast< int >(n_pool), p)) :
                    n_pool,
                agent, model, out
            );
        }

    }

    // Reporting the interactions, only if somebody listens
    const size_t n_sampled = out.size() - first;
    this->trace(out.data() + first, n_sampled, agent);
    if (model.has_post_sampling())
        model.register_sampled_contacts(
            out.data() + first, n_sampled, static_cast< int >(agent)
        );

    return n_sampled;

}

}

#endif
/*//////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

 End of -./include/epiworld/layer-mixing.hpp-

////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////*/


/*//////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

 Start of -./include/epiworld/layer-network.hpp-

////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////*/


#ifndef EPIWORLD_LAYER_NETWORK_HPP
#define EPIWORLD_LAYER_NETWORK_HPP

namespace layer {

/**
 * @brief Contact layer with explicit ties (a network).
 *
 * @details The contacts of an agent in this layer are its ties. A network
 * layer is created with `layer::network()`, either from a graph, which is then
 * *fixed*, or from a *generator*, a function that receives the model and
 * returns a graph, which the layer calls at every reset: each run (and each
 * replicate of `run_multiple()`) then gets its own draw, from its own seed.
 * The `graph::` functions return generators; any function
 * `AdjList(Model &)` is one.
 *
 * ### Ties and their order
 *
 * Each agent's ties are kept in the order they were added (a graph is loaded
 * with each agent's ties in ascending id order, without repeats), and that
 * order is load-bearing: the single draw that picks the infector walks the
 * contacts in this order, so whichever agent sits at a given position is the
 * one recorded. Adding or removing a tie never permutes the others. Agents with
 * more than `EPI_NEIGHBOR_INDEX_THRESHOLD` ties also get a position index, so
 * that looking a tie up is not a scan.
 *
 * ### Edits
 *
 * `add_edge()`, `rm_edge()` and the rewiring function are the only way to
 * change the ties, and they keep the model's queue and agents-by-state index in
 * step, so they are safe at any point of a run. An edit made during a run
 * applies to that run only: the next reset restores a fixed graph, and redraws
 * a generated one. An edit made between runs changes a fixed graph for the runs
 * that follow; a generated graph is redrawn anyway.
 *
 * ### Rewiring
 *
 * `set_rewiring()` gives the layer a function that is called at the end of
 * every step (not on day 0), after the global events, with the layer and the
 * model; `graph::rewire_degseq()` returns the degree-preserving one.
 *
 * @ingroup contact_layers
 */
class Network : public ContactLayer
{
    friend class epiworld::Model;
    friend class epiworld::Queue;

public:

    /// Returns the graph of a run (called at every reset).
    using Generator = std::function< AdjList(Model &) >;

    /// Changes the ties at the end of each step.
    using Rewiring = std::function< void(Network &, Model &) >;

private:

    Generator generator; ///< Empty for a fixed graph
    Rewiring rewiring;
    bool directed = false;

    std::vector< std::vector< size_t > > ties; ///< Neighbors of each agent

    /// Position of each neighbor, only for agents with many ties
    std::unordered_map< size_t, std::unordered_map< size_t, size_t > > index;

    // The fixed graph the next reset restores, one row per agent; empty until
    // the first edit
    std::vector< size_t > baseline_start;
    std::vector< size_t > baseline_ids;
    bool baseline_stale = false; ///< Edited between runs

    void set_graph(const AdjList & graph);
    void capture_baseline();
    void restore_baseline();
    void build_index(size_t i);
    size_t find(size_t i, size_t j) const; ///< Position of `j` in `ties[i]`
    bool append(size_t i, size_t j, bool check);
    void erase_at(size_t i, size_t pos);
    void check_endpoints(size_t i, size_t j) const;
    void edit();

protected:

    // Created with layer::network()
    Network(std::string name, const AdjList & graph);
    Network(std::string name, Generator generator);

    friend std::shared_ptr< Network > network(std::string, const AdjList &);

    friend std::shared_ptr< Network > network(
        std::string, Network::Generator
    );

public:

    size_t size() const { return ties.size(); } ///< Number of agents
    bool is_directed() const { return directed; }

    /// Number of ties of agent `i` (in a directed network, the ties it keeps).
    size_t get_degree(size_t i) const { return ties[i].size(); }

    /**
     * @brief The neighbors of agent `i`, without allocating.
     * @details In order (see the class description). The view is invalidated by
     * any change to the agent's ties. The layer must belong to a model.
     */
    NeighborsView neighbors_view(size_t i);

    /**
     * @name Edit the ties
     *
     * @details Undirected ties only, changed at both ends: `add_edge()` and
     * `rm_edge()` throw in a directed network.
     *
     * @return Whether a tie was added (removed).
     * @throws std::range_error if an id is out of range.
     * @throws std::logic_error if `i == j` or the network is directed.
     */
    ///@{
    bool add_edge(size_t i, size_t j);
    bool rm_edge(size_t i, size_t j);
    ///@}

    /// Whether `i` and `j` are tied (in a directed network, whether `i -> j`).
    bool has_edge(size_t i, size_t j) const;

    /**
     * @brief Swaps two ties: `j`, `i`'s neighbor at position `pos_i`, and `l`,
     * `k`'s neighbor at position `pos_k`, trade places.
     *
     * @details (i-j), (k-l) become (i-l), (k-j); in an undirected network the
     * other ends follow. No degree changes. Used by rewiring functions.
     * @throws std::range_error if an id or a position is out of range.
     * @throws std::logic_error if the network is undirected and the ties share
     * an endpoint (`i`, `j`, `k` and `l` must be four different agents).
     */
    void swap_ties(size_t i, size_t pos_i, size_t k, size_t pos_k);

    /// Sets the function that rewires the ties at the end of each step.
    void set_rewiring(Rewiring fun) { rewiring = std::move(fun); }
    bool has_rewiring() const { return static_cast< bool >(rewiring); }

    /**
     * @brief The ties as an edge list.
     * @details Each undirected tie once (`source <= target`); each directed
     * tie as kept by its source. The vectors are appended to.
     */
    void get_edgelist(
        std::vector< int > & source,
        std::vector< int > & target
    ) const;

    /// Writes the edge list (see `get_edgelist()`) to a file.
    void write_edgelist(const std::string & fn) const;

    /**
     * @brief The neighbors of `agent` that carry a virus, written to `out`
     * (room for `get_degree(agent)` entries).
     * @details Neighbors in a state flagged in `exclude` (if not null) are
     * skipped. With `Record`, each contact is reported to the model's
     * post-sampling callback.
     * @return The number of contacts written.
     */
    template< bool Record >
    size_t pull(
        size_t agent,
        Model & model,
        const std::vector< bool > * exclude,
        size_t * out
    ) const;

    /// The network's part of a pull (see `ContactLayer::sample()`).
    size_t sample(
        size_t agent,
        Model & model,
        const std::vector< bool > * exclude,
        std::vector< size_t > & out
    ) override
    {

        const size_t first = out.size();
        out.resize(first + ties[agent].size());

        // The recording path is chosen once per call, not per contact
        const size_t n = model.post_sampling_on ?
            pull<true>(agent, model, exclude, out.data() + first) :
            pull<false>(agent, model, exclude, out.data() + first);

        out.resize(first + n);
        this->trace(out.data() + first, n, agent);
        return n;

    }

    void reset(Model & model) override;
    void update(Model & model) override;

    layer::Network * as_network() override { return this; }

    /// An undirected network: a carrier's ties are its contacts' ties too.
    bool supports_queue() const override { return !directed; }

    std::shared_ptr< ContactLayer > clone() const override
    {
        return std::make_shared< Network >(*this);
    }

};

/**
 * @brief A fixed network layer (see `Network`).
 * @param name Name of the layer (unique in a model).
 * @param graph The ties; its number of vertices must match the model's
 * population. Directed if the graph is.
 * @ingroup contact_layers
 */
inline std::shared_ptr< Network > network(
    std::string name,
    const AdjList & graph
)
{
    return std::shared_ptr< Network >(
        new Network(std::move(name), graph)
    );
}

/**
 * @brief A network layer drawn at every reset (see `Network`).
 * @param name Name of the layer (unique in a model).
 * @param generator Called with the model at every reset; the graph it returns
 * must have one vertex per agent.
 * @ingroup contact_layers
 */
inline std::shared_ptr< Network > network(
    std::string name,
    Network::Generator generator
)
{
    return std::shared_ptr< Network >(
        new Network(std::move(name), std::move(generator))
    );
}

inline Network::Network(std::string name, const AdjList & graph) :
    ContactLayer(std::move(name))
{
    set_graph(graph);
}

inline Network::Network(std::string name, Generator generator_) :
    ContactLayer(std::move(name)),
    generator(std::move(generator_))
{
    if (!generator)
        throw std::invalid_argument(
            "Network layer '" + this->get_name() + "': empty generator."
        );
}

inline void Network::set_graph(const AdjList & graph)
{

    const size_t n = graph.vcount();

    // AdjList::is_directed() throws on a graph with no vertices
    directed = (n > 0u) && graph.is_directed();

    // Each row of the graph (sorted, without repeats) becomes its agent's
    // ties. Resizing rather than reassigning keeps the rows' memory from one
    // draw of a generator to the next.
    const auto & start = graph.get_start();
    const auto & ids = graph.get_ids();

    ties.resize(n);
    index.clear();
    for (size_t i = 0u; i < n; ++i)
    {

        ties[i].assign(
            ids.begin() + static_cast< std::ptrdiff_t >(start[i]),
            ids.begin() + static_cast< std::ptrdiff_t >(start[i + 1u])
        );

        if (ties[i].size() > EPI_NEIGHBOR_INDEX_THRESHOLD)
            build_index(i);

    }

}

inline void Network::capture_baseline()
{

    const size_t n = ties.size();
    baseline_start.assign(n + 1u, 0u);
    for (size_t i = 0u; i < n; ++i)
        baseline_start[i + 1u] = baseline_start[i] + ties[i].size();

    baseline_ids.resize(baseline_start[n]);
    for (size_t i = 0u; i < n; ++i)
        std::copy(
            ties[i].begin(), ties[i].end(),
            baseline_ids.begin() + static_cast< std::ptrdiff_t >(baseline_start[i])
        );

}

inline void Network::restore_baseline()
{

    const size_t n = baseline_start.size() - 1u;
    ties.resize(n);
    index.clear();
    for (size_t i = 0u; i < n; ++i)
    {

        ties[i].assign(
            baseline_ids.begin() + static_cast< std::ptrdiff_t >(baseline_start[i]),
            baseline_ids.begin() + static_cast< std::ptrdiff_t >(baseline_start[i + 1u])
        );

        if (ties[i].size() > EPI_NEIGHBOR_INDEX_THRESHOLD)
            build_index(i);

    }

}

inline void Network::build_index(size_t i)
{

    auto & pos = index[i];
    pos.clear();
    pos.reserve(ties[i].size());
    for (size_t k = 0u; k < ties[i].size(); ++k)
        pos[ties[i][k]] = k;

}

inline size_t Network::find(size_t i, size_t j) const
{

    const auto & row = ties[i];

    if (row.size() > EPI_NEIGHBOR_INDEX_THRESHOLD)
    {
        const auto & pos = index.at(i);
        auto it = pos.find(j);
        return (it == pos.end()) ? row.size() : it->second;
    }

    for (size_t k = 0u; k < row.size(); ++k)
        if (row[k] == j)
            return k;

    return row.size();

}

inline bool Network::append(size_t i, size_t j, bool check)
{

    if (check && (find(i, j) != ties[i].size()))
        return false;

    ties[i].push_back(j);

    if (ties[i].size() > EPI_NEIGHBOR_INDEX_THRESHOLD)
    {
        auto it = index.find(i);
        if (it == index.end())
            build_index(i);
        else
            it->second[j] = ties[i].size() - 1u;
    }

    return true;

}

inline void Network::erase_at(size_t i, size_t pos)
{

    auto & row = ties[i];
    row.erase(row.begin() + static_cast< std::ptrdiff_t >(pos));

    // Everything after `pos` shifted down by one, so the index has to follow.
    // Moving the last tie into the hole would be O(1) but would permute the
    // others, and their order decides which infector the draw picks.
    if (row.size() > EPI_NEIGHBOR_INDEX_THRESHOLD)
        build_index(i);
    else
        index.erase(i);

}

inline void Network::check_endpoints(size_t i, size_t j) const
{

    if ((i >= ties.size()) || (j >= ties.size()))
        throw std::range_error(
            "Network layer '" + this->get_name() + "': agent ids must be below " +
            std::to_string(ties.size()) + "; got " + std::to_string(i) +
            " and " + std::to_string(j) + "."
        );

    if (i == j)
        throw std::logic_error(
            "Network layer '" + this->get_name() + "': an agent cannot be tied "
            "to itself (agent " + std::to_string(i) + ")."
        );

    if (directed)
        throw std::logic_error(
            "Network layer '" + this->get_name() + "': add_edge() and rm_edge() "
            "change both ends of a tie, which is not meaningful in a directed "
            "network."
        );

}

inline void Network::edit()
{

    // A fixed graph keeps a copy to restore, taken before its first edit, so
    // that a graph never edited is held once
    if (!generator && baseline_start.empty())
        capture_baseline();

    // Between runs, a fixed graph is edited from its baseline: the edits of the
    // last run are dropped first, and the next reset takes the result as the
    // new baseline.
    const bool dirty = this->is_dirty();
    if (!this->edits_baseline() || generator)
        return;

    if (dirty)
    {
        restore_baseline();
        this->mark_clean();
    }

    baseline_stale = true;

}

inline NeighborsView Network::neighbors_view(size_t i)
{

    if (this->model == nullptr)
        throw std::logic_error(
            "Network layer '" + this->get_name() + "': neighbors_view() needs "
            "the layer to be added to a model."
        );

    if (i >= ties.size())
        throw std::range_error(
            "Network layer '" + this->get_name() + "': agent " +
            std::to_string(i) + " is out of range."
        );

    const auto & row = ties[i];
    return NeighborsView(
        row.empty() ? nullptr : row.data(), row.size(),
        &this->model->get_agents()
    );

}

inline bool Network::add_edge(size_t i, size_t j)
{

    check_endpoints(i, j);
    edit();

    // Two statements, so that the second end is visited even when the first
    // already had the tie
    const bool here  = append(i, j, true);
    const bool there = append(j, i, true);

    if (!here && !there)
        return false;

    if (this->model != nullptr)
        this->model->network_tie_changed(i, here ? 1 : 0, j, there ? 1 : 0);

    return true;

}

inline bool Network::rm_edge(size_t i, size_t j)
{

    check_endpoints(i, j);
    edit();

    const size_t pos_i = find(i, j);
    const size_t pos_j = find(j, i);
    const bool rm_i = pos_i != ties[i].size();
    const bool rm_j = pos_j != ties[j].size();

    // Nothing to unwind if the two were never tied
    if (!rm_i && !rm_j)
        return false;

    if (rm_i)
        erase_at(i, pos_i);

    if (rm_j)
        erase_at(j, pos_j);

    if (this->model != nullptr)
        this->model->network_tie_changed(i, rm_i ? -1 : 0, j, rm_j ? -1 : 0);

    return true;

}

inline bool Network::has_edge(size_t i, size_t j) const
{

    if ((i >= ties.size()) || (j >= ties.size()))
        throw std::range_error(
            "Network layer '" + this->get_name() + "': agent ids must be below " +
            std::to_string(ties.size()) + "; got " + std::to_string(i) +
            " and " + std::to_string(j) + "."
        );

    return find(i, j) != ties[i].size();

}

inline void Network::swap_ties(
    size_t i,
    size_t pos_i,
    size_t k,
    size_t pos_k
)
{

    if ((i >= ties.size()) || (k >= ties.size()))
        throw std::range_error(
            "Network layer '" + this->get_name() + "': agent ids must be below " +
            std::to_string(ties.size()) + "."
        );

    if (pos_i >= ties[i].size())
        throw std::range_error(
            "The tie you want to swap is out of range. Agent " +
            std::to_string(i) + " only has " + std::to_string(ties[i].size()) +
            " ties."
        );

    if (pos_k >= ties[k].size())
        throw std::range_error(
            "The tie you want to swap is out of range. Agent " +
            std::to_string(k) + " only has " + std::to_string(ties[k].size()) +
            " ties."
        );

    const size_t j = ties[i][pos_i];
    const size_t l = ties[k][pos_k];

    // The other ends follow in an undirected network, so ties that share an
    // agent would write into the rows being swapped (and leave self-ties)
    if (!directed && ((i == k) || (j == l) || (i == l) || (k == j)))
        throw std::logic_error(
            "Network layer '" + this->get_name() + "': swap_ties() needs four "
            "different agents in an undirected network; the ties (" +
            std::to_string(i) + "-" + std::to_string(j) + ") and (" +
            std::to_string(k) + "-" + std::to_string(l) + ") share one."
        );

    edit();

    std::swap(ties[i][pos_i], ties[k][pos_k]);

    if (!directed)
    {

        // The two agents that were swapped away have to be told: the one that
        // pointed back at `i` now points at `k`, and vice versa. Positions are
        // looked up rather than cached, so ties can be edited without a
        // back-pointer table going stale.
        const size_t back_j = find(j, i);
        const size_t back_l = find(l, k);

        #ifdef EPI_DEBUG
        if ((back_j >= ties[j].size()) || (back_l >= ties[l].size()))
            throw std::logic_error(
                "[epi-debug] swap_ties: the tie is not reciprocated.");
        #endif

        ties[j][back_j] = k;
        ties[l][back_l] = i;

        // Four lists may have changed. Rebuilding is O(degree), the same order
        // as the swap itself, and sidesteps the aliasing cases (an agent on
        // both sides of the swap) that piecemeal updates get wrong.
        for (size_t a : {i, k, j, l})
            if (ties[a].size() > EPI_NEIGHBOR_INDEX_THRESHOLD)
                build_index(a);

    }
    else
    {

        for (size_t a : {i, k})
            if (ties[a].size() > EPI_NEIGHBOR_INDEX_THRESHOLD)
                build_index(a);

    }

    // Nobody's degree changed; only the queue needs to follow
    if (this->model != nullptr)
        this->model->network_ties_swapped(i, j, k, l, directed);

}

inline void Network::get_edgelist(
    std::vector< int > & source,
    std::vector< int > & target
) const
{

    for (size_t i = 0u; i < ties.size(); ++i)
        for (size_t j : ties[i])
            if (directed || (i <= j))
            {
                source.push_back(static_cast< int >(i));
                target.push_back(static_cast< int >(j));
            }

}

inline void Network::write_edgelist(const std::string & fn) const
{

    std::ofstream efile(fn, std::ios_base::out);
    efile << "source target\n";

    for (size_t i = 0u; i < ties.size(); ++i)
        for (size_t j : ties[i])
            if (directed || (i <= j))
                efile << i << " " << j << "\n";

}

template< bool Record >
inline size_t Network::pull(
    size_t agent,
    Model & model,
    const std::vector< bool > * exclude,
    size_t * out
) const
{

    size_t n = 0u;
    const auto & row = ties[agent];

    // During a run, most neighbors do not carry a virus. The model's compact
    // per-agent flag rules them out reading one byte each instead of loading
    // the neighbor's `Agent`; the contacts kept, and their order, are the same.
    const char * carrier = model.get_agent_carrier();
    auto & agents = model.get_agents();

    for (size_t j : row)
    {

        if ((carrier != nullptr) && (carrier[j] == 0))
            continue;

        auto & neighbor = agents[j];

        if ((exclude != nullptr) && (*exclude)[neighbor.get_state()])
            continue;

        if (neighbor.get_virus() == nullptr)
            continue;

        // The post-sampling callback sees every eligible contact
        if constexpr (Record)
            model.register_sampled_contact(j, agent);

        out[n++] = j;

    }

    return n;

}

inline void Network::reset(Model & model)
{

    if (generator)
        set_graph(generator(model));
    else if (this->is_dirty())
        restore_baseline();
    else if (baseline_stale)
        capture_baseline(); // Edits made between runs become the baseline

    baseline_stale = false;

    if (ties.size() != model.size())
        throw std::length_error(
            "Network layer '" + this->get_name() + "': the graph has " +
            std::to_string(ties.size()) + " vertices, but the model has " +
            std::to_string(model.size()) + " agents."
        );

    this->mark_clean();

}

inline void Network::update(Model & model)
{

    // Day 0 is recorded, not stepped: rewiring starts at the end of day 1
    if (rewiring && (model.today() > 0))
        rewiring(*this, model);

}

}

/**
 * @brief Rewires a share of a network layer's ties, keeping every degree.
 *
 * @details Picks two agents with probability proportional to their degree and
 * one tie of each, uniformly, and swaps them (`layer::Network::swap_ties()`),
 * skipping swaps that would create self-ties or repeated ties;
 * `floor(proportion * sum of degrees)` attempts. Use it through
 * `graph::rewire_degseq()`.
 */
inline void rewire_degseq(
    layer::Network & net,
    Model & model,
    epiworld_double proportion
)
{

    const size_t n = net.size();

    #ifdef EPI_DEBUG
    std::vector< size_t > degree0(n);
    for (size_t i = 0u; i < n; ++i)
        degree0[i] = net.get_degree(i);
    #endif

    // Agents with ties, weighted by degree
    std::vector< size_t > non_isolates;
    std::vector< epiworld_double > weights;
    epiworld_double nedges = 0.0;

    for (size_t i = 0u; i < n; ++i)
    {
        if (net.get_degree(i) > 0u)
        {
            non_isolates.push_back(i);
            epiworld_double wtemp = static_cast< epiworld_double >(net.get_degree(i));
            weights.push_back(wtemp);
            nedges += wtemp;
        }
    }

    if (non_isolates.size() == 0u)
        throw std::logic_error("The graph is completely disconnected.");

    // Cumulative probs
    weights[0u] /= nedges;
    for (size_t i = 1u; i < non_isolates.size(); ++i)
    {
         weights[i] /= nedges;
         weights[i] += weights[i - 1u];
    }

    const size_t N = non_isolates.size();
    int nrewires = floor(proportion * nedges);
    while (nrewires-- > 0)
    {

        // Picking egos
        int id0 = static_cast< int >(pick_cumulative(weights, model.runif()));
        int id1 = static_cast< int >(pick_cumulative(weights, model.runif()));

        // Correcting for under or overflow.
        if (id1 == id0)
            id1++;

        if (id1 >= static_cast< int >(N))
            id1 = 0;

        const size_t a0 = non_isolates[id0];
        const size_t a1 = non_isolates[id1];

        // Picking alters (relative location in their lists), uniformly
        const size_t id01 = model.runif_index(net.get_degree(a0));
        const size_t id11 = model.runif_index(net.get_degree(a1));

        const size_t alter0 = net.neighbors_view(a0).id(id01);
        const size_t alter1 = net.neighbors_view(a1).id(id11);

        // After the swap a0 is tied to alter1 and a1 to alter0. Skip swaps of
        // the same alter, and those that would tie an agent to itself or
        // repeat a tie.
        if ((alter0 == alter1) || (alter0 == a1) || (alter1 == a0))
            continue;

        if (net.has_edge(a0, alter1) || net.has_edge(a1, alter0))
            continue;

        net.swap_ties(a0, id01, a1, id11);

    }

    #ifdef EPI_DEBUG
    for (size_t i = 0u; i < n; ++i)
        if (degree0[i] != net.get_degree(i))
            throw std::logic_error("[epi-debug] Degree does not match after rewire_degseq.");
    #endif

}

namespace graph {

/**
 * @name Network generators
 *
 * @details Each returns a `layer::Network::Generator`: a function that
 * draws the graph for a model, with one vertex per agent, using the model's
 * random numbers. The arguments are structure, used once per draw (see the
 * contact-layers plan, point 17).
 */
///@{

/// Small-world network: ring lattice with `k` ties per agent, rewired with
/// probability `p` (see `rgraph_smallworld()`).
inline typename layer::Network::Generator smallworld(
    epiworld_fast_uint k,
    epiworld_double p,
    bool directed = false
)
{
    return [k, p, directed](Model & model) -> AdjList {
        return rgraph_smallworld(
            static_cast< epiworld_fast_uint >(model.size()), k, p, directed, model
        );
    };
}

/// Bernoulli (Erdős–Rényi) network: each tie with probability `p` (see
/// `rgraph_bernoulli()`).
inline typename layer::Network::Generator bernoulli(
    epiworld_double p,
    bool directed = false
)
{
    return [p, directed](Model & model) -> AdjList {
        return rgraph_bernoulli(
            static_cast< epiworld_fast_uint >(model.size()), p, directed, model
        );
    };
}

/// Stochastic block model (see `rgraph_sbm()`); the block sizes must add up to
/// the model's population.
inline typename layer::Network::Generator sbm(
    std::vector< size_t > block_sizes,
    std::vector< double > mixing_matrix,
    bool row_major = true
)
{
    return [block_sizes, mixing_matrix, row_major](Model & model) -> AdjList {
        return rgraph_sbm(block_sizes, mixing_matrix, row_major, model);
    };
}

///@}

/**
 * @brief Degree-preserving rewiring, for `layer::Network::set_rewiring()`.
 *
 * @details Each step, a share of the ties, read from the model parameter
 * `param` (a proportion in `[0, 1]`), is rewired: two ties {(i, j), (k, l)}
 * become {(i, l), (k, j)}, skipping swaps that would create self-ties or
 * repeated ties. In a directed network only the sources' ties move, which keeps
 * every agent's in- and out-degree.
 */
inline typename layer::Network::Rewiring rewire_degseq(std::string param)
{
    return [ref = ParamRef(std::move(param))](
        layer::Network & net, Model & model
    ) -> void {

        const epiworld_double proportion = ref(model);
        if ((proportion < 0.0) || (proportion > 1.0))
            throw std::range_error(
                "The rewiring proportion '" + ref.name() + "' must be in [0, 1]; "
                "it is " + std::to_string(proportion) + "."
            );

        epiworld::rewire_degseq(net, model, proportion);

    };
}

}

#endif
/*//////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

 End of -./include/epiworld/layer-network.hpp-

////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////*/



/*//////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

 Start of -./include/epiworld/assign-meat.hpp-

////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////*/


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
/*//////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

 End of -./include/epiworld/assign-meat.hpp-

////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////*/


/*//////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

 Start of -./include/epiworld/queue-meat.hpp-

////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////*/


#ifndef EPIWORLD_QUEUE_MEAT_HPP
#define EPIWORLD_QUEUE_MEAT_HPP

// The member functions of `Queue` (see queue-bones.hpp)

inline void Queue::shift(size_t id, epiworld_fast_int n)
{

    if (n == 0)
        return;

    epiworld_fast_int before = active[id];
    active[id] += n;

    // The agent enters or leaves the queue only when its count crosses zero.
    // Then the count of queued agents changes, and so does the agent's bit in
    // `bits`: word id / 64 (id >> 6), bit id % 64 (id & 63). Setting and
    // clearing that bit here is what keeps `for_each_nonzero()` -- the ordered
    // walk over queued agents that replaces scanning the whole population --
    // in step with the counts.
    if ((before == 0) && (active[id] != 0))
    {
        n_in_queue++;
        bits[id >> 6] |= (uint64_t(1) << (id & 63u));
    }
    else if ((before != 0) && (active[id] == 0))
    {
        n_in_queue--;
        bits[id >> 6] &= ~(uint64_t(1) << (id & 63u));
    }

}

inline void Queue::shift_neighbors(size_t id, epiworld_fast_int n)
{

    for (const auto * net : model->network_layers)
        for (size_t j : net->ties[id])
            shift(j, n);

    for (const auto * mix : model->group_layers)
    {
        const size_t g = mix->groups[id];
        if (g == layer::Mixing::NO_GROUP)
            continue;

        for (size_t j : mix->members[g])
            if (j != id)
                shift(j, n);
    }

}

inline void Queue::operator+=(Agent * p)
{

    const size_t id = static_cast< size_t >(p->id);
    everyone[id]++;
    shift(id, 1);
    shift_neighbors(id, 1);

}

inline void Queue::operator-=(Agent * p)
{

    const size_t id = static_cast< size_t >(p->id);
    everyone[id]--;
    shift(id, -1);
    shift_neighbors(id, -1);

}

inline bool Queue::tracks(size_t a, size_t b) const
{

    // The counters are sized by reset(), i.e. when a run starts. Editing the
    // network before that -- while the model is still being set up -- has no
    // queue to keep in step, and the counts are built from the finished network
    // anyway.
    return everyone.size() > (a > b ? a : b);

}

inline void Queue::notify_ties_changed(size_t a, int da, size_t b, int db)
{

    if (!tracks(a, b))
        return;

    shift(b, da * everyone[a]);
    shift(a, db * everyone[b]);

}

inline void Queue::notify_group_changed(
    size_t a,
    const std::vector< size_t > * from,
    const std::vector< size_t > * to
)
{

    if (!tracks(a, a))
        return;

    if (from != nullptr)
        for (size_t j : *from)
            if (j != a)
                notify_ties_changed(a, -1, j, -1);

    if (to != nullptr)
        for (size_t j : *to)
            if (j != a)
                notify_ties_changed(a, 1, j, 1);

}

inline void Queue::notify_edges_swapped(
    size_t a,
    size_t b,
    size_t c,
    size_t d,
    bool directed
)
{

    if (!tracks(a, b) || !tracks(c, d))
        return;

    // `a`'s entry for `b` now names `d`, and `c`'s entry for `d` now names `b`.
    epiworld_fast_int delta = everyone[c] - everyone[a];
    shift(b, delta);
    shift(d, -delta);

    if (directed)
        return;

    // `b`'s entry for `a` now names `c`, and `d`'s entry for `c` now names `a`.
    delta = everyone[d] - everyone[b];
    shift(a, delta);
    shift(c, -delta);

}

inline epiworld_fast_int Queue::operator[](epiworld_fast_uint i) const
{
    return active[i];
}

template< typename F >
inline void Queue::for_each_nonzero(F && f)
{

    const size_t nwords = bits.size();
    for (size_t w = 0u; w < nwords; ++w)
    {

        uint64_t word = bits[w];
        while (word != 0u)
        {

            unsigned int b = epi_ctz64(word);
            f((w << 6) + b);

            // Re-read the word: `f` may have queued or dequeued agents ahead
            // of this one. Only the bits above `b` are still to be visited.
            word = (b == 63u) ? 0u : (bits[w] & (~uint64_t(0) << (b + 1u)));

        }

    }

}

inline void Queue::reset()
{

    // Cleared unconditionally: counts that never went through `shift()` (or
    // that went negative) would otherwise survive into the next run.
    size_t n = model->size();
    active.assign(n, 0);
    everyone.assign(n, 0);
    bits.assign((n + 63u) / 64u, 0u);
    n_in_queue = 0;

}

inline bool Queue::operator==(const Queue & other) const 
{
    if (active.size() != other.active.size())
        return false;

    for (size_t i = 0u; i < active.size(); ++i)
    {
        if (active[i] != other.active[i])
            return false;
    }

    if (everyone.size() != other.everyone.size())
        return false;

    for (size_t i = 0u; i < everyone.size(); ++i)
    {
        if (everyone[i] != other.everyone[i])
            return false;
    }

    return true;
}

#endif
/*//////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

 End of -./include/epiworld/queue-meat.hpp-

////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////*/


/*//////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

 Start of -./include/epiworld/userdata-meat.hpp-

////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////*/


#ifndef EPIWORLD_USERDATA_MEAT_HPP
#define EPIWORLD_USERDATA_MEAT_HPP

class UserData;

inline UserData::UserData(std::vector< std::string > names)
{

    k = names.size();
    data_names = names;

}

inline void UserData::add(std::vector<epiworld_double> x)
{

    if (x.size() != k)
        throw std::out_of_range(
            "The size of -x-, " + std::to_string(x.size()) + ", does not match " +
            "the number of elements registered (" + std::to_string(k));

    for (auto & i : x)
        data_data.push_back(i);

    data_dates.push_back(model->today());

    n++;
    last_day = model->today();

}

inline void UserData::add(epiworld_fast_uint j, epiworld_double x)
{

    // Starting with a new day?
    if (static_cast<int>(model->today()) != last_day)
    {

        std::vector< epiworld_double > tmp(k, 0.0);

        tmp[j] = x;

        add(tmp);

    }
    else
    {

        this->operator()(n - 1, j) = x;

    }

}

inline std::vector< std::string > & UserData::get_names() 
{
    return data_names;
}

inline std::vector< int > & UserData::get_dates() 
{
    return data_dates;
}

inline std::vector< epiworld_double > & UserData::get_data() 
{
    return data_data;
}

inline void UserData::get_all(
    std::vector< std::string > * names,
    std::vector< int > * date,
    std::vector< epiworld_double > * data
) 
{
    
    if (names != nullptr)
        names = &this->data_names;

    if (date != nullptr)
        date = &this->data_dates;

    if (data != nullptr)
        data = &this->data_data;

}

inline epiworld_double & UserData::operator()(
    epiworld_fast_uint i,
    epiworld_fast_uint j
)
{

    if (j >= k)
        throw std::out_of_range("j cannot be greater than k - 1.");

    if (i >= n)
        throw std::out_of_range("j cannot be greater than n - 1.");

    return data_data[k * i + j];

}

inline epiworld_double & UserData::operator()(
    epiworld_fast_uint i,
    std::string name
)
{
    int loc = -1;
    for (epiworld_fast_uint l = 0u; l < k; ++l)
    {

        if (name == data_names[l])
        {

            loc = l;
            break;

        }

    }

    if (loc < 0)
        throw std::range_error(
            "The variable \"" + name + "\" is not present " +
            "in the user UserData database."
        );

    return operator()(i, static_cast<epiworld_fast_uint>(loc));

}

inline epiworld_fast_uint UserData::nrow() const
{
    return n;
}

inline epiworld_fast_uint UserData::ncol() const
{
    return k;
}

inline void UserData::write(std::string fn)
{
    std::ofstream file_ud(fn, std::ios_base::out);

    // File header
    file_ud << "\"date\"";
    for (auto & cn : data_names)
        file_ud << " \"" + cn + "\"";
    file_ud << "\n";
    
    epiworld_fast_uint ndata = 0u;
    for (epiworld_fast_uint i = 0u; i < n; ++i)
    {
        file_ud << data_dates[i];

        for (epiworld_fast_uint j = 0u; j < k; ++j)
            file_ud << " " << data_data[ndata++];

        file_ud << "\n";
    }

    return;
}

inline void UserData::print() const
{
    // File header
    printf_epiworld("Total records: %llu\n", n);
    printf_epiworld("date");

    for (auto & cn : data_names)
    {

        printf_epiworld(" %s", cn.c_str());

    }

    printf_epiworld("\n");
    
    epiworld_fast_uint ndata = 0u;
    
    for (epiworld_fast_uint i = 0u; i < n; ++i)
    {

        printf_epiworld("%i", data_dates[i]);

        for (epiworld_fast_uint j = 0u; j < k; ++j)
        {

            printf_epiworld(" %.2f", data_data[ndata++]);

        }

        printf_epiworld("\n");

    }

    return;
}

#endif
/*//////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

 End of -./include/epiworld/userdata-meat.hpp-

////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////*/


/*//////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

 Start of -./include/epiworld/hospitalizationstracker-meat.hpp-

////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////*/


#ifndef EPIWORLD_HOSPITALIZATIONSTRACKER_MEAT_HPP
#define EPIWORLD_HOSPITALIZATIONSTRACKER_MEAT_HPP


inline void HospitalizationsTracker::reset()
{
    _date.clear();
    _virus_id.clear();
    _tool_id.clear();
    _weight.clear();
}

inline void HospitalizationsTracker::record(
    Agent & agent,
    Model & model
)
{
    int current_date = model.today();
    
    // Get the virus ID (-1 if no virus)
    int v_id = -1;
    auto & virus = agent.get_virus();
    if (virus != nullptr)
        v_id = virus->get_id();
    
    // Get the number of tools
    size_t n_tools = agent.get_n_tools();
    
    if (n_tools == 0u)
    {
        // No tools: single record with tool_id = -1 and weight = 1.0
        _date.push_back(current_date);
        _virus_id.push_back(v_id);
        _tool_id.push_back(-1);
        _weight.push_back(1.0);
    }
    else
    {
        // Multiple tools: one record per tool with weight = 1/N
        double w = 1.0 / static_cast<double>(n_tools);
        for (size_t i = 0u; i < n_tools; ++i)
        {
            _date.push_back(current_date);
            _virus_id.push_back(v_id);
            _tool_id.push_back(agent.get_tool(static_cast<int>(i))->get_id());
            _weight.push_back(w);
        }
    }
}

inline void HospitalizationsTracker::get(
    int ndays,
    std::vector<int> & date,
    std::vector<int> & virus_id,
    std::vector<int> & tool_id,
    std::vector<int> & count,
    std::vector<double> & weight
) const
{
    // Clear output vectors
    date.clear();
    virus_id.clear();
    tool_id.clear();
    count.clear();
    weight.clear();
    
    if (ndays <= 0)
        return;
    
    // First, aggregate by (date, virus_id, tool_id) to get count and sum of weights
    // Key: (date, virus_id, tool_id), Value: (count, sum of weight)
    std::map<std::tuple<int, int, int>, std::pair<int, double>> aggregated;
    
    // Collect unique (virus_id, tool_id) combinations
    std::set<std::pair<int, int>> unique_combinations;
    
    for (size_t i = 0u; i < _date.size(); ++i)
    {
        auto key = std::make_tuple(_date[i], _virus_id[i], _tool_id[i]);
        aggregated[key].first += 1;  // Count
        aggregated[key].second += _weight[i];  // Weight
        unique_combinations.insert(std::make_pair(_virus_id[i], _tool_id[i]));
    }
    
    // If no hospitalizations occurred, still output the full time series 
    // with a single combination (virus_id=-1, tool_id=-1) and zeros
    if (unique_combinations.empty())
    {
        unique_combinations.insert(std::make_pair(-1, -1));
    }
    
    // For each unique (virus_id, tool_id) combination, output all days
    for (const auto & combo : unique_combinations)
    {
        int v_id = combo.first;
        int t_id = combo.second;
        
        for (int d = 0; d < ndays; ++d)
        {
            date.push_back(d);
            virus_id.push_back(v_id);
            tool_id.push_back(t_id);
            
            // Look up the count and weight for this (date, virus_id, tool_id) combination
            auto key = std::make_tuple(d, v_id, t_id);
            auto it = aggregated.find(key);
            if (it != aggregated.end())
            {
                count.push_back(it->second.first);
                weight.push_back(it->second.second);
            }
            else
            {
                count.push_back(0);
                weight.push_back(0.0);
            }
        }
    }
}

inline size_t HospitalizationsTracker::size() const
{
    return _date.size();
}

#endif
/*//////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

 End of -./include/epiworld/hospitalizationstracker-meat.hpp-

////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////*/


/*//////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

 Start of -./include/epiworld/database-meat.hpp-

////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////*/


#ifndef EPIWORLD_DATABASE_MEAT_HPP
#define EPIWORLD_DATABASE_MEAT_HPP

#include <vector>
#include <string>
// (already included include/epiworld/config.hpp)
// (already included include/epiworld/epiworld-macros.hpp)
// (already included include/epiworld/misc.hpp)
// (already included include/epiworld/database-bones.hpp)

inline void DataBase::reset()
{

    // Initializing the counts
    today_total.assign(model->nstates, 0);
    for (auto & p : model->get_agents())
        ++today_total[p.get_state()];

    #ifdef EPI_DEBUG
    // Only the first should be different from zero
    {
        int n = static_cast<int>(model->size());
        if (today_total[0] != n)
            throw std::runtime_error("The number of susceptible agents is not equal to the total number of agents.");

        if (std::accumulate(today_total.begin(), today_total.end(), 0) != n)
            throw std::runtime_error("The total number of agents is not equal to the sum of the number of agents in each state.");
            
    }
    #endif

    
    transition_matrix.assign(model->nstates * model->nstates, 0);
    for (size_t s = 0u; s < model->nstates; ++s)
        transition_matrix[s + s * model->nstates] = today_total[s];

    // The last run's variants go; the founders keep their ids
    drop_variants();

    hist_virus_date.clear();
    hist_virus_id.clear();
    hist_virus_state.clear();
    hist_virus_counts.clear();

    hist_tool_date.clear();
    hist_tool_id.clear();
    hist_tool_state.clear();
    hist_tool_counts.clear();    

    today_virus.resize(get_n_viruses());
    for (auto& virus_states : today_virus)
        virus_states.assign(model->nstates, 0);

    today_tool.resize(get_n_tools());
    for (auto& tool_states : today_tool)
        tool_states.assign(model->nstates, 0);

    hist_total_date.clear();
    hist_total_state.clear();
    hist_total_nviruses_active.clear();
    hist_total_counts.clear();
    hist_transition_matrix.clear();

    transmission_date.clear();
    transmission_virus.clear();
    transmission_source.clear();
    transmission_target.clear();
    transmission_source_exposure_date.clear();
    transmission_layer.clear();

    transition_today.assign(
        model->nh ?
            model->nh->get_n_transitions() +
                (model->st ? model->st->get_n_transitions() : 0u) :
            0u,
        0
    );
    hist_transition_date.clear();
    hist_transition_id.clear();
    hist_transition_counts.clear();

    m_hospitalizations.reset();

    return;

}

inline DataBase::DataBase(const DataBase & db) :
    m_hospitalizations(db.m_hospitalizations),
    n_founders(db.n_founders),
    virus_name(db.virus_name),
    virus_origin_date(db.virus_origin_date),
    virus_parent_id(db.virus_parent_id),
    tool_name(db.tool_name),
    tool_origin_date(db.tool_origin_date),
    // {Variant 1: {state 1, state 2, etc.}, Variant 2: {...}, ...}
    today_virus(db.today_virus),
    // {Variant 1: {state 1, state 2, etc.}, Variant 2: {...}, ...}
    today_tool(db.today_tool),
    // {Susceptible, Infected, etc.}
    today_total(db.today_total),
    // Totals
    today_total_nviruses_active(db.today_total_nviruses_active),
    sampling_freq(db.sampling_freq),
    // Variants history
    hist_virus_date(db.hist_virus_date),
    hist_virus_id(db.hist_virus_id),
    hist_virus_state(db.hist_virus_state),
    hist_virus_counts(db.hist_virus_counts),
    // Tools history
    hist_tool_date(db.hist_tool_date),
    hist_tool_id(db.hist_tool_id),
    hist_tool_state(db.hist_tool_state),
    hist_tool_counts(db.hist_tool_counts),
    // Overall hist
    hist_total_date(db.hist_total_date),
    hist_total_nviruses_active(db.hist_total_nviruses_active),
    hist_total_state(db.hist_total_state),
    hist_total_counts(db.hist_total_counts),
    hist_transition_matrix(db.hist_transition_matrix),
    // Transmission network
    transmission_date(db.transmission_date),
    transmission_source(db.transmission_source),
    transmission_target(db.transmission_target),
    transmission_virus(db.transmission_virus),
    transmission_source_exposure_date(db.transmission_source_exposure_date),
    transmission_layer(db.transmission_layer),
    transition_matrix(db.transition_matrix),
    transition_today(db.transition_today),
    hist_transition_date(db.hist_transition_date),
    hist_transition_id(db.hist_transition_id),
    hist_transition_counts(db.hist_transition_counts),
    user_data(nullptr)
{}

// DataBase & DataBase::operator=(const DataBase & m)
// {

// }

inline Model * DataBase::get_model() {
    return model;
}

inline void DataBase::record() 
{

    ////////////////////////////////////////////////////////////////////////////
    // DEBUGGING BLOCK
    ////////////////////////////////////////////////////////////////////////////
    EPI_DEBUG_SUM_INT(today_total, model->size())
    EPI_DEBUG_ALL_NON_NEGATIVE(today_total)

    #ifdef EPI_DEBUG
    // Checking whether the sums correspond
    std::vector< int > _today_total_cp(today_total.size(), 0);
    for (auto & p : model->population)
        _today_total_cp[p.get_state()]++;
    
    EPI_DEBUG_VECTOR_MATCH_INT(
        _today_total_cp, today_total,
        "Sums of __today_total_cp in database-meat.hpp"
        )

    // EPI_DEBUG only: the model's agents-by-state index (see
    // Model::get_agents_in_state()) is maintained incrementally, event by
    // event, so check it against the population once per step, next to the
    // same check for today_total above. Each state's block must hold exactly
    // the agents in that state, each agent's recorded position must be right,
    // and the degree and carrier sums used by the transmission step must match
    // a recount. A drift here would silently bias which agents push, or which
    // mode "auto" picks.
    if (model->state_index_ready)
    {

        std::vector< size_t > _deg(model->nstates, 0u);
        std::vector< size_t > _carriers(model->nstates, 0u);
        std::vector< size_t > _carrier_deg(model->nstates, 0u);
        for (auto & p : model->population)
        {
            const size_t _d = model->network_degree(static_cast< size_t >(p.get_id()));
            _deg[p.get_state()] += _d;
            if (p.get_virus() != nullptr)
            {
                _carriers[p.get_state()]++;
                _carrier_deg[p.get_state()] += _d;
            }
        }

        for (size_t s = 0u; s < model->nstates; ++s)
        {

            const auto members = model->state_index_members(s);
            if (static_cast< int >(members.size()) != today_total[s])
                throw std::logic_error("[epi-debug] DataBase::record state index size doesn't match today_total.");

            for (size_t k = 0u; k < members.size(); ++k)
            {
                if (model->population[members[k]].get_state() != s)
                    throw std::logic_error("[epi-debug] DataBase::record state index lists an agent in another state.");
                if (model->agent_state[members[k]] != s)
                    throw std::logic_error("[epi-debug] DataBase::record agent_state is out of step.");
                if (model->agent_carrier[members[k]] != (model->population[members[k]].get_virus() != nullptr))
                    throw std::logic_error("[epi-debug] DataBase::record agent_carrier is out of step.");
                if (model->state_member_pos[members[k]] != model->state_start[s] + k)
                    throw std::logic_error("[epi-debug] DataBase::record state_member_pos is out of step.");
            }

            if (
                (_deg[s] != model->state_degree[s]) ||
                (_carriers[s] != model->state_carriers[s]) ||
                (_carrier_deg[s] != model->state_carrier_degree[s])
            )
                throw std::logic_error("[epi-debug] DataBase::record state degree/carrier sums are out of step.");

        }

    }

    if (model->today() == 0)
    {
        if (hist_total_date.size() != 0)
            EPI_DEBUG_ERROR(std::logic_error, "DataBase::record hist_total_date should be of length 0.")
        if (hist_total_nviruses_active.size() != 0)
            EPI_DEBUG_ERROR(std::logic_error, "DataBase::record hist_total_nviruses_active should be of length 0.")
        if (hist_total_state.size() != 0)
            EPI_DEBUG_ERROR(std::logic_error, "DataBase::record hist_total_state should be of length 0.")
        if (hist_total_counts.size() != 0)
            EPI_DEBUG_ERROR(std::logic_error, "DataBase::record hist_total_counts should be of length 0.")
        if (hist_virus_date.size() != 0)
            EPI_DEBUG_ERROR(std::logic_error, "DataBase::record hist_virus_date should be of length 0.")
        if (hist_virus_id.size() != 0)
            EPI_DEBUG_ERROR(std::logic_error, "DataBase::record hist_virus_id should be of length 0.")
        if (hist_virus_state.size() != 0)
            EPI_DEBUG_ERROR(std::logic_error, "DataBase::record hist_virus_state should be of length 0.")
        if (hist_virus_counts.size() != 0)
            EPI_DEBUG_ERROR(std::logic_error, "DataBase::record hist_virus_counts should be of length 0.")
        if (hist_tool_date.size() != 0)
            EPI_DEBUG_ERROR(std::logic_error, "DataBase::record hist_tool_date should be of length 0.")
        if (hist_tool_id.size() != 0)
            EPI_DEBUG_ERROR(std::logic_error, "DataBase::record hist_tool_id should be of length 0.")
        if (hist_tool_state.size() != 0)
            EPI_DEBUG_ERROR(std::logic_error, "DataBase::record hist_tool_state should be of length 0.")
        if (hist_tool_counts.size() != 0)
            EPI_DEBUG_ERROR(std::logic_error, "DataBase::record hist_tool_counts should be of length 0.")
    }
    #endif
    ////////////////////////////////////////////////////////////////////////////

    // Only store every now and then
    if ((model->today() % sampling_freq) == 0)
    {

        // Recording virus's history
        for (size_t v = 0u; v < today_virus.size(); ++v)
        {

            for (epiworld_fast_uint s = 0u; s < model->nstates; ++s)
            {

                hist_virus_date.push_back(model->today());
                hist_virus_id.push_back(static_cast< int >(v));
                hist_virus_state.push_back(s);

                #ifdef EPI_DEBUG
                if (today_virus[v][s] < 0)
                    throw std::logic_error(
                        "The count of viruses in DataBase::record cannot be negative."
                    );
                #endif

                hist_virus_counts.push_back(today_virus[v][s]);

            }

        }

        // Recording tool's history
        for (size_t t = 0u; t < today_tool.size(); ++t)
        {

            for (epiworld_fast_uint s = 0u; s < model->nstates; ++s)
            {

                hist_tool_date.push_back(model->today());
                hist_tool_id.push_back(static_cast< int >(t));
                hist_tool_state.push_back(s);
                hist_tool_counts.push_back(today_tool[t][s]);

            }

        }

        // Recording the overall history
        for (epiworld_fast_uint s = 0u; s < model->nstates; ++s)
        {
            hist_total_date.push_back(model->today());
            hist_total_nviruses_active.push_back(today_total_nviruses_active);
            hist_total_state.push_back(s);
            hist_total_counts.push_back(today_total[s]);
        }

        for (const auto& cell : transition_matrix)
            hist_transition_matrix.push_back(cell);

        for (size_t k = 0u; k < transition_today.size(); ++k)
        {
            hist_transition_date.push_back(model->today());
            hist_transition_id.push_back(static_cast< int >(k));
            hist_transition_counts.push_back(transition_today[k]);
            transition_today[k] = 0;
        }

        // Now the diagonal must reflect the state
        for (size_t s_i = 0u; s_i < model->nstates; ++s_i)
        {

            for (size_t s_j = 0u; s_j < model->nstates; ++s_j)
            {
                
                if ((s_i != s_j) && (transition_matrix[s_i + s_j * model->nstates] > 0))
                {
                    transition_matrix[s_j + s_j * model->nstates] +=
                        transition_matrix[s_i + s_j * model->nstates];

                    transition_matrix[s_i + s_j * model->nstates] = 0;
                }
         
            }

        }

        #ifdef EPI_DEBUG
        for (size_t s_i = 0u; s_i < model->nstates; ++s_i)
        {
            if (transition_matrix[s_i + s_i * model->nstates] != 
                today_total[s_i])
                throw std::logic_error(
                    "The diagonal of the updated transition Matrix should match the daily totals"
                    );
        }
        #endif

    }

}

inline void DataBase::drop_variants()
{

    const size_t n = n_founders;
    if (virus_name.size() <= n)
        return;

    virus_name.resize(n);
    virus_origin_date.resize(n);
    virus_parent_id.resize(n);
    today_virus.resize(n);
    today_total_nviruses_active = static_cast< int >(n);

}

inline void DataBase::record_virus(Virus & v, bool founder)
{

    if (founder)
    {
        drop_variants();
        ++n_founders;
    }
    else if (v.get_id() < 0)
        throw std::logic_error(
            "The virus \"" + v.get_name() + "\" mutated but is not on record."
        );

    const int parent = founder ? -99 : v.get_id();
    const int id = static_cast< int >(virus_name.size());

    virus_name.push_back(v.get_name());
    virus_origin_date.push_back(model->today());
    virus_parent_id.push_back(parent);
    today_virus.push_back(std::vector< int >(model->nstates, 0));
    today_total_nviruses_active++;

    v.set_id(id);
    v.set_date(model->today());

    // A founder starts a lineage, which its mutations keep. Lineages 63 and
    // above share the overflow bit, which tools cannot target.
    if (founder)
    {
        v.lineage_id  = id;
        v.lineage_bit = uint64_t(1) << std::min(id, 63);
    }
    else if (v.get_agent() != nullptr)
    {
        // The agent's counts move from the parent to the variant
        const auto s = v.get_agent()->get_state();
        today_virus[parent][s]--;
        today_virus[id][s]++;
    }

}

inline void DataBase::record_tool(Tool & t)
{

    t.set_id(static_cast< int >(tool_name.size()));
    t.set_date(model->today());
    tool_name.push_back(t.get_name());
    tool_origin_date.push_back(model->today());
    today_tool.push_back(std::vector< int >(model->nstates, 0));

}

inline size_t DataBase::size() const
{
    return virus_name.size();
}

inline void DataBase::update_state(
        epiworld_fast_uint prev_state,
        epiworld_fast_uint new_state,
        bool undo
) {

    if (prev_state == new_state)
        return; // No need to update if the state is the same

    #ifdef EPI_DEBUG
    // Checking ranges (should be within expected)
    if ((prev_state >= model->nstates))
        throw std::out_of_range(
            "prev_state is out of range in DataBase::update_state"
        );
    if ((new_state >= model->nstates))
        throw std::out_of_range(
            "new_state is out of range in DataBase::update_state"
        );
    if (prev_state == new_state)
        throw std::logic_error(
            "prev_state and new_state are the same in DataBase::update_state"
        );
    #endif

    if (undo)
    {

        today_total[prev_state]++;
        today_total[new_state]--;
        
    } else {

        today_total[prev_state]--;
        today_total[new_state]++;

    }

    record_transition(prev_state, new_state, undo);
    
    return;
}

inline void DataBase::update_virus(
        epiworld_fast_uint virus_id,
        epiworld_fast_uint prev_state,
        epiworld_fast_uint new_state,
        bool undo
) {

    if (undo)
    {
        today_virus[virus_id][prev_state]++;
        today_virus[virus_id][new_state]--;
    }
    else
    {
        today_virus[virus_id][prev_state]--;
        today_virus[virus_id][new_state]++;
    }

    #ifdef EPI_DEBUG
    if (today_virus[virus_id][prev_state] < 0)
        throw std::logic_error("An entry in today_virus is negative.");
    if (today_virus[virus_id][new_state] < 0)
        throw std::logic_error("An entry in today_virus is negative (new_state).");
    #endif

    return;
    
}

inline void DataBase::update_tool(
        epiworld_fast_uint tool_id,
        epiworld_fast_uint prev_state,
        epiworld_fast_uint new_state,
        bool undo
) {

    if (undo)
    {
        today_tool[tool_id][prev_state]++;
        today_tool[tool_id][new_state]--;
    }
    else
    {
        today_tool[tool_id][prev_state]--;
        today_tool[tool_id][new_state]++;
    }

    return;

}

inline void DataBase::record_transition(
    epiworld_fast_uint from,
    epiworld_fast_uint to,
    bool undo
) {

    if (undo)
    {   

        transition_matrix[to * model->nstates + from]--;
        transition_matrix[from * model->nstates + from]++;

    } else {

        transition_matrix[to * model->nstates + from]++;
        transition_matrix[from * model->nstates + from]--;

    }

    #ifdef EPI_DEBUG
    if (transition_matrix[from * model->nstates + from] < 0)
        throw std::logic_error("An entry in transition matrix is negative.");
    #endif

}

inline void DataBase::split_state(
    epiworld_fast_uint state,
    std::string & status,
    std::string & stage
) const
{

    if (!model->nh)
    {
        status = "Free";
        stage = model->states_labels[state];
        return;
    }

    const unsigned int c = static_cast< unsigned int >(state);
    stage = model->nh->get_stages()[static_cast< size_t >(model->nh_stage_of(c))];
    status = model->st ?
        model->st->get_stages()[static_cast< size_t >(model->nh_status_of(c))] :
        std::string("Free");

}

inline void DataBase::split_states(
    const std::vector< epiworld_fast_uint > & states,
    std::vector< std::string > * status,
    std::vector< std::string > * stage
) const
{

    std::string k, s;
    if (status != nullptr)
        status->resize(states.size());
    if (stage != nullptr)
        stage->resize(states.size());

    for (size_t i = 0u; i < states.size(); ++i)
    {
        split_state(states[i], k, s);
        if (status != nullptr)
            (*status)[i] = k;
        if (stage != nullptr)
            (*stage)[i] = s;
    }

}

inline int DataBase::today_total_of(
    const std::string & name,
    bool by_stage
) const
{

    bool found = false;
    int total = 0;
    std::string k, s;
    for (epiworld_fast_uint i = 0u; i < today_total.size(); ++i)
    {
        split_state(i, k, s);
        if ((by_stage ? s : k) == name)
        {
            found = true;
            total += today_total[i];
        }
    }

    if (!found)
        throw std::range_error(
            "'" + name + "' is not a " + (by_stage ? "stage" : "status") +
            " of the model."
        );

    return total;

}

inline int DataBase::get_today_total_stage(
    const std::string & stage
) const
{
    return today_total_of(stage, true);
}

inline int DataBase::get_today_total_status(
    const std::string & status
) const
{
    return today_total_of(status, false);
}

inline void DataBase::get_today_total(
    std::vector< std::string > * status,
    std::vector< std::string > * stage,
    std::vector< int > * counts
) const
{

    std::vector< epiworld_fast_uint > states(today_total.size());
    for (size_t i = 0u; i < states.size(); ++i)
        states[i] = static_cast< epiworld_fast_uint >(i);

    split_states(states, status, stage);

    if (counts != nullptr)
        *counts = today_total;

}

inline void DataBase::get_today_virus(
    std::vector< std::string > & status,
    std::vector< std::string > & stage,
    std::vector< int > & id,
    std::vector< int > & counts
    ) const
{

    // One row per virus and state
    const size_t n_viruses = today_virus.size();
    const size_t n_states = model->states_labels.size();

    std::vector< epiworld_fast_uint > states;
    states.reserve(n_viruses * n_states);
    id.clear();
    counts.clear();

    for (epiworld_fast_uint v = 0u; v < n_viruses; ++v)
        for (epiworld_fast_uint s = 0u; s < n_states; ++s)
        {
            states.push_back(s);
            id.push_back(static_cast<int>(v));
            counts.push_back(today_virus[v][s]);
        }

    split_states(states, &status, &stage);

}

inline void DataBase::get_hist_total(
    std::vector< int > * date,
    std::vector< std::string > * status,
    std::vector< std::string > * stage,
    std::vector< int > * counts
) const
{

    if (date != nullptr)
        *date = hist_total_date;

    split_states(hist_total_state, status, stage);

    if (counts != nullptr)
        *counts = hist_total_counts;

}

inline void DataBase::get_hist_virus(
    std::vector< int > & date,
    std::vector< int > & id,
    std::vector< std::string > & status,
    std::vector< std::string > & stage,
    std::vector< int > & counts
) const {

    date = hist_virus_date;
    id = hist_virus_id;
    split_states(hist_virus_state, &status, &stage);
    counts = hist_virus_counts;

}

inline void DataBase::get_hist_tool(
    std::vector< int > & date,
    std::vector< int > & id,
    std::vector< std::string > & status,
    std::vector< std::string > & stage,
    std::vector< int > & counts
) const {

    date = hist_tool_date;
    id = hist_tool_id;
    split_states(hist_tool_state, &status, &stage);
    counts = hist_tool_counts;

}

inline void DataBase::get_today_transition_matrix(
    std::vector< int > & counts
) const
{

    counts = transition_matrix;

    return;

}

inline void DataBase::get_hist_transition_matrix(
    std::vector< std::string > & status_from,
    std::vector< std::string > & stage_from,
    std::vector< std::string > & status_to,
    std::vector< std::string > & stage_to,
    std::vector< int > & date,
    std::vector< int > & counts,
    bool skip_zeros
) const
{

    size_t n = this->hist_transition_matrix.size();

    std::vector< epiworld_fast_uint > from, to;
    from.reserve(n);
    to.reserve(n);
    date.clear();
    counts.clear();
    date.reserve(n);
    counts.reserve(n);

    size_t n_states = model->nstates;
    size_t n_steps  = model->get_ndays();

    // If n is zero, then we are done
    if (n == 0u)
    {
        split_states(from, &status_from, &stage_from);
        split_states(to, &status_to, &stage_to);
        return;
    }

    for (size_t step = 0u; step <= n_steps; ++step) // The final step counts
    {
        for (size_t j = 0u; j < n_states; ++j) // Column major storage
        {
            for (size_t i = 0u; i < n_states; ++i)
            {
                // Retrieving the value of the day
                int v = hist_transition_matrix[
                    step * n_states * n_states + // Day of the data
                    j * n_states +               // Column (to)
                    i                            // Row (from)
                    ];

                // If we are skipping the zeros and it is zero, then don't save
                if (skip_zeros && v == 0)
                    continue;
                                
                from.push_back(static_cast< epiworld_fast_uint >(i));
                to.push_back(static_cast< epiworld_fast_uint >(j));
                date.push_back(hist_total_date[step * n_states]);
                counts.push_back(v);

            }

        }
    }

    split_states(from, &status_from, &stage_from);
    split_states(to, &status_to, &stage_to);


}


inline void DataBase::get_active_cases(
    std::vector<int> & date,
    std::vector<int> & virus_id,
    std::vector<int> & count
) const 
{

    // Extracting useful sizes
    size_t n_days    = model->get_ndays() + 1u; // Including day 0
    size_t n_viruses = model->get_n_viruses();
    
    // Making room
    date.assign(n_days * n_viruses, 0);
    virus_id.assign(n_days * n_viruses, 0);
    count.assign(n_days * n_viruses, 0);

    for (size_t i = 0u; i < hist_virus_id.size(); ++i)
    {

        // With more viruses, there are more days
        auto location = hist_virus_date[i] + hist_virus_id[i] * n_days;

        date[location] = hist_virus_date[i];
        virus_id[location] = hist_virus_id[i];
        count[location] += hist_virus_counts[i];

    }

    return;
    
}

inline void DataBase::get_outbreak_size(
    std::vector<int> & date,
    std::vector<int> & virus_id,
    std::vector<int> & outbreak_size
) const 
{

    // Extracting useful sizes
    size_t n_days    = model->get_ndays() + 1u; // Including day 0
    size_t n_viruses = model->get_n_viruses();
    
    // Making room
    date.assign(n_days * n_viruses, 0);
    std::iota(date.begin(), date.end(), 0);

    virus_id.assign(n_days * n_viruses, 0);
    for (size_t v = 0u; v < n_viruses; ++v)
        for (size_t d = 0u; d < n_days; ++d)
            virus_id[d + v * n_days] = static_cast<int>(v);

    outbreak_size.assign(n_days * n_viruses, 0);

    // With more viruses, there are more days;
    for (size_t i = 0u; i < transmission_date.size(); ++i)
    {
        outbreak_size[
            transmission_date[i] + transmission_virus[i] * n_days
        ] += 1;
    }

    // Now, we generate the cumulative sum
    for (size_t v = 0u; v < n_viruses; ++v)
    {
        for (size_t d = 1u; d < n_days; ++d)
        {
            auto location = d + v * n_days;
            outbreak_size[location] += outbreak_size[location - 1u];
        }
    }

    return;
    
}

inline void DataBase::get_transmissions(
    std::vector<int> & date,
    std::vector<int> & source,
    std::vector<int> & target,
    std::vector<int> & virus,
    std::vector<int> & source_exposure_date,
    std::vector<int> & layer
) const 
{

    size_t nevents = transmission_date.size();

    date.resize(nevents);
    source.resize(nevents);
    target.resize(nevents);
    virus.resize(nevents);
    source_exposure_date.resize(nevents);
    layer.resize(nevents);

    get_transmissions(
        date.data(),
        source.data(),
        target.data(),
        virus.data(),
        source_exposure_date.data(),
        layer.data()
    );

}

inline void DataBase::get_transmissions(
    int * date,
    int * source,
    int * target,
    int * virus,
    int * source_exposure_date,
    int * layer
) const 
{

    size_t nevents = transmission_date.size();

    for (size_t i = 0u; i < nevents; ++i)
    {

        *(date + i) = transmission_date.at(i);
        *(source + i) = transmission_source.at(i);
        *(target + i) = transmission_target.at(i);
        *(virus + i) = transmission_virus.at(i);
        *(source_exposure_date + i) = transmission_source_exposure_date.at(i);
        *(layer + i) = transmission_layer.at(i);

    }

}

inline RunOutputs DataBase::get_run_outputs(
    const std::vector< std::string > & whats
) const
{

    // Names of the viruses, given their ids
    auto vnames = [this](const std::vector< int > & ids) {
        std::vector< std::string > names;
        names.reserve(ids.size());
        for (auto i : ids)
            names.push_back(virus_name[i]);
        return names;
    };

    // Names of the layers, given their ids ("" for -1: not from a contact)
    auto lnames = [this](const std::vector< int > & ids) {
        std::vector< std::string > names;
        names.reserve(ids.size());
        for (auto i : ids)
            names.push_back(
                i < 0 ? std::string() :
                model->get_layer(static_cast< size_t >(i)).get_name()
            );
        return names;
    };

    // The status and stage columns of a vector of states
    auto add_pairs = [this](
        OutputTable & t,
        const std::vector< epiworld_fast_uint > & states,
        const std::string & prefix
    ) {
        std::vector< std::string > status, stage;
        split_states(states, &status, &stage);
        t.add(prefix + "status", status);
        t.add(prefix + "stage", stage);
    };

    RunOutputs out;

    for (const auto & what : whats)
    {

        OutputTable t;

        if (what == "virus_info")
        {
            std::vector< int > id(virus_name.size());
            std::iota(id.begin(), id.end(), 0);
            t.add("virus_id", id);
            t.add("virus", virus_name);
            t.add("date_recorded", virus_origin_date);
            t.add("parent", virus_parent_id);
        }
        else if (what == "virus_hist")
        {
            t.add("date", hist_virus_date);
            t.add("virus_id", hist_virus_id);
            t.add("virus", vnames(hist_virus_id));
            add_pairs(t, hist_virus_state, "");
            t.add("n", hist_virus_counts);
        }
        else if (what == "tool_info")
        {
            std::vector< int > id(tool_name.size());
            std::iota(id.begin(), id.end(), 0);
            t.add("id", id);
            t.add("tool_name", tool_name);
            t.add("date_recorded", tool_origin_date);
        }
        else if (what == "tool_hist")
        {
            t.add("date", hist_tool_date);
            t.add("id", hist_tool_id);
            add_pairs(t, hist_tool_state, "");
            t.add("n", hist_tool_counts);
        }
        else if (what == "total_hist")
        {
            t.add("date", hist_total_date);
            t.add("nviruses", hist_total_nviruses_active);
            add_pairs(t, hist_total_state, "");
            t.add("counts", hist_total_counts);
        }
        else if (what == "transmission")
        {
            t.add("date", transmission_date);
            t.add("virus_id", transmission_virus);
            t.add("virus", vnames(transmission_virus));
            t.add("source_exposure_date", transmission_source_exposure_date);
            t.add("source", transmission_source);
            t.add("target", transmission_target);
            t.add("layer_id", transmission_layer);
            t.add("layer", lnames(transmission_layer));
        }
        else if (what == "transition")
        {
            std::vector< int > date, counts;
            std::vector< epiworld_fast_uint > from, to;
            int ns = model->nstates;
            for (int i = 0; i <= model->today(); ++i)
                for (int f = 0; f < ns; ++f)
                    for (int s = 0; s < ns; ++s)
                    {
                        int n = hist_transition_matrix[i * (ns * ns) + s * ns + f];
                        if (n == 0)
                            continue;
                        date.push_back(i);
                        from.push_back(static_cast< epiworld_fast_uint >(f));
                        to.push_back(static_cast< epiworld_fast_uint >(s));
                        counts.push_back(n);
                    }
            t.add("date", date);
            add_pairs(t, from, "from_");
            add_pairs(t, to, "to_");
            t.add("counts", counts);
        }
        else if (what == "reproductive")
        {
            std::vector< int > id, source, date, rt;
            for (const auto & m : get_reproductive_number())
            {
                id.push_back(m.first[0u]);
                source.push_back(m.first[1u]);
                date.push_back(m.first[2u]);
                rt.push_back(m.second);
            }
            t.add("virus_id", id);
            t.add("virus", vnames(id));
            t.add("source", source);
            t.add("source_exposure_date", date);
            t.add("rt", rt);
        }
        else if (what == "generation")
        {
            std::vector< int > agent, virus, time, gentime;
            get_generation_time(agent, virus, time, gentime);
            t.add("virus", virus);
            t.add("source", agent);
            t.add("source_exposure_date", time);
            t.add("gentime", gentime);
        }
        else if (what == "active_cases" || what == "outbreak_size")
        {
            std::vector< int > date, id, n;
            if (what == "active_cases")
                get_active_cases(date, id, n);
            else
                get_outbreak_size(date, id, n);

            // Skipping the zeros
            std::vector< int > date_, id_, n_;
            for (size_t i = 0u; i < n.size(); ++i)
                if (n[i] > 0)
                {
                    date_.push_back(date[i]);
                    id_.push_back(id[i]);
                    n_.push_back(n[i]);
                }

            t.add("date", date_);
            t.add("virus_id", id_);
            t.add("virus", vnames(id_));
            t.add(what, n_);
        }
        else if (what == "hospitalizations")
        {
            std::vector< int > date, id, tool, count;
            std::vector< double > weight;
            get_hospitalizations(date, id, tool, count, weight);

            // Skipping the zeros
            std::vector< int > date_, id_, tool_, count_;
            std::vector< double > weight_;
            for (size_t i = 0u; i < date.size(); ++i)
                if (count[i] > 0 || weight[i] > 0.0)
                {
                    date_.push_back(date[i]);
                    id_.push_back(id[i]);
                    tool_.push_back(tool[i]);
                    count_.push_back(count[i]);
                    weight_.push_back(weight[i]);
                }

            t.add("date", date_);
            t.add("virus_id", id_);
            t.add("tool_id", tool_);
            t.add("count", count_);
            t.add("weight", weight_);
        }
        else
        {
            std::string valid;
            for (const auto & n : run_output_names())
                valid += " \"" + n + "\"";
            throw std::invalid_argument(
                "Unknown output \"" + what + "\". Valid outputs are:" + valid + "."
            );
        }

        out[what] = std::move(t);

    }

    return out;

}

inline void DataBase::write_data(
    std::string fn_virus_info,
    std::string fn_virus_hist,
    std::string fn_tool_info,
    std::string fn_tool_hist,
    std::string fn_total_hist,
    std::string fn_transmission,
    std::string fn_transition,
    std::string fn_reproductive_number,
    std::string fn_generation_time,
    std::string fn_active_cases,
    std::string fn_outbreak_size,
    std::string fn_hospitalizations
) const
{

    // Same order as run_output_names()
    const std::vector< std::string > fns = {
        fn_virus_info, fn_virus_hist, fn_tool_info, fn_tool_hist,
        fn_total_hist, fn_transmission, fn_transition,
        fn_reproductive_number, fn_generation_time, fn_active_cases,
        fn_outbreak_size, fn_hospitalizations
    };

    std::vector< std::string > whats;
    for (size_t i = 0u; i < fns.size(); ++i)
        if (fns[i] != "")
            whats.push_back(run_output_names()[i]);

    auto out = get_run_outputs(whats);

    for (size_t i = 0u; i < fns.size(); ++i)
        if (fns[i] != "")
            write_table(fns[i], out.at(run_output_names()[i]));

}

inline void DataBase::record_transmission(
    int i,
    int j,
    int virus,
    int i_expo_date,
    int layer
) {

    transmission_date.push_back(model->today());
    transmission_source.push_back(i);
    transmission_target.push_back(j);
    transmission_virus.push_back(virus);
    transmission_source_exposure_date.push_back(i_expo_date);
    transmission_layer.push_back(layer);

}

inline size_t DataBase::get_n_viruses() const
{
    return virus_name.size();
}

inline size_t DataBase::get_n_tools() const
{
    return tool_name.size();
}


inline void DataBase::set_user_data(
    std::vector< std::string > names
)
{
    user_data = UserData(names);
    user_data.model = model;
}

inline void DataBase::add_user_data(
    std::vector< epiworld_double > x
)
{

    user_data.add(x);

}

inline void DataBase::add_user_data(
    epiworld_fast_uint k,
    epiworld_double x
)
{

    user_data.add(k, x);

}

inline UserData & DataBase::get_user_data()
{
    return user_data;
}

inline MapVec_type<int,int> DataBase::get_reproductive_number()
const {

    // Checking size
    MapVec_type<int,int> map;

    // Number of digits of maxid
    for (size_t i = 0u; i < transmission_date.size(); ++i)
    {
        // Fabricating id
        std::vector< int > h = {
            transmission_virus[i],
            transmission_source[i],
            transmission_source_exposure_date[i]
        };

        // Adding to counter
        if (map.find(h) == map.end())
            map[h] = 1;
        else
            map[h]++;

        // The target is added
        std::vector< int > h_target = {
            transmission_virus[i],
            transmission_target[i],
            transmission_date[i]
        };

        map[h_target] = 0;
        
    }

    return map;

}

inline void DataBase::get_reproductive_number(
    std::string fn
) const {

    write_table(fn, get_run_outputs({"reproductive"}).at("reproductive"));

}

inline std::vector< epiworld_double > DataBase::get_transition_probability(
    bool print,
    bool normalize
) const {

    const auto& states_labels = model->states_labels;
    size_t n_state = states_labels.size();
    size_t n_days   = model->get_ndays();
    std::vector< epiworld_double > res(n_state * n_state, 0.0);
    std::vector< epiworld_double > rowsums(n_state, 0.0);

    for (size_t t = 0; t < n_days; ++t)
    {

        for (size_t s_i = 0; s_i < n_state; ++s_i)
        {

            for (size_t s_j = 0u; s_j < n_state; ++s_j)
            {
                res[s_i + s_j * n_state] += (
                    hist_transition_matrix[
                        s_i + s_j * n_state +
                        t * (n_state * n_state)
                    ]
                );
                
                rowsums[s_i] += hist_transition_matrix[
                    s_i + s_j * n_state +
                    t * (n_state * n_state)
                ];
            
            }

        }

    }

    if (normalize)
    {
        for (size_t s_i = 0; s_i < n_state; ++s_i)
        {
            // Nothing to normalize if the row is zero
            if (rowsums[s_i] == 0)
                continue;

            for (size_t s_j = 0; s_j < n_state; ++s_j)
                res[s_i + s_j * n_state] /= rowsums[s_i];
        }
    }

    if (print)
    {   

        size_t nchar = 0u;
        for (auto & l : states_labels)
            if (l.length() > nchar)
                nchar = l.length();

        std::string fmt = " - %-" + std::to_string(nchar) + "s";

        std::string fmt_entry = " % 4.2f";
        if (!normalize)
        {
            nchar = 0u;
            for (auto & l: res)
            {
                std::string tmp = std::to_string(l);
                if (tmp.length() > nchar)
                    nchar = tmp.length();
            }

            fmt_entry = " % " + std::to_string(nchar) + ".0f";
        } 

        
        printf_epiworld("\nTransition Probabilities:\n");
        for (size_t s_i = 0u; s_i < n_state; ++s_i)
        {
            printf_epiworld(fmt.c_str(), states_labels[s_i].c_str());
            for (size_t s_j = 0u; s_j < n_state; ++s_j)
            {
                if (
                    std::isnan(res[s_i + s_j * n_state]) ||
                    (res[s_i + s_j * n_state] < 1e-10)
                )
                {
                    printf_epiworld("     -");
                } else {
                    printf_epiworld(
                        fmt_entry.c_str(), res[s_i + s_j * n_state]
                    );
                }
            }
            printf_epiworld("\n");
        }

        printf_epiworld("\n");

    }

    return res;


} 

#define VECT_MATCH(a, b, c) \
    EPI_DEBUG_FAIL_AT_TRUE(a.size() != b.size(), c) \
    for (size_t __i = 0u; __i < a.size(); ++__i) \
    {\
        EPI_DEBUG_FAIL_AT_TRUE(a[__i] != b[__i], c) \
    }

inline bool DataBase::operator==(const DataBase & other) const
{
    VECT_MATCH(
        virus_name,
        other.virus_name,
        "DataBase:: virus_name[i] don't match"
    )

    VECT_MATCH(
        virus_origin_date,
        other.virus_origin_date,
        "DataBase:: virus_origin_date[i] don't match"
    )

    VECT_MATCH(
        virus_parent_id,
        other.virus_parent_id,
        "DataBase:: virus_parent_id[i] don't match"
    )

    VECT_MATCH(
        tool_name,
        other.tool_name,
        "DataBase:: tool_name[i] don't match"
    )

    VECT_MATCH(
        tool_origin_date,
        other.tool_origin_date,
        "DataBase:: tool_origin_date[i] don't match"
    )

    
    EPI_DEBUG_FAIL_AT_TRUE(
        sampling_freq != other.sampling_freq,
        "DataBase:: sampling_freq don't match."
    )

    // Variants history
    VECT_MATCH(
        hist_virus_date,
        other.hist_virus_date,
        "DataBase:: hist_virus_date[i] don't match"
    )

    VECT_MATCH(
        hist_virus_id,
        other.hist_virus_id,
        "DataBase:: hist_virus_id[i] don't match"
    )

    VECT_MATCH(
        hist_virus_state,
        other.hist_virus_state,
        "DataBase:: hist_virus_state[i] don't match"
    )

    VECT_MATCH(
        hist_virus_counts,
        other.hist_virus_counts,
        "DataBase:: hist_virus_counts[i] don't match"
    )

    // Tools history
    VECT_MATCH(
        hist_tool_date,
        other.hist_tool_date,
        "DataBase:: hist_tool_date[i] don't match"
    )

    VECT_MATCH(
        hist_tool_id,
        other.hist_tool_id,
        "DataBase:: hist_tool_id[i] don't match"
    )

    VECT_MATCH(
        hist_tool_state,
        other.hist_tool_state,
        "DataBase:: hist_tool_state[i] don't match"
    )

    VECT_MATCH(
        hist_tool_counts,
        other.hist_tool_counts,
        "DataBase:: hist_tool_counts[i] don't match"
    )

    // Overall hist
    VECT_MATCH(
        hist_total_date,
        other.hist_total_date,
        "DataBase:: hist_total_date[i] don't match"
    )

    VECT_MATCH(
        hist_total_nviruses_active,
        other.hist_total_nviruses_active,
        "DataBase:: hist_total_nviruses_active[i] don't match"
    )

    VECT_MATCH(
        hist_total_state,
        other.hist_total_state,
        "DataBase:: hist_total_state[i] don't match"
    )

    VECT_MATCH(
        hist_total_counts,
        other.hist_total_counts,
        "DataBase:: hist_total_counts[i] don't match"
    )

    VECT_MATCH(
        hist_transition_matrix,
        other.hist_transition_matrix,
        "DataBase:: hist_transition_matrix[i] don't match"
    )

    // {Variant 1: {state 1, state 2, etc.}, Variant 2: {...}, ...}
    EPI_DEBUG_FAIL_AT_TRUE(
        today_virus.size() != other.today_virus.size(),
        "DataBase:: today_virus.size() don't match."
    )
    
    for (size_t i = 0u; i < today_virus.size(); ++i)
    {
        VECT_MATCH(
            today_virus[i], other.today_virus[i],
            "DataBase:: today_virus[i] don't match"
            )
    }

    // {Variant 1: {state 1, state 2, etc.}, Variant 2: {...}, ...}
    EPI_DEBUG_FAIL_AT_TRUE(
        today_tool.size() != other.today_tool.size(),
        "DataBase:: today_tool.size() don't match."
    )
    
    for (size_t i = 0u; i < today_tool.size(); ++i)
    {
        VECT_MATCH(
            today_tool[i], other.today_tool[i],
            "DataBase:: today_tool[i] don't match"
            )
    }

    // {Susceptible, Infected, etc.}
    VECT_MATCH(
        today_total, other.today_total,
        "DataBase:: today_total[i] don't match"
        )

    // Totals
    EPI_DEBUG_FAIL_AT_TRUE(
        today_total_nviruses_active != other.today_total_nviruses_active,
        "DataBase:: today_total_nviruses_active don't match."
    )

    // Transmission network
    VECT_MATCH( ///< Date of the transmission eve
        transmission_date,
        other.transmission_date,
        "DataBase:: transmission_date[i] don't match"
    )

    VECT_MATCH( ///< Id of the sour
        transmission_source,
        other.transmission_source,
        "DataBase:: transmission_source[i] don't match"
    )

    VECT_MATCH( ///< Id of the targ
        transmission_target,
        other.transmission_target,
        "DataBase:: transmission_target[i] don't match"
    )

    VECT_MATCH( ///< Id of the varia
        transmission_virus,
        other.transmission_virus,
        "DataBase:: transmission_virus[i] don't match"
    )

    VECT_MATCH( ///< Date when the source acquired the varia
        transmission_source_exposure_date,
        other.transmission_source_exposure_date,
        "DataBase:: transmission_source_exposure_date[i] don't match"
    )

    VECT_MATCH(
        transmission_layer,
        other.transmission_layer,
        "DataBase:: transmission_layer[i] don't match"
    )

    VECT_MATCH(
        transition_matrix,
        other.transition_matrix,
        "DataBase:: transition_matrix[i] don't match"
    )

    return true;

}

inline void DataBase::get_generation_time(
    std::vector< int > & agent_id,
    std::vector< int > & virus_id,
    std::vector< int > & time,
    std::vector< int > & gentime
) const {
    
    size_t nevents = transmission_date.size();

    agent_id.reserve(nevents);
    virus_id.reserve(nevents);
    time.reserve(nevents);
    gentime.reserve(nevents);

    // Iterating through the individuals
    for (size_t i = 0u; i < nevents; ++i)
    {
        int agent_id_i = transmission_target[i];
        agent_id.push_back(agent_id_i);
        virus_id.push_back(transmission_virus[i]);
        time.push_back(transmission_date[i]);

        bool found = false;
        for (size_t j = i; j < nevents; ++j)
        {

            if (transmission_source[j] == agent_id_i)
            {
                gentime.push_back(transmission_date[j] - time[i]);
                found = true;
                break;
            }

        }

        // If there's no transmission, we set the generation time to
        // minus 1;
        if (!found)
            gentime.push_back(-1);

    }

    agent_id.shrink_to_fit();
    virus_id.shrink_to_fit();
    time.shrink_to_fit();
    gentime.shrink_to_fit();

    return;

}

inline void DataBase::get_generation_time(
    std::string fn
) const
{

    write_table(fn, get_run_outputs({"generation"}).at("generation"));

}

inline void DataBase::record_hospitalization(Agent & agent)
{
    m_hospitalizations.record(agent, *model);
}

inline void DataBase::get_hospitalizations(
    std::vector<int> & date,
    std::vector<int> & virus_id,
    std::vector<int> & tool_id,
    std::vector<int> & count,
    std::vector<double> & weight
) const
{
    // Get the number of days from the model and add 1 since days are 0-indexed
    // today() returns the last day, so we need today() + 1 for the full count
    int ndays = model->today() + 1;
    m_hospitalizations.get(ndays, date, virus_id, tool_id, count, weight);
}

#undef VECT_MATCH

inline void DataBase::get_hist_transitions(
    std::vector< int > & date,
    std::vector< std::string > & transition,
    std::vector< int > & counts
) const
{

    if (!model->nh)
        throw std::logic_error(
            "get_hist_transitions() needs a model with a natural history."
        );

    date = hist_transition_date;
    counts = hist_transition_counts;
    // Stage transitions first, then the status ones
    const int n_stage = static_cast< int >(model->nh->get_n_transitions());
    transition.resize(hist_transition_id.size());
    for (size_t k = 0u; k < hist_transition_id.size(); ++k)
    {
        const int t = hist_transition_id[k];
        transition[k] = (t < n_stage) ?
            model->nh->get_transition(t).name :
            model->st->get_transition(t - n_stage).name;
    }

}

inline void SaverMemory::operator()(size_t sim_id, Model * model) const
{
    auto out = model->get_db().get_run_outputs(state->whats);
    if (sim_id >= state->runs.size())
        state->runs.resize(sim_id + 1u);
    state->runs[sim_id] = std::move(out);
}

#endif
/*//////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

 End of -./include/epiworld/database-meat.hpp-

////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////*/


/*//////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

 Start of -./include/epiworld/adjlist-meat.hpp-

////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////*/


#ifndef EPIWORLD_ADJLIST_MEAT_HPP
#define EPIWORLD_ADJLIST_MEAT_HPP

#include <vector>
#include <algorithm>
#include <numeric>
#include <string>
#include <stdexcept>
#include <fstream>
// (already included include/epiworld/config.hpp)
// (already included include/epiworld/adjlist-bones.hpp)

inline AdjList::AdjList(
    const std::vector< int > & source,
    const std::vector< int > & target,
    int size,
    bool directed
) : directed(directed) {

    if (size < 0)
        throw std::length_error(
            "The size of the network cannot be negative (" +
            std::to_string(size) + ")."
            );

    if (source.size() != target.size())
        throw std::length_error(
            "source and target must have the same length (" +
            std::to_string(source.size()) + " vs " +
            std::to_string(target.size()) + ")."
            );

    N = size;
    E = source.size();
    int max_id = size - 1;

    // Counting sort of the tie ends by vertex: first the size of each row (an
    // undirected tie counts in both), then the rows themselves
    start.assign(N + 1u, 0u);
    for (size_t m = 0u; m < source.size(); ++m)
    {

        int i = source[m];
        int j = target[m];

        if ((i < 0) || (i > max_id))
            throw std::range_error(
                "The source["+std::to_string(m)+"] = " + std::to_string(i) +
                " is above the max_id " + std::to_string(max_id)
                );

        if ((j < 0) || (j > max_id))
            throw std::range_error(
                "The target["+std::to_string(m)+"] = " + std::to_string(j) +
                " is above the max_id " + std::to_string(max_id)
                );

        start[i + 1]++;
        if (!directed)
            start[j + 1]++;

    }

    std::partial_sum(start.begin(), start.end(), start.begin());

    ids.resize(start[N]);
    std::vector< size_t > next(start.begin(), start.end() - 1);
    for (size_t m = 0u; m < source.size(); ++m)
    {
        ids[next[source[m]]++] = target[m];
        if (!directed)
            ids[next[target[m]]++] = source[m];
    }

    // Each row sorted and without repeats, moved down over the repeats
    // dropped before it
    size_t kept = 0u;
    for (size_t i = 0u; i < N; ++i)
    {

        auto first = ids.begin() + static_cast< std::ptrdiff_t >(start[i]);
        auto last  = ids.begin() + static_cast< std::ptrdiff_t >(start[i + 1u]);

        std::sort(first, last);
        last = std::unique(first, last);

        start[i] = kept;
        for (auto it = first; it != last; ++it)
            ids[kept++] = *it;

    }

    start[N] = kept;
    ids.resize(kept);

    return;

}

inline void AdjList::read_edgelist(
    std::string fn,
    int size,
    int skip,
    bool directed
) {

    int i,j;
    std::ifstream filei(fn);

    if (!filei)
        throw std::logic_error("The file " + fn + " was not found.");

    int linenum = 0;
    std::vector< int > source_;
    std::vector< int > target_;

    source_.reserve(1e5);
    target_.reserve(1e5);

    int max_id = size - 1;

    while (!filei.eof())
    {

        if (linenum++ < skip)
            continue;

        filei >> i >> j;

        // Looking for exceptions
        if (filei.bad())
            throw std::logic_error(
                "I/O error while reading the file " +
                fn
            );

        if (filei.fail())
            break;

        if (i > max_id)
            throw std::range_error(
                "The source["+std::to_string(linenum)+"] = " + std::to_string(i) +
                " is above the max_id " + std::to_string(max_id)
                );

        if (j > max_id)
            throw std::range_error(
                "The target["+std::to_string(linenum)+"] = " + std::to_string(j) +
                " is above the max_id " + std::to_string(max_id)
                );

        source_.push_back(i);
        target_.push_back(j);

    }

    // Now using the right constructor
    *this = AdjList(source_, target_, size, directed);

    return;

}

inline std::vector< int > AdjList::operator()(
    epiworld_fast_uint i
    ) const {

    if (i >= N)
        throw std::range_error(
            "The vertex id " + std::to_string(i) + " is not in the network."
            );

    return std::vector< int >(
        ids.begin() + static_cast< std::ptrdiff_t >(start[i]),
        ids.begin() + static_cast< std::ptrdiff_t >(start[i + 1u])
    );

}

inline void AdjList::print(epiworld_fast_uint limit) const {

    printf_epiworld("Nodeset:\n");
    for (epiworld_fast_uint i = 0u; (i < N) && (i <= limit); ++i)
    {

        printf_epiworld("  % 3i: {", static_cast<int>(i));
        for (size_t k = start[i]; k < start[i + 1u]; ++k)
        {
            printf_epiworld("%s%i", (k > start[i]) ? ", " : "", ids[k]);
        }
        printf_epiworld("}\n");

    }

    if (limit < N)
    {
        printf_epiworld(
            "  (... skipping %i records ...)\n",
            static_cast<int>(N - limit)
            );
    }

}

inline size_t AdjList::vcount() const 
{
    return N;
}

inline size_t AdjList::ecount() const 
{
    return E;
}

inline bool AdjList::is_directed() const {

    if (N == 0u)
        throw std::logic_error("The edgelist is empty.");
    
    return directed;
    
}

#endif
/*//////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

 End of -./include/epiworld/adjlist-meat.hpp-

////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////*/


/*//////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

 Start of -./include/epiworld/contacttracing-meat.hpp-

////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////*/


#ifndef EPIWORLD_CONTACTTRACING_MEAT_H
#define EPIWORLD_CONTACTTRACING_MEAT_H

// (already included include/epiworld/contacttracing-bones.hpp)

inline size_t ContactTracing::get_location(size_t row, size_t col) const
{
    return col * n_agents + row;
}

inline ContactTracing::ContactTracing()
{
    n_agents = 0u;
    max_contacts = 0u;
}

inline ContactTracing::ContactTracing(size_t n_agents, size_t max_contacts)
{
    this->n_agents = n_agents;
    this->max_contacts = max_contacts;

    contact_matrix.resize(n_agents * max_contacts, 0u);
    contacts_per_agent.resize(n_agents, 0);
    contact_date.resize(n_agents * max_contacts, 0);

    cached_contacts.resize(n_agents);
    up_to_date.assign(n_agents, false);
}

inline void ContactTracing::add_contact(size_t agent_a, size_t agent_b, size_t day)
{
    
    // Checking overflow
    size_t col_location = contacts_per_agent[agent_a] % max_contacts;
    size_t array_location = get_location(agent_a, col_location);

    contact_matrix[array_location] = agent_b;
    contact_date[array_location] = day;

    contacts_per_agent[agent_a] += 1;

    // Invalidate the cache for this agent
    up_to_date[agent_a] = false;

}

inline size_t ContactTracing::get_n_contacts(size_t agent)
{
    return contacts_per_agent[agent];
}

inline size_t ContactTracing::get_max_contacts() const
{
    return max_contacts;
}

inline size_t ContactTracing::get_n_agents() const
{
    return n_agents;
}

inline std::pair< size_t, int> ContactTracing::get_contact(size_t agent, size_t idx)
{
    if (
        (idx >= contacts_per_agent[agent]) ||
        (idx >= max_contacts)
    )
        throw std::out_of_range("Index out of range in get_contact");

    size_t col_location = idx % max_contacts;
    size_t array_location = get_location(agent, col_location);

    return { contact_matrix[array_location], contact_date[array_location] };
}

inline void ContactTracing::reset(size_t n_agents, size_t max_contacts)
{

    this->n_agents = n_agents;
    this->max_contacts = max_contacts;

    contact_matrix.assign(n_agents * max_contacts, 0u);
    contacts_per_agent.assign(n_agents, 0u);
    contact_date.assign(n_agents * max_contacts, 0u);

    cached_contacts.assign(n_agents, std::vector<ContactRecord>());
    up_to_date.assign(n_agents, false);
}

inline const std::vector<ContactRecord> & ContactTracing::get_contacts(size_t agent)
{
    if (!up_to_date[agent])
    {
        // Rebuild the cache for this agent by grouping stored contacts by
        // contact id and collecting all recorded days into a std::set<int>.
        std::map<size_t, std::set<int>> contact_map;

        size_t actual_n = contacts_per_agent[agent];
        if (actual_n > max_contacts)
            actual_n = max_contacts;

        for (size_t i = 0u; i < actual_n; ++i)
        {
            size_t array_location = get_location(agent, i);
            contact_map[contact_matrix[array_location]].insert(
                static_cast<int>(contact_date[array_location])
            );
        }

        cached_contacts[agent].clear();
        cached_contacts[agent].reserve(contact_map.size());
        for (const auto & kv : contact_map)
            cached_contacts[agent].emplace_back(kv.first, kv.second);

        up_to_date[agent] = true;
    }

    return cached_contacts[agent];
}

inline void ContactTracing::print(size_t agent)
{

    size_t n_contacts = contacts_per_agent[agent];
    if (n_contacts > max_contacts)
        n_contacts = max_contacts;

    printf_epiworld("Agent %zu has %zu contacts: ", agent, n_contacts);
    for (size_t i = 0u; i < n_contacts; ++i)
    {
        auto [contact_id, contact_day] = get_contact(agent, i);
        printf_epiworld("(%zu, day %i) ", contact_id, contact_day);
    }
    printf_epiworld("\n");

    return;

}

#endif
/*//////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

 End of -./include/epiworld/contacttracing-meat.hpp-

////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////*/


/*//////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

 Start of -./include/epiworld/globalevent-meat.hpp-

////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////*/


#ifndef EPIWORLD_GLOBALEVENT_MEAT_HPP
#define EPIWORLD_GLOBALEVENT_MEAT_HPP

#include <memory>
// (already included include/epiworld/globalevent-bones.hpp)

inline GlobalEvent::GlobalEvent(
    GlobalFun fun,
    std::string name,
    int day
    )
{
    this->fun = fun;
    this->name = name;
    this->day = day;
}

inline void GlobalEvent::operator()(Model * m, int day)
{   
    
    if (this->fun == nullptr)
        return;

    // events apply if day is negative or if day is equal to the day of the action
    if (this->day < 0 || this->day == day)
        this->fun(m);
    
    return;

}

inline void GlobalEvent::reset(Model *)
{
    return;
}

inline void GlobalEvent::set_name(std::string name)
{
    this->name = name;
}

inline std::string GlobalEvent::get_name() const
{
    return this->name;
}

inline void GlobalEvent::set_day(int day)
{
    this->day = day;
}

inline int GlobalEvent::get_day() const
{
    return this->day;
}

inline void GlobalEvent::print() const
{
    printf_epiworld(
        "Global action: %s\n"
        "  - Day: %i\n",
        this->name.c_str(),
        this->day
        );
}

inline bool GlobalEvent::operator==(const GlobalEvent & other) const
{
    return (this->name == other.name) && (this->day == other.day);
}

inline bool GlobalEvent::operator!=(const GlobalEvent & other) const
{
    return !(*this == other);
}

inline std::unique_ptr<GlobalEvent> GlobalEvent::clone_ptr() const
{
    return std::make_unique<GlobalEvent>(*this);
}

#endif
/*//////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

 End of -./include/epiworld/globalevent-meat.hpp-

////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////*/


/*//////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

 Start of -./include/epiworld/model-rand-meat.hpp-

////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////*/


#ifndef EPIWORLD_MODEL_RAND_MEAT_HPP
#define EPIWORLD_MODEL_RAND_MEAT_HPP

// (already included include/epiworld/rng-utils.hpp)
// (already included include/epiworld/model-bones.hpp)

inline void Model::set_rand_gamma(epiworld_double alpha, epiworld_double beta)
{
    rgammad = std::gamma_distribution<>(alpha,beta);
}

inline void Model::set_rand_norm(epiworld_double mean, epiworld_double sd)
{
    rnormd  = std::normal_distribution<>(mean, sd);
}

inline void Model::set_rand_unif(epiworld_double a, epiworld_double b)
{
    runifd_a = a;
    runifd_b = b;
}

inline void Model::set_rand_lognormal(epiworld_double mean, epiworld_double shape)
{
    rlognormald  = std::lognormal_distribution<>(mean, shape);
}

inline void Model::set_rand_exp(epiworld_double lambda)
{
    rexpd  = std::exponential_distribution<>(lambda);
}

inline void Model::set_rand_binom(int n, epiworld_double p)
{
    rbinomd  = std::binomial_distribution<>(n, p);
    rbinomd_n = n;
#ifdef EPI_FAST_BINOM
    rbinomd_fast_lambda = static_cast<epiworld_double>(n) * p;
    // A Poisson needs a positive mean; with n = 0 or p = 0 the binomial
    // gives 0 anyway
    rbinomd_use_poisson =
        (rbinomd_fast_lambda > 0.0) &&
        (p <= static_cast<epiworld_double>(0.01)) &&
        (static_cast<epiworld_double>(n) * p * p <= static_cast<epiworld_double>(0.1));
#else
    rbinomd_fast_lambda = 0.0;
    rbinomd_use_poisson = false;
#endif
}

inline void Model::set_rand_nbinom(int n, epiworld_double p)
{
    rnbinomd  = std::negative_binomial_distribution<>(n, p);
}

inline void Model::set_rand_geom(epiworld_double p)
{
    rgeomd  = std::geometric_distribution<>(p);
}

inline void Model::set_rand_poiss(epiworld_double lambda)
{
    rpoissd  = std::poisson_distribution<>(lambda);
}

inline std::shared_ptr< epi_xoshiro256ss > & Model::get_rand_endgine()
{
    return engine;
}

inline void Model::set_rand_engine(std::shared_ptr< epi_xoshiro256ss > & eng)
{
    engine = eng;
}

inline epiworld_double Model::runif() {
    // CHECK_INIT()
    epiworld_double res = runif_epi(*engine);
    return res * (runifd_b - runifd_a) + runifd_a;
}

inline int Model::runif_int(int a, int b) {
    // CHECK_INIT()
    return a + runif_index(static_cast<uint32_t>(b - a + 1));
}

inline uint32_t Model::runif_index(uint32_t n) {

    // If asking for a 0-range, return 0 to prevent division by zero in the bias check
    if (n == 0) return 0;

    // Grab 32 perfectly uniform random bits directly from xoshiro256ss
    uint32_t x = static_cast<uint32_t>((*engine)());
    
    // Multiply by the bound N to get a 64-bit result
    uint64_t m = static_cast<uint64_t>(x) * static_cast<uint64_t>(n);
    
    // The fractional part (lower 32 bits)
    uint32_t l = static_cast<uint32_t>(m);
    
    // Unbiased rejection sampling for the rare edge case
    if (l < n) {
        uint32_t t = -n % n; // Two's complement trick to get (2^32 - n) % n
        while (l < t) {
            x = static_cast<uint32_t>((*engine)());
            m = static_cast<uint64_t>(x) * static_cast<uint64_t>(n);
            l = static_cast<uint32_t>(m);
        }
    }
    
    // The integer part (upper 32 bits) is our unbiased scaled random index
    return static_cast<uint32_t>(m >> 32);
}

inline epiworld_double Model::runif(epiworld_double a, epiworld_double b) {
    // CHECK_INIT()
    return runif_epi(*engine) * (b - a) + a;
}

inline epiworld_double Model::rnorm() {
    // CHECK_INIT()
    return rnormd(*engine);
}

inline epiworld_double Model::rnorm(epiworld_double mean, epiworld_double sd) {
    // CHECK_INIT()
    return rnormd(*engine) * sd + mean;
}

inline epiworld_double Model::rgamma() {
    return rgammad(*engine);
}

inline epiworld_double Model::rgamma(epiworld_double alpha, epiworld_double beta) {

    return rgammad(
        *engine,
        std::gamma_distribution<>::param_type(alpha, beta)
    );

}

inline epiworld_double Model::rexp() {
    return rexpd(*engine);
}

inline epiworld_double Model::rexp(epiworld_double lambda) {

    return rexpd(
        *engine,
        std::exponential_distribution<>::param_type(lambda)
    );

}

inline epiworld_double Model::rlognormal() {
    return rlognormald(*engine);
}

inline epiworld_double Model::rlognormal(epiworld_double mean, epiworld_double shape) {

    return rlognormald(
        *engine,
        std::lognormal_distribution<>::param_type(mean, shape)
    );
}

inline int Model::rbinom() {
#ifdef EPI_FAST_BINOM
    if (rbinomd_use_poisson)
    {
        int res = rpoiss(rbinomd_fast_lambda);
        return std::min(res, rbinomd_n);
    }
#endif
    return rbinomd(*engine);
}

inline int Model::rbinom(int n, epiworld_double p) {

    if (n == 0 || p == 0.0)
        return 0;

#ifdef EPI_FAST_BINOM
    if (
        (p <= static_cast<epiworld_double>(0.01)) &&
        (static_cast<epiworld_double>(n) * p * p <= static_cast<epiworld_double>(0.1))
    )
    {
        int res = rpoiss(static_cast<epiworld_double>(n) * p);
        return std::min(res, n);
    }
#endif

    return rbinomd(
        *engine,
        std::binomial_distribution<>::param_type(n, p)
    );

}

inline int Model::rnbinom() {
    return rnbinomd(*engine);
}

inline int Model::rnbinom(int n, epiworld_double p) {

    return rnbinomd(
        *engine,
        std::negative_binomial_distribution<>::param_type(n, p)
    );
}

inline int Model::rgeom() {
    return rgeomd(*engine);
}

inline int Model::rgeom(epiworld_double p) {

    return rgeomd(
        *engine,
        std::geometric_distribution<>::param_type(p)
    );

}

inline int Model::rpoiss() {
    return rpoissd(*engine);
}

inline int Model::rpoiss(epiworld_double lambda) {

    return rpoissd(
        *engine,
        std::poisson_distribution<>::param_type(lambda)
    );

}

inline size_t Model::sample_from_probs(size_t n) {

    epiworld_double p_total = runif();
    size_t ans;
    for (ans = 0u; ans < n; ++ans)
    {
        if (p_total < array_double_tmp[ans])
            break;
        array_double_tmp[ans + 1] += array_double_tmp[ans];
    }
    return ans;

}

inline void Model::reserve_tmp(size_t n) {

    if (n <= array_virus_tmp.size())
        return;

    // Doubling keeps the growth amortized
    n = std::max(n, 2u * array_virus_tmp.size());
    array_virus_tmp.resize(n);
    array_layer_tmp.resize(n);
    array_double_tmp.resize(2u * n);

}

inline void Model::seed(size_t s) {

    this->engine->seed(s);

    // Some distributions keep state between calls (e.g., the normal draws
    // made in pairs, also inside the binomial); without a reset, a run would
    // depend on the runs before it, not only on its seed
    rnormd.reset();
    rgammad.reset();
    rlognormald.reset();
    rexpd.reset();
    rbinomd.reset();
    rnbinomd.reset();
    rgeomd.reset();
    rpoissd.reset();

}

/**
 * @brief Conditional Weighted Sampling
 *
 * @details
 * The sampling function will draw one of `{-1, 0,...,probs.size() - 1}` in a
 * weighted fashion. The probabilities are drawn given that either one or none
 * of the cases is drawn; in the latter returns -1.
 *
 * @param probs Vector of probabilities.
 * @param m A `Model`. This is used to draw random uniform numbers.
 * @return int If -1 then it means that none got sampled, otherwise the index
 * of the entry that got drawn.
 */
template<typename TDbl = epiworld_double>
inline int roulette(
    const std::vector< TDbl > & probs,
    Model * m
    )
{

    // Step 1: a probability of 1 is a certain event; if there are any, one of
    // them is drawn uniformly. (This used to test `> 1 - 1e-100`, which is
    // `> 1` in floating point, so p == 1 fell through to 0/0 below and the
    // last entry won regardless.)
    std::vector< int > certain_infection;
    for (epiworld_fast_uint p = 0u; p < probs.size(); ++p)
        if (probs[p] >= 1.0)
            certain_infection.push_back(static_cast<int>(p));

    TDbl r = static_cast<TDbl>(m->runif());
    if (certain_infection.size() > 0)
        return certain_infection[std::floor(r * certain_infection.size())];

    // Step 2: the odds r_p = p / (1 - p) and their sum R. Then
    // P(none) = 1 / (1 + R) and P(p) = r_p / (1 + R), the same as normalizing
    // by prod(1 - p), which underflows to 0 with many entries (0/0 weights).
    std::vector< TDbl > odds(probs.size());
    TDbl odds_total = 0.0;
    for (epiworld_fast_uint p = 0u; p < probs.size(); ++p)
    {
        odds[p] = probs[p] / (1.0 - probs[p]);
        odds_total += odds[p];
    }

    // Step 3: Roulette
    TDbl cumsum = 1.0 / (1.0 + odds_total);
    if (r < cumsum)
        return -1;

    for (epiworld_fast_uint p = 0u; p < probs.size(); ++p)
    {
        // If it yield here, then bingo, the individual will acquire the disease
        cumsum += odds[p] / (1.0 + odds_total);
        if (r < cumsum)
            return static_cast<int>(p);

    }

    #ifdef EPI_DEBUG
    printf_epiworld("[epi-debug] roulette::cumsum = %.4f\n", cumsum);
    #endif

    return static_cast<int>(probs.size() - 1u);

}

inline int roulette(std::vector< double > & probs, Model * m)
{
    return roulette<double>(probs, m);
}

inline int roulette(std::vector< float > & probs, Model * m)
{
    return roulette<float>(probs, m);
}


inline int roulette(
    epiworld_fast_uint nelements,
    Model * m
    )
{

    if ((nelements * 2) > m->array_double_tmp.size())
    {
        throw std::logic_error(
            "Trying to sample from more data than there is in roulette!" +
            std::to_string(nelements) + " vs " +
            std::to_string(m->array_double_tmp.size())
            );
    }

    auto & tmp = m->array_double_tmp;

    // Step 1: certain events (p >= 1; see the vector version), drawn uniformly
    epiworld_fast_uint ncertain = 0u;
    for (epiworld_fast_uint p = 0u; p < nelements; ++p)
        if (tmp[p] >= 1.0)
            tmp[nelements + ncertain++] = p;

    epiworld_double r = m->runif();
    if (ncertain > 0u)
        return tmp[nelements + std::floor(ncertain * r)];

    // Step 2: the odds and their sum, normalized by 1 + sum (see the vector
    // version; prod(1 - p) underflows with many exposures)
    epiworld_double odds_total = 0.0;
    for (epiworld_fast_uint p = 0u; p < nelements; ++p)
    {
        tmp[nelements + p] = tmp[p] / (1.0 - tmp[p]);
        odds_total += tmp[nelements + p];
    }

    // Step 3: Roulette
    epiworld_double cumsum = 1.0 / (1.0 + odds_total);
    if (r < cumsum)
        return -1;

    for (epiworld_fast_uint p = 0u; p < nelements; ++p)
    {
        // If it yield here, then bingo, the individual will acquire the disease
        cumsum += tmp[nelements + p] / (1.0 + odds_total);
        if (r < cumsum)
            return static_cast<int>(p);

    }

    return static_cast<int>(nelements - 1u);

}

#endif
/*//////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

 End of -./include/epiworld/model-rand-meat.hpp-

////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////*/


/*//////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

 Start of -./include/epiworld/model-meat.hpp-

////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////*/


#ifndef EPIWORLD_MODEL_MEAT_HPP
#define EPIWORLD_MODEL_MEAT_HPP

#include <vector>
#include <algorithm>
#include <numeric>
#include <stdexcept>
#include <functional>
#include <memory>
#include <random>
#include <string>
#include <map>
#include <unordered_map>
// (already included include/epiworld/config.hpp)
// (already included include/epiworld/userdata-bones.hpp)
// (already included include/epiworld/adjlist-bones.hpp)
// (already included include/epiworld/model-bones.hpp)
// (already included include/epiworld/virus-bones.hpp)
// (already included include/epiworld/agent-bones.hpp)
// (already included include/epiworld/tool-bones.hpp)
// (already included include/epiworld/rng-utils.hpp)
// (already included include/epiworld/modeldiagram-bones.hpp)

/**
 * @brief Function factory for saving model runs
 *
 * @details This function is the default behavior of the `run_multiple`
 * member of `Model`. By default only the total history (
 * case counts by unit of time.)
 *
 * @param fmt
 * @param total_hist
 * @param virus_info
 * @param virus_hist
 * @param tool_info
 * @param tool_hist
 * @param transmission
 * @param transition
 * @return std::function<void(size_t,Model*)>
 */
inline std::function<void(size_t,Model*)> make_save_run(
    std::string fmt,
    bool total_hist,
    bool virus_info,
    bool virus_hist,
    bool tool_info,
    bool tool_hist,
    bool transmission,
    bool transition,
    bool reproductive,
    bool generation,
    bool active_cases,
    bool outbreak_size,
    bool hospitalizations
    )
{

    // Counting number of %
    int n_fmt = 0;
    for (auto & f : fmt)
        if (f == '%')
            n_fmt++;

    if (n_fmt != 1)
        throw std::logic_error("The -fmt- argument must have only one \"%\" symbol.");

    // Outputs to save, in the order of the arguments (as run_output_names())
    const std::vector< bool > what_to_save = {
        virus_info, virus_hist, tool_info, tool_hist, total_hist,
        transmission, transition, reproductive, generation, active_cases,
        outbreak_size, hospitalizations
    };

    std::vector< std::string > whats;
    for (size_t i = 0u; i < what_to_save.size(); ++i)
        if (what_to_save[i])
            whats.push_back(run_output_names()[i]);

    std::function<void(size_t,Model*)> saver = [fmt,whats](
        size_t niter, Model * m
    ) -> void {

        auto out = m->get_db().get_run_outputs(whats);

        for (const auto & what : whats)
        {
            char buff[1024u];
            snprintf(buff, sizeof(buff), (fmt + "_" + what + ".csv").c_str(), niter);
            write_table(buff, out.at(what));
        }

    };

    return saver;
}


inline void Model::_add_event(
    Agent * agent_,
    VirusPtr virus_,
    ToolPtr tool_,
    epiworld_fast_int new_state_,
    epiworld_fast_int queue_,
    EventAction action_,
    int layer_
) {

    ++nactions;

    #ifdef EPI_DEBUG
    if (nactions == 0)
        throw std::logic_error("Events cannot be zero!!");
    #endif

    if (nactions > events.size())
    {

        events.emplace_back(
            Event(
                agent_, virus_, tool_, new_state_, queue_, action_,
                layer_
            ));
        events.back().transition = nh_pending;

    }
    else
    {

        Event & A = events.at(nactions - 1u);

        A.agent      = std::move(agent_);
        A.virus      = std::move(virus_);
        A.tool       = std::move(tool_);
        A.new_state  = std::move(new_state_);
        A.queue      = std::move(queue_);
        A.action     = std::move(action_);
        A.layer      = layer_;
        A.transition = nh_pending;

    }

    return;

}

inline void Model::state_index_build()
{

    const size_t ns = static_cast< size_t >(nstates);
    const size_t n  = population.size();

    // Counting sort of the agents by state
    state_start.assign(ns + 1u, 0u);
    for (auto & p : population)
        state_start[p.state + 1u]++;

    for (size_t s = 0u; s < ns; ++s)
        state_start[s + 1u] += state_start[s];

    state_order.resize(n);
    state_member_pos.resize(n);
    agent_state.resize(n);
    agent_carrier.assign(n, 0);
    state_degree.assign(ns, 0u);
    state_carriers.assign(ns, 0u);
    state_carrier_degree.assign(ns, 0u);

    std::vector< size_t > next(state_start.begin(), state_start.end() - 1);
    for (auto & p : population)
    {

        const size_t id = static_cast< size_t >(p.id);
        const size_t pos = next[p.state]++;
        state_order[pos] = id;
        state_member_pos[id] = pos;
        agent_state[id] = p.state;

        const size_t deg = network_degree(id);
        state_degree[p.state] += deg;
        if (p.virus != nullptr)
        {
            agent_carrier[id] = 1;
            state_carriers[p.state]++;
            state_carrier_degree[p.state] += deg;
        }

    }

    state_index_ready = true;

    // The push scratch space is sized (and cleared) on first use; a step that
    // was interrupted by an exception must not leave marks for the next run.
    push_slot.clear();
    push_visit.clear();
    push_sources.clear();
    push_targets.clear();

}

inline void Model::state_index_move(
    size_t id,
    unsigned int state_old,
    unsigned int state_new
)
{

    // Swaps the entries at positions `a` and `b` of state_order
    auto swap_pos = [this](size_t a, size_t b) -> void {
        size_t ida = state_order[a];
        size_t idb = state_order[b];
        state_order[a] = idb;
        state_order[b] = ida;
        state_member_pos[idb] = a;
        state_member_pos[ida] = b;
    };

    size_t pos = state_member_pos[id];

    if (state_old < state_new)
    {

        // Move to the end of each block and shift that block's end down, so
        // the agent becomes the first of the next block.
        for (unsigned int s = state_old; s < state_new; ++s)
        {
            size_t last = state_start[s + 1u] - 1u;
            swap_pos(pos, last);
            state_start[s + 1u]--;
            pos = last;
        }

    }
    else
    {

        // Mirror: move to the front of the block and shift its start up, so
        // the agent becomes the last of the previous block.
        for (unsigned int s = state_old; s > state_new; --s)
        {
            size_t first = state_start[s];
            swap_pos(pos, first);
            state_start[s]++;
            pos = first;
        }

    }

    agent_state[id] = state_new;

}

inline AgentIdsView Model::state_index_members(size_t state) const
{
    const size_t from = state_start[state];
    return AgentIdsView(
        state_order.data() + from, state_start[state + 1u] - from
    );
}

inline void Model::state_index_update(
    Agent * p,
    unsigned int state_old,
    bool had_virus
)
{

    const unsigned int state_new = p->state;
    const bool has_virus = (p->virus != nullptr);

    if ((state_new == state_old) && (has_virus == had_virus))
        return;

    const size_t id  = static_cast< size_t >(p->id);
    const size_t deg = network_degree(id);

    if (state_new != state_old)
    {

        state_index_move(id, state_old, state_new);

        state_degree[state_old] -= deg;
        state_degree[state_new] += deg;

    }

    if (has_virus != had_virus)
        agent_carrier[id] = has_virus ? 1 : 0;

    if (had_virus)
    {
        state_carriers[state_old]--;
        state_carrier_degree[state_old] -= deg;
    }

    if (has_virus)
    {
        state_carriers[state_new]++;
        state_carrier_degree[state_new] += deg;
    }

}

inline void Model::state_index_degree(size_t id, int delta)
{

    if (!state_index_ready || (delta == 0))
        return;

    // Unsigned arithmetic wraps, so adding a "negative" delta is exact
    const auto & p = population[id];
    const size_t d = static_cast< size_t >(static_cast< std::ptrdiff_t >(delta));
    state_degree[p.state] += d;
    if (p.virus != nullptr)
        state_carrier_degree[p.state] += d;

}

inline AgentIdsView Model::get_agents_in_state(
    epiworld_fast_uint state
) const
{

    if (!state_index_ready)
        throw std::logic_error(
            "The agents-by-state index is built when the model runs. Call "
            "run() (or run_multiple()) before get_agents_in_state()."
        );

    if (state >= nstates)
        throw std::range_error(
            "The state " + std::to_string(state) + " is out of range. " +
            "The model currently has " + std::to_string(nstates) + " states."
        );

    return state_index_members(state);

}

inline void Model::events_run()
{
    // Making the call
    size_t nevents_tmp = 0;
    while (nevents_tmp < nactions)
    {

        Event & a = events[nevents_tmp++];

        // A declared transition applies only if the agent is still in one of
        // its sources (it may have moved earlier in the same pass). Its event
        // carries the target stage or status, made a cell here.
        const int transition = a.transition;
        if (nh && (transition != -1))
        {

            const unsigned int c = a.agent->state;
            const int s = nh_stage_of(c);
            const int k = nh_status_of(c);

            if (nh_is_status_tr(transition))
            {
                if (!st->is_source(nh_status_tr(transition), k) || nh_removed(s))
                    continue;
                a.new_state = static_cast< epiworld_fast_int >(
                    nh_cell(s, static_cast< int >(a.new_state))
                );
            }
            else
            {
                if ((transition >= 0) && !nh->is_source(transition, s))
                    continue;
                a.new_state = static_cast< epiworld_fast_int >(
                    nh_cell(static_cast< int >(a.new_state), k)
                );
            }

            // The status it had at the start of the day
            if (a.agent->state_last_changed != today())
                nh_status_sod[static_cast< size_t >(a.agent->id)] = k;

        }

        // Everything read from the event after the handler runs is copied
        // first: a handler can schedule more events (e.g., a virus's
        // post-recovery hook adding a tool), which may grow `events` and leave
        // `a` dangling.
        Agent * p = a.agent;
        const epiworld_fast_int new_state = a.new_state;
        const epiworld_fast_int queue_change = a.queue;
        const unsigned int state_old = p->state;
        const bool had_virus = (p->virus != nullptr);

        #ifdef EPI_DEBUG
        if (new_state >= static_cast<epiworld_fast_int>(nstates))
        {
            throw std::range_error(
                "The proposed state " + std::to_string(new_state) + " is out of range. " +
                "The model currently has " + std::to_string(nstates - 1) + " states.");

        }
        else if ((new_state != -99) && (new_state < 0))
        {
            throw std::range_error(
                "The proposed state " + std::to_string(new_state) + " is out of range. " +
                "The state cannot be negative.");
        }
        #endif

        // Undoing the change in the transition matrix
        if (
            (new_state != -99) &&
            (p->state_last_changed == today()) &&
            (static_cast<int>(p->state) != new_state)
        )
        {
            // Undoing state change in the transition matrix
            // The previous state is already recorded
            db.update_state(p->state_prev, p->state, true);

        } else if (p->state_last_changed != today())
            p->state_prev = p->state; // Recording the previous state

        switch (a.action)
        {
        case EventAction::AddVirus:
            _event_add_virus(a);
            break;
        case EventAction::AddTool:
            _event_add_tool(a);
            break;
        case EventAction::RemoveVirus:
            _event_rm_virus(a);
            break;
        case EventAction::RemoveTool:
            _event_rm_tool(a);
            break;
        case EventAction::ChangeState:
            _event_change_state(a);
            break;
        default:
            throw std::logic_error("The requested event action is not supported.");
        }

        if (new_state != -99)
            p->state = new_state;

        // Registering that the last change was today
        p->state_last_changed = today();

        if (state_index_ready)
            state_index_update(p, state_old, had_virus);

        // The natural history's bookkeeping: entry days, the stage's duration,
        // the transition counts, and the queue (derived from the stages)
        if (nh)
        {

            const int s_old = nh_stage_of(state_old);
            const int s_new = nh_stage_of(p->state);

            // Entering a stage: entry days, the stage's duration, episodes
            if (
                (s_new != s_old) ||
                (transition == NH_SEED) ||
                (transition == NH_INITIAL)
            )
                nh_enter(*p, static_cast< unsigned int >(s_old), transition);

            // Entering a status: its day, and its duration (which may count
            // from a stage's entry)
            if (st && (nh_status_of(p->state) != nh_status_of(state_old)))
            {

                const size_t id = static_cast< size_t >(p->id);
                const int k = nh_status_of(p->state);
                nh_status_since[id] = today();
                const int after = st->stages[static_cast< size_t >(k)].after;
                if (after >= 0)
                {
                    const auto & d = st->transitions[after].duration;
                    int base = today();
                    if (d.get_anchor() >= 0)
                    {
                        const int e = nh_entered[id * nh->get_n_stages() +
                            static_cast< size_t >(d.get_anchor())];
                        if (e >= 0)
                            base = e;
                    }
                    nh_status_until[id] = base + d.draw(*p, *this);
                }
                else
                    nh_status_until[id] = INT_MAX;

            }

            // Counted, if a declared transition did it
            if ((transition != -1) && (transition != NH_SEED) &&
                (transition != NH_INITIAL) && (p->state != state_old))
                db.record_declared_transition(nh_db_id(transition));

            if (use_queuing && (p->state != state_old))
                nh_queue_move(*p, state_old);

            // Reactions (they may schedule more events, applied in this pass)
            if (p->state != state_old)
                nh_react(*p, transition, s_old);

            continue;

        }


        #ifdef EPI_DEBUG
        if (static_cast<int>(p->state) >= static_cast<int>(nstates))
                throw std::range_error(
                    "The new state " + std::to_string(p->state) + " is out of range. " +
                    "The model currently has " + std::to_string(nstates - 1) + " states.");
        #endif

        // Updating queue
        if (use_queuing && queue_change != -99)
        {

            if (queue_change == Queue::Everyone)
                queue += p;
            else if (queue_change == -Queue::Everyone)
                queue -= p;
            else if (queue_change == Queue::OnlySelf)
                queue.shift(static_cast< size_t >(p->get_id()), 1);
            else if (queue_change == -Queue::OnlySelf)
                queue.shift(static_cast< size_t >(p->get_id()), -1);
            else if (queue_change != Queue::NoOne)
                throw std::logic_error(
                    "The proposed queue change is not valid. Queue values can be {-2, -1, 0, 1, 2}."
                    );

        }

    }

    // Go back to square 1
    nactions = 0u;

    return;

}

/**
 * @name Default function for combining susceptibility_reduction levels
 *
 * @param pt
 * @return epiworld_double
 */
///@{
inline epiworld_double Model::susceptibility_reduction_mixer(
    Agent* p,
    VirusPtr & v
)
{
    epiworld_double total = 1.0;
    for (auto & tool : p->get_tools())
    {
        if (!(tool->target_mask & v->lineage_bit))
            continue;

        total *= (1.0 - tool->get_susceptibility_reduction(v, this));
    }

    return 1.0 - total;

}

inline epiworld_double Model::transmission_reduction_mixer(
    Agent* p,
    VirusPtr & v
)
{
    epiworld_double total = 1.0;
    for (auto & tool : p->get_tools())
    {
        if (!(tool->target_mask & v->lineage_bit))
            continue;

        total *= (1.0 - tool->get_transmission_reduction(v, this));
    }

    return (1.0 - total);

}

inline epiworld_double Model::recovery_enhancer_mixer(
    Agent* p,
    VirusPtr & v
)
{
    epiworld_double total = 1.0;
    for (auto & tool : p->get_tools())
    {
        if (!(tool->target_mask & v->lineage_bit))
            continue;

        total *= (1.0 - tool->get_recovery_enhancer(v, this));
    }

    return 1.0 - total;

}

inline epiworld_double Model::death_reduction_mixer(
    Agent* p,
    VirusPtr & v
) {

    epiworld_double total = 1.0;
    for (auto & tool : p->get_tools())
    {
        if (!(tool->target_mask & v->lineage_bit))
            continue;

        total *= (1.0 - tool->get_death_reduction(v, this));
    }

    return 1.0 - total;

}
///@}

inline std::unique_ptr<Model> Model::clone_ptr()
{
    // Everything is copied
    auto ptr = std::make_unique<Model>(*this);

    #ifdef EPI_DEBUG
    if (*this != *ptr)
        throw std::logic_error("Model::clone_ptr The copies of the model don't match.");
    #endif

    return ptr;
}

inline Model::Model()
{
    db.model = this;
    db.user_data = this;
    if (use_queuing)
        queue.model = this;
}

inline Model::Model(size_t n) : Model()
{
    set_population_size(n);
}

inline Model::Model(const Model & model) :
    name(model.name),
    db(model.db),
    population(model.population),
    viruses(),
    tools(),
    virus_assign(model.virus_assign),
    tool_assign(model.tool_assign),
    param_values(model.param_values),
    param_index(model.param_index),
    param_layout_id(model.param_layout_id),
    ndays(model.ndays),
    pb(model.pb),
    state_fun(model.state_fun),
    states_labels(model.states_labels),
    nstates(model.nstates),
    verbose(model.verbose),
    current_date(model.current_date),
    globalevents(),
    queue(model.queue),
    use_queuing(model.use_queuing),
    sim_id(model.sim_id),
    post_sampling_fun(model.post_sampling_fun),
    post_sampling_on(model.post_sampling_on),
    state_order(model.state_order),
    state_start(model.state_start),
    state_member_pos(model.state_member_pos),
    agent_state(model.agent_state),
    agent_carrier(model.agent_carrier),
    state_degree(model.state_degree),
    state_carriers(model.state_carriers),
    state_carrier_degree(model.state_carrier_degree),
    state_index_ready(model.state_index_ready),
    infectious_states(model.infectious_states),
    excluded_states(model.excluded_states),
    state_not_infectious(model.state_not_infectious),
    state_excluded(model.state_excluded),
    transmission_mode(model.transmission_mode),
    transmission_mode_last(model.transmission_mode_last),
    transmission_kappa(model.transmission_kappa),
    nh(model.nh),
    nh_until(model.nh_until),
    nh_entered(model.nh_entered),
    nh_episode(model.nh_episode),
    nh_queue(model.nh_queue),
    nh_initial(model.nh_initial),
    st(model.st),
    nh_status_until(model.nh_status_until),
    nh_status_since(model.nh_status_since),
    nh_status_sod(model.nh_status_sod),
    nh_reactions(model.nh_reactions)
{

    // Pointing to the right place. This needs
    // to be done afterwards since the state zero is set as a function
    // of the population.
    db.model = this;
    db.user_data.model = this;

    if (use_queuing)
        queue.model = this;

    agents_data = model.agents_data;
    agents_data_ncols = model.agents_data_ncols;

    rbinomd = model.rbinomd;
    rbinomd_n = model.rbinomd_n;
    rbinomd_fast_lambda = model.rbinomd_fast_lambda;
    rbinomd_use_poisson = model.rbinomd_use_poisson;

    // Deep-copy model-level objects so clones can run independently in parallel.
    viruses.reserve(model.viruses.size());
    for (const auto & v : model.viruses)
        viruses.emplace_back(std::shared_ptr<Virus>(v->clone_ptr()));

    tools.reserve(model.tools.size());
    for (const auto & t : model.tools)
        tools.emplace_back(std::shared_ptr<Tool>(t->clone_ptr()));

    globalevents.reserve(model.globalevents.size());
    for (const auto & ge : model.globalevents)
        globalevents.emplace_back(std::shared_ptr<GlobalEvent>(ge->clone_ptr()));

    layers.reserve(model.layers.size());
    for (const auto & layer : model.layers)
        layers.emplace_back(layer->clone());

    layers_attach();

    // A copy is not in a run, even if the source is
    set_running(running);

}

inline Model::Model(Model && model) :
    name(std::move(model.name)),
    db(std::move(model.db)),
    population(std::move(model.population)),
    agents_data(std::move(model.agents_data)),
    agents_data_ncols(std::move(model.agents_data_ncols)),
    // Virus
    viruses(std::move(model.viruses)),
    // Tools
    tools(std::move(model.tools)),
    virus_assign(std::move(model.virus_assign)),
    tool_assign(std::move(model.tool_assign)),
    // Pseudo-RNG
    engine(std::move(model.engine)),
    runifd_a(model.runifd_a),
    runifd_b(model.runifd_b),
    rnormd(std::move(model.rnormd)),
    rgammad(std::move(model.rgammad)),
    rlognormald(std::move(model.rlognormald)),
    rexpd(std::move(model.rexpd)),
    rbinomd(std::move(model.rbinomd)),
    rbinomd_n(model.rbinomd_n),
    rbinomd_fast_lambda(model.rbinomd_fast_lambda),
    rbinomd_use_poisson(model.rbinomd_use_poisson),
    param_values(std::move(model.param_values)),
    param_index(std::move(model.param_index)),
    param_layout_id(model.param_layout_id),
    // Others
    ndays(model.ndays),
    pb(std::move(model.pb)),
    state_fun(std::move(model.state_fun)),
    states_labels(std::move(model.states_labels)),
    nstates(model.nstates),
    verbose(model.verbose),
    current_date(std::move(model.current_date)),
    globalevents(std::move(model.globalevents)),
    layers(std::move(model.layers)),
    queue(std::move(model.queue)),
    use_queuing(model.use_queuing),
    sim_id(model.sim_id),
    post_sampling_fun(std::move(model.post_sampling_fun)),
    post_sampling_on(model.post_sampling_on),
    state_order(std::move(model.state_order)),
    state_start(std::move(model.state_start)),
    state_member_pos(std::move(model.state_member_pos)),
    agent_state(std::move(model.agent_state)),
    agent_carrier(std::move(model.agent_carrier)),
    state_degree(std::move(model.state_degree)),
    state_carriers(std::move(model.state_carriers)),
    state_carrier_degree(std::move(model.state_carrier_degree)),
    state_index_ready(model.state_index_ready),
    infectious_states(std::move(model.infectious_states)),
    excluded_states(std::move(model.excluded_states)),
    state_not_infectious(std::move(model.state_not_infectious)),
    state_excluded(std::move(model.state_excluded)),
    transmission_mode(model.transmission_mode),
    transmission_mode_last(model.transmission_mode_last),
    transmission_kappa(model.transmission_kappa),
    nh(std::move(model.nh)),
    nh_until(std::move(model.nh_until)),
    nh_entered(std::move(model.nh_entered)),
    nh_episode(std::move(model.nh_episode)),
    nh_queue(std::move(model.nh_queue)),
    nh_initial(std::move(model.nh_initial)),
    st(std::move(model.st)),
    nh_status_until(std::move(model.nh_status_until)),
    nh_status_since(std::move(model.nh_status_since)),
    nh_status_sod(std::move(model.nh_status_sod)),
    nh_reactions(std::move(model.nh_reactions))
{

    db.model = this;
    db.user_data.model = this;

    if (use_queuing)
        queue.model = this;

    layers_attach();

}

inline Model & Model::operator=(const Model & m)
{
    if (this == &m)
        return *this;

    name = m.name;

    population = m.population;

    db = m.db;
    db.model = this;
    db.user_data.model = this;

    viruses.clear();
    viruses.reserve(m.viruses.size());
    for (const auto & v : m.viruses)
        viruses.emplace_back(std::shared_ptr<Virus>(v->clone_ptr()));

    tools.clear();
    tools.reserve(m.tools.size());
    for (const auto & t : m.tools)
        tools.emplace_back(std::shared_ptr<Tool>(t->clone_ptr()));

    virus_assign = m.virus_assign;
    tool_assign = m.tool_assign;

    param_values    = m.param_values;
    param_index     = m.param_index;
    param_layout_id = m.param_layout_id;
    ndays      = m.ndays;
    pb         = m.pb;

    state_fun    = m.state_fun;
    states_labels = m.states_labels;
    nstates       = m.nstates;

    verbose     = m.verbose;

    current_date = m.current_date;

    globalevents.clear();
    globalevents.reserve(m.globalevents.size());
    for (const auto & ge : m.globalevents)
        globalevents.emplace_back(std::shared_ptr<GlobalEvent>(ge->clone_ptr()));

    layers_detach();
    layers.clear();
    layers.reserve(m.layers.size());
    for (const auto & layer : m.layers)
        layers.emplace_back(layer->clone());

    layers_attach();

    // The layers follow this model's run state, not the source's
    set_running(running);

    queue = m.queue;
    use_queuing = m.use_queuing;

    post_sampling_fun = m.post_sampling_fun;
    post_sampling_on = m.post_sampling_on;
    post_sampling_scratch.reset(0u);

    state_order = m.state_order;
    state_start = m.state_start;
    state_member_pos = m.state_member_pos;
    agent_state = m.agent_state;
    agent_carrier = m.agent_carrier;
    state_degree = m.state_degree;
    state_carriers = m.state_carriers;
    state_carrier_degree = m.state_carrier_degree;
    state_index_ready = m.state_index_ready;

    infectious_states = m.infectious_states;
    excluded_states = m.excluded_states;
    state_not_infectious = m.state_not_infectious;
    state_excluded = m.state_excluded;

    transmission_mode = m.transmission_mode;
    transmission_mode_last = m.transmission_mode_last;
    transmission_kappa = m.transmission_kappa;

    nh = m.nh;
    nh_until = m.nh_until;
    nh_entered = m.nh_entered;
    nh_episode = m.nh_episode;
    nh_queue = m.nh_queue;
    nh_initial = m.nh_initial;
    st = m.st;
    nh_status_until = m.nh_status_until;
    nh_status_since = m.nh_status_since;
    nh_status_sod = m.nh_status_sod;
    nh_reactions = m.nh_reactions;

    agents_data = m.agents_data;
    agents_data_ncols = m.agents_data_ncols;

    rbinomd = m.rbinomd;
    rbinomd_n = m.rbinomd_n;
    rbinomd_fast_lambda = m.rbinomd_fast_lambda;
    rbinomd_use_poisson = m.rbinomd_use_poisson;

    // Figure out the queuing
    if (use_queuing)
        queue.model = this;

    sim_id = m.sim_id;

    return *this;

}

inline DataBase & Model::get_db()
{
    return db;
}

inline const DataBase & Model::get_db() const
{
    return db;
}


inline std::vector<Agent> & Model::get_agents()
{
    return population;
}

inline Agent & Model::get_agent(size_t i)
{
    return population[i];
}

inline std::vector< epiworld_fast_uint > Model::get_agents_states() const
{
    std::vector< epiworld_fast_uint > states(population.size());
    for (size_t i = 0u; i < population.size(); ++i)
        states[i] = population[i].get_state();

    return states;
}


inline void Model::set_population_size(size_t n)
{

    population.clear();
    population.resize(n);
    state_index_ready = false;

    size_t i = 0u;
    for (auto & p : population)
        p.id = static_cast< int >(i++);

}


inline epiworld_double Model::operator()(std::string_view pname) const {

    const auto iter = param_index.find(pname);
    if (iter == param_index.end())
        throw std::range_error(
            "The parameter '" + std::string(pname) + "' is not in the model."
        );

    return param_values[iter->second];

}

inline size_t Model::size() const {
    return population.size();
}

inline void Model::dist_virus()
{

    // Who can get a virus: no virus, and (with a natural history) a stage
    // that can be infected
    std::vector< char > eligible(population.size());

    for (size_t k = 0u; k < viruses.size(); ++k)
    {

        for (auto & p : population)
            eligible[static_cast< size_t >(p.id)] =
                (p.virus == nullptr) &&
                (!nh || (nh->stages[static_cast< size_t >(nh_stage_of(p.state))].infection >= 0));

        const std::vector< int > labels = virus_assign[k].draw(*this, eligible);
        const Virus & v = *viruses[k];

        for (size_t i = 0u; i < labels.size(); ++i)
        {

            const int label = labels[i];
            if (label == -1)
                continue;

            if (label == assign::ANY)
            {
                if (nh)
                    nh_pending = NH_SEED;
                population[i].set_virus(*this, v);
            }
            else
            {
                if (!nh || (label < 0) || (static_cast< size_t >(label) >= nh->get_n_stages()) || !nh->is_carrier(label))
                    throw std::logic_error(
                        "The virus \"" + v.get_name() + "\" is assigned to "
                        "the label " + std::to_string(label) + ", which is not "
                        "a carrier stage of the natural history."
                    );
                nh_pending = NH_SEED;
                population[i].set_virus(*this, v, label, Queue::NoOne);
            }
            nh_pending = -1;

        }

        // Apply the events
        events_run();
    }

}

inline void Model::dist_tools()
{

    // Everyone can get a tool
    const std::vector< char > eligible(population.size(), 1);

    for (size_t k = 0u; k < tools.size(); ++k)
    {

        const std::vector< int > labels = tool_assign[k].draw(*this, eligible);
        for (size_t i = 0u; i < labels.size(); ++i)
            if (labels[i] != -1)
                population[i].add_tool(*this, *tools[k]);

        // Apply the events
        events_run();

    }

}

inline void Model::chrono_start() {
    time_start = std::chrono::steady_clock::now();
}

inline void Model::chrono_end() {
    time_end = std::chrono::steady_clock::now();
    time_elapsed += (time_end - time_start);
    n_replicates++;
}


inline void Model::add_virus(
    const Virus & v,
    Assign a
    )
{

    // The stages say where a virus takes the agent, and the transitions name
    // the parameters it reads
    if (!v.nh_def)
        throw std::logic_error(
            "The virus \"" + v.get_name() + "\" has no natural history: "
            "declare its stages and transitions (add_stage(), infection(), "
            "rate(), after(), ...) before adding it."
        );

    // The model keeps its own copy (of the derived class, if any); its id and
    // lineage are recorded on the copy
    auto cloned = v.clone_ptr();

    // Its own copy of the natural history, with numbers turned into model
    // parameters (named after the virus); the first one gives the stages
    if (v.nh_def)
    {

        auto def = std::make_shared< NaturalHistory >(*v.nh_def);
        def->finalize(*this, v.get_name());

        if (!nh)
            nh_install(def);
        else if (!nh->same_structure(*def))
            throw std::logic_error(
                "The natural history of \"" + v.get_name() + "\" differs from "
                "the model's (\"" + nh->get_name() + "\"): the viruses of a "
                "model have the same stages and transitions (their values may "
                "differ)."
            );

        cloned->nh_def = def;

    }

    db.record_virus(*cloned, true);
    viruses.push_back(std::shared_ptr<Virus>(std::move(cloned)));
    virus_assign.push_back(std::move(a));

}

inline void Model::add_tool(const Tool & t, Assign a)
{

    // The model keeps its own copy (of the derived class, if any)
    auto cloned = t.clone_ptr();
    db.record_tool(*cloned);
    tools.push_back(std::shared_ptr<Tool>(std::move(cloned)));
    tool_assign.push_back(std::move(a));

}

inline void Model::assign_virus(size_t id, Assign a)
{
    if (id >= viruses.size())
        throw std::range_error(
            "There is no virus " + std::to_string(id) + " (the model has " +
            std::to_string(viruses.size()) + ")."
        );
    virus_assign[id] = std::move(a);
}

inline void Model::assign_tool(size_t id, Assign a)
{
    if (id >= tools.size())
        throw std::range_error(
            "There is no tool " + std::to_string(id) + " (the model has " +
            std::to_string(tools.size()) + ")."
        );
    tool_assign[id] = std::move(a);
}

inline void Model::rm_virus(size_t virus_pos)
{

    if (viruses.size() <= virus_pos)
        throw std::range_error(
            std::string("The specified virus (") +
            std::to_string(virus_pos) +
            std::string(") is out of range. ") +
            std::string("There are only ") +
            std::to_string(viruses.size()) +
            std::string(" viruses.")
            );

    // Flipping with the last one
    std::swap(viruses[virus_pos], viruses[viruses.size() - 1]);
    viruses.pop_back();
    std::swap(virus_assign[virus_pos], virus_assign[virus_assign.size() - 1]);
    virus_assign.pop_back();

    return;

}

inline void Model::rm_tool(size_t tool_pos)
{

    if (tools.size() <= tool_pos)
        throw std::range_error(
            std::string("The specified tool (") +
            std::to_string(tool_pos) +
            std::string(") is out of range. ") +
            std::string("There are only ") +
            std::to_string(tools.size()) +
            std::string(" tools.")
            );

    // Flipping with the last one
    std::swap(tools[tool_pos], tools[tools.size() - 1]);

    /* There's an error on windows:
    https://github.com/UofUEpiBio/epiworldR/actions/runs/4801482395/jobs/8543744180#step:6:84

    More clear here:
    https://stackoverflow.com/questions/58660207/why-doesnt-stdswap-work-on-vectorbool-elements-under-clang-win
    */

    tools.pop_back();
    std::swap(tool_assign[tool_pos], tool_assign[tool_assign.size() - 1]);
    tool_assign.pop_back();

    return;

}

inline int Model::today() const {

    if (ndays == 0)
      return 0;

    return this->current_date;
}

inline void Model::next() {

    #ifdef EPI_DEBUG
    // Checking all the agents have proper states
    for (auto & p : population)
    {
        if ((p.state >= nstates) || (p.state < 0))
        {
            throw std::range_error(
                "The agent " + std::to_string(p.id) +
                " has state " + std::to_string(p.state) +
                " which is above the maximum state of " +
                std::to_string(nstates - 1) + "."
            );
        }

        if ((p.state_prev >= nstates) || (p.state_prev < 0))
        {
            throw std::range_error(
                "The agent " + std::to_string(p.id) +
                " has previous state " + std::to_string(p.state_prev) +
                " which is above the maximum state of " +
                std::to_string(nstates - 1) + "."
            );
        }

    }

    #endif

    // The layers see what changed during the step before the next one samples
    update_layers();

    db.record();
    ++this->current_date;

    // Advancing the progress bar
    if ((this->current_date >= 1) && verbose)
        pb.next();

    return ;
}

inline Model & Model::run(
    epiworld_fast_uint ndays,
    int seed
)
{

    // Set this model as the current model in scope for this thread.
    // This enables Model::the() calls from agents, etc.

    if (size() == 0u)
        throw std::logic_error("There are no agents in this model!");

    if (nstates == 0u)
        throw std::logic_error(
            std::string("The model has no stages: add a virus with a ") +
            std::string("natural history (Model::add_virus()).")
            );

    // Setting up the number of steps
    this->ndays = ndays;

    if (seed >= 0)
        this->seed(static_cast< size_t >(seed));

    last_seed = seed;

    // From here on get_ndays() is the run's horizon (see is_running()). The
    // guard clears the flag however the run ends, exceptions included.
    struct RunningGuard {
        Model & model;
        explicit RunningGuard(Model & m) : model(m) { model.set_running(true); }
        ~RunningGuard() { model.set_running(false); }
    } running_guard(*this);

    // Starting first infection and tools
    reset();

    // Record the baseline (day 0) and advance to day 1
    next();

    // Initializing the simulation
    chrono_start();

    // Verifying if the user wants to see the progress bar
    if (get_verbose())
    {
        printf_epiworld("Running the model...\n");
    }

    for (epiworld_fast_uint niter = 0; niter < get_ndays(); ++niter)
    {

        #ifdef EPI_DEBUG
        db.n_transmissions_potential = 0;
        db.n_transmissions_today = 0;
        #endif

        // We can execute these components in whatever order the
        // user needs.
        this->update_state();

        // We start with the Global events
        this->run_globalevents();

        // This locks all the changes (the layers, including their rewiring,
        // are updated first)
        this->next();

        // Mutation must happen at the very end of all
        this->mutate_virus();

    }

    // The last reaches the end...
    this->current_date--;

    chrono_end();

    sim_id++;

    return *this;

}

inline Model & Model::run_multiple(
    epiworld_fast_uint ndays,
    epiworld_fast_uint nexperiments,
    int seed_,
    std::function<void(size_t,Model*)> fun,
    bool reset,
    bool verbose,
    #ifdef _OPENMP
    int nthreads
    #else
    int
    #endif
)
{

    if (seed_ >= 0)
        this->seed(seed_);

    if (nexperiments == 0u)
        throw std::logic_error("The number of experiments must be above 0.");

    // Seeds will be reproducible by default
    std::vector< int > seeds_n(nexperiments);
    for (auto & s : seeds_n)
    {
        s = static_cast<int>(
            std::floor(
                runif() * static_cast<double>(std::numeric_limits<int>::max())
                )
        );
    }
    // #endif

    if (verbose)
    {
        EPI_DEBUG_NOTIFY_ACTIVE()
    }

    bool old_verb = this->verbose;
    verbose_off();

    #ifdef _OPENMP

    // Not more than the number of experiments
    nthreads =
        static_cast<size_t>(nthreads) > nexperiments ? nexperiments : nthreads;
    
    omp_set_num_threads(nthreads);

    // Generating copies of the model (done serially to avoid races on original)
    std::vector< std::unique_ptr< Model > > these;

    for (size_t i = 1u; i < static_cast<size_t>(nthreads); ++i)
    {
        these.emplace_back(clone_ptr());
    }


    if (verbose)
    {

        printf_epiworld(
            "Starting multiple runs (%i) using %i thread(s)\n",
            static_cast<int>(nexperiments),
            static_cast<int>(nthreads)
        );

    }

    #ifdef EPI_DEBUG
    // Checking the initial state of all the models. Throw an
    // exception if they are not the same.
    for (size_t i = 1; i < static_cast<size_t>(std::max(nthreads - 1, 0)); ++i)
    {

        if (db != these[i]->db)
        {
            throw std::runtime_error(
                "The initial state of the models is not the same"
            );
        }
    }
    #endif

    // Replicates run by this model (thread 0), which counts them itself
    size_t nreplicates_this = 0u;

    #pragma omp parallel shared(these, nreplicates_this) \
        firstprivate(nexperiments, fun, reset, verbose, ndays, seeds_n) \
        default(none)
    {

        // The replicates are split over the team OpenMP actually provides,
        // which can be smaller than nthreads (OMP_THREAD_LIMIT, OMP_DYNAMIC,
        // nested parallel regions, etc.): each thread runs a contiguous block
        // of run ids, the remainder going to the first threads.
        auto iam = static_cast<size_t>(omp_get_thread_num());
        auto nteam = static_cast<size_t>(omp_get_num_threads());
        Model * model_ptr = iam == 0 ? this : &(*these[iam - 1u]);

        size_t base_replicates = nexperiments / nteam;
        size_t remainder = nexperiments % nteam;
        size_t my_replicates = base_replicates + (iam < remainder ? 1u : 0u);
        size_t my_replicates_csum =
            iam * base_replicates + std::min(iam, remainder);

        // Only the first thread reports progress, on its own replicates
        Progress pb_multiple(my_replicates, EPIWORLD_PROGRESS_BAR_WIDTH);

        if (iam == 0)
        {

            nreplicates_this = my_replicates;

            if (verbose)
                pb_multiple.start();

        }

        for (size_t n = 0u; n < my_replicates; ++n)
        {
            size_t run_id = my_replicates_csum + n;
            if (iam == 0)
            {

                // Checking if the user interrupted the simulation
                EPI_CHECK_USER_INTERRUPT(n);

                // Setting the simulation id
                model_ptr->set_sim_id(run_id);

                // Initializing the seed
                model_ptr->run(ndays, seeds_n[run_id]);

                // Only the first one prints
                if (verbose)
                    pb_multiple.next();

            } else {

                // Setting the simulation id
                model_ptr->set_sim_id(run_id);

                // Initializing the seed
                model_ptr->run(ndays, seeds_n[run_id]);

            }

            if (fun)
            {
                // User callbacks often write into shared result containers.
                // Serialize callback execution to avoid callback-induced races.
                #pragma omp critical(epiworld_run_multiple_fun)
                {
                    fun(run_id, model_ptr);
                }
            }

        }

    }

    // Adjusting the number of replicates
    n_replicates += (nexperiments - nreplicates_this);

    #else

    Progress pb_multiple(
        nexperiments,
        EPIWORLD_PROGRESS_BAR_WIDTH
        )
        ;
    if (verbose)
    {

        printf_epiworld(
            "Starting multiple runs (%i)\n",
            static_cast<int>(nexperiments)
        );

        pb_multiple.start();

    }

    for (size_t n = 0u; n < nexperiments; ++n)
    {

        // Checking if the user interrupted the simulation
        EPI_CHECK_USER_INTERRUPT(n);

        set_sim_id(n);
        run(ndays, seeds_n[n]);

        if (fun)
            fun(n, this);

        if (verbose)
            pb_multiple.next();

    }
    #endif

    if (old_verb)
        verbose_on();

    return *this;

}

inline void Model::update_state() {

    // Susceptible states using the default sampler can be updated by pushing
    // infection odds from the carriers (see model-meat-transmission.hpp) --
    // same distribution, and cheaper while few agents carry a virus. Directed
    // networks always pull: a tie there is kept by its source only (see
    // layer::Network::is_directed()), so it is not visible from both ends.
    //
    // With a post-sampling callback, pull and push report the same contacts.
    if (post_sampling_on)
        post_sampling_scratch.clear();

    const bool directed = has_pull_only_layer();

    const bool push =
        transmission_prepare() && !directed && transmission_choose_push();

    transmission_mode_last = push ?
        TransmissionMode::push : TransmissionMode::pull;

    if (push)
    {

        // Susceptibles were handled by the push; only the agents in the other
        // states with an update function are left (see
        // transmission_update_others()).
        transmission_push();
        transmission_update_others();

    }
    else if (use_queuing && !has_queue_blind_layer())
    {

        // Only queued agents, in ascending id order (the order fixes the
        // random number stream).
        //
        // Directed networks (and mixing across groups) visit everyone instead.
        // Registering an agent queues the agents on its own list, which in a
        // directed network are the ones it is exposed to. The ones that can
        // catch something from it are the ones that list *it*, and the queue
        // never sees those.
        queue.for_each_nonzero([this](size_t i) -> void {

            if (queue[i] <= 0)
                return;

            auto & p = population[i];
            if (state_fun[p.state])
                state_fun[p.state](&p, this);

        });

    }
    else
    {

        for (auto & p: population)
            if (state_fun[p.state])
                state_fun[p.state](&p, this);

    }

    if (post_sampling_on)
        post_sampling_dispatch();

    events_run();

}

inline void Model::mutate_virus() {

    // Checking if any virus has mutation
    size_t nmutates = 0u;
    for (const auto & v: viruses)
        if (v->mutation)
            nmutates++;

    if (nmutates == 0u)
        return;

    // Layers the queue does not see (see update_state()) must not start
    // depending on it here: a carrier registered without `Everyone`, or a count
    // left stale by rewiring, would then mutate only with queuing off.
    if (use_queuing && !has_queue_blind_layer())
    {

        queue.for_each_nonzero([this](size_t i) -> void {

            auto & p = population[i];
            if (p.virus != nullptr)
                p.virus->mutate(this);

        });

    }
    else
    {

        for (auto & p: population)
        {

            if (p.virus != nullptr)
                p.virus->mutate(this);

        }

    }


}

inline size_t Model::get_n_viruses() const {
    return db.size();
}

inline size_t Model::get_n_tools() const {
    return tools.size();
}

inline epiworld_fast_uint Model::get_ndays() const {
    return ndays;
}

inline bool Model::is_running() const {
    return running;
}

inline epiworld_fast_uint Model::get_n_replicates() const
{
    return n_replicates;
}

inline size_t Model::get_sim_id() const
{
    return sim_id;
}

inline void Model::set_sim_id(size_t id)
{
    sim_id = id;
}

inline void Model::set_ndays(epiworld_fast_uint ndays) {
    this->ndays = ndays;
}

inline bool Model::get_verbose() const {
    return verbose;
}

inline Model & Model::verbose_on() {
    verbose = true;
    return *this;
}

inline Model & Model::verbose_off() {
    verbose = false;
    return *this;
}



inline void Model::write_data(
    std::string fn_virus_info,
    std::string fn_virus_hist,
    std::string fn_tool_info,
    std::string fn_tool_hist,
    std::string fn_total_hist,
    std::string fn_transmission,
    std::string fn_transition,
    std::string fn_reproductive_number,
    std::string fn_generation_time,
    std::string fn_active_cases,
    std::string fn_outbreak_size,
    std::string fn_hospitalizations
    ) const
{

    db.write_data(
        fn_virus_info, fn_virus_hist,
        fn_tool_info, fn_tool_hist,
        fn_total_hist, fn_transmission, fn_transition,
        fn_reproductive_number, fn_generation_time,
        fn_active_cases, fn_outbreak_size,
        fn_hospitalizations
        );

}


inline std::map<std::string, epiworld_double> Model::params() const
{
    std::map<std::string, epiworld_double> res;
    for (const auto & p : param_index)
        res.emplace(p.first, param_values[p.second]);
    return res;
}

inline void Model::reset() {


    // Restablishing people
    pb = Progress(ndays, 80);

    for (auto & p : population)
        p.reset();

    // Who transmits and who is excluded, which the layers read
    state_masks_build();

    // The layers come first: what is distributed below (viruses, tools,
    // the global events' set-up) sees the run's contacts -- the queue
    // follows the ties as agents get a virus, and an intervention may edit
    // them.
    for (auto & layer : layers)
    {
        layer->reset(*this);
        layer->reset_tracing(population.size());
    }

    // A generator decides the graph's direction at its reset, so the
    // classification waits until every layer has one.
    layers_classify();

    sampled_layer = -1;

    // Everyone is now in the baseline state with no virus; from here on
    // events_run() keeps the index current.
    state_index_build();

    #ifdef EPI_DEBUG
    for (auto & a: population)
    {
        if (a.get_state() != 0u)
            throw std::logic_error("Model::reset population doesn't match."
                "Some agents are not in the baseline state.");
    }
    #endif

    current_date = 0;

    db.reset();

    // This also clears the queue
    if (use_queuing)
        queue.reset();

    // The natural history's per-agent records (and the first stage's queue)
    for (auto & v : viruses)
        v->check_params(*this);

    if (nh)
    {
        for (auto & v : viruses)
            if (!v->nh_def || ((v->nh_def.get() != nh.get()) && !nh->same_structure(*v->nh_def)))
                throw std::logic_error(
                    "The natural history of the virus \"" + v->get_name() +
                    "\" no longer matches the model's (was it edited after "
                    "add_virus()?)."
                );
        nh_reset();
    }

    // The batch of sampled contacts
    if (post_sampling_on)
    {
        post_sampling_scratch.reset(0u);
        post_sampling_prepare_scratch();
    }

    // Re distributing tools and virus
    dist_virus();
    dist_tools();

    // The initial stages, if any
    if (nh)
        nh_initial_place();

    // Global events set themselves up for the run. This happens last, with the
    // RNG seeded and the population in its initial state, so that an event that
    // needs to act on the model it is running in (e.g. one that hands a tool to
    // every agent) is in force from day 1 -- global events themselves only run
    // *after* each day's transitions.
    for (auto & event : globalevents)
        event->reset(this);

    events_run();

    // Recording day 0 and advancing to day 1 is handled by Model::run().
    // Keeping reset() side-effect free from virtual next() prevents
    // derived-model update code from running before derived reset state
    // is fully initialized.


}

// Too big to keep here
/*//////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

 Start of -include/epiworld/model-meat-print.hpp-

////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////*/


#ifndef EPIWORLD_MODEL_MEAT_PRINT_HPP
#define EPIWORLD_MODEL_MEAT_PRINT_HPP

inline const Model & Model::print(bool lite) const
{

    // Horizontal line
    std::string line = "";
    for (epiworld_fast_uint i = 0u; i < 80u; ++i)
        line += "_";

    // Prints a message if debugging is on
    EPI_DEBUG_NOTIFY_ACTIVE()

    printf_epiworld("%s\n",line.c_str());

    if (lite)
    {
        // Printing the name of the model
        printf_epiworld("%s", name.c_str());

        // Printing the number of agents, viruses, and tools
        printf_epiworld(
            "\nIt features %i agents, %i virus(es), and %i tool(s).\n",
            static_cast<int>(size()),
            static_cast<int>(get_n_viruses()),
            static_cast<int>(get_n_tools())
            );

        printf_epiworld(
            "The model has %i states.",
            static_cast<int>(nstates)
            );

        if (today() != 0)
        {
            printf_epiworld(
                "\nThe final distribution is: "
            );

            int nstate_int = static_cast<int>(nstates);

            for (int i = 0u; i < nstate_int; ++i)
            {
                printf_epiworld(
                    "%i %s%s",
                    static_cast<int>(db.today_total[ i ]),
                    states_labels[i].c_str(),
                    (
                        i == (nstate_int - 2)
                        ) ? ", and " : (
                            (i == (nstate_int - 1)) ? ".\n" : ", "
                            )
                );
            }
        } else {
            printf_epiworld(" The model hasn't been run yet.\n");
        }

        return *this;
    }

    printf_epiworld("%s\n%s\n\n",line.c_str(), "SIMULATION STUDY");

    printf_epiworld("Name of the model   : %s\n", (this->name == "") ? std::string("(none)").c_str() : name.c_str());
    printf_epiworld("Population size     : %i\n", static_cast<int>(size()));

    auto ncols = get_agents_data_ncols();

    if (ncols > 0)
    {
        printf_epiworld("Agents' data loaded : yes (%i columns/features)\n", static_cast<int>(ncols));
    }
    else
    {
        printf_epiworld("Agents' data        : (none)\n");
    }

    printf_epiworld("Days (duration)     : %i (of %i)\n", today(), static_cast<int>(ndays));
    printf_epiworld("Number of viruses   : %i\n", static_cast<int>(db.get_n_viruses()));
    if (n_replicates > 0u)
    {
        std::string abbr;
        epiworld_double elapsed;
        epiworld_double total;
        get_elapsed("auto", &elapsed, &total, &abbr, false);
        printf_epiworld("Last run elapsed t  : %.2f%s\n", elapsed, abbr.c_str());
        if (n_replicates > 1u)
        {
            printf_epiworld("Total elapsed t     : %.2f%s (%i runs)\n", total, abbr.c_str(), static_cast<int>(n_replicates));
        }

        // Elapsed time in speed
        get_elapsed("microseconds", &elapsed, &total, &abbr, false);
        printf_epiworld("Last run speed      : %.2f million agents x day / second\n",
            static_cast<double>(this->size()) *
            static_cast<double>(this->get_ndays()) /
            static_cast<double>(elapsed)
            );
        if (n_replicates > 1u)
        {
            printf_epiworld("Average run speed   : %.2f million agents x day / second\n",
                static_cast<double>(this->size()) *
                static_cast<double>(this->get_ndays()) *
                static_cast<double>(n_replicates) /
                static_cast<double>(total)
            );
        }

    } else {
        printf_epiworld("Last run elapsed t  : -\n");
    }
    
    
    printf_epiworld("Contact layers      : %i\n", static_cast<int>(layers.size()));
    for (const auto & layer : layers)
    {
        auto * net = layer->as_network();
        if (net != nullptr)
        {
            printf_epiworld(
                "  - %s (network%s%s)\n", layer->get_name().c_str(),
                net->is_directed() ? ", directed" : "",
                net->has_rewiring() ? ", rewired" : ""
            );
        }
        else
        {
            printf_epiworld("  - %s\n", layer->get_name().c_str());
        }
    }

    printf_epiworld("Last seed used      : %i\n\n", static_cast<int>(last_seed));
    
    // Printing Global events
    printf_epiworld("Global events:\n");
    for (auto & a : globalevents)
    {
        if (a->get_day() < 0)
        {
            printf_epiworld(" - %s (runs daily)\n", a->get_name().c_str());
        } else {
            printf_epiworld(" - %s (day %i)\n", a->get_name().c_str(), a->get_day());
        }
    }

    if (globalevents.size() == 0u)
    {
        printf_epiworld(" (none)\n");
    }

    printf_epiworld("\nVirus(es):\n");
    size_t n_viruses_model = viruses.size();
    for (size_t i = 0u; i < n_viruses_model; ++i)
    {    

        
        const auto & virus = viruses[i];
        if ((n_viruses_model > 10) && (i >= 10))
        {
            printf_epiworld(" ...and %i more viruses...\n",
                static_cast<int>(n_viruses_model) - 
                static_cast<int>(i)
                );
            break;
        }

        if (i < n_viruses_model)
        {

            printf_epiworld(
                " - %s\n",
                virus->get_name().c_str()
            );

        } else {

            printf_epiworld(
                " - %s (originated in the model...)\n",
                virus->get_name().c_str()
            );

        }

    }

    auto nvariants = db.get_n_viruses() - n_viruses_model;
    if (nvariants > 0)
    {

        printf_epiworld(" ...and %i more variants...\n", static_cast<int>(nvariants));

    }

    if (viruses.size() == 0u)
    {
        printf_epiworld(" (none)\n");
    }

    printf_epiworld("\nTool(s):\n");
    size_t n_tools_model = tools.size();
    for (size_t i = 0u; i < tools.size(); ++i)
    {   
        const auto & tool = tools[i];

        if ((n_tools_model > 10) && (i >= 10))
        {
            printf_epiworld(
                " ...and %i more tools...\n",
                static_cast<int>(n_tools_model) - static_cast<int>(i)
                );
            break;
        }

        if (i < n_tools_model)
        {
            printf_epiworld(
                " - %s\n",
                tool->get_name().c_str()
                );


        } else {

            printf_epiworld(
                " - %s (originated in the model...)\n",
                tool->get_name().c_str()
            );

        }
        

    }

    if (tools.size() == 0u)
    {
        printf_epiworld(" (none)\n");
    }

    // Information about the parameters included
    printf_epiworld("\nModel parameters:\n");
    epiworld_fast_uint nchar = 0u;
    for (auto & p : param_index)
        if (p.first.length() > nchar)
            nchar = p.first.length();

    std::string fmt = " - %-" + std::to_string(nchar + 1) + "s: ";
    for (auto & p : param_index)
    {
        const epiworld_double value = param_values[p.second];
        std::string fmt_tmp = fmt;
        if (std::fabs(value) < 0.0001)
            fmt_tmp += "%.1e\n";
        else
            fmt_tmp += "%.4f\n";

        printf_epiworld(
            fmt_tmp.c_str(),
            p.first.c_str(),
            value
        );
        
    }

    if (param_values.size() == 0u)
    {
        printf_epiworld(" (none)\n");
    }

    nchar = 0u;
    for (auto & p : states_labels)
        if (p.length() > nchar)
            nchar = p.length();

    

    if (today() != 0)
    {
        fmt =
            std::string("  - (%") +
                std::to_string(std::to_string(nstates).length()) +
            std::string("d) %-") + std::to_string(nchar) +
            std::string("s : %") +
            std::to_string(std::to_string(size()).length()) +
            std::string("i -> %i\n");
    } else {
        fmt =
            std::string("  - (%") +
                std::to_string(std::to_string(nstates).length()) +
            std::string("d) %-") + std::to_string(nchar) +
            std::string("s : %i\n");
    }
        
    if (today() != 0)
    {
        printf_epiworld("\nDistribution of the population at time %i:\n", today());
        for (size_t s = 0u; s < nstates; ++s)
        {

                printf_epiworld(
                    fmt.c_str(),
                    s,
                    states_labels[s].c_str(),
                    db.hist_total_counts[s],
                    db.today_total[ s ]
                    );

        }
    }

    if (today() != 0)
        (void) db.get_transition_probability(true);

    return *this;

}

#endif
/*//////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

 End of -include/epiworld/model-meat-print.hpp-

////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////*/



inline size_t Model::get_n_states() const
{
    return nstates;
}

inline epiworld_double Model::add_param(
    epiworld_double initial_value,
    std::string pname,
    bool overwrite
    ) {

    auto iter = param_index.find(pname);
    if (iter == param_index.end())
    {
        param_index.emplace(std::move(pname), param_values.size());
        param_values.push_back(initial_value);
        // Names now map to positions no other model layout has
        param_layout_id = new_param_layout_id();
    }
    else if (!overwrite)
        throw std::logic_error("The parameter " + pname + " already exists.");
    else
        param_values[iter->second] = initial_value;

    return initial_value;

}

inline Model & Model::read_params(std::string fn, bool overwrite)
{

    auto params_map = read_yaml<epiworld_double>(fn);

    for (auto & p : params_map)
        add_param(p.second, p.first, overwrite);

    return *this;

}

inline epiworld_double Model::get_param(std::string_view pname) const
{
    const auto iter = param_index.find(pname);
    if (iter == param_index.end())
        throw std::logic_error(
            "The parameter " + std::string(pname) + " does not exists."
        );

    return param_values[iter->second];
}

inline bool Model::has_param(std::string_view pname) const
{
    return param_index.find(pname) != param_index.end();
}

inline void Model::set_param(std::string_view pname, epiworld_double value)
{
    auto iter = param_index.find(pname);
    if (iter == param_index.end())
        throw std::logic_error(
            "The parameter '" + std::string(pname) + "' does not exists."
        );

    param_values[iter->second] = value;

    return;

}

inline epiworld_double Model::par(std::string_view pname) const
{
    const auto iter = param_index.find(pname);
    if (iter == param_index.end())
        throw std::logic_error(
            "The parameter '" + std::string(pname) + "' does not exists."
        );
    return param_values[iter->second];
}

inline ParamId Model::get_param_id(std::string_view pname) const
{
    const auto iter = param_index.find(pname);
    if (iter == param_index.end())
        throw std::logic_error(
            "The parameter '" + std::string(pname) + "' does not exists."
        );
    return ParamId{iter->second};
}

inline epiworld_double Model::par_at(ParamId id) const
{
    if (id.idx >= param_values.size())
        throw std::out_of_range(
            "The parameter position " + std::to_string(id.idx) +
            " is out of range (the model has " +
            std::to_string(param_values.size()) + " parameters)."
        );
    return param_values[id.idx];
}

inline void Model::set_param_at(ParamId id, epiworld_double value)
{
    if (id.idx >= param_values.size())
        throw std::out_of_range(
            "The parameter position " + std::to_string(id.idx) +
            " is out of range (the model has " +
            std::to_string(param_values.size()) + " parameters)."
        );
    param_values[id.idx] = value;
}

#define DURCAST(tunit,txtunit) {\
        elapsed       = std::chrono::duration_cast<std::chrono:: tunit>(\
            time_end - time_start).count(); \
        elapsed_total = std::chrono::duration_cast<std::chrono:: tunit>(time_elapsed).count(); \
        abbr_unit     = txtunit;}

inline void Model::get_elapsed(
    std::string unit,
    epiworld_double * last_elapsed,
    epiworld_double * total_elapsed,
    std::string * unit_abbr,
    bool print
) const {

    // Preparing the result
    epiworld_double elapsed, elapsed_total;
    std::string abbr_unit;

    // Figuring out the length
    if (unit == "auto")
    {

        size_t tlength = std::to_string(
            static_cast<int>(floor(time_elapsed.count()))
            ).length();

        if (tlength <= 1)
            unit = "nanoseconds";
        else if (tlength <= 3)
            unit = "microseconds";
        else if (tlength <= 6)
            unit = "milliseconds";
        else if (tlength <= 8)
            unit = "seconds";
        else if (tlength <= 9)
            unit = "minutes";
        else
            unit = "hours";

    }

    if (unit == "nanoseconds")       DURCAST(nanoseconds,"ns")
    else if (unit == "microseconds") DURCAST(microseconds,"\xC2\xB5s")
    else if (unit == "milliseconds") DURCAST(milliseconds,"ms")
    else if (unit == "seconds")      DURCAST(seconds,"s")
    else if (unit == "minutes")      DURCAST(minutes,"m")
    else if (unit == "hours")        DURCAST(hours,"h")
    else
        throw std::range_error("The time unit " + unit + " is not supported.");


    if (last_elapsed != nullptr)
        *last_elapsed = elapsed;
    if (total_elapsed != nullptr)
        *total_elapsed = elapsed_total;
    if (unit_abbr != nullptr)
        *unit_abbr = abbr_unit;

    if (!print)
        return;

    if (n_replicates > 1u)
    {
        printf_epiworld("last run elapsed time : %.2f%s\n",
            elapsed, abbr_unit.c_str());
        printf_epiworld("total elapsed time    : %.2f%s\n",
            elapsed_total, abbr_unit.c_str());
        printf_epiworld("total runs            : %i\n",
            static_cast<int>(n_replicates));
        printf_epiworld("mean run elapsed time : %.2f%s\n",
            elapsed_total/static_cast<epiworld_double>(n_replicates), abbr_unit.c_str());

    } else {
        printf_epiworld("last run elapsed time : %.2f%s.\n", elapsed, abbr_unit.c_str());
    }
}

inline void Model::set_user_data(std::vector< std::string > names)
{
    db.set_user_data(names);
}

inline void Model::add_user_data(epiworld_fast_uint j, epiworld_double x)
{
    db.add_user_data(j, x);
}

inline void Model::add_user_data(std::vector<epiworld_double> x)
{
    db.add_user_data(x);
}

inline UserData & Model::get_user_data()
{
    return db.get_user_data();
}

inline void Model::add_globalevent(
    std::function<void(Model*)> fun,
    std::string name,
    int date
)
{
    auto event = GlobalEvent(fun, name, date);
    add_globalevent(event);

}

inline void Model::add_globalevent(
    GlobalEvent & action
)
{
    auto ptr = action.clone_ptr();
    globalevents.push_back(GlobalEventPtr(std::move(ptr)));
}

inline GlobalEvent & Model::get_globalevent(
    std::string name
)
{

    for (auto & a : globalevents)
        if (a->get_name() == name)
            return *a;

    throw std::logic_error("The global action " + name + " was not found.");

}

inline bool Model::has_globalevent(std::string_view name) const
{
    for (const auto & a : globalevents)
        if (a->get_name() == name)
            return true;
    return false;
}

inline GlobalEvent & Model::get_globalevent(
    size_t index
)
{

    if (index >= globalevents.size())
        throw std::range_error("The index " + std::to_string(index) + " is out of range.");

    return *globalevents[index];

}

// Remove implementation
inline void Model::rm_globalevent(
    std::string name
)
{

    for (auto it = globalevents.begin(); it != globalevents.end(); ++it)
    {
        if ((*it)->get_name() == name)
        {
            globalevents.erase(it);
            return;
        }
    }

    throw std::logic_error("The global action " + name + " was not found.");

}

// Same as above, but the index implementation
inline void Model::rm_globalevent(
    size_t index
)
{

    if (index >= globalevents.size())
        throw std::range_error("The index " + std::to_string(index) + " is out of range.");

    globalevents.erase(globalevents.begin() + index);

}

inline size_t Model::get_n_globalevents() const
{
    return globalevents.size();
}

inline void Model::run_globalevents()
{

    // An infection added by an event is not one drawn from a layer
    sampled_layer = -1;

    for (auto & event: globalevents)
    {
        event->operator()(this, today());
        events_run();
    }    

}

inline void Model::add_layer_ptr(
    std::shared_ptr< ContactLayer > layer
)
{

    if (!layer)
        throw std::invalid_argument("Cannot add a null contact layer.");

    if (layer->model != nullptr)
        throw std::logic_error(
            "The contact layer '" + layer->get_name() + "' already belongs to "
            "a model; a layer belongs to one model at most (add a copy)."
        );

    // Layers are part of the model's setup: a run resets them all before day 0
    if (running)
        throw std::logic_error(
            "Cannot add the contact layer '" + layer->get_name() + "' during a "
            "run; add layers before calling run()."
        );

    if (has_layer(layer->get_name()))
        throw std::logic_error(
            "There is already a contact layer named '" + layer->get_name() + "'."
        );

    layers.push_back(std::move(layer));
    layers_attach();

}

inline size_t Model::get_n_layers() const
{
    return layers.size();
}

inline bool Model::has_layer(std::string_view name) const
{
    for (const auto & layer : layers)
        if (layer->get_name() == name)
            return true;

    return false;
}

inline size_t Model::get_layer_id(std::string_view name) const
{
    for (const auto & layer : layers)
        if (layer->get_name() == name)
            return layer->get_id();

    throw std::out_of_range(
        "There is no contact layer named '" + std::string(name) + "'."
    );
}

inline ContactLayer & Model::get_layer(size_t id)
{
    if (id >= layers.size())
        throw std::out_of_range(
            "Contact layer " + std::to_string(id) + " is out of range (the model has " +
            std::to_string(layers.size()) + ")."
        );

    return *layers[id];
}

inline ContactLayer & Model::get_layer(std::string_view name)
{
    return *layers[get_layer_id(name)];
}

inline void Model::set_running(bool value)
{
    running = value;
    for (auto & layer : layers)
        layer->in_run = value;
}

inline void Model::update_layers()
{
    for (auto & layer : layers)
        layer->update(*this);
}

inline void Model::layers_attach()
{

    network_layers.clear();
    for (size_t i = 0u; i < layers.size(); ++i)
    {
        layers[i]->id = i;
        layers[i]->model = this;
        if (auto * net = layers[i]->as_network())
            network_layers.push_back(net);
    }

    layers_classify();

}

inline void Model::layers_detach()
{

    for (auto & layer : layers)
    {
        layer->model = nullptr;
        layer->in_run = false;
    }

    network_layers.clear();
    group_layers.clear();

}

inline size_t Model::network_degree(size_t id) const
{

    size_t deg = 0u;
    for (const auto * net : network_layers)
        if (id < net->ties.size())
            deg += net->ties[id].size();

    return deg;

}

inline void Model::layers_classify()
{

    pull_only = false;
    queue_blind = false;
    group_layers.clear();
    for (const auto & layer : layers)
    {
        const auto * net = layer->as_network();
        pull_only = pull_only || (net == nullptr) || net->is_directed();
        queue_blind = queue_blind || !layer->supports_queue();

        auto * mix = layer->as_mixing();
        if ((mix != nullptr) && mix->supports_queue())
            group_layers.push_back(mix);
    }

}

inline void Model::network_tie_changed(
    size_t i, int di,
    size_t j, int dj
)
{

    state_index_degree(i, di);
    state_index_degree(j, dj);

    // Each end that gained (lost) the tie now credits (no longer credits) the
    // other end's registrations
    if (use_queuing)
        queue.notify_ties_changed(i, di, j, dj);

}

inline void Model::network_ties_swapped(
    size_t i, size_t j,
    size_t k, size_t l,
    bool directed
)
{

    // Nobody's degree changed, so the degree sums need nothing
    if (use_queuing)
        queue.notify_edges_swapped(i, j, k, l, directed);

}

inline void Model::group_member_moved(
    size_t i,
    const std::vector< size_t > * from,
    const std::vector< size_t > * to
)
{

    if (use_queuing)
        queue.notify_group_changed(i, from, to);

}

inline void Model::queuing_on()
{
    use_queuing = true;
}

inline Model & Model::queuing_off()
{
    use_queuing = false;
    return *this;
}

inline bool Model::is_queuing_on() const
{
    return use_queuing;
}

inline Queue & Model::get_queue()
{
    return queue;
}

inline const std::vector< VirusPtr > & Model::get_viruses() const
{
    return viruses;
}

inline const std::vector< ToolPtr > & Model::get_tools() const
{
    return tools;
}

inline Virus & Model::get_virus(size_t id)
{

    if (viruses.size() <= id)
        throw std::length_error("The specified id for the virus is out of range");

    return *viruses[id];

}

inline Virus & Model::get_virus(std::string_view name)
{

    for (auto & v : viruses)
        if (v->get_name() == name)
            return *v;

    throw std::logic_error("The virus " + std::string(name) + " was not found.");

}

inline Tool & Model::get_tool(size_t id)
{

    if (tools.size() <= id)
        throw std::length_error("The specified id for the tools is out of range");

    return *tools[id];

}

inline Tool & Model::get_tool(std::string_view name)
{
    for (auto & t : tools)
        if (t->get_name() == name)
            return *t;

    throw std::logic_error("The tool " + std::string(name) + " was not found.");

}

inline bool Model::has_virus(std::string_view name) const
{
    for (const auto & v : viruses)
        if (v->get_name() == name)
            return true;

    return false;
}

inline bool Model::has_tool(std::string_view name) const
{
    for (const auto & t : tools)
        if (t->get_name() == name)
            return true;
    return false;
}


inline void Model::set_agents_data(double * data_, size_t ncols_)
{
    agents_data = data_;
    agents_data_ncols = ncols_;
}

inline double * Model::get_agents_data() {
    return this->agents_data;
}

inline size_t Model::get_agents_data_ncols() const {
    return this->agents_data_ncols;
}


inline void Model::set_name(std::string name)
{
    this->name = name;
}

inline std::string Model::get_name() const
{
    return this->name;
}

#define VECT_MATCH(a, b, c) \
    EPI_DEBUG_FAIL_AT_TRUE(a.size() != b.size(), c) \
    for (size_t __i = 0u; __i < a.size(); ++__i) \
    {\
        EPI_DEBUG_FAIL_AT_TRUE(a[__i] != b[__i], c) \
    }

inline bool Model::operator==(const Model & other) const
{
    EPI_DEBUG_FAIL_AT_TRUE(name != other.name, "names don't match")
    EPI_DEBUG_FAIL_AT_TRUE(db != other.db, "database don't match")

    VECT_MATCH(population, other.population, "population doesn't match")

    EPI_DEBUG_FAIL_AT_TRUE(
        agents_data != other.agents_data,
        "Model:: agents_data don't match"
    )

    EPI_DEBUG_FAIL_AT_TRUE(
        agents_data_ncols != other.agents_data_ncols,
        "Model:: agents_data_ncols don't match"
    )

    // Viruses -----------------------------------------------------------------
    EPI_DEBUG_FAIL_AT_TRUE(
        viruses.size() != other.viruses.size(),
        "Model:: viruses.size() don't match"
        )

    for (size_t i = 0u; i < viruses.size(); ++i)
    {
        EPI_DEBUG_FAIL_AT_TRUE(
            *viruses[i] != *other.viruses[i],
            "Model:: *viruses[i] don't match"
        )

    }

    // Tools -------------------------------------------------------------------
    EPI_DEBUG_FAIL_AT_TRUE(
        tools.size() != other.tools.size(),
        "Model:: tools.size() don't match"
        )

    for (size_t i = 0u; i < tools.size(); ++i)
    {
        EPI_DEBUG_FAIL_AT_TRUE(
            *tools[i] != *other.tools[i],
            "Model:: *tools[i] don't match"
        )

    }

    EPI_DEBUG_FAIL_AT_TRUE(
        param_values.size() != other.param_values.size(),
        "Model:: () don't match"
    )

    EPI_DEBUG_FAIL_AT_TRUE(
        // By name, so the order parameters were added in does not matter
        params() != other.params(),
        "Model:: parameters don't match"
    )

    EPI_DEBUG_FAIL_AT_TRUE(
        ndays != other.ndays,
        "Model:: ndays don't match"
    )

    VECT_MATCH(
        states_labels,
        other.states_labels,
        "state labels don't match"
    )

    EPI_DEBUG_FAIL_AT_TRUE(
        nstates != other.nstates,
        "Model:: nstates don't match"
    )

    EPI_DEBUG_FAIL_AT_TRUE(
        verbose != other.verbose,
        "Model:: verbose don't match"
    )

    EPI_DEBUG_FAIL_AT_TRUE(
        current_date != other.current_date,
        "Model:: current_date don't match"
    )

    // Global events are held by pointer and deep-copied when a model is copied,
    // so they must be compared through the pointer (as viruses and tools are).
    EPI_DEBUG_FAIL_AT_TRUE(
        globalevents.size() != other.globalevents.size(),
        "Model:: globalevents.size() don't match"
    )

    for (size_t i = 0u; i < globalevents.size(); ++i)
    {
        EPI_DEBUG_FAIL_AT_TRUE(
            *globalevents[i] != *other.globalevents[i],
            "Model:: *globalevents[i] don't match"
        )
    }

    EPI_DEBUG_FAIL_AT_TRUE(
        queue != other.queue,
        "Model:: queue don't match"
    )


    EPI_DEBUG_FAIL_AT_TRUE(
        use_queuing != other.use_queuing,
        "Model:: use_queuing don't match"
    )

    EPI_DEBUG_FAIL_AT_TRUE(
        transmission_mode != other.transmission_mode,
        "Model:: transmission_mode don't match"
    )

    EPI_DEBUG_FAIL_AT_TRUE(
        transmission_kappa != other.transmission_kappa,
        "Model:: transmission_kappa don't match"
    )

    return true;

}

inline void Model::draw(
    DiagramType diagram_type,
    const std::string & fn_output,
    bool self
) {

    ModelDiagram diagram;

    diagram.draw_from_data(
        diagram_type,
        states_labels,
        this->get_db().get_transition_probability(false),
        fn_output,
        self
    );

    return;

}

inline void Model::record_hospitalization(Agent & agent)
{
    db.record_hospitalization(agent);
}

inline void Model::get_hospitalizations(
    std::vector<int> & date,
    std::vector<int> & virus_id,
    std::vector<int> & tool_id,
    std::vector<int> & count,
    std::vector<double> & weight
) const
{
    db.get_hospitalizations(date, virus_id, tool_id, count, weight);
}

#undef VECT_MATCH
#undef DURCAST
#undef CASES_PAR
#undef CASE_PAR
#undef CHECK_INIT
#endif
/*//////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

 End of -./include/epiworld/model-meat.hpp-

////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////*/


/*//////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

 Start of -./include/epiworld/postsampling-meat.hpp-

////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////*/


#ifndef EPIWORLD_POSTSAMPLING_MEAT_HPP
#define EPIWORLD_POSTSAMPLING_MEAT_HPP

inline Model & Model::set_post_sampling(PostSamplingFun fun)
{

    post_sampling_fun = std::move(fun);
    post_sampling_on = static_cast< bool >(post_sampling_fun);

    // A model that already has agents sizes the scratch now
    if (post_sampling_on)
        post_sampling_prepare_scratch();

    return *this;

}

/**
 * Sizes the scratch for the current population. Copies, moves and assignments
 * of a model leave the scratch empty, so this is also what makes them safe to
 * continue stepping without a reset.
 */
inline void Model::post_sampling_prepare_scratch()
{

    if (post_sampling_scratch.counts.size() == population.size())
        return;

    // In 64 bits: on wasm32, size_t is 32 bits and shifting it by 32 is UB
    if (static_cast< uint64_t >(population.size()) >= (uint64_t(1) << 32))
        throw std::length_error(
            "The post-sampling callback supports populations below 2^32 agents."
        );

    // Only `counts` (all zero between steps): the pairs of this step are kept
    post_sampling_scratch.touched.clear();
    post_sampling_scratch.counts.assign(population.size(), 0u);

}

inline Model & Model::clear_post_sampling()
{
    return set_post_sampling(nullptr);
}

inline void Model::register_sampled_contact(
    size_t infectious_id,
    size_t contacted_id
)
{
    post_sampling_scratch.pairs.push_back(static_cast< uint32_t >(infectious_id));
    post_sampling_scratch.pairs.push_back(static_cast< uint32_t >(contacted_id));
}

inline void Model::register_sampled_contacts(
    const size_t * infectious_ids,
    size_t n,
    size_t contacted_id
)
{
    auto & pairs = post_sampling_scratch.pairs;
    for (size_t k = 0u; k < n; ++k)
    {
        pairs.push_back(static_cast< uint32_t >(infectious_ids[k]));
        pairs.push_back(static_cast< uint32_t >(contacted_id));
    }
}

/**
 * Groups the pairs of the step by infectious agent (only the distinct ids are
 * sorted, never an N-sized array) and runs the callback on each group, in
 * ascending id order. The scratch is left clean even if the callback throws.
 */
inline void Model::post_sampling_dispatch()
{

    auto & sc = post_sampling_scratch;
    const size_t npairs = sc.pairs.size() / 2u;

    if (npairs == 0u)
        return;

    // The callback gets the model: it can clear or replace the callback. The
    // whole dispatch runs on a snapshot, so that takes effect next step.
    const PostSamplingFun fun = post_sampling_fun;

    post_sampling_prepare_scratch();

    try
    {

        // A push visits the carriers in ascending id order, so its pairs are
        // already grouped by infectious agent: no counting, no sorting.
        bool grouped_already = true;
        for (size_t k = 1u; k < npairs; ++k)
            if (sc.pairs[2u * k] < sc.pairs[2u * (k - 1u)])
            {
                grouped_already = false;
                break;
            }

        const size_t nt = [&]() -> size_t {

            sc.grouped.resize(npairs);

            if (grouped_already)
            {

                for (size_t k = 0u; k < npairs; ++k)
                {

                    const uint32_t i = sc.pairs[2u * k];
                    if ((k == 0u) || (i != sc.pairs[2u * (k - 1u)]))
                    {
                        sc.touched.push_back(i);
                        sc.starts.push_back(k);
                    }

                    sc.grouped[k] = sc.pairs[2u * k + 1u];

                }

                sc.starts.push_back(npairs);
                return sc.touched.size();

            }

            // Counting per infectious agent, remembering the distinct ones
            for (size_t k = 0u; k < npairs; ++k)
            {
                const uint32_t i = sc.pairs[2u * k];
                if (sc.counts[i]++ == 0u)
                    sc.touched.push_back(i);
            }

            std::sort(sc.touched.begin(), sc.touched.end());

            // Turning the counts into start positions
            const size_t nd = sc.touched.size();
            sc.starts.resize(nd + 1u);
            size_t pos = 0u;
            for (size_t t = 0u; t < nd; ++t)
            {
                const uint32_t i = sc.touched[t];
                sc.starts[t] = pos;
                pos += sc.counts[i];
                sc.counts[i] = static_cast< uint32_t >(sc.starts[t]);
            }
            sc.starts[nd] = pos;

            // Scattering the contacted ids
            for (size_t k = 0u; k < npairs; ++k)
                sc.grouped[sc.counts[sc.pairs[2u * k]]++] = sc.pairs[2u * k + 1u];

            return nd;

        }();

        // Callbacks must not retain the view: the buffers are reused.
        for (size_t t = 0u; t < nt; ++t)
        {

            const size_t len = sc.starts[t + 1u] - sc.starts[t];
            SampledContactsView view(sc.grouped.data() + sc.starts[t], len);
            fun(&population[sc.touched[t]], view, this);

        }

    }
    catch (...)
    {
        sc.clear();
        throw;
    }

    sc.clear();

}

#endif
/*//////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

 End of -./include/epiworld/postsampling-meat.hpp-

////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////*/


/*//////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

 Start of -./include/epiworld/virus-meat.hpp-

////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////*/


#ifndef EPIWORLD_VIRUS_MEAT_HPP
#define EPIWORLD_VIRUS_MEAT_HPP

// (already included include/epiworld/config.hpp)

inline Virus::Virus()
{

    #ifdef EPI_DEBUG_VIRUS
    counter_construct++;
    #endif

}

inline Virus::Virus(
    std::string name
    ) {

    #ifdef EPI_DEBUG_VIRUS
    counter_construct++;
    #endif
    
    set_name(name);
}

#ifdef EPI_DEBUG_VIRUS
inline std::atomic<int> Virus::counter_construct = 0;

inline std::atomic<int> Virus::counter_copy_construct = 0;

inline std::atomic<int> Virus::counter_move_construct = 0;

inline std::atomic<int> Virus::counter_copy_assign = 0;

inline std::atomic<int> Virus::counter_move_assign = 0;

inline std::atomic<int> Virus::counter_destruct = 0;

inline Virus::~Virus()
{
    counter_destruct++;
}

// Copy constructor
inline Virus::Virus(const Virus& other)
    : agent(other.agent),
      virus_name(other.virus_name),
      date(other.date),
      id(other.id),
      lineage_id(other.lineage_id),
      lineage_bit(other.lineage_bit),
      param_redirects(other.param_redirects),
      nh_def(other.nh_def)
{
    counter_copy_construct++;
}

// Move constructor
inline Virus::Virus(Virus&& other) noexcept
    : agent(other.agent),
      virus_name(std::move(other.virus_name)),
      date(other.date),
      id(other.id),
      lineage_id(other.lineage_id),
      lineage_bit(other.lineage_bit),
      param_redirects(std::move(other.param_redirects)),
      nh_def(std::move(other.nh_def))
{
    counter_move_construct++;
    // other.agent = nullptr;
}

// Copy assignment
inline Virus& Virus::operator=(const Virus& other)
{
    if (this != &other) {
        agent = other.agent;
        virus_name = other.virus_name;
        date = other.date;
        id = other.id;
        lineage_id = other.lineage_id;
        lineage_bit = other.lineage_bit;
        param_redirects = other.param_redirects;
        nh_def = other.nh_def;
        counter_copy_assign++;
    }
    return *this;
}

// Move assignment
inline Virus& Virus::operator=(Virus&& other) noexcept
{
    if (this != &other) {
        agent = other.agent;
        virus_name = std::move(other.virus_name);
        date = other.date;
        id = other.id;
        lineage_id = other.lineage_id;
        lineage_bit = other.lineage_bit;
        param_redirects = std::move(other.param_redirects);
        nh_def = std::move(other.nh_def);
        other.agent = nullptr;
        counter_move_assign++;
    }
    return *this;
}
#endif

inline void Virus::mutate(
    Model * model
) {

    if (mutation)
        if (mutation(agent, *this, model))
        {
            check_params(*model);
            model->get_db().record_virus(*this);
        }

    return;
    
}

inline void Virus::set_mutation(
    MutFun fun
) {
    mutation = MutFun(fun);
}


inline Agent * Virus::get_agent()
{

    return agent;

}

inline void Virus::set_agent(Agent * p)
{
    agent = p;
}

inline void Virus::set_id(int idx)
{

    id = idx;
    return;

}

inline int Virus::get_id() const
{
    
    return id;

}

inline int Virus::get_lineage_id() const
{
    return lineage_id;
}

inline void Virus::set_date(int d) 
{

    date = d;
    return;

}

inline int Virus::get_date() const
{
    
    return date;
    
}

inline void Virus::set_name(std::string name)
{

    virus_name = name;

}

inline std::string Virus::get_name() const
{

    return virus_name;

}

inline bool Virus::operator==(const Virus & other) const
{
    
    EPI_DEBUG_FAIL_AT_TRUE(
        virus_name != other.virus_name,
        "Virus:: virus_name don't match"
    )
    






    return true;

}

inline void Virus::print() const
{

    printf_epiworld("Virus         : %s\n", virus_name.c_str());
    printf_epiworld("Id            : %s\n", (id < 0)? std::string("(empty)").c_str() : std::to_string(id).c_str());

}

inline std::unique_ptr<Virus> Virus::clone_ptr() const
{
    auto cloned = std::make_unique<Virus>(*this);
    return cloned;
}

inline void Virus::use_param(std::string param, std::string replacement)
{
    for (auto & r : param_redirects)
        if (r.first.name() == param)
        {
            r.second = ParamRef(std::move(replacement));
            return;
        }

    param_redirects.emplace_back(
        ParamRef(std::move(param)), ParamRef(std::move(replacement))
    );
}

inline ParamId Virus::param_redirect(
    ParamId id,
    const Model & model
) const
{
    for (const auto & r : param_redirects)
        if (r.first.id(model).idx == id.idx)
            return r.second.id(model);

    return id;
}

inline NaturalHistory & Virus::nh_edit()
{
    // A virus in a model (its copy there, or an agent's) keeps the model's
    // stages and transitions; a strain changes parameters instead
    if (id >= 0)
        throw std::logic_error(
            "The virus \"" + virus_name + "\" is in a model: its natural "
            "history cannot change (a strain reads other parameters with "
            "use_param())."
        );

    // Copies share the natural history until one of them is edited
    if (!nh_def)
        nh_def = std::make_shared< NaturalHistory >(virus_name);
    else if (nh_def.use_count() > 1)
        nh_def = std::make_shared< NaturalHistory >(*nh_def);

    return *nh_def;
}

inline int Virus::add_stage(std::string label, unsigned int flags)
{
    return nh_edit().add_stage(std::move(label), flags);
}

inline void Virus::set_contact(int s, epiworld_double weight)
{
    nh_edit().set_contact(s, weight);
}

inline TransitionRef Virus::infection(
    Nodes from, int to, Value per_contact
)
{
    return nh_edit().infection(std::move(from), to, std::move(per_contact));
}

inline TransitionRef Virus::rate(
    Nodes from, int to, Value daily_probability
)
{
    return nh_edit().rate(std::move(from), to, std::move(daily_probability));
}

inline TransitionRef Virus::after(int from, int to, Duration d)
{
    return nh_edit().after(from, to, std::move(d));
}

inline TransitionRef Virus::external(
    Nodes from, int to, std::string label
)
{
    return nh_edit().external(std::move(from), to, std::move(label));
}

inline void Virus::check_params(const Model & model) const
{
    for (const auto & r : param_redirects)
        if (!model.has_param(r.second.name()))
            throw std::logic_error(
                "The virus \"" + virus_name + "\" reads the parameter \"" +
                r.second.name() + "\" in place of \"" + r.first.name() +
                "\", but the model has no such parameter (add it with "
                "add_param())."
            );
}

#endif
/*//////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

 End of -./include/epiworld/virus-meat.hpp-

////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////*/


/*//////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

 Start of -./include/epiworld/tool-meat.hpp-

////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////*/



#ifndef EPIWORLD_TOOLS_MEAT_HPP
#define EPIWORLD_TOOLS_MEAT_HPP

/**
 * @brief Factory function of ToolFun base on logit
 * 
 * @param vars Vector indicating the position of the variables to use.
 * @param coefs Vector of coefficients.
 * @return ToolFun 
 */
inline ToolFun tool_fun_logit(
    std::vector< int > vars,
    std::vector< double > coefs,
    Model * model
) {

    // Checking that there are features
    if (coefs.size() == 0u)
        throw std::logic_error(
            "The -coefs- argument should feature at least one element."
            );

    if (coefs.size() != vars.size())
        throw std::length_error(
            std::string("The length of -coef- (") +
            std::to_string(coefs.size()) + 
            std::string(") and -vars- (") +
            std::to_string(vars.size()) +
            std::string(") should match. ")            
            );

    // Checking that there are variables in the model
    if (model != nullptr)
    {

        size_t K = model->get_agents_data_ncols();
        for (const auto & var: vars)
        {
            if ((var >= static_cast<int>(K)) | (var < 0))
                throw std::range_error(
                    std::string("The variable ") +
                    std::to_string(var) +
                    std::string(" is out of range.") +
                    std::string(" The agents only feature ") +
                    std::to_string(K) + 
                    std::string("variables (features).")
                );
        }
        
    }

    std::vector< epiworld_double > coefs_f;
    for (auto c: coefs)
        coefs_f.push_back(static_cast<epiworld_double>(c));

    ToolFun fun_ = [coefs_f,vars](
        Tool&,
        Agent * agent,
        VirusPtr &,
        Model * model
        ) -> epiworld_double {

        size_t K = coefs_f.size();
        epiworld_double res = 0.0;

        #if defined(__OPENMP) || defined(_OPENMP)
        #pragma omp simd reduction(+:res)
        #endif
        for (size_t i = 0u; i < K; ++i)
            res += agent->operator()(vars.at(i), *model) * coefs_f.at(i);

        return 1.0/(1.0 + std::exp(-res));

    };

    return fun_;

}

inline Tool::Tool()
{
    set_name("Tool");
}

inline Tool::Tool(std::string name)
{
    set_name(name);
}

inline epiworld_double Tool::get_susceptibility_reduction(
    VirusPtr & v,
    Model * model
)
{

    if (susceptibility_reduction)
        return susceptibility_reduction(
            *this, this->agent, v, model
        );

    return DEFAULT_TOOL_CONTAGION_REDUCTION;

}

inline epiworld_double Tool::get_transmission_reduction(
    VirusPtr & v,
    Model * model
)
{

    if (transmission_reduction)
        return transmission_reduction(
            *this, this->agent, v, model
        );

    return DEFAULT_TOOL_TRANSMISSION_REDUCTION;

}

inline epiworld_double Tool::get_recovery_enhancer(
    VirusPtr & v,
    Model * model
)
{

    if (recovery_enhancer)
        return recovery_enhancer(*this, this->agent, v, model);

    return DEFAULT_TOOL_RECOVERY_ENHANCER;

}

inline epiworld_double Tool::get_death_reduction(
    VirusPtr & v,
    Model * model
)
{

    if (death_reduction)
        return death_reduction(*this, this->agent, v, model);

    return DEFAULT_TOOL_DEATH_REDUCTION;

}

inline void Tool::set_susceptibility_reduction_fun(
    ToolFun fun
)
{
    susceptibility_reduction = fun;
}

inline void Tool::set_transmission_reduction_fun(
    ToolFun fun
)
{
    transmission_reduction = fun;
}

inline void Tool::set_recovery_enhancer_fun(
    ToolFun fun
)
{
    recovery_enhancer = fun;
}

inline void Tool::set_death_reduction_fun(
    ToolFun fun
)
{
    death_reduction = fun;
}

inline void Tool::set_susceptibility_reduction(std::string param)
{

    auto param_ref = std::make_shared< const ParamRef >(std::move(param));

    ToolFun tmpfun =
        [param_ref](Tool &, Agent *, VirusPtr&, Model* model)
        {
            return (*param_ref)(*model);
        };

    susceptibility_reduction = tmpfun;

}

// EPIWORLD_SET_LAMBDA(susceptibility_reduction)
inline void Tool::set_transmission_reduction(std::string param)
{

    auto param_ref = std::make_shared< const ParamRef >(std::move(param));
    
    ToolFun tmpfun =
        [param_ref](Tool &, Agent *, VirusPtr&, Model* model)
        {
            return (*param_ref)(*model);
        };

    transmission_reduction = tmpfun;

}

// EPIWORLD_SET_LAMBDA(transmission_reduction)
inline void Tool::set_recovery_enhancer(std::string param)
{

    auto param_ref = std::make_shared< const ParamRef >(std::move(param));

    ToolFun tmpfun =
        [param_ref](Tool &, Agent *, VirusPtr&, Model* model)
        {
            return (*param_ref)(*model);
        };

    recovery_enhancer = tmpfun;

}

// EPIWORLD_SET_LAMBDA(recovery_enhancer)
inline void Tool::set_death_reduction(std::string param)
{

    auto param_ref = std::make_shared< const ParamRef >(std::move(param));

    ToolFun tmpfun =
        [param_ref](Tool &, Agent *, VirusPtr&, Model* model)
        {
            return (*param_ref)(*model);
        };

    death_reduction = tmpfun;

}

// EPIWORLD_SET_LAMBDA(death_reduction)

// #undef EPIWORLD_SET_LAMBDA
inline void Tool::set_susceptibility_reduction(
    epiworld_double prob
)
{

    ToolFun tmpfun = 
        [prob](Tool &, Agent *, VirusPtr&, Model *)
        {
            return prob;
        };

    susceptibility_reduction = tmpfun;

}

inline void Tool::set_transmission_reduction(
    epiworld_double prob
)
{

    ToolFun tmpfun = 
        [prob](Tool &, Agent *, VirusPtr&, Model *)
        {
            return prob;
        };

    transmission_reduction = tmpfun;

}

inline void Tool::set_recovery_enhancer(
    epiworld_double prob
)
{

    ToolFun tmpfun = 
        [prob](Tool &, Agent *, VirusPtr&, Model *)
        {
            return prob;
        };

    recovery_enhancer = tmpfun;

}

inline void Tool::set_death_reduction(
    epiworld_double prob
)
{

    ToolFun tmpfun = 
        [prob](Tool &, Agent *, VirusPtr&, Model *)
        {
            return prob;
        };

    death_reduction = tmpfun;

}

inline void Tool::set_name(std::string name)
{
    tool_name = name;
}

inline std::string Tool::get_name() const {

    return tool_name;

}

inline uint64_t Tool::target_bit(int lineage_id)
{

    if ((lineage_id < 0) || (lineage_id >= 63))
        throw std::range_error(
            std::string("The virus lineage id ") +
            std::to_string(lineage_id) +
            std::string(" cannot be targeted. Only lineages 0 to 62 can be ") +
            std::string("targeted.")
        );

    return uint64_t(1) << lineage_id;

}

inline void Tool::add_target(int lineage_id)
{

    uint64_t bit = target_bit(lineage_id);

    // The first target replaces the default (every virus)
    if (target_mask == ~uint64_t(0))
        target_mask = 0u;

    target_mask |= bit;

}

inline void Tool::add_target(const Virus & v)
{

    if (v.get_lineage_id() < 0)
        throw std::logic_error(
            std::string("The virus \"") + v.get_name() +
            std::string("\" has no lineage id. Add it to the model with ") +
            std::string("Model::add_virus() before targeting it.")
        );

    add_target(v.get_lineage_id());

}

inline void Tool::set_targets(const std::vector< int > & lineage_ids)
{

    // Validate every id before touching the current targets
    uint64_t mask = lineage_ids.empty() ? ~uint64_t(0) : 0u;
    for (auto id : lineage_ids)
        mask |= target_bit(id);

    target_mask = mask;

}

inline std::vector< int > Tool::get_targets() const
{

    std::vector< int > res;
    if (target_mask == ~uint64_t(0))
        return res;

    for (int i = 0; i < 63; ++i)
        if (target_mask & (uint64_t(1) << i))
            res.push_back(i);

    return res;

}

inline void Tool::clear_targets()
{
    target_mask = ~uint64_t(0);
}

inline bool Tool::targets(const Virus & v) const
{
    return (target_mask & v.lineage_bit) != 0u;
}

inline Agent * Tool::get_agent()
{
    return this->agent;
}

inline void Tool::set_agent(Agent * p, size_t idx)
{
    agent        = p;
    pos_in_agent = static_cast<int>(idx);
}

inline int Tool::get_id() const {
    return id;
}


inline void Tool::set_id(int id)
{
    this->id = id;
}

inline void Tool::set_date(int d)
{
    this->date = d;
}

inline int Tool::get_date() const
{
    return date;
}

inline bool Tool::operator==(const Tool & other) const
{

    if (tool_name != other.tool_name)
        return false;
    




    if (target_mask != other.target_mask)
        return false;

    return true;

}


inline void Tool::print() const
{

    printf_epiworld("Tool       : %s\n", this->get_name().c_str());
    printf_epiworld("Id         : %s\n", (id < 0)? std::string("(empty)").c_str() : std::to_string(id).c_str());

    if (target_mask != ~uint64_t(0))
    {
        std::string tgts;
        for (auto i : get_targets())
            tgts += (tgts.empty() ? "" : ", ") + std::to_string(i);
        printf_epiworld("targets    : %s\n", tgts.c_str());
    }

}

inline std::unique_ptr<Tool> Tool::clone_ptr() const
{
    auto cloned = std::make_unique<Tool>(*this);
    return cloned;
}

#endif
/*//////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

 End of -./include/epiworld/tool-meat.hpp-

////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////*/


/*//////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

 Start of -./include/epiworld/agent-meat.hpp-

////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////*/


#ifndef EPIWORLD_PERSON_MEAT_HPP
#define EPIWORLD_PERSON_MEAT_HPP

#include <vector>
#include <string>
// (already included include/epiworld/config.hpp)
// (already included include/epiworld/epiworld-macros.hpp)
// (already included include/epiworld/agent-bones.hpp)
/*//////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

 Start of -include/epiworld/agent-events-meat.hpp-

////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////*/


#ifndef EPIWORLD_AGENT_EVENTS_MEAT_HPP
#define EPIWORLD_AGENT_EVENTS_MEAT_HPP

inline void Model::_event_add_virus(Event & a)
{

    Agent *  p = a.agent;
    VirusPtr & v = a.virus;
    
    db.record_transmission(
        v->get_agent() ? v->get_agent()->get_id() : -1,
        p->get_id(),
        v->get_id(),
        v->get_date(),
        a.layer
    );
    
    p->virus = std::move(v);
    p->virus->set_date(today());
    p->virus->set_agent(p);

    // Change of state needs to be recorded and updated on the
    // tools.
    if ((a.new_state != -99) && (static_cast<int>(p->state) != a.new_state))
    {
        db.update_state(p->state_prev, a.new_state);

        // For tool counts, use current state (p->state) not state_prev
        // because state_prev may be stale if multiple changes occurred today
        for (size_t i = 0u; i < p->tools.size(); ++i)
            db.update_tool(
                p->tools[i]->get_id(),
                p->state,
                a.new_state
            );
    }

    // Lastly, we increase the daily count of the virus
    #ifdef EPI_DEBUG
    db.today_virus.at(p->virus->get_id()).at(
        a.new_state != -99 ? a.new_state : p->state
    )++;
    #else
    db.today_virus[p->virus->get_id()][
        a.new_state != -99 ? a.new_state : p->state
    ]++;
    #endif

}

inline void Model::_event_add_tool(Event & a)
{

    Agent * p = a.agent;
    ToolPtr & t = a.tool;
    
    // Update tool accounting
    p->tools.emplace_back(std::move(t));
    size_t tool_pos = p->tools.size() - 1u;

    p->tools[tool_pos]->set_date(today());
    p->tools[tool_pos]->set_agent(p, tool_pos);

    // Change of state needs to be recorded and updated on the
    // tools.
    if ((a.new_state != -99) && static_cast<int>(p->state) != a.new_state)
    {
        db.update_state(p->state_prev, a.new_state);

        // For virus counts, use current state (p->state) not state_prev
        // because state_prev may be stale if multiple changes occurred today
        if (p->virus)
            db.update_virus(
                p->virus->get_id(),
                p->state,
                a.new_state
            );
    }

    db.today_tool[p->tools[tool_pos]->get_id()][
        a.new_state != -99 ? a.new_state : p->state
    ]++;


}

inline void Model::_event_rm_virus(Event & a)
{

    Agent * p    = a.agent;
    VirusPtr & v = a.virus;

    p->virus = nullptr;

    // Change of state needs to be recorded and updated on the
    // tools.
    if ((a.new_state != -99) && (static_cast<int>(p->state) != a.new_state))
    {
        db.update_state(p->state_prev, a.new_state);

        // For tool counts, use current state (p->state) not state_prev
        // because state_prev may be stale if multiple changes occurred today
        for (size_t i = 0u; i < p->tools.size(); ++i)
            db.update_tool(
                p->tools[i]->get_id(),
                p->state,
                a.new_state
            );
    }

    // The counter of the removed virus must be decremented from the
    // agent's current state (pre-transition), not state_prev.
    // state_prev may refer to an earlier same-day state.
    #ifdef EPI_DEBUG
    db.today_virus.at(v->get_id()).at(p->state)--;
    #else
    db.today_virus[v->get_id()][p->state]--;
    #endif

    
    return;

}

inline void Model::_event_rm_tool(Event & a)
{

    Agent * p = a.agent;
    ToolPtr & t = a.tool;
    bool removed = false;

    if (t)
    {
        // Remove tool(s) by id, following an erase/remove approach.
        auto new_end = std::remove_if(
            p->tools.begin(),
            p->tools.end(),
            [&t](const ToolPtr & tool_ptr) -> bool
            {
                return tool_ptr && (tool_ptr->get_id() == t->get_id());
            }
        );

        removed = (new_end != p->tools.end());
        p->tools.erase(new_end, p->tools.end());

        for (size_t i = 0u; i < p->tools.size(); ++i)
            p->tools[i]->pos_in_agent = static_cast<int>(i);
    }

    // Change of state needs to be recorded and updated on the
    // tools.
    if ((a.new_state != -99) && (static_cast<int>(p->state) != a.new_state))
    {
        db.update_state(p->state_prev, a.new_state);

        // For virus counts, use current state (p->state) not state_prev
        // because state_prev may be stale if multiple changes occurred today
        if (p->virus)
            db.update_virus(
                p->virus->get_id(),
                p->state,
                a.new_state
            );
    }

    // Like rm_virus, remove the tool count from the agent's current
    // state (pre-transition). Using state_prev can underflow when
    // the agent changed state earlier in the day.
    if (removed)
    {
        #ifdef EPI_DEBUG
        db.today_tool.at(t->get_id()).at(p->state)--;
        #else
        db.today_tool[t->get_id()][p->state]--;
        #endif

        t->agent = nullptr;
        t->pos_in_agent = -99;
    }

    return;

}

inline void Model::_event_change_state(Event & a)
{

    Agent * p = a.agent;

    if ((a.new_state != -99) && (static_cast<int>(p->state) != a.new_state))
    {
        db.update_state(p->state_prev, a.new_state);

        // For virus and tool counts, use current state (p->state) not state_prev
        // because state_prev may be stale if multiple changes occurred today
        if (p->virus)
            db.update_virus(
                p->virus->get_id(), p->state, a.new_state
            );

        for (size_t i = 0u; i < p->tools.size(); ++i)
            db.update_tool(
                p->tools[i]->get_id(),
                p->state,
                a.new_state
            );

    }

}

#endif
/*//////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

 End of -include/epiworld/agent-events-meat.hpp-

////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////*/



inline Agent::Agent(Agent && p) :
    state(p.state),
    state_prev(p.state_prev), 
    state_last_changed(p.state_last_changed),
    id(p.id),
    tools(std::move(p.tools)) /// Needs to be adjusted
{

    state = p.state;
    id     = p.id;
    
    // Dealing with the virus
    if (p.virus != nullptr)
    {
        virus = std::move(p.virus);
        virus->set_agent(this);
    }

    int loc = 0;
    for (auto & t : tools)
    {
        
        // Will create a copy of the virus, with the exeption of
        // the virus code
        t->agent     = this;
        t->pos_in_agent = loc++;

    }
    
}

// Copy constructor
inline Agent::Agent(const Agent & p)
{

    state              = p.state;
    state_prev         = p.state_prev;
    state_last_changed = p.state_last_changed;
    id                 = p.id;
    
    // Dealing with the virus
    if (p.virus != nullptr)
    {
        virus = std::shared_ptr<Virus>(p.virus->clone_ptr());
        virus->set_agent(this);
    }
    

    tools.clear();
    tools.reserve(p.get_n_tools());
    for (size_t i = 0u; i < p.get_n_tools(); ++i)
    {
        tools.emplace_back(std::shared_ptr<Tool>(p.tools[i]->clone_ptr()));
        tools.back()->set_agent(this, i);

    }
    
}

inline Agent & Agent::operator=(
    const Agent & other_agent
)
{

    state              = other_agent.state;
    state_prev         = other_agent.state_prev;
    state_last_changed = other_agent.state_last_changed;
    id                  = other_agent.id;
    
    if (other_agent.virus != nullptr)
    {
        virus = std::shared_ptr<Virus>(other_agent.virus->clone_ptr());
        virus->set_agent(this);
    } else
        virus = nullptr;
    
    
    tools.clear();
    tools.reserve(other_agent.get_n_tools());
    for (size_t i = 0u; i < other_agent.get_n_tools(); ++i)
    {
        tools.emplace_back(std::shared_ptr<Tool>(other_agent.tools[i]->clone_ptr()));
        tools.back()->set_agent(this, i);
    }
    
    return *this;
    
}

inline Agent::~Agent() = default;

inline void Agent::add_tool(Model & model, const Tool & tool)
{
    add_tool(model, tool, -99, -99);
}

inline void Agent::set_virus(Model & model, const Virus & virus)
{
    set_virus(model, virus, -99, -99);
}

inline void Agent::rm_tool(Model & model, epiworld_fast_uint tool_idx)
{
    rm_tool(model, tool_idx, -99, -99);
}

inline void Agent::rm_tool(Model & model, ToolPtr & tool)
{
    rm_tool(model, tool, -99, -99);
}

inline void Agent::add_tool(
    Model & model,
    const Tool & tool,
    epiworld_fast_int state_new,
    epiworld_fast_int queue
) {

    // Checking the tool exists
    if (tool.get_id() >= static_cast<int>(model.get_db().get_n_tools()))
        throw std::range_error("The tool with id: " + std::to_string(tool.get_id()) +
            " has not been registered. There are only " + std::to_string(model.get_n_tools()) +
            " included in the model.");

    ToolPtr tool_ptr = std::shared_ptr<Tool>(tool.clone_ptr());

    model._add_event(
        this, nullptr, tool_ptr, state_new, queue, EventAction::AddTool
        );

}

inline void Agent::set_virus(
    Model & model,
    const Virus & virus,
    epiworld_fast_int state_new,
    epiworld_fast_int queue
)
{
    
        // Checking the virus exists
    if (virus.get_id() >= static_cast<int>(model.get_db().get_n_viruses()))
        throw std::range_error("The virus with id: " + std::to_string(virus.get_id()) +
            " has not been registered. There are only " + std::to_string(model.get_n_viruses()) +
            " included in the model.");

    // With a natural history, a virus given from outside (seeding, an
    // import) follows the infection transition out of the agent's stage
    if (model.nh && (state_new == -99))
    {

        const int tr = model.nh->get_stage_info(model.nh_stage_of(state)).infection;
        if (tr < 0)
            throw std::logic_error(
                "Agent " + std::to_string(id) + " is in the stage \"" +
                model.nh->get_stages()[model.nh_stage_of(state)] + "\", which has no infection "
                "transition, so it cannot be given a virus."
            );

        state_new = model.nh->get_transition(tr).to;
        queue = Queue::NoOne;
        if (model.nh_pending == -1)
            model.nh_pending = Model::NH_SEED;

    }

    if (state_new == -99)
        throw std::logic_error(
            "A virus is given to agents of a model with a natural history."
        );

    if (queue == -99)
        queue = Queue::NoOne;

    VirusPtr virus_ptr = std::shared_ptr<Virus>(virus.clone_ptr());

    model._add_event(
        this, virus_ptr, nullptr, state_new, queue, EventAction::AddVirus,
        model.take_sampled_layer()
        );

}

inline void Agent::rm_tool(
    Model & model,
    epiworld_fast_uint tool_idx,
    epiworld_fast_int state_new,
    epiworld_fast_int queue
)
{

    if (tool_idx >= tools.size())
        throw std::range_error(
            "The Tool you want to remove is out of range. This Agent only has " +
            std::to_string(tools.size()) + " tools."
        );

    model._add_event(
        this, nullptr, tools[tool_idx], state_new, queue, EventAction::RemoveTool
        );

}

inline void Agent::rm_tool(
    Model & model,
    ToolPtr & tool,
    epiworld_fast_int state_new,
    epiworld_fast_int queue
)
{

    if (tool->agent != this)
        throw std::logic_error("Cannot remove a virus from another agent!");

    model._add_event(
        this, nullptr, tool, state_new, queue, EventAction::RemoveTool
        );

}

inline void Agent::rm_virus(
    Model & model,
    epiworld_fast_int state_new,
    epiworld_fast_int queue
)
{

    if (virus == nullptr)
        throw std::logic_error(
            "There is no virus to remove here!"
        );

    if (state_new == -99)
        throw std::logic_error(
            "Removing a virus needs the stage to move to: declare an external "
            "transition and fire() it."
        );

    if (queue == -99)
        queue = Queue::NoOne;

    model._add_event(
        this, virus, nullptr,
        state_new,
        queue,
        EventAction::RemoveVirus
        );

}

inline epiworld_double Agent::get_susceptibility_reduction(
    VirusPtr & v,
    Model & model
) {

    return model.susceptibility_reduction_mixer(this, v);
}

inline epiworld_double Agent::get_transmission_reduction(
    VirusPtr & v,
    Model & model
) {
    return model.transmission_reduction_mixer(this, v);
}

inline epiworld_double Agent::get_recovery_enhancer(
    VirusPtr & v,
    Model & model
) {
    return model.recovery_enhancer_mixer(this, v);
}

inline epiworld_double Agent::get_death_reduction(
    VirusPtr & v,
    Model & model
) {
    return model.death_reduction_mixer(this, v);
}

inline int Agent::get_id() const
{
    return id;
}

inline VirusPtr & Agent::get_virus() {
    return virus;
}

inline const VirusPtr & Agent::get_virus() const {
    return virus;
}


inline std::vector< ToolPtr > Agent::get_tools() {
    return tools;
}

inline const std::vector< ToolPtr > Agent::get_tools() const {
    return tools;
}

inline ToolPtr & Agent::get_tool(int i)
{
    return tools.at(i);
}

inline ToolPtr & Agent::get_tool(std::string name)
{
    for (auto & tool : tools)
        if (tool->get_name() == name)
            return tool;

    throw std::logic_error(
        "The agent does not have a tool with name: " + name
    );

}

inline size_t Agent::get_n_tools() const noexcept
{
    return tools.size();
}

inline void Agent::mutate_virus(Model & model)
{

    if (virus == nullptr)
        throw std::logic_error(
            "Agent " + std::to_string(id) + " has no virus to mutate."
        );

    virus->mutate(&model);

}

inline void Agent::change_state(
    Model & model,
    epiworld_fast_uint new_state,
    epiworld_fast_int queue
    )
{

    model._add_event(
        this, nullptr, nullptr, new_state, queue,
        EventAction::ChangeState
    );

    return;

}

inline unsigned int Agent::get_state() const {
    return state;
}

inline unsigned int Agent::get_state_prev() const {
    return state_prev;
}

inline int Agent::get_state_last_changed() const {
    return state_last_changed;
}

inline void Agent::reset()
{

    this->virus = nullptr;

    this->tools.clear();
    decltype(this->tools)().swap(this->tools);

    this->state = 0u;
    this->state_prev = 0u;

    this->state_last_changed = -1;
    
}

inline bool Agent::has_tool(epiworld_fast_uint t) const
{

    for (auto & tool : tools)
        if (tool->get_id() == static_cast<int>(t))
            return true;

    return false;

}

inline bool Agent::has_tool(std::string_view name) const
{

    for (auto & tool : tools)
        if (tool->get_name() == name)
            return true;

    return false;

}

inline bool Agent::has_tool(const Tool & tool) const
{

    return has_tool(tool.get_id());

}

inline bool Agent::has_virus(epiworld_fast_uint t) const
{
    if (virus->get_id() == static_cast<int>(t))
        return true;

    return false;
}

inline bool Agent::has_virus(std::string_view name) const
{
    
    if (virus->get_name() == name)
        return true;

    return false;

}

inline bool Agent::has_virus(const Virus & virus) const
{

    return has_virus(virus.get_id());

}

inline void Agent::print(
    Model & model,
    bool compressed
    ) const
{

    if (compressed)
    {
        printf_epiworld(
            "Agent: %i, state: %s (%i), Has virus: %s, NTools: %ii NNeigh: %i\n",
            static_cast<int>(id),
            model.states_labels[state].c_str(),
            static_cast<int>(state),
            virus == nullptr ? std::string("no").c_str() : std::string("yes").c_str(),
            static_cast<int>(tools.size()),
            static_cast<int>(model.network_degree(static_cast< size_t >(id)))
        );
    }
    else {

        printf_epiworld("Information about agent id %i\n",
            static_cast<int>(this->id));
        printf_epiworld("  State        : %s (%i)\n",
            model.states_labels[state].c_str(), static_cast<int>(state));
        printf_epiworld("  Has virus    : %s\n", virus == nullptr ?
            std::string("no").c_str() : std::string("yes").c_str());
        printf_epiworld("  Tool count   : %i\n", static_cast<int>(tools.size()));
        printf_epiworld("  Neigh. count : %i\n",
            static_cast<int>(model.network_degree(static_cast< size_t >(id))));

        size_t nfeats = model.get_agents_data_ncols();
        if (nfeats > 0)
        {

            printf_epiworld(
                "This model includes features (%i): [ ",
                static_cast<int>(nfeats)
                );

            int max_to_show = static_cast<int>((nfeats > 10)? 10 : nfeats);

            for (int k = 0; k < max_to_show; ++k)
            {
                printf_epiworld("%.2f", this->operator()(k, model));

                if (k != (max_to_show - 1))
                {
                    printf_epiworld(", ");
                } else {
                    printf_epiworld(" ]\n");
                }

            }
            
        }

    }

    return;

}

inline double & Agent::operator()(size_t j, Model & model)
{

    if (model.agents_data_ncols <= j)
        throw std::logic_error("The requested feature of the agent is out of range.");

    return *(model.agents_data + j * model.size() + id);

}

inline double Agent::operator()(size_t j, const Model & model) const
{

    if (model.agents_data_ncols <= j)
        throw std::logic_error("The requested feature of the agent is out of range.");

    return *(model.agents_data + j * model.size() + id);

}

inline bool Agent::operator==(const Agent & other) const
{

    // Checking the address
    if (this == &other)
        return true;

    EPI_DEBUG_FAIL_AT_TRUE(
        state != other.state,
        "Agent:: state don't match"
        )
        

    EPI_DEBUG_FAIL_AT_TRUE(
        state_prev != other.state_prev,
        "Agent:: state_prev don't match"
        )
        

    // EPI_DEBUG_FAIL_AT_TRUE(
    //     state_last_changed != other.state_last_changed,
    //     "Agent:: state_last_changed don't match"
    //     ) ///< Last time the agent was updated.

    EPI_DEBUG_FAIL_AT_TRUE(
        ((virus == nullptr) && (other.virus != nullptr)) ||
            ((virus != nullptr) && (other.virus == nullptr)),
        "Agent:: virus don't match"
    )

    if ((virus != nullptr) && (other.virus != nullptr))
    {
        EPI_DEBUG_FAIL_AT_TRUE(
            *virus != *other.virus,
            "Agent:: virus doesn't match"
        )
    }
    
    EPI_DEBUG_FAIL_AT_TRUE(
        tools.size() != other.tools.size(),
        "Agent:: n_tools don't match"
    )

    for (size_t i = 0u; i < tools.size(); ++i)
    {
        
        EPI_DEBUG_FAIL_AT_TRUE(
            *tools[i] != *other.tools[i],
            "Agent:: tools[i] don't match"
        )
         
    }   
    
    return true;
    
}

#endif
/*//////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

 End of -./include/epiworld/agent-meat.hpp-

////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////*/


/*//////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

 Start of -./include/epiworld/model-meat-transmission.hpp-

////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////*/


#ifndef EPIWORLD_MODEL_MEAT_TRANSMISSION_HPP
#define EPIWORLD_MODEL_MEAT_TRANSMISSION_HPP

/**
 * @file model-meat-transmission.hpp
 * @brief Transmission: from contacts to infections, and network transmission
 * by pushing infection odds.
 *
 * @details A susceptible agent `i` in a stage with a default infection
 * *pulls*: it scans its neighbors, collects the per-contact probabilities
 * `p_ij` of those carrying a virus (in an infectious stage), and `roulette()` draws "no infection" or a
 * single infector. That draw depends only on the odds `r_ij = p_ij / (1 - p_ij)`:
 *
 *     P(no infection) = 1 / (1 + R_i),   P(infected by j) = r_ij / (1 + R_i),
 *
 * with `R_i` the sum of the odds. So the same outcome can be *pushed*: every
 * agent carrying a virus adds its odds to its susceptible neighbors, each of
 * which keeps a weighted reservoir sample of its infectors; then each touched
 * agent makes one draw. The cost follows the carriers' ties instead of the
 * susceptibles', which is much cheaper while an outbreak is small.
 *
 * Contacts with `p_ij >= 1` are certain: the infection happens and the
 * infector is drawn uniformly among them (the limit of the odds as `p -> 1`).
 *
 * The docs page "Push and Pull Transmission" (docs/impl/transmission-sampling.md)
 * has the derivation.
 */

inline void Model::state_masks_build()
{

    const size_t ns = static_cast< size_t >(nstates);

    auto check = [ns](int s, const char * what) {
        if ((s < 0) || (static_cast< size_t >(s) >= ns))
            throw std::range_error(
                std::string("The ") + what + " state " + std::to_string(s) +
                " is out of range (the model has " + std::to_string(ns) +
                " states)."
            );
    };

    state_excluded.assign(ns, 0);
    for (int s : excluded_states)
    {
        check(s, "excluded");
        state_excluded[s] = 1;
    }

    // An excluded agent does not transmit either
    state_not_infectious.clear();
    if (infectious_states.empty() && excluded_states.empty())
        return;

    state_not_infectious.assign(ns, !infectious_states.empty());
    for (int s : infectious_states)
    {
        check(s, "infectious");
        state_not_infectious[s] = false;
    }

    for (int s : excluded_states)
        state_not_infectious[s] = true;

}

inline size_t Model::add_exposures(
    Agent * agent,
    const ContactLayer * layer,
    const size_t * contacts,
    size_t n,
    size_t n_exposures
)
{

    const bool weighted = (layer != nullptr) && layer->has_weight();
    const epiworld_double weight = weighted ? layer->get_weight(*this) : 1.0;
    const int layer_id = (layer == nullptr) ? -1 : static_cast< int >(layer->get_id());

    // A layer of weight 0 transmits nothing
    if (weighted && (weight <= 0.0))
        return n_exposures;

    reserve_tmp(n_exposures + n);

    for (size_t k = 0u; k < n; ++k)
    {

        auto & contact = population[contacts[k]];
        auto & v = contact.get_virus();
        if (v == nullptr)
            continue;

        if ((layer != nullptr) && !layer->admits(*v))
            continue;

        epiworld_double p =
            (1.0 - agent->get_susceptibility_reduction(v, *this)) *
            nh_infection_prob(*agent, *v) *
            (1.0 - contact.get_transmission_reduction(v, *this))
            ;

        // The weight scales the odds p / (1 - p); a certain exposure stays so
        if (weighted && (p < 1.0))
            p = weight * p / (1.0 - p + weight * p);

        array_double_tmp[n_exposures] = p;
        array_virus_tmp[n_exposures] = &(*v);
        array_layer_tmp[n_exposures++] = layer_id;

    }

    return n_exposures;

}

inline Virus * Model::draw_exposure(size_t n_exposures)
{

    sampled_layer = -1;

    if (n_exposures == 0u)
        return nullptr;

    const int which = roulette(n_exposures, this);

    if (which < 0)
        return nullptr;

    sampled_layer = array_layer_tmp[which];
    return array_virus_tmp[which];

}

inline Virus * Model::sample_virus(
    Agent * agent,
    const size_t * contacts,
    size_t n,
    const ContactLayer * layer
)
{

    return draw_exposure(add_exposures(agent, layer, contacts, n, 0u));

}

inline Virus * Model::sample_virus(Agent * agent)
{

    if (agent->get_virus() != nullptr)
        throw std::logic_error(
            "Sampling a virus for agent " + std::to_string(agent->get_id()) +
            ", which already has one, makes no sense."
        );

    const size_t id = static_cast< size_t >(agent->get_id());
    const std::vector< bool > * exclude =
        state_not_infectious.empty() ? nullptr : &state_not_infectious;

    size_t n_contacts = 0u;
    size_t n_exposures = 0u;
    for (const auto & layer : layers)
    {

        layer_contacts.clear();
        const size_t n = layer->sample(id, *this, exclude, layer_contacts);
        if (n == 0u)
            continue;

        n_contacts += n;
        n_exposures = add_exposures(
            agent, layer.get(), layer_contacts.data(), n, n_exposures
        );

    }

    if (n_contacts == 0u)
    {
        sampled_layer = -1;
        return nullptr;
    }

    #ifdef EPI_DEBUG
    db.n_transmissions_potential++;
    #endif

    Virus * virus = draw_exposure(n_exposures);

    #ifdef EPI_DEBUG
    if (virus != nullptr)
        db.n_transmissions_today++;
    #endif

    return virus;

}

inline bool Model::transmission_prepare()
{

    const size_t ns = static_cast< size_t >(nstates);

    push_pushable.assign(ns, 0);
    push_source_ok.assign(ns, 0);

    bool any = false;

    // A natural history says which stages are pushed to
    if (nh)
    {
        // A cell is pushed to if its stage's only drawn exit is a default
        // infection, its status has no drawn exit, and it is in the layers
        for (size_t c = 0u; c < ns; ++c)
        {
            const unsigned int cu = static_cast< unsigned int >(c);
            const bool status_drawn = st &&
                st->stages[static_cast< size_t >(nh_status_of(cu))].needs_visit;
            const bool excluded = !state_excluded.empty() && state_excluded[c];
            if (
                nh->stages[static_cast< size_t >(nh_stage_of(cu))].pushable &&
                !status_drawn && !excluded
            )
            {
                push_pushable[c] = 1;
                any = true;
            }
        }
    }

    // The infectious states are the sources, if anyone is pushed to
    if (any)
        for (size_t s = 0u; s < ns; ++s)
            push_source_ok[s] =
                state_not_infectious.empty() || !state_not_infectious[s];

    return any;

}

inline bool Model::transmission_choose_push() const
{

    if (transmission_mode == TransmissionMode::pull)
        return false;

    if (transmission_mode == TransmissionMode::push)
        return true;

    // Pushing visits every carrier that can transmit and walks its ties;
    // pulling visits every susceptible agent and walks its ties. Each visit
    // also costs a fixed amount, EPI_TRANSMISSION_AGENT_COST ties: at the low
    // degrees of contact networks that is comparable to the ties themselves,
    // so it matters which side has more agents. The sums are kept per state, so
    // this is O(number of states). It deliberately ignores the queue: the
    // decision -- and so the random stream -- is the same with queuing on or
    // off. kappa < 1 makes pushing harder to choose: the queue spares a pull
    // the susceptibles with no infectious neighbor, which the sums do not see,
    // and the per-agent term raises the pull side wherever susceptibles
    // outnumber carriers. The default was fitted so the two balance.
    const TransmissionSums sums = get_transmission_sums();

    const double cost_push =
        static_cast< double >(sums.carrier_degree) +
        EPI_TRANSMISSION_AGENT_COST * static_cast< double >(sums.carriers);

    const double cost_pull =
        static_cast< double >(sums.susceptible_degree) +
        EPI_TRANSMISSION_AGENT_COST * static_cast< double >(sums.susceptibles);

    return cost_push <= transmission_kappa * cost_pull;

}

inline typename Model::TransmissionSums
Model::get_transmission_sums() const
{

    TransmissionSums sums = {0u, 0u, 0u, 0u};

    if (!state_index_ready || (push_pushable.size() != static_cast< size_t >(nstates)))
        return sums;

    for (size_t s = 0u; s < static_cast< size_t >(nstates); ++s)
    {
        if (push_source_ok[s])
        {
            sums.carrier_degree += state_carrier_degree[s];
            sums.carriers += state_carriers[s];
        }
        if (push_pushable[s])
        {
            sums.susceptible_degree += state_degree[s];
            sums.susceptibles += state_start[s + 1u] - state_start[s];
        }
    }

    return sums;

}

inline void Model::transmission_push()
{

    // The recording path is chosen once per step, not per contact
    bool record = post_sampling_on;
    for (const auto * net : network_layers)
        record = record || net->is_tracing();

    if (record)
        transmission_push_impl<true>();
    else
        transmission_push_impl<false>();

}

template<bool Record>
inline void Model::transmission_push_impl()
{

    const size_t ns = static_cast< size_t >(nstates);

    if (push_slot.size() != population.size())
        push_slot.assign(population.size(), -1);

    push_targets.clear();

    // Phase 1: every carrier that can transmit adds its odds to its eligible
    // neighbors. Nothing changes state until events_run(), so this sees the
    // model as it was at the start of the step, as pulling does.
    //
    // The carriers are visited in ascending id order (marked in a bitset, then
    // walked), not in the index's order: networks are usually built with
    // neighbors close in id, so this sweeps memory the way a pull does, and on
    // large populations it saves most of the cache misses.
    const size_t nwords = (population.size() + 63u) / 64u;
    if (push_sources.size() != nwords)
        push_sources.assign(nwords, 0u);

    for (size_t s = 0u; s < ns; ++s)
    {

        if (!push_source_ok[s] || (state_carriers[s] == 0u))
            continue;

        for (size_t id : state_index_members(s))
            push_sources[id >> 6] |= (uint64_t(1) << (id & 63u));

    }

    for (size_t w = 0u; w < nwords; ++w)
    {

        uint64_t word = push_sources[w];
        push_sources[w] = 0u;

        while (word != 0u)
        {

            const size_t j_id = (w << 6) + epi_ctz64(word);
            word &= (word - 1u);

            Agent & j = population[j_id];
            if (j.virus == nullptr)
                continue;

            VirusPtr & v = j.virus;

            // Every network, in the order the layers were added, as a pull
            // reads them
            for (auto * net : network_layers)
            {

                // Layer rules, as add_exposures() applies them in a pull
                const bool weighted = net->has_weight();
                const epiworld_double weight = weighted ?
                    net->get_weight(*this) : 1.0;
                const bool admitted = net->admits(*v);
                const int layer_id = static_cast< int >(net->get_id());

                for (size_t i_id : net->ties[j_id])
                {

                    // Most neighbors are usually not susceptible; the compact copy
                    // of the states rules them out without loading the agent.
                    const size_t t = agent_state[i_id];

                    if (!push_pushable[t])
                        continue;

                    Agent & i = population[i_id];

                    // An agent with a virus is not susceptible (pulling would
                    // refuse it; update_state() reports it).
                    if (i.virus != nullptr)
                        continue;

                    // Pulling only updates queued agents.
                    if (use_queuing && (queue[i_id] <= 0))
                        continue;

                    // The post-sampling callback and the layer's log see every
                    // eligible contact, also the ones that cannot transmit, as a
                    // pull reports them. The carriers are visited in ascending id
                    // order, so the pairs come out grouped by infectious agent.
                    if constexpr (Record)
                    {
                        if (post_sampling_on)
                            register_sampled_contact(j_id, i_id);
                        net->trace(j_id, i_id);
                    }

                    // A layer of weight 0, or closed to this virus, transmits
                    // nothing (its contacts were still reported above)
                    if (!admitted || (weighted && (weight <= 0.0)))
                        continue;

                    // Exactly the expression the pull uses, in the same order.
                    epiworld_double p =
                        (1.0 - i.get_susceptibility_reduction(v, *this)) *
                        nh_infection_prob(i, *v) *
                        (1.0 - j.get_transmission_reduction(v, *this))
                        ;

                    // The weight scales the odds; a certain exposure stays so
                    if (weighted && (p < 1.0))
                        p = weight * p / (1.0 - p + weight * p);

                    // No chance of transmission (also catches NaN)
                    if (!(p > 0.0))
                        continue;

                    int & slot = push_slot[i_id];
                    if (slot < 0)
                    {

                        slot = static_cast< int >(push_targets.size());
                        push_targets.push_back({i_id, 0.0, 0u, nullptr, -1});

                        #ifdef EPI_DEBUG
                        db.n_transmissions_potential++;
                        #endif

                    }

                    PushTarget & target = push_targets[static_cast< size_t >(slot)];

                    // Certain transmission: uniform reservoir among these
                    if (p >= 1.0)
                    {

                        if (
                            (++target.n_certain == 1u) ||
                            (runif() * static_cast< double >(target.n_certain) < 1.0)
                        )
                        {
                            target.candidate = &(*v);
                            target.layer = layer_id;
                        }

                        continue;

                    }

                    // A certain transmission wins outright
                    if (target.n_certain > 0u)
                        continue;

                    // Weighted reservoir: keep this infector with probability
                    // r / (sum of odds so far).
                    const double odds = static_cast< double >(p) /
                        (1.0 - static_cast< double >(p));
                    target.odds += odds;

                    if (
                        (target.candidate == nullptr) ||
                        (runif() * target.odds < odds)
                    )
                    {
                        target.candidate = &(*v);
                        target.layer = layer_id;
                    }

                }

            }

        }

    }

    // Phase 2: one draw per touched agent. P(no infection) = 1 / (1 + R).
    for (auto & target : push_targets)
    {

        push_slot[target.id] = -1;

        if (target.candidate == nullptr)
            continue;

        if (
            (target.n_certain == 0u) &&
            (runif() < 1.0 / (1.0 + target.odds))
        )
            continue;

        Agent & i = population[target.id];

        #ifdef EPI_DEBUG
        db.n_transmissions_today++;
        #endif

        sampled_layer = target.layer;
        if (nh)
            nh_propose(
                nh->stages[static_cast< size_t >(nh_stage_of(i.state))].infection,
                i, target.candidate
            );
        else
            i.set_virus(*this, *target.candidate);

    }

}

inline void Model::transmission_update_others()
{

    const size_t ns = static_cast< size_t >(nstates);

    // Pulling refuses an agent in a susceptible state that carries a virus;
    // so does pushing, for the agents a pull would have visited.
    for (size_t s = 0u; s < ns; ++s)
    {

        if (!push_pushable[s] || (state_carriers[s] == 0u))
            continue;

        for (size_t id : state_index_members(s))
        {

            const auto & p = population[id];
            if ((p.virus != nullptr) && (!use_queuing || (queue[id] > 0)))
                throw std::logic_error(
                    std::string("A pushed-to stage has agents WITH viruses! ") +
                    std::string("Agent id ") + std::to_string(p.get_id()) +
                    std::string(" has a virus.")
                    );

        }

    }

    // Everyone in a state with an update function, other than the pushed ones.
    // The index gives them directly, so neither the population nor the queue
    // (which holds every neighbor of every carrier) needs to be scanned. They
    // are marked in a bitset and visited in ascending id order -- the same
    // order whether queuing is on or off -- in O(agents + N / 64), no sort.
    const size_t nwords = (population.size() + 63u) / 64u;
    if (push_visit.size() != nwords)
        push_visit.assign(nwords, 0u);

    for (size_t s = 0u; s < ns; ++s)
        if (state_fun[s] && !push_pushable[s])
            for (size_t id : state_index_members(s))
                push_visit[id >> 6] |= (uint64_t(1) << (id & 63u));

    for (size_t w = 0u; w < nwords; ++w)
    {

        uint64_t word = push_visit[w];
        push_visit[w] = 0u;

        while (word != 0u)
        {

            const size_t id = (w << 6) + epi_ctz64(word);
            word &= (word - 1u);

            // Queued agents only, read as the loop reaches them (a state
            // function may change the queue by editing ties).
            if (use_queuing && (queue[id] <= 0))
                continue;

            auto & p = population[id];
            state_fun[p.state](&p, this);

        }

    }

}

inline Model & Model::set_transmission_mode(
    TransmissionMode mode,
    double kappa
)
{

    if (!(kappa >= 0.0) || std::isinf(kappa))
        throw std::range_error(
            "The transmission kappa must be a finite, non-negative number."
        );

    transmission_mode = mode;
    transmission_kappa = kappa;
    return *this;

}

inline Model & Model::set_transmission_mode(
    std::string_view mode,
    double kappa
)
{

    if (mode == "auto")
        return set_transmission_mode(TransmissionMode::automatic, kappa);
    else if (mode == "push")
        return set_transmission_mode(TransmissionMode::push, kappa);
    else if (mode == "pull")
        return set_transmission_mode(TransmissionMode::pull, kappa);

    throw std::invalid_argument(
        "Unknown transmission mode \"" + std::string(mode) +
        "\". Use \"auto\", \"push\", or \"pull\"."
    );

}

inline TransmissionMode Model::get_transmission_mode() const
{
    return transmission_mode;
}

inline TransmissionMode Model::get_last_transmission_mode() const
{
    return transmission_mode_last;
}

inline double Model::get_transmission_kappa() const
{
    return transmission_kappa;
}

#endif
/*//////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

 End of -./include/epiworld/model-meat-transmission.hpp-

////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////*/


/*//////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

 Start of -./include/epiworld/naturalhistory-meat.hpp-

////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////*/


#ifndef EPIWORLD_NATURALHISTORY_MEAT_HPP
#define EPIWORLD_NATURALHISTORY_MEAT_HPP

inline TransitionRef::operator int() const
{
    return nh->status_chart ? (-2 - id) : id;
}

// Value -----------------------------------------------------------------------

inline Value Value::inverse(std::string name)
{
    Value v(std::move(name));
    v.kind = Kind::inverse;
    return v;
}

inline Value Value::complement(std::string name)
{
    Value v(std::move(name));
    v.kind = Kind::complement;
    return v;
}

inline epiworld_double Value::eval(
    Agent & agent,
    const Virus * virus,
    Model & model
) const
{

    switch (kind)
    {
    case Kind::number:
        return number;
    case Kind::fun:
        return fun(agent, model);
    case Kind::none:
        throw std::logic_error("A value of a transition is not set.");
    default:
        break;
    }

    ParamId id = param.id(model);
    if (virus != nullptr)
        id = virus->param_redirect(id, model);

    const epiworld_double x = model.par_at(id);

    if (kind == Kind::inverse)
        return 1.0 / x;
    if (kind == Kind::complement)
        return 1.0 - x;

    return x;

}

inline epiworld_double Value::eval(
    Agent & agent,
    Model & model
) const
{
    const auto & v = agent.get_virus();
    return eval(agent, v ? v.get() : nullptr, model);
}

// Duration --------------------------------------------------------------------

inline Duration Duration::fixed(Value days)
{
    Duration d;
    d.kind = Kind::fixed;
    d.mean = std::move(days);
    return d;
}

inline Duration Duration::geometric(Value mean)
{
    Duration d;
    d.kind = Kind::geometric;
    d.mean = std::move(mean);
    return d;
}

inline Duration Duration::lognormal(Value mean, Value sd)
{
    Duration d;
    d.kind = Kind::lognormal;
    d.mean = std::move(mean);
    d.sd = std::move(sd);
    return d;
}

inline Duration Duration::gamma(Value mean, Value sd)
{
    Duration d;
    d.kind = Kind::gamma;
    d.mean = std::move(mean);
    d.sd = std::move(sd);
    return d;
}

inline Duration Duration::since(int s) const
{
    Duration d(*this);
    d.anchor = s;
    return d;
}

inline int Duration::draw(Agent & agent, Model & model) const
{

    auto whole = [](epiworld_double days) -> int {
        if (!(days < static_cast< epiworld_double >(INT_MAX / 2)))
            return INT_MAX / 2;
        return std::max(1, static_cast< int >(std::lround(days)));
    };

    switch (kind)
    {
    case Kind::fun:
        return std::max(1, fun(agent, model));

    case Kind::fixed:
        return whole(mean.eval(agent, model));

    case Kind::geometric:
    {
        // Days until the first success of a daily Bernoulli(1 / mean): the
        // same as leaving each day with that probability. Drawn by inversion
        // in floating point and capped like the other kinds, since an integer
        // geometric draw overflows for long means (e.g., 1e9 days).
        const epiworld_double m = mean.eval(agent, model);
        if (!(m > 1.0))
            return 1;
        const epiworld_double u = 1.0 - model.runif(); // in (0, 1]
        return whole(
            1.0 + std::floor(std::log(u) / std::log1p(-1.0 / m))
        );
    }

    case Kind::lognormal:
    {
        const epiworld_double m = mean.eval(agent, model);
        const epiworld_double s = sd.eval(agent, model);
        if (!(s > 0.0) || !(m > 0.0))
            return whole(m);

        // A standard normal from two uniforms (Box-Muller), which keeps no
        // state between calls, so a run's draws depend only on its seed.
        const epiworld_double u1 = 1.0 - model.runif(); // in (0, 1]
        const epiworld_double u2 = model.runif();
        const epiworld_double z  = std::sqrt(-2.0 * std::log(u1)) *
            std::cos(2.0 * M_PI * u2);

        const epiworld_double s2 = std::log(1.0 + (s * s) / (m * m));
        return whole(std::exp(std::log(m) - s2 / 2.0 + std::sqrt(s2) * z));
    }

    case Kind::gamma:
    {
        const epiworld_double m = mean.eval(agent, model);
        const epiworld_double s = sd.eval(agent, model);
        if (!(s > 0.0) || !(m > 0.0))
            return whole(m);

        // Shape m^2 / s^2 and scale s^2 / m give mean m and sd s
        return whole(model.rgamma((m * m) / (s * s), (s * s) / m));
    }

    default:
        throw std::logic_error("A duration of a transition is not set.");
    }

}

// TransitionRef ---------------------------------------------------------------

inline TransitionRef & TransitionRef::name(std::string n)
{
    for (auto & t : nh->transitions)
        if ((t.name == n) && (&t != &nh->transitions[id]))
            throw std::logic_error(
                "The natural history \"" + nh->name +
                "\" already has a transition named \"" + n + "\"."
            );

    nh->transitions[id].name = std::move(n);
    return *this;
}

inline TransitionRef & TransitionRef::effect(Effect e)
{
    auto & t = nh->transitions[id];
    if ((e != Effect::none) && (t.kind != TransitionKind::rate))
        throw std::logic_error(
            "Only a rate takes a tool effect (transition \"" + t.name + "\")."
        );

    t.effect = e;
    return *this;
}

inline TransitionRef & TransitionRef::to(
    std::vector< std::pair< int, Value > > targets
)
{
    auto & t = nh->transitions[id];
    if (targets.size() < 2u)
        throw std::logic_error(
            "A branch needs two or more targets (transition \"" + t.name +
            "\")."
        );

    t.branch_to.clear();
    t.branch_prob.clear();
    for (auto & target : targets)
    {
        nh->check_stage(target.first, "target");
        t.branch_to.push_back(target.first);
        t.branch_prob.push_back(std::move(target.second));
    }

    t.to = t.branch_to[0u];
    return *this;
}

inline TransitionRef & TransitionRef::sampler(InfectionSampler f)
{
    auto & t = nh->transitions[id];
    if (t.kind != TransitionKind::infection)
        throw std::logic_error(
            "Only an infection takes a sampler (transition \"" + t.name + "\")."
        );

    t.sampler = std::move(f);
    return *this;
}

inline TransitionRef & TransitionRef::when(Nodes stages)
{
    auto & t = nh->transitions[id];
    if (!nh->status_chart)
        throw std::logic_error(
            "Only a status transition takes when() (transition \"" + t.name +
            "\")."
        );
    t.when = std::move(stages.ids);
    return *this;
}

// NaturalHistory --------------------------------------------------------------

inline NaturalHistory::NaturalHistory(std::string name_) :
    name(std::move(name_)) {}

inline void NaturalHistory::check_stage(int s, const char * what) const
{
    if ((s < 0) || (static_cast< size_t >(s) >= stage_labels.size()))
        throw std::range_error(
            std::string("The ") + what + " stage " + std::to_string(s) +
            " is not a stage of the natural history \"" + name + "\" (it has " +
            std::to_string(stage_labels.size()) + ")."
        );
}

inline int NaturalHistory::add_stage(std::string label, unsigned int flags)
{

    for (auto & l : stage_labels)
        if (l == label)
            throw std::logic_error(
                "The natural history \"" + name + "\" already has a stage \"" +
                label + "\"."
            );

    if ((flags & ~stage::infectious) != 0u)
        throw std::invalid_argument(
            "Unknown stage flags for \"" + label +
            "\": use stage::carrier and stage::infectious."
        );

    stage_labels.push_back(std::move(label));
    stage_flags.push_back(flags);
    stage_contact.push_back(1.0);
    finalized = false;

    return static_cast< int >(stage_labels.size() - 1u);

}

inline void NaturalHistory::set_contact(int s, epiworld_double weight)
{
    check_stage(s, "");

    // Fractional weights come with the statuses (NH-2)
    if ((weight != 0.0) && (weight != 1.0))
        throw std::invalid_argument(
            "For now, a stage's contact weight is 0 or 1 (stage \"" +
            stage_labels[s] + "\")."
        );

    stage_contact[s] = weight;
}

inline int NaturalHistory::add_transition(Transition t)
{

    if (t.from.empty())
        throw std::logic_error("A transition needs at least one source stage.");

    for (int s : t.from)
        check_stage(s, "source");

    check_stage(t.to, "target");

    // Two default names may coincide until one is renamed; finalize() checks
    if (t.name.empty())
        t.name = stage_labels[t.from[0u]] + " -> " + stage_labels[t.to];
    else
        for (auto & other : transitions)
            if (other.name == t.name)
                throw std::logic_error(
                    "The natural history \"" + name + "\" already has a "
                    "transition named \"" + t.name + "\"."
                );

    transitions.push_back(std::move(t));
    finalized = false;

    return static_cast< int >(transitions.size() - 1u);

}

inline TransitionRef NaturalHistory::infection(
    Nodes from, int to, Value per_contact
)
{
    Transition t;
    t.kind = TransitionKind::infection;
    t.from = std::move(from.ids);
    t.to = to;
    t.value = std::move(per_contact);
    return TransitionRef(this, add_transition(std::move(t)));
}

inline TransitionRef NaturalHistory::rate(
    Nodes from, int to, Value daily_probability
)
{
    Transition t;
    t.kind = TransitionKind::rate;
    t.from = std::move(from.ids);
    t.to = to;
    t.value = std::move(daily_probability);
    return TransitionRef(this, add_transition(std::move(t)));
}

inline TransitionRef NaturalHistory::after(
    int from, int to, Duration d
)
{
    if (d.get_kind() == Duration::Kind::none)
        throw std::logic_error("A duration transition needs a duration.");

    Transition t;
    t.kind = TransitionKind::after;
    t.from = {from};
    t.to = to;
    t.duration = std::move(d);
    return TransitionRef(this, add_transition(std::move(t)));
}

inline TransitionRef NaturalHistory::external(
    Nodes from, int to, std::string label
)
{
    Transition t;
    t.kind = TransitionKind::external;
    t.from = std::move(from.ids);
    t.to = to;
    t.name = std::move(label);
    return TransitionRef(this, add_transition(std::move(t)));
}

inline int NaturalHistory::stage_id(std::string_view label) const
{
    for (size_t s = 0u; s < stage_labels.size(); ++s)
        if (stage_labels[s] == label)
            return static_cast< int >(s);

    throw std::logic_error(
        "The natural history \"" + name + "\" has no stage \"" +
        std::string(label) + "\"."
    );
}

inline int NaturalHistory::transition_id(std::string_view label) const
{
    for (size_t t = 0u; t < transitions.size(); ++t)
        if (transitions[t].name == label)
            return status_chart ? (-2 - static_cast< int >(t)) : static_cast< int >(t);

    throw std::logic_error(
        "The natural history \"" + name + "\" has no transition \"" +
        std::string(label) + "\"."
    );
}

inline bool NaturalHistory::is_source(int transition, int s) const
{
    for (int f : transitions[transition].from)
        if (f == s)
            return true;
    return false;
}

inline bool NaturalHistory::same_structure(
    const NaturalHistory & o
) const
{

    if (
        (stage_labels != o.stage_labels) ||
        (stage_flags != o.stage_flags) ||
        (stage_contact != o.stage_contact) ||
        (transitions.size() != o.transitions.size())
    )
        return false;

    for (size_t k = 0u; k < transitions.size(); ++k)
    {
        const auto & a = transitions[k];
        const auto & b = o.transitions[k];
        if (
            (a.name != b.name) || (a.kind != b.kind) || (a.from != b.from) ||
            (a.to != b.to) || (a.branch_to != b.branch_to) ||
            (a.effect != b.effect) ||
            (static_cast< bool >(a.sampler) != static_cast< bool >(b.sampler))
        )
            return false;
    }

    return true;

}

inline void NaturalHistory::finalize(
    Model & model,
    const std::string & owner
)
{

    if (stage_labels.empty())
        throw std::logic_error(
            "The natural history \"" + name + "\" has no stages."
        );

    const size_t ns = stage_labels.size();
    stages.assign(ns, StageInfo());

    for (size_t a = 0u; a < transitions.size(); ++a)
        for (size_t b = a + 1u; b < transitions.size(); ++b)
            if (transitions[a].name == transitions[b].name)
                throw std::logic_error(
                    "The natural history \"" + name + "\" has two transitions "
                    "named \"" + transitions[a].name + "\"; name one of them "
                    "with .name()."
                );

    // A number becomes a model parameter, named after the transition
    auto as_param = [&model](Value & v, const std::string & pname) {
        if (v.kind != Value::Kind::number)
            return;
        model.add_param(v.number, pname, true);
        v = Value(pname);
    };

    for (size_t id = 0u; id < transitions.size(); ++id)
    {

        auto & t = transitions[id];
        const std::string where = "transition \"" + t.name + "\"";

        // The targets of a branch are all carriers or all not
        std::vector< int > targets = t.branch_to.empty() ?
            std::vector< int >({t.to}) : t.branch_to;

        for (int target : targets)
            if (is_carrier(target) != is_carrier(t.to))
                throw std::logic_error(
                    "The targets of the " + where + " must be all carrier "
                    "stages or all non-carrier stages."
                );

        for (int s : t.from)
        {

            if (is_carrier(s) != is_carrier(t.from[0u]))
                throw std::logic_error(
                    "The sources of the " + where + " must be all carrier "
                    "stages or all non-carrier stages."
                );

            if (t.kind == TransitionKind::infection)
            {

                if (is_carrier(s))
                    throw std::logic_error(
                        "The " + where + " leaves a carrier stage (\"" +
                        stage_labels[s] + "\"); an infection leaves a stage "
                        "without the virus."
                    );

                if (stages[s].infection >= 0)
                    throw std::logic_error(
                        "The stage \"" + stage_labels[s] + "\" has two "
                        "infection transitions."
                    );

                stages[s].infection = static_cast< int >(id);

            }
            else if (!is_carrier(s) && is_carrier(t.to))
                throw std::logic_error(
                    "The " + where + " gives the virus without an infection: "
                    "only an infection (or seeding) enters a carrier stage "
                    "from a stage without the virus."
                );

            if (t.kind == TransitionKind::rate)
                stages[s].rates.push_back(static_cast< int >(id));

            if (t.kind == TransitionKind::after)
            {
                if (stages[s].after >= 0)
                    throw std::logic_error(
                        "The stage \"" + stage_labels[s] + "\" has two "
                        "durations; a stage has at most one."
                    );
                stages[s].after = static_cast< int >(id);
            }

        }

        if ((t.kind == TransitionKind::infection) && !is_carrier(t.to))
            throw std::logic_error(
                "The " + where + " must lead to a carrier stage."
            );

        // An anchor is a disease stage: the model checks the statuses' own
        // against the virus (as it does `.when()`)
        if (
            !status_chart &&
            (t.kind == TransitionKind::after) &&
            (t.duration.get_anchor() >= 0)
        )
            check_stage(t.duration.get_anchor(), "anchor");

        // Numbers become parameters
        const std::string pname = owner + ": " + t.name;
        as_param(t.value, pname);
        as_param(t.duration.mean, pname + " (mean)");
        as_param(t.duration.sd, pname + " (sd)");
        for (size_t k = 0u; k < t.branch_prob.size(); ++k)
            as_param(
                t.branch_prob[k],
                pname + " (to " + stage_labels[t.branch_to[k]] + ")"
            );

    }

    for (auto & s : stages)
    {
        s.needs_visit = !s.rates.empty() || (s.after >= 0);
        s.pushable = (s.infection >= 0) && !s.needs_visit &&
            !transitions[s.infection].sampler;
    }

    finalized = true;

}

inline std::string NaturalHistory::draw_mermaid(
    const std::string & prefix,
    const NaturalHistory * stage_names
) const
{

    if (stage_names == nullptr)
        stage_names = this;

    auto number = [](epiworld_double x) {
        std::ostringstream out;
        out << x;
        return out.str();
    };

    auto value = [&number](const Value & v) -> std::string {
        switch (v.kind)
        {
            case Value::Kind::number:     return number(v.number);
            case Value::Kind::param:      return v.param.name();
            case Value::Kind::inverse:    return "1 / " + v.param.name();
            case Value::Kind::complement: return "1 - " + v.param.name();
            case Value::Kind::fun:        return "a function";
            default:                      return "";
        }
    };

    auto duration = [&](const Duration & d) -> std::string {
        std::string out;
        switch (d.kind)
        {
            case Duration::Kind::fixed:
                out = "fixed(" + value(d.mean) + ")"; break;
            case Duration::Kind::geometric:
                out = "geometric(" + value(d.mean) + ")"; break;
            case Duration::Kind::lognormal:
                out = "lognormal(" + value(d.mean) + ", " + value(d.sd) + ")"; break;
            case Duration::Kind::gamma:
                out = "gamma(" + value(d.mean) + ", " + value(d.sd) + ")"; break;
            case Duration::Kind::fun:
                out = "a function"; break;
            default: break;
        }
        if (d.anchor >= 0)
            out += ", since " + stage_names->get_stages().at(d.anchor);
        return out;
    };

    // Mermaid labels are double-quoted
    auto quote = [](std::string x) {
        std::string out;
        for (char c : x)
            out += (c == '"') ? std::string("#quot;") : std::string(1, c);
        return "\"" + out + "\"";
    };

    std::ostringstream out;

    for (size_t s = 0u; s < stage_labels.size(); ++s)
    {
        out << "    " << prefix << s << "[" << quote(stage_labels[s]) << "]";
        if (stage_flags[s] & 2u)
            out << ":::infectious";
        else if (stage_flags[s] & stage::carrier)
            out << ":::carrier";
        else if (stage_contact[s] == 0.0)
            out << ":::away";
        out << "\n";
    }

    for (const auto & t : transitions)
    {

        // The label: the name (unless the default), the kind and value, and
        // the stages a status transition is restricted to
        std::string label;
        std::string arrow = "-->";
        // (a default name is "From -> To", or "From -> A or B" for a branch)
        const std::string from_label = stage_labels[t.from[0u]] + " -> ";
        const bool default_name =
            (t.name == from_label + stage_labels[t.to]) ||
            (!t.branch_to.empty() && (t.name.rfind(from_label, 0u) == 0u));
        if (!default_name)
            label = t.name + "<br>";

        switch (t.kind)
        {
            case TransitionKind::infection:
                label += "infection: " + value(t.value); break;
            case TransitionKind::rate:
                label += "rate: " + value(t.value); break;
            case TransitionKind::after:
                label += "after: " + duration(t.duration); arrow = "==>"; break;
            case TransitionKind::external:
                label += "external"; arrow = "-.->"; break;
        }

        if (!t.when.empty())
        {
            label += "<br>when: ";
            for (size_t k = 0u; k < t.when.size(); ++k)
                label += (k ? ", " : "") + stage_names->get_stages().at(t.when[k]);
        }

        // A branch goes to each of its targets, with its probability
        std::vector< std::pair< int, std::string > > targets;
        if (t.branch_to.empty())
            targets.push_back({t.to, label});
        else
            for (size_t k = 0u; k < t.branch_to.size(); ++k)
                targets.push_back(
                    {t.branch_to[k], label + "<br>p = " + value(t.branch_prob[k])}
                );

        for (int from : t.from)
            for (const auto & target : targets)
                out << "    " << prefix << from << " " << arrow << "|" <<
                    quote(target.second) << "| " << prefix << target.first <<
                    "\n";

    }

    return out.str();

}

#endif
/*//////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

 End of -./include/epiworld/naturalhistory-meat.hpp-

////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////*/


/*//////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

 Start of -./include/epiworld/model-meat-naturalhistory.hpp-

////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////*/


#ifndef EPIWORLD_MODEL_MEAT_NATURALHISTORY_HPP
#define EPIWORLD_MODEL_MEAT_NATURALHISTORY_HPP

/**
 * @file model-meat-naturalhistory.hpp
 * @brief The model's side of natural histories and statuses.
 *
 * @details An agent has a stage (from its virus's natural history) and a
 * status (from the model's `Statuses`). The engine keeps one index for the
 * pair, a *cell*: `cell = stage * n_statuses + status`, which is the agent's
 * `state`. With one status, the cells are the stages. Events carry the target
 * stage or status; the cell is computed when the event is applied, so a stage
 * change and a status change on the same day combine.
 */

// Installing ------------------------------------------------------------------

inline void Model::nh_install(std::shared_ptr< NaturalHistory > def)
{

    if (nstates != 0u)
        throw std::logic_error(
            "The model already has states. A virus with a natural history "
            "gives the model its stages as its states: add it before anything "
            "else."
        );

    if (def->is_carrier(0))
        throw std::logic_error(
            "The first stage of \"" + def->get_name() + "\" (\"" +
            def->get_stages()[0] + "\") is where agents start, so it cannot be "
            "a carrier stage."
        );

    bool default_infection = false;
    bool infectious = false;
    for (int s = 0; s < static_cast< int >(def->get_n_stages()); ++s)
    {
        const auto & info = def->get_stage_info(s);
        if ((info.infection >= 0) && !def->get_transition(info.infection).sampler)
            default_infection = true;
        if (def->is_infectious(s))
            infectious = true;
    }

    if (default_infection && !infectious)
        throw std::logic_error(
            "The natural history \"" + def->get_name() + "\" has an infection "
            "by contact but no infectious stage (stage::infectious)."
        );

    nh = std::move(def);
    nh_build_states();

}

inline void Model::set_statuses(Statuses s)
{

    if (running)
        throw std::logic_error("set_statuses() is called between runs.");

    if (s.get_n_stages() == 0u)
        throw std::logic_error("The statuses have no status.");

    s.finalize(*this, "Statuses");
    st = std::make_shared< const Statuses >(std::move(s));

    if (nh)
        nh_build_states();

}

inline void Model::nh_build_states()
{

    const int ns = static_cast< int >(nh->get_n_stages());
    const int nk = static_cast< int >(nh_nstatus());

    // A status transition's stages (`.when()` and a duration's `.since()`)
    // are the virus's, and its name is not one of the virus's
    // (transition_id() finds either by name)
    if (st)
        for (size_t t = 0u; t < st->get_n_transitions(); ++t)
        {
            const auto & tr = st->get_transition(static_cast< int >(t));
            for (int s : tr.when)
                nh->check_stage(s, "when");
            if (
                (tr.kind == TransitionKind::after) &&
                (tr.duration.get_anchor() >= 0)
            )
                nh->check_stage(tr.duration.get_anchor(), "anchor");
            for (size_t u = 0u; u < nh->get_n_transitions(); ++u)
                if (nh->get_transition(static_cast< int >(u)).name == tr.name)
                    throw std::logic_error(
                        "The virus and the statuses both have a transition "
                        "named \"" + tr.name + "\"; give them different names."
                    );
        }

    // Every cell with a drawn exit (of its stage or its status) is updated
    UpdateFun update = [](Agent * p, Model * m) -> void {
        m->nh_step(*p);
    };

    states_labels.clear();
    state_fun.clear();
    nstates = 0u;
    infectious_states.clear();
    excluded_states.clear();
    nh_queue.assign(static_cast< size_t >(ns * nk), 0);

    for (int s = 0; s < ns; ++s)
        for (int k = 0; k < nk; ++k)
        {

            const auto & sinfo = nh->get_stage_info(s);
            const bool status_drawn = st && st->get_stage_info(k).needs_visit;
            const bool drawn = (sinfo.infection >= 0) || sinfo.needs_visit ||
                status_drawn;

            states_labels.push_back(
                (nk == 1) ? nh->get_stages()[s] :
                st->get_stages()[k] + " / " + nh->get_stages()[s]
            );
            state_fun.push_back(drawn ? update : nullptr);

            const int c = s * nk + k;
            const bool excluded = (nh->get_contact(s) <= 0.0) ||
                (st && (st->get_contact(k) <= 0.0));

            if (excluded)
                excluded_states.push_back(c);
            else if (nh->is_infectious(s))
                infectious_states.push_back(c);

            // Carriers in the layers queue their contacts; the others with a
            // drawn exit queue themselves, except those whose only exit is an
            // infection drawn from their contacts (the default sampler),
            // which the carriers queue. A custom sampler need not use the
            // contacts, so its agents queue themselves.
            const bool contact_driven = sinfo.pushable && !status_drawn;
            nh_queue[c] = (nh->is_carrier(s) && !excluded) ? 2 :
                ((drawn && !contact_driven) ? 1 : 0);

        }

    nstates = static_cast< epiworld_fast_uint >(ns * nk);

    // The index has one slot per state; it is rebuilt when the next run starts
    state_index_ready = false;

}

// Accessors -------------------------------------------------------------------

inline int Model::get_stage(const Agent & agent) const
{
    if (!nh)
        throw std::logic_error("The model has no natural history.");
    return nh_stage_of(agent.state);
}

inline int Model::get_status(const Agent & agent) const
{
    if (!nh)
        throw std::logic_error("The model has no natural history.");
    return nh_status_of(agent.state);
}

inline int Model::transition_id(std::string_view name) const
{

    if (!nh)
        throw std::logic_error("The model has no natural history.");

    for (size_t t = 0u; t < nh->get_n_transitions(); ++t)
        if (nh->get_transition(static_cast< int >(t)).name == name)
            return static_cast< int >(t);

    if (st)
        return st->transition_id(name);

    throw std::logic_error(
        "The model has no transition \"" + std::string(name) + "\"."
    );

}

inline int Model::get_stage_entered(size_t id, int stage) const
{
    if (!nh)
        throw std::logic_error("The model has no natural history.");
    nh->check_stage(stage, "");
    return nh_entered.at(id * nh->get_n_stages() + static_cast< size_t >(stage));
}

inline int Model::get_stage_until(size_t id) const
{
    if (!nh)
        throw std::logic_error("The model has no natural history.");
    return nh_until.at(id);
}

inline int Model::get_episode(size_t id) const
{
    if (!nh)
        throw std::logic_error("The model has no natural history.");
    return nh_episode.at(id);
}

inline bool Model::nh_removed(int stage) const
{
    // A stage that takes agents out of every layer and has no exit
    if (nh->get_contact(stage) > 0.0)
        return false;
    for (size_t t = 0u; t < nh->get_n_transitions(); ++t)
        if (nh->is_source(static_cast< int >(t), stage))
            return false;
    return true;
}

// Moving agents from code ------------------------------------------------------

inline bool Model::fire(int transition, Agent & agent)
{

    if (!nh)
        throw std::logic_error("fire() needs a natural history.");

    const int s = nh_stage_of(agent.state);

    if (nh_is_status_tr(transition))
    {

        const int k = nh_status_tr(transition);
        if (!st || (static_cast< size_t >(k) >= st->get_n_transitions()))
            throw std::range_error(
                "There is no status transition " + std::to_string(k) + "."
            );

        if (!st->is_source(k, nh_status_of(agent.state)) || nh_removed(s))
            return false;

    }
    else
    {

        if ((transition < 0) || (static_cast< size_t >(transition) >= nh->get_n_transitions()))
            throw std::range_error(
                "There is no transition " + std::to_string(transition) + "."
            );

        if (nh->get_transition(transition).kind == TransitionKind::infection)
            throw std::logic_error(
                "An infection is not fired; seed the virus instead."
            );

        if (!nh->is_source(transition, s))
            return false;

    }

    nh_propose(transition, agent);
    return true;

}

inline void Model::react(Trigger on, int transition)
{
    if (!nh)
        throw std::logic_error("react() needs a natural history.");
    nh_reactions.push_back({std::move(on), transition, nullptr});
}

inline void Model::react(
    Trigger on,
    std::function< void(Agent &, Model &) > fun
)
{
    if (!nh)
        throw std::logic_error("react() needs a natural history.");
    nh_reactions.push_back({std::move(on), -1, std::move(fun)});
}

inline void Model::set_initial(Assign a)
{

    if (!nh)
        throw std::logic_error("set_initial() needs a natural history.");

    nh_initial = std::move(a);

}

// The run ---------------------------------------------------------------------

inline void Model::nh_reset()
{

    const size_t n  = population.size();
    const size_t ns = nh->get_n_stages();

    nh_until.assign(n, INT_MAX);
    nh_entered.assign(n * ns, -1);
    nh_episode.assign(n, 0);
    nh_status_until.assign(n, INT_MAX);
    nh_status_since.assign(n, 0);
    nh_status_sod.assign(n, 0);
    nh_pending = -1;

    // Everyone starts in the first stage and the first status, on day 0
    const int after = nh->stages[0u].after;
    const int s_after = st ? st->stages[0u].after : -1;
    for (auto & p : population)
    {
        const size_t id = static_cast< size_t >(p.id);
        nh_entered[id * ns] = 0;
        if (after >= 0)
            nh_until[id] = nh->transitions[after].duration.draw(p, *this);
        if (s_after >= 0)
            nh_status_until[id] = st->transitions[s_after].duration.draw(p, *this);
    }

    if (use_queuing && (nh_queue[0u] == 1))
        for (size_t id = 0u; id < n; ++id)
            queue.shift(id, 1);

}

inline void Model::nh_step(Agent & p)
{

    const unsigned int c = p.state;
    const int s = nh_stage_of(c);
    const StageInfo & si = nh->stages[static_cast< size_t >(s)];

    // The stage's proposal --------------------------------------------------
    bool proposed = false;

    // 1. Infection by contact (not out of the layers)
    if (
        (si.infection >= 0) && (p.virus == nullptr) &&
        (state_excluded.empty() || !state_excluded[c])
    )
    {

        const auto & tr = nh_of(p).transitions[si.infection];
        Virus * v = tr.sampler ? tr.sampler(p, *this) : sample_virus(&p);
        if (v != nullptr)
        {
            nh_propose(si.infection, p, v);
            proposed = true;
        }

    }

    // 2. The rates compete: one draw picks one, or none
    const size_t nr = si.rates.size();
    if (!proposed && (nr > 0u))
    {

        reserve_tmp(nr);
        for (size_t k = 0u; k < nr; ++k)
        {

            const auto & tr = nh_of(p).transitions[si.rates[k]];
            epiworld_double x = tr.value.eval(p, *this);

            if ((tr.effect != Effect::none) && (p.virus != nullptr))
            {
                if (tr.effect == Effect::recovery)
                    x = 1.0 - (1.0 - x) * (1.0 - p.get_recovery_enhancer(p.virus, *this));
                else
                    x = x * (1.0 - p.get_death_reduction(p.virus, *this));
            }

            array_double_tmp[k] = x;

        }

        const int which = roulette(static_cast< epiworld_fast_uint >(nr), this);
        if (which >= 0)
        {
            nh_propose(si.rates[static_cast< size_t >(which)], p);
            proposed = true;
        }

    }

    // 3. The duration, if it ended (a deadline: the rates went first)
    if (!proposed && (si.after >= 0) && (nh_until[static_cast< size_t >(p.id)] <= today()))
        nh_propose(si.after, p);

    // The status's proposal (judged on the stage at the start of the day) ---
    if (!st)
        return;

    const int k = nh_status_of(c);
    const StageInfo & ki = st->stages[static_cast< size_t >(k)];

    auto applies = [s](const Transition & tr) -> bool {
        return tr.when.empty() ||
            (std::find(tr.when.begin(), tr.when.end(), s) != tr.when.end());
    };

    const size_t nkr = ki.rates.size();
    if (nkr > 0u)
    {

        reserve_tmp(nkr);
        for (size_t r = 0u; r < nkr; ++r)
        {
            const auto & tr = st->transitions[ki.rates[r]];
            array_double_tmp[r] = applies(tr) ? tr.value.eval(p, *this) : 0.0;
        }

        const int which = roulette(static_cast< epiworld_fast_uint >(nkr), this);
        if (which >= 0)
        {
            nh_propose(-2 - ki.rates[static_cast< size_t >(which)], p);
            return;
        }

    }

    if (
        (ki.after >= 0) &&
        (nh_status_until[static_cast< size_t >(p.id)] <= today()) &&
        applies(st->transitions[ki.after])
    )
        nh_propose(-2 - ki.after, p);

}

inline void Model::nh_propose(
    int transition,
    Agent & p,
    Virus * virus
)
{

    const bool status = nh_is_status_tr(transition);

    // The infection's target is the infecting virus's; the others, the agent's
    // (a status transition, the model's statuses)
    const auto & tr = status ?
        st->transitions[nh_status_tr(transition)] :
        (((virus != nullptr) && virus->nh_def) ?
            virus->nh_def->transitions[transition] :
            nh_of(p).transitions[transition]);

    // A branch draws its target
    int to = tr.to;
    if (!tr.branch_to.empty())
    {

        const epiworld_double u = runif();
        epiworld_double cum = 0.0;
        for (size_t k = 0u; k < tr.branch_to.size(); ++k)
        {
            cum += tr.branch_prob[k].eval(p, *this);
            to = tr.branch_to[k];
            if (u < cum)
                break;
        }

    }

    // The event carries the target (a stage or a status); events_run() makes
    // it a cell when it applies it
    nh_pending = transition;

    const int s = nh_stage_of(p.state);
    if (status)
        p.change_state(*this, static_cast< epiworld_fast_uint >(to), Queue::NoOne);
    else if (tr.kind == TransitionKind::infection)
        p.set_virus(*this, *virus, to, Queue::NoOne);
    else if (nh->is_carrier(s) && !nh->is_carrier(to))
        p.rm_virus(*this, to, Queue::NoOne);
    else
        p.change_state(*this, static_cast< epiworld_fast_uint >(to), Queue::NoOne);

    nh_pending = -1;

}

inline void Model::nh_enter(
    Agent & p,
    unsigned int stage_old,
    int transition
)
{

    const size_t id = static_cast< size_t >(p.id);
    const size_t ns = nh->get_n_stages();
    const int s = nh_stage_of(p.state);

    // A new infection starts a new episode: its entry days start afresh
    const bool infected =
        (transition == NH_SEED) ||
        (!nh->is_carrier(static_cast< int >(stage_old)) && nh->is_carrier(s));

    int * entered = nh_entered.data() + id * ns;
    if (infected)
    {
        ++nh_episode[id];
        std::fill(entered, entered + ns, -1);
    }

    entered[s] = today();

    const int after = nh->stages[static_cast< size_t >(s)].after;
    if (after >= 0)
    {

        const auto & d = nh_of(p).transitions[after].duration;
        int base = today();
        if ((d.get_anchor() >= 0) && (entered[d.get_anchor()] >= 0))
            base = entered[d.get_anchor()];

        nh_until[id] = base + d.draw(p, *this);

    }
    else
        nh_until[id] = INT_MAX;

}

inline void Model::nh_react(Agent & p, int transition, int stage_old)
{

    if (nh_reactions.empty())
        return;

    const size_t id = static_cast< size_t >(p.id);
    const int s = nh_stage_of(p.state);

    // The status at the start of the day (the agent changed today)
    const int status_sod = nh_status_sod[id];

    for (const auto & r : nh_reactions)
    {

        const bool hit = (r.on.kind == Trigger::Kind::entered) ?
            ((s == r.on.target) && (stage_old != s)) :
            (transition == r.on.target);

        if (!hit)
            continue;

        if (
            !r.on.was_status.empty() &&
            (std::find(r.on.was_status.begin(), r.on.was_status.end(), status_sod) ==
                r.on.was_status.end())
        )
            continue;

        if (r.fun)
            r.fun(p, *this);
        else
            fire(r.transition, p);

    }

}

inline void Model::nh_queue_move(Agent & p, unsigned int state_old)
{

    const char from = nh_queue[state_old];
    const char to = nh_queue[p.state];
    if (from == to)
        return;

    const size_t id = static_cast< size_t >(p.id);

    if (from == 2)
        queue -= &p;
    else if (from == 1)
        queue.shift(id, -1);

    if (to == 2)
        queue += &p;
    else if (to == 1)
        queue.shift(id, 1);

}

inline void Model::nh_initial_place()
{

    if (!nh_initial.is_set())
        return;

    // The agents left without the virus, in the first stage
    std::vector< char > eligible(population.size());
    for (auto & p : population)
        eligible[static_cast< size_t >(p.id)] =
            (p.virus == nullptr) && (nh_stage_of(p.state) == 0);

    const std::vector< int > labels = nh_initial.draw(*this, eligible);

    for (size_t i = 0u; i < labels.size(); ++i)
    {

        const int s = labels[i];
        if (s == -1)
            continue;

        if (
            (s < 0) || (static_cast< size_t >(s) >= nh->get_n_stages()) ||
            nh->is_carrier(s)
        )
            throw std::logic_error(
                "set_initial(): the label " + std::to_string(s) + " is not a "
                "stage without the virus (an initial stage is a stage, e.g., "
                "assign::shares({{R, 0.1}}); carriers are seeded with the "
                "virus)."
            );

        nh_pending = NH_INITIAL;
        population[i].change_state(
            *this, static_cast< epiworld_fast_uint >(s), Queue::NoOne
        );
        nh_pending = -1;

    }

    events_run();

}

inline epiworld_double Model::nh_infection_prob(
    Agent & target,
    const Virus & v
)
{
    const int tr = nh->stages[static_cast< size_t >(nh_stage_of(target.state))].infection;
    if (tr < 0)
        return 0.0;

    // The infector's virus says how infectious it is
    const auto & h = v.nh_def ? *v.nh_def : *nh;
    return h.transitions[tr].value.eval(target, &v, *this);
}

inline std::string Model::draw_natural_history() const
{

    if (!nh)
        throw std::logic_error(
            "The model has no natural history to draw (add a virus first)."
        );

    std::string out = "flowchart LR\n";

    auto subgraph = [&out](
        const std::string & id,
        const std::string & title,
        const std::string & body
    ) {
        out += "    subgraph " + id + "[\"" + title + "\"]\n";
        out += "        direction LR\n";
        std::istringstream lines(body);
        std::string line;
        while (std::getline(lines, line))
            out += "    " + line + "\n";
        out += "    end\n";
    };

    subgraph("stages", "Stages: " + nh->get_name(), nh->draw_mermaid("s"));
    if (st)
        subgraph("statuses", "Statuses", st->draw_mermaid("q", nh.get()));

    out +=
        "    classDef infectious fill:#f8d7da,stroke:#b02a37\n"
        "    classDef carrier fill:#fff3cd,stroke:#997404\n"
        "    classDef away fill:#e2e3e5,stroke:#6c757d\n";

    return out;

}

#endif
/*//////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

 End of -./include/epiworld/model-meat-naturalhistory.hpp-

////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////*/



/*//////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

 Start of -./include/epiworld/tools/vaccine.hpp-

////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////*/


#ifndef EPIWORLD_TOOLS_VACCINE_HPP
#define EPIWORLD_TOOLS_VACCINE_HPP

#include <memory>
#include <vector>
#include <cassert>
// (already included include/epiworld/tools/../config.hpp)
// (already included include/epiworld/tools/../tool-bones.hpp)

/**
 * @brief Template for a common vaccine
 * 
 * The main difference with a regular tool is that the reduction
 * in susceptibility is at the agent level and fixed for the
 * entire simulation. In other words, if the efficacy is 65%,
 * then 65% of the agents that receive the vaccine will be fully
 * protected (100% reduction in susceptibility) and 35% will not be
 * protected at all (0% reduction in susceptibility).
 * 
 * @ingroup tools
 * 
 */
class ToolVaccine: public Tool {
private:

    int _immune = -1;
    epiworld_double _efficacy = 0.0;
    epiworld_double _set_immunity(Model & model);

public:
    ToolVaccine(std::string name = "Vaccine") : Tool(name) {};

    virtual epiworld_double get_susceptibility_reduction(
        VirusPtr & v,
        Model * model
    ) override;
    
    virtual void set_susceptibility_reduction_fun(ToolFun fun) override;
    virtual void set_susceptibility_reduction(std::string param) override;
    virtual void set_susceptibility_reduction(epiworld_double prob) override;

    std::unique_ptr<Tool> clone_ptr() const override;

};

inline epiworld_double ToolVaccine::get_susceptibility_reduction(
    VirusPtr &,
    Model * model
)
{

    // Updating a single agent (if needed)
    if (_immune == -1)
    {
        _set_immunity(*model);
    }

    return  (_immune == 1) ? 1.0 : 0.0;

}

inline void ToolVaccine::set_susceptibility_reduction_fun(
    ToolFun
)
{
    throw std::logic_error(
        std::string(
            "The susceptibility reduction function cannot be set for the "
         ) +
        std::string("ToolVaccine (Tool).")
    );
}

inline void ToolVaccine::set_susceptibility_reduction(std::string)
{
    throw std::logic_error(
        std::string(
            "The susceptibility reduction probability cannot be set for the "
        ) +
        std::string(
            "ToolVaccine (Tool) using a pointer. Use the version that takes a "
        ) +
        std::string("double instead.")
    );
}

inline void ToolVaccine::set_susceptibility_reduction(epiworld_double prob)
{

    // assertm(prob >= 0 && prob <= 1, "The efficacy must be between 0 and 1.");
    _efficacy = prob;

}

inline epiworld_double ToolVaccine::_set_immunity(Model & model)
{
    _immune = (model.runif() < _efficacy) ? 1 : 0;
    return _immune;
}

inline std::unique_ptr<Tool> ToolVaccine::clone_ptr() const
{
    auto ans =  std::make_unique<ToolVaccine>(*this);
    ans->_immune = -1;
    return ans;
}

#endif
/*//////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

 End of -./include/epiworld/tools/vaccine.hpp-

////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////*/


/*//////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

 Start of -./include/epiworld/globalevents/quarantinetrigger-meat.hpp-

////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////*/


#ifndef QUARANTINETRIGGER_MEAT_HPP
#define QUARANTINETRIGGER_MEAT_HPP
// (already included include/epiworld/globalevents/quarantinetrigger-bones.hpp)

inline void QuarantineTrigger::_setup(const Model & model) {

    if (
        (static_cast<int>(model.get_sim_id()) != _model_sim_id) ||
        (static_cast<int>(model.today()) != _day)
    ) {
        _model_sim_id = static_cast<int>(model.get_sim_id());
        _day = static_cast<int>(model.today());
        _agents_triggering_quarantine.clear();
        _date_infectious.clear();
    }

}

inline void QuarantineTrigger::add_triggering_agent(
    const Model & model,
    const Agent & agent,
    int date_infectious
) {
    
    // Ensuring we are not in a different run
    _setup(model);
    this->_agents_triggering_quarantine.push_back(agent.get_id());
    this->_date_infectious.push_back(date_infectious);
}

inline std::vector< size_t > & QuarantineTrigger::get_triggering_agents() {
    return this->_agents_triggering_quarantine;
}

inline std::vector< int > & QuarantineTrigger::get_date_infectious() {
    return this->_date_infectious;
}

#endif
/*//////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

 End of -./include/epiworld/globalevents/quarantinetrigger-meat.hpp-

////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////*/


/*//////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

 Start of -./include/epiworld/globalevents/bubbles-meat.hpp-

////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////*/


#ifndef EPIWORLD_GLOBALEVENTS_BUBBLES_MEAT_HPP
#define EPIWORLD_GLOBALEVENTS_BUBBLES_MEAT_HPP

// Standard library headers are included at global scope by epiworld.hpp (this
// file is included from within `namespace epiworld`, so system headers must not
// be re-included here).
// (already included include/epiworld/globalevents/bubbles-bones.hpp)

inline BubbleTool::BubbleTool(
    std::string name,
    std::string event_name
) : Tool(name), _event_name(std::move(event_name))
{
}

inline epiworld_double BubbleTool::get_susceptibility_reduction(
    VirusPtr & v,
    Model * model
)
{

    // Binding to the policy of *this* model, once. clone_ptr() clears the
    // pointer, so a copy of this tool -- in another agent, or in a copy of the
    // model -- resolves against its own model rather than inheriting ours.
    if (_policy == nullptr)
    {

        _policy = Bubbles::get_from(*model, _event_name);

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

inline std::unique_ptr<Tool> BubbleTool::clone_ptr() const
{
    auto ans = std::make_unique<BubbleTool>(*this);
    ans->_policy = nullptr; // the copy resolves against its own model
    return ans;
}

inline Bubbles::Bubbles(
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
    BubbleTies ties,
    std::string layer,
    double bubble_contact_prob
) :
    flavor(flavor),
    group_size(group_size),
    max_households(max_households),
    transmission_factor(transmission_factor),
    start_day(start_day),
    end_day(end_day),
    rewire_every(rewire_every),
    param_name(std::move(param_name)),
    ties(ties),
    layer(std::move(layer)),
    bubble_contact_prob(bubble_contact_prob)
{

    // Households by compact index, in order of first appearance
    std::unordered_map< size_t, size_t > index;
    household.reserve(household_id.size());
    for (size_t h : household_id)
        household.push_back(index.emplace(h, index.size()).first->second);

    n_households = index.size();

    this->set_name(name);
    this->set_day(-99); // runs at the end of every day

    if ((this->transmission_factor < 0.0) || (this->transmission_factor > 1.0))
        throw std::range_error(
            "Bubbles: transmission_factor must be in [0, 1]."
        );

    if (!((bubble_contact_prob >= 0.0) && (bubble_contact_prob <= 1.0)))
        throw std::range_error(
            "Bubbles: bubble_contact_prob must be in [0, 1]."
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

inline void Bubbles::partition_household(Model * model)
{

    size_t nh = n_households;

    // Build the household contact graph: households h1 and h2 are adjacent when
    // at least one member of h1 is connected to a member of h2 in the contact
    // network. Bubbles are grown along these ties.
    //
    // Grouping households that share NO tie would be a no-op: the intervention
    // can only suppress transmission along existing edges, never create new
    // ones, so bubbling two unconnected households changes nothing. Pairing at
    // random therefore degenerates to the household-only lockdown. It also
    // matches the policy being modelled: a household picks a bubble partner it
    // actually socialises with. Only network layers count: a bubble layer is
    // a mixing layer (see make_layer()).
    std::vector< std::vector< size_t > > hh_adj(nh);
    for (size_t a = 0u; a < household.size(); ++a)
    {
        size_t ha = household[a];
        for (auto * net : model->get_network_layers())
        for (auto * nb : net->neighbors_view(a))
        {
            size_t hb = household[static_cast< size_t >(nb->get_id())];
            if (hb != ha)
                hh_adj[ha].push_back(hb);
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
    for (size_t a = 0u; a < household.size(); ++a)
        bubble_id[a] = hh_bubble[household[a]];

}

inline void Bubbles::partition_peer(Model * model)
{

    size_t n  = household.size();
    size_t nh = n_households;

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

    std::vector< size_t > ext;

    for (size_t oi = 0u; oi < n; ++oi)
    {

        size_t a  = agent_order[oi];
        size_t ha = household[a];

        // This agent's household is already in a full bubble: it is out of the
        // pool and cannot take anyone else in.
        if (set_size[find(ha)] >= max_households)
            continue;

        // Households of this agent's contacts outside its own household
        ext.clear();
        for (auto * net : model->get_network_layers())
        for (auto * nb : net->neighbors_view(a))
        {
            size_t hb = household[static_cast< size_t >(nb->get_id())];
            if (hb != ha)
                ext.push_back(hb);
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
        size_t root = find(household[a]);
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

inline std::shared_ptr< layer::Mixing > Bubbles::make_layer(
    double household_contact_prob
) const
{

    if (layer.empty())
        throw std::logic_error(
            "Bubbles: make_layer() needs the layer's name; give it with "
            "set_ties(BubbleTies::Complete, name)."
        );

    // Everyone with their household: the bubbles are larger groups of the
    // same layer (households are never split)
    auto res = layer::mixing(layer);
    res->set_groups(assign::labels(household), n_households);
    res->set_contact_prob(household_contact_prob);

    return res;

}

inline layer::Mixing & Bubbles::bubble_layer(
    Model * model
) const
{

    layer::Mixing * res = (!layer.empty() && model->has_layer(layer)) ?
        model->get_layer(layer).as_mixing() : nullptr;

    if ((res == nullptr) || !res->within_groups())
        throw std::logic_error(
            "Bubbles: BubbleTies::Complete needs its within-group mixing layer "
            "'" + layer + "' in the model; add it with "
            "model.add_layer(bubbles.make_layer())."
        );

    return *res;

}

inline void Bubbles::sync_groups(Model * model)
{

    // Global events run after the day's transitions, so the moves take effect
    // at the end of the step (the layer applies them in its update): what
    // matters is whether the policy applies the next day, which is also when
    // the tool starts (or stops) damping.
    bool want = (ties == BubbleTies::Complete) &&
        is_active(static_cast< int >(model->today()) + 1);

    // The partition agents should be in (-1: their homes), and the one they are
    int target = want ? last_epoch : -1;
    if (target == placed_epoch)
        return;

    auto & groups = bubble_layer(model);
    for (size_t i = 0u; i < household.size(); ++i)
        groups.set_group(
            i, want ? static_cast< size_t >(bubble_id[i]) : household[i]
        );

    // One contact probability per group: a bubble that merged households
    // meets at the bubble's; a household on its own keeps its own
    if (want)
    {
        std::vector< double > q(home_prob.size(), bubble_contact_prob);
        std::vector< size_t > first(home_prob.size(), layer::Mixing::NO_GROUP);
        std::vector< bool > merged(home_prob.size(), false);
        for (size_t i = 0u; i < household.size(); ++i)
        {
            const size_t b = static_cast< size_t >(bubble_id[i]);
            if (first[b] == layer::Mixing::NO_GROUP)
                first[b] = household[i];
            else if (first[b] != household[i])
                merged[b] = true;
        }

        for (size_t b = 0u; b < q.size(); ++b)
            if (!merged[b] && (first[b] != layer::Mixing::NO_GROUP))
                q[b] = home_prob[first[b]];

        groups.set_contact_prob(std::move(q));
    }
    else
        groups.set_contact_prob(home_prob);

    placed_epoch = target;

}

inline void Bubbles::compute_partition(Model * model)
{

    bubble_id.assign(household.size(), -1);

    // Both rules read the network layers only, so a bubble layer (a mixing
    // layer) never pulls the next bubble towards the last one: BubbleTies
    // does not change the grouping.
    if (flavor == BubbleFlavor::Household)
        partition_household(model);
    else
        partition_peer(model);

}

inline bool Bubbles::is_active(int today) const
{
    return (today >= start_day) && ((end_day < 0) || (today < end_day));
}

inline epiworld_double Bubbles::susceptibility_reduction(
    const Agent * p,
    const Agent * transmitter,
    Model * model
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

inline void Bubbles::_setup(Model * model)
{

    if (household.size() != model->size())
        throw std::length_error(
            "Bubbles: household_id length (" +
            std::to_string(household.size()) +
            ") must equal the number of agents (" +
            std::to_string(model->size()) + ")."
        );

    if (ties == BubbleTies::Complete)
    {

        auto & groups = bubble_layer(model);

        // An agent is in one group per layer, so two policies need two layers
        for (size_t e = 0u; e < model->get_n_globalevents(); ++e)
        {
            const auto * other = dynamic_cast< const Bubbles * >(
                &model->get_globalevent(e)
            );

            if ((other != nullptr) && (other != this) &&
                (other->ties == BubbleTies::Complete) && (other->layer == layer))
                throw std::logic_error(
                    "Bubbles: two policies move agents in layer '" + layer +
                    "'; give each its own layer (make_layer())."
                );
        }

        // A move outside a run would edit the layer's baseline for good
        if (!model->is_running())
            throw std::logic_error(
                "Bubbles: BubbleTies::Complete runs within Model::run() (the "
                "layer's reset puts everyone back in their household)."
            );

        // The policy moves agents from their households and back, so the
        // layer must start from the household partition (as make_layer()
        // builds it)
        bool households = (groups.get_n_groups() == n_households);
        for (size_t i = 0u; households && (i < household.size()); ++i)
            households = (groups.get_group(i) == household[i]);

        if (!households)
            throw std::logic_error(
                "Bubbles: the layer '" + layer + "' does not have one group "
                "per household, as make_layer() builds it (" +
                std::to_string(groups.get_n_groups()) + " groups for " +
                std::to_string(n_households) + " households); add it with "
                "model.add_layer(bubbles.make_layer())."
            );

        // How often each household meets
        home_prob = groups.get_contact_prob();

    }

    placed_epoch = -1;

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

    // ---- The bubble in its layer -------------------------------------------
    // Under BubbleTies::Complete agents move to their bubble's group of the
    // layer; the moves take effect in the next() that follows the reset, so
    // the bubbles meet from day 1.
    sync_groups(model);

    // ---- The tool: dampens out-of-bubble transmission -----------------------
    // It carries no state of its own: it finds the model's intervention by name
    // the first time it is used. It is registered without a distribution
    // function on purpose -- handing it out here, on every run, keeps the first
    // run and the ones after it (where the tool is already registered)
    // identical.
    if (!model->has_tool(this->get_name()))
    {
        BubbleTool bubble_tool(this->get_name(), this->get_name());
        model->add_tool(bubble_tool);
    }

    auto & bubble_tool = model->get_tool(this->get_name());
    for (size_t i = 0u; i < model->size(); ++i)
        model->get_agent(i).add_tool(*model, bubble_tool);

}

inline void Bubbles::reset(Model * model)
{

    // Model::reset() runs this once per run -- and once per replicate of
    // run_multiple(), on that replicate's own copy of the model -- just before
    // day 1, which is why the user has nothing to call: adding the intervention
    // to the model is the whole installation.
    this->model_id = static_cast< int >(model->get_sim_id());
    this->_setup(model);

}

inline void Bubbles::operator()(Model * model, int day)
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
    // rewiring epoch, and keep the bubble layer matching the policy. The tool gates
    // itself by day. Because global events run after update_state(), both take
    // effect the following simulation step.
    if ((rewire_every > 0) && is_active(day))
    {

        int epoch = (day - start_day) / rewire_every;

        if (last_epoch != epoch)
        {

            compute_partition(model);
            last_epoch = epoch;

        }

    }

    // Moves agents into their bubbles on the day before the policy starts,
    // into the new ones at a rewiring epoch, and back when the policy lifts
    sync_groups(model);

}

inline Bubbles * Bubbles::get_from(
    Model & model,
    const std::string & name
)
{

    if (!model.has_globalevent(name))
        return nullptr;

    return dynamic_cast< Bubbles * >(&model.get_globalevent(name));

}

inline const std::vector< int > & Bubbles::get_bubble_id() const
{
    return bubble_id;
}

inline int Bubbles::get_last_epoch() const
{
    return last_epoch;
}

inline BubbleFlavor Bubbles::get_flavor() const
{
    return flavor;
}

inline BubbleTies Bubbles::get_ties() const
{
    return ties;
}

inline void Bubbles::set_ties(
    BubbleTies ties,
    std::string layer,
    double bubble_contact_prob
)
{

    if (!((bubble_contact_prob >= 0.0) && (bubble_contact_prob <= 1.0)))
        throw std::range_error(
            "Bubbles: bubble_contact_prob must be in [0, 1]."
        );

    this->ties = ties;
    this->layer = std::move(layer);
    this->bubble_contact_prob = bubble_contact_prob;

}

inline const std::string & Bubbles::get_param_name() const
{
    return param_name;
}

inline std::unique_ptr< GlobalEvent > Bubbles::clone_ptr() const
{
    return std::make_unique< Bubbles >(*this);
}

#endif
/*//////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

 End of -./include/epiworld/globalevents/bubbles-meat.hpp-

////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////*/



/*//////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

 Start of -./include/epiworld/registry-bones.hpp-

////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////*/


#ifndef EPIWORLD_REGISTRY_BONES_HPP
#define EPIWORLD_REGISTRY_BONES_HPP

/**
 * @brief One argument of a registered model's constructor.
 * @ingroup epi_models
 *
 * @details `arg` is the argument's name, the one every wrapper uses (R, Python
 * and JS take `transmission_prob`, not each their own). `name` is the model
 * parameter the argument sets, as `Model::get_param()` knows it
 * ("Transmission probability"); it is empty for the arguments that are not
 * parameters (the population size `n`, the prevalence). `min` and `max` are
 * hard limits, checked by `ModelRegistry::make_model()` before the model is
 * built.
 *
 * The static functions build the kinds that repeat across models.
 */
struct ParamInfo {
    std::string arg;           ///< Argument name, e.g. "transmission_prob".
    std::string name;          ///< Model parameter it sets, or empty.
    double value;              ///< Default value.
    double min;                ///< Smallest valid value.
    double max;                ///< Largest valid value (may be infinite).
    bool integer;              ///< Whether the value must be a whole number.
    std::string description;   ///< One sentence, for help pages and UIs.

    /// A probability or proportion, in [0, 1].
    static ParamInfo probability(
        std::string arg, std::string name, double value,
        std::string description
    );

    /// A mean duration in days, in [0, inf).
    static ParamInfo duration(
        std::string arg, std::string name, double value,
        std::string description
    );

    /// A whole number of days, where a negative value (-1) turns it off.
    static ParamInfo period(
        std::string arg, std::string name, double value,
        std::string description
    );

    /// A whole number of agents or days, in [0, max].
    static ParamInfo count(
        std::string arg, std::string name, double value,
        std::string description
    );

    /// The population size `n`, at least 1.
    static ParamInfo agents(double value);

    /// The proportion of agents infected at the start, `prevalence`.
    static ParamInfo prevalence(double value);
};

/**
 * @brief The values a registered model's builder receives.
 * @ingroup epi_models
 *
 * @details Complete (every argument of the model, defaults filled in) and
 * checked against the model's `ParamInfo`. `a("transmission_prob")` reads a
 * value; `a.virus_name()` the name of the model's virus.
 */
class ModelArgs {
private:
    std::map< std::string, double > values;
    std::string vname;

public:
    ModelArgs(std::map< std::string, double > values_, std::string vname_);

    /// @throws std::logic_error if `arg` is not one of the model's arguments.
    double operator()(const std::string & arg) const;

    const std::string & virus_name() const noexcept;
};

/**
 * @brief A model the registry can build: its arguments and builder.
 * @ingroup epi_models
 *
 * @details Each registered model returns its own from a static
 * `model_info()`, next to its constructor. `params` lists the constructor's
 * arguments in its order. The builder calls the constructor and nothing else:
 * the model it returns has no layers, which are added the same way as for a
 * model built directly (`Model::add_layer()`). A virus a user adds to the
 * model is a strain of the model's own: a copy of `get_virus(0)` with its own
 * name and, if need be, its own parameters (`Virus::use_param()`), so it has
 * the same stages.
 */
struct ModelInfo {
    std::string id;                   ///< e.g. "SEIR", for make_model()
    std::string label;                ///< e.g. "Susceptible-Exposed-Infected-Recovered"
    std::string description;          ///< One paragraph
    std::string virus_name;           ///< Default virus name; empty if the model names its own
    std::vector< ParamInfo > params;  ///< Constructor arguments, in order
    std::function< std::unique_ptr< Model >(const ModelArgs &) > build;
};

/**
 * @brief Builds models by name, and lists them with their arguments.
 * @ingroup epi_models
 *
 * @details One place per model: the wrappers bind `list_models()` and
 * `make_model()` once instead of every constructor, and read argument names,
 * defaults, limits and descriptions from `ParamInfo`.
 *
 * A registry starts empty. Each library adds its models with its own
 * `register_models()`, defined in its umbrella header next to the includes of
 * the models it lists; a model's entry is its static `model_info()`.
 *
 * ```cpp
 * ModelRegistry registry;
 * epimodels::register_models(registry);            // the core models
 * measles::register_models(registry);              // and, optionally, measles'
 *
 * auto model = registry.make_model(
 *     "SEIR", {{"n", 5000}, {"transmission_prob", 0.1}}, "flu"
 * );                                               // the rest take defaults
 * model->add_layer(layer::network("contacts", graph::smallworld(10, 0.1)));
 * model->run(100, 123);
 * ```
 */
class ModelRegistry {
private:
    std::vector< ModelInfo > models;

public:
    /**
     * @brief Adds a model.
     * @throws std::invalid_argument if the id is empty or already taken, an
     * argument is repeated, a default is outside its limits, or there is no
     * builder.
     */
    ModelRegistry & add(ModelInfo info);

    /// Every model, in the order they were added.
    const std::vector< ModelInfo > & list_models() const noexcept;

    /// @throws std::invalid_argument naming the known ids.
    const ModelInfo & get_model_info(const std::string & id) const;

    /**
     * @brief Builds a model from its id and the values of its arguments.
     * @param id The model, as in `list_models()`.
     * @param values Argument values by `ParamInfo::arg`; the arguments left out
     * take their defaults.
     * @param virus_name Name of the model's virus; empty for the default. Only
     * for the models whose `ModelInfo::virus_name` is not empty.
     * @throws std::invalid_argument for an unknown id or argument, a value
     * outside its limits or not whole when it must be, or a virus name for a
     * model that names its own virus.
     */
    std::unique_ptr< Model > make_model(
        const std::string & id,
        const std::map< std::string, double > & values = {},
        const std::string & virus_name = ""
    ) const;
};

#endif
/*//////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

 End of -./include/epiworld/registry-bones.hpp-

////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////*/


/*//////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

 Start of -./include/epiworld/registry-meat.hpp-

////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////*/


#ifndef EPIWORLD_REGISTRY_MEAT_HPP
#define EPIWORLD_REGISTRY_MEAT_HPP

inline ParamInfo ParamInfo::probability(
    std::string arg, std::string name, double value, std::string description
)
{
    return {std::move(arg), std::move(name), value, 0.0, 1.0, false,
        std::move(description)};
}

inline ParamInfo ParamInfo::duration(
    std::string arg, std::string name, double value, std::string description
)
{
    return {std::move(arg), std::move(name), value, 0.0,
        std::numeric_limits< double >::infinity(), false,
        std::move(description)};
}

inline ParamInfo ParamInfo::period(
    std::string arg, std::string name, double value, std::string description
)
{
    return {std::move(arg), std::move(name), value, -1.0,
        std::numeric_limits< double >::infinity(), true,
        std::move(description)};
}

inline ParamInfo ParamInfo::count(
    std::string arg, std::string name, double value, std::string description
)
{
    return {std::move(arg), std::move(name), value, 0.0,
        static_cast< double >(std::numeric_limits< epiworld_fast_uint >::max()),
        true, std::move(description)};
}

inline ParamInfo ParamInfo::agents(double value)
{
    ParamInfo p = count("n", "", value, "Number of agents.");
    p.min = 1.0;
    return p;
}

inline ParamInfo ParamInfo::prevalence(double value)
{
    return probability("prevalence", "", value,
        "Proportion of agents infected at the start.");
}

inline ModelArgs::ModelArgs(
    std::map< std::string, double > values_, std::string vname_
) : values(std::move(values_)), vname(std::move(vname_)) {}

inline double ModelArgs::operator()(const std::string & arg) const
{
    auto it = values.find(arg);
    if (it == values.end())
        throw std::logic_error(
            "The model's builder reads \"" + arg +
            "\", which is not one of its arguments."
        );
    return it->second;
}

inline const std::string & ModelArgs::virus_name() const noexcept
{
    return vname;
}

inline ModelRegistry & ModelRegistry::add(ModelInfo info)
{

    if (info.id.empty())
        throw std::invalid_argument("A registered model needs an id.");

    for (const auto & m : models)
        if (m.id == info.id)
            throw std::invalid_argument(
                "The registry already has a model \"" + info.id + "\"."
            );

    if (!info.build)
        throw std::invalid_argument(
            "The model \"" + info.id + "\" has no builder."
        );

    for (size_t i = 0u; i < info.params.size(); ++i)
    {
        const auto & p = info.params[i];

        for (size_t j = 0u; j < i; ++j)
            if (info.params[j].arg == p.arg)
                throw std::invalid_argument(
                    "The model \"" + info.id + "\" lists the argument \"" +
                    p.arg + "\" twice."
                );

        if ((p.value < p.min) || (p.value > p.max))
            throw std::invalid_argument(
                "The default of \"" + p.arg + "\" in the model \"" + info.id +
                "\" is outside its limits."
            );
    }

    models.push_back(std::move(info));
    return *this;

}

inline const std::vector< ModelInfo > &
ModelRegistry::list_models() const noexcept
{
    return models;
}

inline const ModelInfo & ModelRegistry::get_model_info(
    const std::string & id
) const
{

    for (const auto & m : models)
        if (m.id == id)
            return m;

    std::string known;
    for (const auto & m : models)
        known += (known.empty() ? "" : ", ") + m.id;

    throw std::invalid_argument(
        "Unknown model \"" + id + "\". The registry has: " + known + "."
    );

}

inline std::unique_ptr< Model > ModelRegistry::make_model(
    const std::string & id,
    const std::map< std::string, double > & values,
    const std::string & virus_name
) const
{

    const auto & info = get_model_info(id);

    // Every value given must be an argument of the model
    for (const auto & kv : values)
    {
        bool found = false;
        for (const auto & p : info.params)
            if (p.arg == kv.first)
            {
                found = true;
                break;
            }

        if (!found)
        {
            std::string known;
            for (const auto & p : info.params)
                known += (known.empty() ? "" : ", ") + p.arg;

            throw std::invalid_argument(
                "The model \"" + id + "\" has no argument \"" + kv.first +
                "\". Its arguments are: " + known + "."
            );
        }
    }

    // Complete with the defaults, and check
    std::map< std::string, double > complete;
    for (const auto & p : info.params)
    {
        auto it = values.find(p.arg);
        const double v = (it == values.end()) ? p.value : it->second;

        if (!(v >= p.min && v <= p.max))
            throw std::invalid_argument(
                "The argument \"" + p.arg + "\" of the model \"" + id +
                "\" must be in [" + std::to_string(p.min) + ", " +
                std::to_string(p.max) + "]; got " + std::to_string(v) + "."
            );

        if (p.integer && (v != std::floor(v)))
            throw std::invalid_argument(
                "The argument \"" + p.arg + "\" of the model \"" + id +
                "\" must be a whole number; got " + std::to_string(v) + "."
            );

        complete[p.arg] = v;
    }

    if (!virus_name.empty() && info.virus_name.empty())
        throw std::invalid_argument(
            "The model \"" + id + "\" names its own virus; it takes no "
            "virus name."
        );

    return info.build(ModelArgs(
        std::move(complete),
        virus_name.empty() ? info.virus_name : virus_name
    ));

}

#endif
/*//////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

 End of -./include/epiworld/registry-meat.hpp-

////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////*/



/*//////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

 Start of -./include/epiworld/models/models.hpp-

////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////*/


#ifndef EPIWORLD_MODELS_HPP
#define EPIWORLD_MODELS_HPP

/**
 * @defgroup epi_models Epidemiological Models
 * @brief Collection of predefined epidemiological models for disease simulation
 * 
 * This module provides a comprehensive collection of epidemiological models commonly
 * used in disease simulation and modeling. A model's states, parameters and
 * update rules describe the course of infection; its layers describe who meets
 * whom (networks, mixing by group, or both; see `Model::add_layer()`). A model
 * can carry several viruses (`Model::add_virus()`), which share its states. The
 * models range from basic compartmental models to quarantine measures and
 * disease-specific implementations.
 * 
 * **Implementation details:**
 * <a href="../impl/core-simulation-models.md">Core Simulation Models</a>,
 * <a href="../impl/extending-the-library-with-models.md">Extending the Library with Models</a>
 */

/**
 * @defgroup basic_compartmental Basic Compartmental Models
 * @ingroup epi_models
 * @brief Fundamental compartmental models (SIS, SIR, SEIR, SEIR with quarantine)
 * 
 * These are the foundational compartmental models in epidemiology that divide 
 * populations into distinct states based on disease status.
 * 
 * **Implementation details:**
 * <a href="../impl/core-simulation-models.md">Core Simulation Models</a>
 */

/**
 * @defgroup death_compartmental Models with Death Compartment
 * @ingroup epi_models
 * @brief Models that include a death state (SIRD, SISD, SEIRD)
 * 
 * Extended compartmental models that explicitly track mortality as a separate 
 * compartment in the disease progression.
 * 
 * **Implementation details:**
 * <a href="../impl/core-simulation-models.md">Core Simulation Models</a>
 */

/**
 * @defgroup disease_specific Disease-Specific Models
 * @ingroup epi_models
 * @brief Models tailored for specific diseases (Measles variants, Cholera)
 * 
 * Specialized implementations designed to capture the unique transmission dynamics 
 * and characteristics of specific infectious diseases.
 * 
 * **Implementation details:**
 * <a href="../impl/core-simulation-models.md">Core Simulation Models</a>,
 * <a href="../impl/quarantine-isolation-and-contact-tracing.md">Quarantine, Isolation, and Contact Tracing</a>
 */

/**
 * @defgroup special_models Specialized Models
 * @ingroup epi_models
 * @brief Other specialized models (DiffNet, SIRLogit, Surveillance)
 * 
 * Models for specific scenarios including diffusion networks, logistic regression-based 
 * transmission, and surveillance systems.
 */

/**
 * @defgroup model_utilities Model Utilities
 * @ingroup epi_models
 * @brief Utility functions for model initialization and global events
 * 
 * Helper functions and global event handlers that can be used across different 
 * epidemiological models for initialization and event management.
 * 
 * **Implementation details:**
 * <a href="../impl/events-and-multi-event-handling.md">Events and Multi-Event Handling</a>,
 * <a href="../impl/extending-the-library.md">Extending the Library</a>
 */

/**
 * @namespace epimodels
 * @brief Namespace containing predefined epidemiological models and related functions
 * @ingroup epi_models
 * 
 * This namespace provides a collection of epidemiological models commonly used in
 * disease simulation and modeling. It includes various compartmental models such as
 * SIS, SIR, SEIR, and their variations, as well as specialized models for specific
 * scenarios like quarantine measures and school environments.
 * 
 * The namespace also contains initialization functions, global event handlers, and
 * surveillance mechanisms that can be used across different epidemiological models.
 * 
 * Available model categories include:
 * 
 *   - Basic compartmental models (SIS, SIR, SEIR)
 *   - Models with death compartments (SIRD, SISD, SEIRD)
 *   - Logistic regression models (SIRLogit)
 *   - Quarantine models (SEIRQuarantine)
 *   - School-specific models (MeaslesSchool)
 *   - Disease-specific models (MeaslesMixing, MeaslesMixingRiskQuarantine)
 *   - Diffusion network models (DiffNet)
 *   - Surveillance systems
 * 
 */
namespace epimodels {


/*//////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

 Start of -include/epiworld/models/globalevents.hpp-

////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////*/


#ifndef EPIWORLD_GLOBALEVENTS_HPP
#define EPIWORLD_GLOBALEVENTS_HPP

// (already included include/epiworld/models/../model-bones.hpp)

// This function creates a global action that distributes a tool
// to agents with probability p.
/**
 * @brief Global event that distributes a tool to agents with probability p.
 * @ingroup model_utilities
 * 
 * @param p Probability of distributing the tool.
 * @param tool Tool function.
 * @return std::function<void(Model*)> 
 */
inline std::function<void(Model*)> globalevent_tool(
    Tool & tool,
    double p
) {

    std::function<void(Model*)> fun = [p,&tool](
        Model * model
        ) -> void {

        for (auto & agent : model->get_agents())
        {

            // Check if the agent has the tool
            if (agent.has_tool(tool))
                continue;

            // Adding the tool
            if (model->runif() < p)
                agent.add_tool(*model, tool);
            
        
        }

        #ifdef EPIWORLD_DEBUG
        tool.print();
        #endif

        return;
            

    };

    return fun;

}

// Same function as above, but p is now a function of a vector of coefficients
// and a vector of variables.
/**
 * @brief Global event that distributes a tool to agents with probability
 * p = 1 / (1 + exp(-\sum_i coef_i * agent(vars_i))).
 * @ingroup model_utilities
 * 
 * @param coefs Vector of coefficients.
 * @param vars Vector of variables.
 * @param tool_fun Tool function.
 * @return std::function<void(Model*)> 
 */
inline std::function<void(Model*)> globalevent_tool_logit(
    Tool & tool,
    std::vector< size_t > vars,
    std::vector< double > coefs
) {

    std::function<void(Model*)> fun = [coefs,vars,&tool](
        Model * model
        ) -> void {

        for (auto & agent : model->get_agents())
        {

            // Check if the agent has the tool
            if (agent.has_tool(tool))
                continue;

            // Computing the probability using a logit. Uses OpenMP reduction
            // to sum the coefficients.
            double p = 0.0;
            #if defined(__OPENMP) || defined(_OPENMP)
            #pragma omp parallel for reduction(+:p)
            #endif
            for (size_t i = 0u; i < coefs.size(); ++i)
                p += coefs.at(i) * agent(vars[i], *model);

            p = 1.0 / (1.0 + std::exp(-p));

            // Adding the tool
            if (model->runif() < p)
                agent.add_tool(*model, tool);
            
        
        }

        #ifdef EPIWORLD_DEBUG
        tool.print();
        #endif

        return;
            

    };

    return fun;

}

// A global action that updates a parameter in the model.
/**
 * @brief Global event that updates a parameter in the model.
 * @ingroup model_utilities
 * 
 * @param param Parameter to update.
 * @param value Value to update the parameter to.
 * @return std::function<void(Model*)> 
 */
inline std::function<void(Model*)> globalevent_set_param(
    std::string param,
    double value
) {

    std::function<void(Model*)> fun = [value,param](
        Model * model
        ) -> void {

        model->set_param(param, value);

        return;
            

    };

    return fun;

}
#endif
/*//////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

 End of -include/epiworld/models/globalevents.hpp-

////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////*/



/*//////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

 Start of -include/epiworld/models/sis.hpp-

////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////*/


#ifndef EPIWORLD_MODELS_SIS_HPP 
#define EPIWORLD_MODELS_SIS_HPP

// (already included include/epiworld/models/../model-bones.hpp)

/**
 * @brief A Susceptible-Infected-Susceptible (SIS) model
 * 
 * ![Model Diagram](../assets/img/sis.png)
 * 
 * @ingroup basic_compartmental
 * 
 * @param vname std::string Name of the virus
 * @param n Number of agents.
 * @param prevalence epiworld_double Initial share of infected agents
 * @param transmission_prob epiworld_double Probability of transmission per contact
 * @param recovery_rate epiworld_double Daily probability of recovery
 */
class ModelSIS : public Model
{

public:

    static constexpr int SUSCEPTIBLE = 0;
    static constexpr int INFECTED    = 1;

    ModelSIS() = delete;

    ModelSIS(
        const std::string & vname,
        epiworld_fast_uint n,
        epiworld_double prevalence,
        epiworld_double transmission_prob,
        epiworld_double recovery_rate
    );


    /// This model's entry in a `ModelRegistry`: arguments and builder
    static ModelInfo model_info();
};

inline ModelSIS::ModelSIS(
    const std::string & vname,
    epiworld_fast_uint n,
    epiworld_double prevalence,
    epiworld_double transmission_prob,
    epiworld_double recovery_rate
    )
{

    this->set_population_size(n);

    this->set_name("Susceptible-Infected-Susceptible (SIS)");

    this->add_param(transmission_prob, "Transmission probability");
    this->add_param(recovery_rate, "Recovery rate");

    Virus virus(vname);
    virus.add_stage("Susceptible");
    virus.add_stage("Infected", stage::infectious);
    virus.infection(SUSCEPTIBLE, INFECTED, "Transmission probability");
    virus.rate(INFECTED, SUSCEPTIBLE, "Recovery rate").effect(Effect::recovery);

    this->add_virus(virus, assign::share(prevalence));

}

inline ModelInfo ModelSIS::model_info()
{

    using uint_t = epiworld_fast_uint;

    return {"SIS", "Susceptible-Infected-Susceptible",
        "Infected agents become susceptible again when they recover.",
        "Disease",
        {
            ParamInfo::agents(10000),
            ParamInfo::prevalence(0.01),
            ParamInfo::probability("transmission_prob", "Transmission probability",
                0.05, "Probability that a contact with an infected agent infects."),
            ParamInfo::probability("recovery_rate", "Recovery rate", 0.14,
                "Daily probability that an infected agent recovers.")
        },
        [](const ModelArgs & a) -> std::unique_ptr< Model > {
            return std::make_unique< ModelSIS >(
                a.virus_name(), static_cast< uint_t >(a("n")), a("prevalence"),
                a("transmission_prob"), a("recovery_rate")
            );
        }};
}

#endif
/*//////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

 End of -include/epiworld/models/sis.hpp-

////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////*/


/*//////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

 Start of -include/epiworld/models/sir.hpp-

////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////*/


#ifndef EPIWORLD_SIR_H 
#define EPIWORLD_SIR_H

// (already included include/epiworld/models/../model-bones.hpp)

/**
 * @brief A Susceptible-Infected-Removed (SIR) model
 * 
 * ![Model Diagram](../assets/img/sir.png)
 * 
 * @ingroup basic_compartmental
 * 
 * @param model A Model object where to set up the SIR.
 * @param vname std::string Name of the virus
 * @param n Number of agents.
 * @param initial_prevalence epiworld_double Initial prevalence
 * @param initial_efficacy epiworld_double Initial susceptibility_reduction of the immune system
 * @param initial_recovery epiworld_double Initial recovery_rate rate of the immune system
 */
class ModelSIR : public Model
{
public:

    static constexpr int SUSCEPTIBLE = 0;
    static constexpr int INFECTED    = 1;
    static constexpr int RECOVERED   = 2;

    ModelSIR() = delete;

    ModelSIR(
        const std::string & vname,
        epiworld_fast_uint n,
        epiworld_double prevalence,
        epiworld_double transmission_prob,
        epiworld_double recovery_rate
    );

    


    /// This model's entry in a `ModelRegistry`: arguments and builder
    static ModelInfo model_info();
};

inline ModelSIR::ModelSIR(
    const std::string & vname,
    epiworld_fast_uint n,
    epiworld_double prevalence,
    epiworld_double transmission_prob,
    epiworld_double recovery_rate
    )
{

    this->set_population_size(n);

    this->add_param(recovery_rate, "Recovery rate");
    this->add_param(transmission_prob, "Transmission probability");

    Virus virus(vname);
    virus.add_stage("Susceptible");
    virus.add_stage("Infected", stage::infectious);
    virus.add_stage("Recovered");
    virus.infection(SUSCEPTIBLE, INFECTED, "Transmission probability");
    virus.rate(INFECTED, RECOVERED, "Recovery rate").effect(Effect::recovery);

    this->add_virus(virus, assign::share(prevalence));

    this->set_name("Susceptible-Infected-Recovered (SIR)");
   
}


inline ModelInfo ModelSIR::model_info()
{

    using uint_t = epiworld_fast_uint;

    return {"SIR", "Susceptible-Infected-Recovered",
        "Infected agents recover with lasting immunity.",
        "Disease",
        {
            ParamInfo::agents(10000),
            ParamInfo::prevalence(0.01),
            ParamInfo::probability("transmission_prob", "Transmission probability",
                0.05, "Probability that a contact with an infected agent infects."),
            ParamInfo::probability("recovery_rate", "Recovery rate", 0.14,
                "Daily probability that an infected agent recovers.")
        },
        [](const ModelArgs & a) -> std::unique_ptr< Model > {
            return std::make_unique< ModelSIR >(
                a.virus_name(), static_cast< uint_t >(a("n")), a("prevalence"),
                a("transmission_prob"), a("recovery_rate")
            );
        }};
}

#endif
/*//////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

 End of -include/epiworld/models/sir.hpp-

////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////*/


/*//////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

 Start of -include/epiworld/models/seir.hpp-

////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////*/


#ifndef EPIWORLD_MODELS_SEIR_HPP
#define EPIWORLD_MODELS_SEIR_HPP

// (already included include/epiworld/models/../model-bones.hpp)

/**
 * @brief A Susceptible-Exposed-Infected-Removed (SEIR) model
 * 
 * ![Model Diagram](../assets/img/seir.png)
 * 
 * @ingroup basic_compartmental
 *
 * @param model A Model object where to set up the SIR.
 * @param vname std::string Name of the virus
 * @param n Number of agents.
 * @param prevalence epiworld_double Initial prevalence the immune system
 * @param transmission_prob epiworld_double Probability that a contact with an
 * infected agent infects.
 * @param latent_period epiworld_double Mean days in Exposed (carrying the
 * virus, not yet infectious).
 * @param recovery_rate epiworld_double Recovery rate of the virus.
 */
class ModelSEIR : public Model
{

public:
    static constexpr int SUSCEPTIBLE = 0;
    static constexpr int EXPOSED     = 1;
    static constexpr int INFECTED    = 2;
    static constexpr int REMOVED     = 3;

    ModelSEIR() = delete;

    ModelSEIR(
        const std::string & vname,
        epiworld_fast_uint n,
        epiworld_double prevalence,
        epiworld_double transmission_prob,
        epiworld_double latent_period,
        epiworld_double recovery_rate
    );



    /// This model's entry in a `ModelRegistry`: arguments and builder
    static ModelInfo model_info();
};


inline ModelSEIR::ModelSEIR(
    const std::string & vname,
    epiworld_fast_uint n,
    epiworld_double prevalence,
    epiworld_double transmission_prob,
    epiworld_double latent_period,
    epiworld_double recovery_rate
    )
{

    this->set_population_size(n);

    this->add_param(transmission_prob, "Transmission probability");
    this->add_param(latent_period, "Latent period");
    this->add_param(recovery_rate, "Recovery rate");

    Virus virus(vname);
    virus.add_stage("Susceptible");
    virus.add_stage("Exposed", stage::carrier);
    virus.add_stage("Infected", stage::infectious);
    virus.add_stage("Removed");
    virus.infection(SUSCEPTIBLE, EXPOSED, "Transmission probability");
    virus.after(EXPOSED, INFECTED, Duration::geometric("Latent period"));
    virus.rate(INFECTED, REMOVED, "Recovery rate").effect(Effect::recovery);

    this->add_virus(virus, assign::share(prevalence));

    this->set_name("Susceptible-Exposed-Infected-Removed (SEIR)");

}


inline ModelInfo ModelSEIR::model_info()
{

    using uint_t = epiworld_fast_uint;

    return {"SEIR", "Susceptible-Exposed-Infected-Recovered",
        "Infected agents go through a latent (exposed, not infectious) period "
        "before they become infectious.",
        "Disease",
        {
            ParamInfo::agents(10000),
            ParamInfo::prevalence(0.01),
            ParamInfo::probability("transmission_prob", "Transmission probability",
                0.05, "Probability that a contact with an infected agent infects."),
            ParamInfo::duration("latent_period", "Latent period", 7.0,
                "Mean days in the exposed state (infected, not yet infectious)."),
            ParamInfo::probability("recovery_rate", "Recovery rate", 0.14,
                "Daily probability that an infected agent recovers.")
        },
        [](const ModelArgs & a) -> std::unique_ptr< Model > {
            return std::make_unique< ModelSEIR >(
                a.virus_name(), static_cast< uint_t >(a("n")), a("prevalence"),
                a("transmission_prob"), a("latent_period"), a("recovery_rate")
            );
        }};
}

#endif
/*//////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

 End of -include/epiworld/models/seir.hpp-

////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////*/


/*//////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

 Start of -include/epiworld/models/surveillance.hpp-

////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////*/


#ifndef EPIWORLD_MODELS_SURVEILLANCE_HPP
#define EPIWORLD_MODELS_SURVEILLANCE_HPP

// (already included include/epiworld/models/../model-bones.hpp)

/**
 * @brief A Surveillance model
 * @ingroup special_models
 */
class ModelSURV : public Model {

public:
    // state
    static constexpr int SUSCEPTIBLE           = 0;
    static constexpr int LATENT                = 1;
    static constexpr int SYMPTOMATIC           = 2;
    static constexpr int SYMPTOMATIC_ISOLATED  = 3; // sampled and discovered
    static constexpr int ASYMPTOMATIC          = 4;
    static constexpr int ASYMPTOMATIC_ISOLATED = 5;
    static constexpr int RECOVERED             = 6;
    static constexpr int REMOVED               = 7;

    /**
     * @brief Vector of days spent in latent and infectious states
     * A row-major matrix
     */
    /**
     * @name Construct a new ModelSURV object
     * 
     * The ModelSURV class simulates a survaillence model where agents can be
     * isolated, even if asyptomatic.
     * 
     * @param vname String. Name of the virus
 * @param n Number of agents.
     * @param prevalence Integer. Number of initial cases of the virus.
     * @param efficacy_vax Double. Efficacy of the vaccine (1 - P(acquire the disease)).
     * @param latent_period Double. Shape parameter of a `Gamma(latent_period, 1)`
     *   distribution. This coincides with the expected number of latent days.
     * @param infect_period Double. Shape parameter of a `Gamma(infected_period, 1)`
     *   distribution. This coincides with the expected number of infectious days.
     * @param prob_symptoms Double. Probability of generating symptoms.
     * @param prop_vaccinated Double. Probability of vaccination. Coincides with
     *   the initial prevalence of vaccinated individuals.
     * @param prop_vax_redux_transm Double. Factor by which the vaccine reduces
     *   transmissibility.
     * @param prop_vax_redux_infect Double. Factor by which the vaccine reduces
     *   the chances of becoming infected.
     * @param surveillance_prob Double. Probability of testing an agent.
     * @param prob_transmission Double. Raw transmission probability.
     * @param prob_death Double. Raw probability of death for symptomatic individuals.
     * @param prob_noreinfect Ignored: recovered agents are not reinfected.
     * 
     * @details
     * This model features the following states:
     * 
     * - Susceptible
     * - Latent
     * - Symptomatic
     * - Symptomatic isolated
     * - Asymptomatic
     * - Asymptomatic isolated
     * - Recovered
     * - Removed    
     * 
     * @returns An object of class `epiworld_surv`
     * 
     */
    ///@{
    ModelSURV() = delete;

    ModelSURV(
        const std::string & vname,
        epiworld_fast_uint n,
        epiworld_fast_uint prevalence         = 50,
        epiworld_double efficacy_vax          = 0.9,
        epiworld_double latent_period         = 3u,
        epiworld_double infect_period         = 6u,
        epiworld_double prob_symptoms         = 0.6,
        epiworld_double prop_vaccinated       = 0.25,
        epiworld_double prop_vax_redux_transm = 0.5,
        epiworld_double prop_vax_redux_infect = 0.5,
        epiworld_double surveillance_prob     = 0.001,
        epiworld_double prob_transmission     = 1.0,
        epiworld_double prob_death            = 0.001,
        epiworld_double prob_noreinfect       = 0.9
    );
    ///@}


    /// This model's entry in a `ModelRegistry`: arguments and builder
    static ModelInfo model_info();

};

inline ModelSURV::ModelSURV(
    const std::string & vname,
    epiworld_fast_uint n,
    epiworld_fast_uint prevalence,
    epiworld_double efficacy_vax,
    epiworld_double latent_period,
    epiworld_double infect_period,
    epiworld_double prob_symptoms,
    epiworld_double prop_vaccinated,
    epiworld_double prop_vax_redux_transm,
    epiworld_double,
    epiworld_double surveillance_prob,
    epiworld_double prob_transmission,
    epiworld_double prob_death,
    epiworld_double prob_noreinfect
    )
{

    this->set_population_size(n);

    std::vector< int > exposed_state = {
        SYMPTOMATIC,
        SYMPTOMATIC_ISOLATED,
        ASYMPTOMATIC,
        ASYMPTOMATIC_ISOLATED,
        LATENT
    };

    GlobalFun surveillance_program = 
    [exposed_state](
        Model* m
        ) -> void
    {

        const auto * nh = m->get_natural_history();
        const int detected = nh->transition_id("Detected");
        const int detected_asympt = nh->transition_id("Detected asymptomatic");

        // How many will we find
        std::binomial_distribution<> bdist(m->size(), EPI_PAR(m, "Surveilance prob."));
        int nsampled = bdist(*m->get_rand_endgine());

        int to_go = nsampled + 1;

        epiworld_double ndetected        = 0.0;
        epiworld_double ndetected_asympt = 0.0;
        
        auto & pop = m->get_agents();
        std::vector< bool > sampled(m->size(), false);
        
        while (to_go-- > 0)
        {

            // Who is the lucky one
            epiworld_fast_uint i = m->runif_index(m->size());

            if (sampled[i])
                continue;

            sampled[i] = true;
            Agent * p = &pop[i];
            
            // If still exposed for the next term
            if (IN(m->get_stage(*p), exposed_state ))
            {

                // The ones already isolated stay so
                ndetected += 1.0;
                if (m->get_stage(*p) == ModelSURV::ASYMPTOMATIC)
                {
                    ndetected_asympt += 1.0;
                    m->fire(detected_asympt, *p);
                }
                else
                    m->fire(detected, *p);

            }

        }

        // Writing the user data
        std::vector< int > totals;
        m->get_db().get_today_total(nullptr, nullptr, &totals);
        m->add_user_data(
            {
                static_cast<epiworld_double>(nsampled),
                ndetected,
                ndetected_asympt,
                static_cast<epiworld_double>(totals[ModelSURV::ASYMPTOMATIC])
            }
            );


    };

    // General model parameters
    this->add_param(latent_period, "Latent period");
    this->add_param(infect_period, "Infect period");
    this->add_param(prob_symptoms, "Prob of symptoms");
    this->add_param(surveillance_prob, "Surveilance prob.");
    this->add_param(efficacy_vax, "Vax efficacy");
    this->add_param(prop_vax_redux_transm, "Vax redux transmission");
    this->add_param(prob_transmission, "Prob of transmission");
    this->add_param(prob_death, "Prob. death");
    this->add_param(prob_noreinfect, "Prob. no reinfect");

    // The latent and infectious periods are gamma with shape the parameter
    // and scale 1 (mean the parameter, variance the same)
    auto gamma_days = [](const char * pname) -> DurationFun {
        const std::string name(pname);
        return [name](Agent &, Model & m) -> int {
            return static_cast< int >(std::lround(m.rgamma(m.par(name), 1.0)));
        };
    };

    Virus covid(vname);
    covid.add_stage("Susceptible");
    covid.add_stage("Latent", stage::carrier);
    covid.add_stage("Symptomatic", stage::infectious);
    covid.add_stage("Symptomatic isolated", stage::carrier);
    covid.add_stage("Asymptomatic", stage::infectious);
    covid.add_stage("Asymptomatic isolated", stage::carrier);
    covid.add_stage("Recovered");
    covid.add_stage("Removed");
    covid.set_contact(REMOVED, 0.0);

    covid.infection(SUSCEPTIBLE, LATENT, "Prob of transmission");

    // After the latent period, symptoms or not
    covid.after(LATENT, SYMPTOMATIC, Duration(gamma_days("Latent period")))
      .name("Latent -> Symptomatic or Asymptomatic")
      .to({
        {SYMPTOMATIC, Value("Prob of symptoms")},
        {ASYMPTOMATIC, Value::complement("Prob of symptoms")}
      });

    // The infectious period counts from the end of the latent one; isolation
    // keeps counting from there
    const Duration infectious(gamma_days("Infect period"));
    covid.after(SYMPTOMATIC, RECOVERED, infectious);
    covid.after(SYMPTOMATIC_ISOLATED, RECOVERED, infectious.since(SYMPTOMATIC));
    covid.after(ASYMPTOMATIC, RECOVERED, infectious);
    covid.after(ASYMPTOMATIC_ISOLATED, RECOVERED, infectious.since(ASYMPTOMATIC));

    // Each infectious day, a risk of death (competing with the recovery)
    covid.rate(
        {SYMPTOMATIC, SYMPTOMATIC_ISOLATED, ASYMPTOMATIC, ASYMPTOMATIC_ISOLATED},
        REMOVED,
        "Prob. death"
    ).name("Death").effect(Effect::death);

    // Surveillance finds infected agents
    covid.external({LATENT, SYMPTOMATIC}, SYMPTOMATIC_ISOLATED, "Detected");
    covid.external(ASYMPTOMATIC, ASYMPTOMATIC_ISOLATED, "Detected asymptomatic");


    // Virus ------------------------------------------------------------------
    this->add_virus(covid, assign::count(prevalence));

    this->set_user_data({"nsampled", "ndetected", "ndetected_asympt", "nasymptomatic"});
    this->add_globalevent(surveillance_program, "Surveilance program", -1);
   
    // Vaccine tool -----------------------------------------------------------
    Tool vax("Vaccine");
    vax.set_susceptibility_reduction("Vax efficacy");
    vax.set_transmission_reduction("Vax redux transmission");

    this->add_tool(vax, assign::share(prop_vaccinated));

    this->set_name("Surveillance");


}

inline ModelInfo ModelSURV::model_info()
{

    using uint_t = epiworld_fast_uint;

    return {"SURV", "Surveillance (SEIR with symptoms and vaccination)",
        "Latent agents become symptomatic or asymptomatic; surveillance "
        "detects and isolates cases, and a share of agents is vaccinated.",
        "Disease",
        {
            ParamInfo::agents(10000),
            ParamInfo::count("prevalence", "", 50.0,
                "Number of agents infected at the start."),
            ParamInfo::probability("efficacy_vax", "Vax efficacy", 0.9,
                "Probability that the vaccine prevents infection."),
            ParamInfo::duration("latent_period", "Latent period", 3.0,
                "Mean days in the latent state."),
            ParamInfo::duration("infect_period", "Infect period", 6.0,
                "Mean days infectious."),
            ParamInfo::probability("prob_symptoms", "Prob of symptoms", 0.6,
                "Probability that an infected agent develops symptoms."),
            ParamInfo::probability("prop_vaccinated", "", 0.25,
                "Proportion of agents vaccinated."),
            ParamInfo::probability("prop_vax_redux_transm",
                "Vax redux transmission", 0.5,
                "Factor by which the vaccine reduces transmission."),
            ParamInfo::probability("prop_vax_redux_infect", "", 0.5,
                "Factor by which the vaccine reduces infection (not used by "
                "the model)."),
            ParamInfo::probability("surveillance_prob", "Surveilance prob.",
                0.001, "Daily probability that an agent is tested."),
            ParamInfo::probability("transmission_prob", "Prob of transmission",
                1.0, "Probability that a contact with an infected agent "
                "infects."),
            ParamInfo::probability("prob_death", "Prob. death", 0.001,
                "Probability that an infected agent dies."),
            ParamInfo::probability("prob_noreinfect", "Prob. no reinfect", 0.9,
                "Probability that a recovered agent is not reinfected.")
        },
        [](const ModelArgs & a) -> std::unique_ptr< Model > {
            return std::make_unique< ModelSURV >(
                a.virus_name(), static_cast< uint_t >(a("n")),
                static_cast< uint_t >(a("prevalence")), a("efficacy_vax"),
                a("latent_period"), a("infect_period"), a("prob_symptoms"),
                a("prop_vaccinated"), a("prop_vax_redux_transm"),
                a("prop_vax_redux_infect"), a("surveillance_prob"),
                a("transmission_prob"), a("prob_death"), a("prob_noreinfect")
            );
        }};
}

#endif
/*//////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

 End of -include/epiworld/models/surveillance.hpp-

////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////*/


/*//////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

 Start of -include/epiworld/models/sird.hpp-

////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////*/


#ifndef EPIWORLD_SIRD_H 
#define EPIWORLD_SIRD_H

// (already included include/epiworld/models/../model-bones.hpp)

/**
 * @brief A Susceptible-Infected-Removed-Deceased (SIRD) model
 * 
 * ![Model Diagram](../assets/img/sird.png)
 * 
 * @ingroup death_compartmental
 */
class ModelSIRD : public Model
{
public:

    static constexpr int SUSCEPTIBLE = 0;
    static constexpr int INFECTED    = 1;
    static constexpr int RECOVERED   = 2;
    static constexpr int DECEASED    = 3;

    ModelSIRD() = delete;

    /**
     * @brief Constructs a new SIRD model with the given parameters.
     * 
     * @param vname The name of the vertex associated with this model.
 * @param n Number of agents.
     * @param prevalence The initial prevalence of the disease in the population.
     * @param transmission_prob Probability that a contact with an infected agent infects.
     * @param recovery_rate The rate at which infected individuals recover and become immune.
     * @param death_rate The rate at which infected individuals die.
     */
    ///@{
    ModelSIRD(
        const std::string & vname,
        epiworld_fast_uint n,
        epiworld_double prevalence,
        epiworld_double transmission_prob,
        epiworld_double recovery_rate, 
        epiworld_double death_rate
    );
    ///@}

    


    /// This model's entry in a `ModelRegistry`: arguments and builder
    static ModelInfo model_info();
};

inline ModelSIRD::ModelSIRD(
    const std::string & vname,
    epiworld_fast_uint n,
    epiworld_double prevalence,
    epiworld_double transmission_prob,
    epiworld_double recovery_rate, 
    epiworld_double death_rate
    )
{

    this->set_population_size(n);

    this->add_param(recovery_rate, "Recovery rate");
    this->add_param(transmission_prob, "Transmission probability");
    this->add_param(death_rate, "Death rate");

    Virus virus(vname);
    virus.add_stage("Susceptible");
    virus.add_stage("Infected", stage::infectious);
    virus.add_stage("Recovered");
    virus.add_stage("Deceased");
    virus.set_contact(DECEASED, 0.0);
    virus.infection(SUSCEPTIBLE, INFECTED, "Transmission probability");
    virus.rate(INFECTED, RECOVERED, "Recovery rate").effect(Effect::recovery);
    virus.rate(INFECTED, DECEASED, "Death rate").effect(Effect::death);

    this->add_virus(virus, assign::share(prevalence));

    this->set_name("Susceptible-Infected-Recovered-Deceased (SIRD)");

   
}


inline ModelInfo ModelSIRD::model_info()
{

    using uint_t = epiworld_fast_uint;

    return {"SIRD", "Susceptible-Infected-Recovered-Deceased",
        "SIR, where infected agents may also die.",
        "Disease",
        {
            ParamInfo::agents(10000),
            ParamInfo::prevalence(0.01),
            ParamInfo::probability("transmission_prob", "Transmission probability",
                0.05, "Probability that a contact with an infected agent infects."),
            ParamInfo::probability("recovery_rate", "Recovery rate", 0.14,
                "Daily probability that an infected agent recovers."),
            ParamInfo::probability("death_rate", "Death rate", 0.01,
                "Daily probability that an infected agent dies.")
        },
        [](const ModelArgs & a) -> std::unique_ptr< Model > {
            return std::make_unique< ModelSIRD >(
                a.virus_name(), static_cast< uint_t >(a("n")), a("prevalence"),
                a("transmission_prob"), a("recovery_rate"), a("death_rate")
            );
        }};
}

#endif
/*//////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

 End of -include/epiworld/models/sird.hpp-

////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////*/


/*//////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

 Start of -include/epiworld/models/sisd.hpp-

////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////*/


#ifndef EPIWORLD_MODELS_SISD_HPP 
#define EPIWORLD_MODELS_SISD_HPP

// (already included include/epiworld/models/../model-bones.hpp)

/**
 * @brief A Susceptible-Infected-Susceptible-Deceased (SISD) model
 * 
 * ![Model Diagram](../assets/img/sisd.png)
 * 
 * @ingroup death_compartmental
 * 
 * @param vname std::string Name of the virus
 * @param n Number of agents.
 * @param initial_prevalence epiworld_double Initial prevalence
 * @param initial_efficacy epiworld_double Initial susceptibility_reduction of the immune system
 * @param initial_recovery epiworld_double Initial recovery_rate rate of the immune system
 * @param inital_death epiworld_double Initial death_rate of the immune system
 */
class ModelSISD : public Model
{

public:

    static constexpr int SUSCEPTIBLE = 0;
    static constexpr int INFECTED    = 1;
    static constexpr int DECEASED    = 2;

    ModelSISD() = delete;

    ModelSISD(
        const std::string & vname,
        epiworld_fast_uint n,
        epiworld_double prevalence,
        epiworld_double transmission_prob,
        epiworld_double recovery_rate,
        epiworld_double death_rate
    );


    /// This model's entry in a `ModelRegistry`: arguments and builder
    static ModelInfo model_info();
};

inline ModelSISD::ModelSISD(
    const std::string & vname,
    epiworld_fast_uint n,
    epiworld_double prevalence,
    epiworld_double transmission_prob,
    epiworld_double recovery_rate,
    epiworld_double death_rate
    )
{

    this->set_population_size(n);

    this->set_name("Susceptible-Infected-Susceptible-Deceased (SISD)");

    this->add_param(transmission_prob, "Transmission probability");
    this->add_param(recovery_rate, "Recovery rate");
    this->add_param(death_rate, "Death rate");

    Virus virus(vname);
    virus.add_stage("Susceptible");
    virus.add_stage("Infected", stage::infectious);
    virus.add_stage("Deceased");
    virus.set_contact(DECEASED, 0.0);
    virus.infection(SUSCEPTIBLE, INFECTED, "Transmission probability");
    virus.rate(INFECTED, SUSCEPTIBLE, "Recovery rate").effect(Effect::recovery);
    virus.rate(INFECTED, DECEASED, "Death rate").effect(Effect::death);

    this->add_virus(virus, assign::share(prevalence));

}

inline ModelInfo ModelSISD::model_info()
{

    using uint_t = epiworld_fast_uint;

    return {"SISD", "Susceptible-Infected-Susceptible-Deceased",
        "SIS, where infected agents may also die.",
        "Disease",
        {
            ParamInfo::agents(10000),
            ParamInfo::prevalence(0.01),
            ParamInfo::probability("transmission_prob", "Transmission probability",
                0.05, "Probability that a contact with an infected agent infects."),
            ParamInfo::probability("recovery_rate", "Recovery rate", 0.14,
                "Daily probability that an infected agent recovers."),
            ParamInfo::probability("death_rate", "Death rate", 0.01,
                "Daily probability that an infected agent dies.")
        },
        [](const ModelArgs & a) -> std::unique_ptr< Model > {
            return std::make_unique< ModelSISD >(
                a.virus_name(), static_cast< uint_t >(a("n")), a("prevalence"),
                a("transmission_prob"), a("recovery_rate"), a("death_rate")
            );
        }};
}

#endif
/*//////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

 End of -include/epiworld/models/sisd.hpp-

////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////*/


/*//////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

 Start of -include/epiworld/models/seird.hpp-

////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////*/


#ifndef EPIWORLD_MODELS_SEIRD_HPP
#define EPIWORLD_MODELS_SEIRD_HPP

// (already included include/epiworld/models/../model-bones.hpp)

/**
 * @brief A Susceptible-Exposed-Infected-Removed-Deceased (SEIRD) model
 * 
 * ![Model Diagram](../assets/img/seird.png)
 * 
 * @ingroup death_compartmental
*/
class ModelSEIRD : public Model
{
  
public:
  static constexpr int SUSCEPTIBLE = 0;
  static constexpr int EXPOSED     = 1;
  static constexpr int INFECTED    = 2;
  static constexpr int REMOVED     = 3;
  static constexpr int DECEASED    = 4;
  
  ModelSEIRD() = delete;

  /**
   * @brief Constructor for the SEIRD model.
   * 
   * @param vname Name of the model.
 * @param n Number of agents.
   * @param prevalence Initial prevalence of the disease.
   * @param transmission_prob Probability that a contact with an infected
   * agent infects.
   * @param latent_period Mean days in Exposed (carrying the virus, not yet
   * infectious).
   * @param recovery_rate Recovery rate of the disease.
   * @param death_rate Death rate of the disease.
   */
  ModelSEIRD(
    const std::string & vname,
    epiworld_fast_uint n,
    epiworld_double prevalence,
    epiworld_double transmission_prob,
    epiworld_double latent_period,
    epiworld_double recovery_rate,
    epiworld_double death_rate
  );
  


    /// This model's entry in a `ModelRegistry`: arguments and builder
    static ModelInfo model_info();
};



inline ModelSEIRD::ModelSEIRD(
    const std::string & vname,
    epiworld_fast_uint n,
    epiworld_double prevalence,
    epiworld_double transmission_prob,
    epiworld_double latent_period,
    epiworld_double recovery_rate,
    epiworld_double death_rate
)
{

    this->set_population_size(n);
  
  this->add_param(transmission_prob, "Transmission probability");
  this->add_param(latent_period, "Latent period");
  this->add_param(recovery_rate, "Recovery rate");
  this->add_param(death_rate, "Death rate");

  Virus virus(vname);
  virus.add_stage("Susceptible");
  virus.add_stage("Exposed", stage::carrier);
  virus.add_stage("Infected", stage::infectious);
  virus.add_stage("Removed");
  virus.add_stage("Deceased");
  virus.set_contact(DECEASED, 0.0);
  virus.infection(SUSCEPTIBLE, EXPOSED, "Transmission probability");
  virus.after(EXPOSED, INFECTED, Duration::geometric("Latent period"));
  virus.rate(INFECTED, REMOVED, "Recovery rate").effect(Effect::recovery);
  virus.rate(INFECTED, DECEASED, "Death rate").effect(Effect::death);

  this->add_virus(virus, assign::share(prevalence));

  this->set_name("Susceptible-Exposed-Infected-Removed-Deceased (SEIRD)");
  
}



inline ModelInfo ModelSEIRD::model_info()
{

    using uint_t = epiworld_fast_uint;

    return {"SEIRD", "Susceptible-Exposed-Infected-Recovered-Deceased",
        "SEIR, where infected agents may also die.",
        "Disease",
        {
            ParamInfo::agents(10000),
            ParamInfo::prevalence(0.01),
            ParamInfo::probability("transmission_prob", "Transmission probability",
                0.05, "Probability that a contact with an infected agent infects."),
            ParamInfo::duration("latent_period", "Latent period", 7.0,
                "Mean days in the exposed state (infected, not yet infectious)."),
            ParamInfo::probability("recovery_rate", "Recovery rate", 0.14,
                "Daily probability that an infected agent recovers."),
            ParamInfo::probability("death_rate", "Death rate", 0.01,
                "Daily probability that an infected agent dies.")
        },
        [](const ModelArgs & a) -> std::unique_ptr< Model > {
            return std::make_unique< ModelSEIRD >(
                a.virus_name(), static_cast< uint_t >(a("n")), a("prevalence"),
                a("transmission_prob"), a("latent_period"), a("recovery_rate"),
                a("death_rate")
            );
        }};
}

#endif
/*//////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

 End of -include/epiworld/models/seird.hpp-

////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////*/


/*//////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

 Start of -include/epiworld/models/sirlogit.hpp-

////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////*/


#ifndef EPIWORLD_MODELS_SIRLOGIT_HPP 
#define EPIWORLD_MODELS_SIRLOGIT_HPP

// (already included include/epiworld/models/../model-bones.hpp)

/**
 * @brief A Susceptible-Infected-Removed (SIR) model
 * 
 * ![Model Diagram](../assets/img/sirlogit.png)
 * 
 * @ingroup special_models
 * 
 * @details
 * In this model, infection and recoveru probabilities are computed
 * using a logit model. Particularly, the probability of infection
 * is computed as:
 * 
 * \f[
 * \frac{1}{1 + \exp\left(-\left(\beta_0 E_i + \sum_{i=1}^{n} \beta_i x_i\right)\right)}
 * \f]
 * 
 * where \f$\beta_0\f$ is the exposure coefficient and \f$E_i\f$ is the exposure
 * number, \f$\beta_i\f$ are the
 * coefficients for the features \f$x_i\f$ of the agents, and \f$n\f$ is the
 * number of features. The probability of recovery is computed as:
 * 
 * \f[
 * \frac{1}{1 + \exp\left(-\left(\sum_{i=1}^{n} \beta_i x_i\right)\right)}
 * \f]
 * 
 * where \f$\beta_i\f$ are the coefficients for the features \f$x_i\f$ of the agents,
 * and \f$n\f$ is the number of features.
 * 
 
*/
class ModelSIRLogit : public Model
{
public:
    static constexpr int SUSCEPTIBLE = 0;
    static constexpr int INFECTED    = 1;
    static constexpr int RECOVERED   = 2;

    ModelSIRLogit() = delete;

    /**
      * @param vname Name of the virus.
 * @param n Number of agents.
      * @param coefs_infect Double ptr. Infection coefficients.
      * @param coefs_recover Double ptr. Recovery coefficients.
      * @param ncoef_infect Unsigned int. Number of infection coefficients.
      * @param ncoef_recover Unsigned int. Number of recovery coefficients.
      * @param coef_infect_cols Vector<unsigned int>. Ids of infection vars.
      * @param coef_recover_cols Vector<unsigned int>. Ids of recover vars.
    */
    ModelSIRLogit(
        const std::string & vname,
        epiworld_fast_uint n,
        double * data,
        size_t ncols,
        std::vector< double > coefs_infect,
        std::vector< double > coefs_recover,
        std::vector< size_t > coef_infect_cols,
        std::vector< size_t > coef_recover_cols,
        epiworld_double transmission_prob,
        epiworld_double recovery_rate,
        epiworld_double prevalence
    );

    std::unique_ptr< Model > clone_ptr() override;

    void reset() override;
    
    std::vector< double > coefs_infect;
    std::vector< double > coefs_recover;
    std::vector< size_t > coef_infect_cols;
    std::vector< size_t > coef_recover_cols;

};

inline std::unique_ptr<Model> ModelSIRLogit::clone_ptr()
{
    
    return std::make_unique<ModelSIRLogit>(*this);

}

inline void ModelSIRLogit::reset()
{

    /* Checking specified columns in the model */
    for (const auto & c : coef_infect_cols)
    {
        if (c >= Model::agents_data_ncols)
            throw std::range_error("Columns specified in coef_infect_cols out of range.");
    }

    for (const auto & c : coef_recover_cols)
    {
        if (c >= Model::agents_data_ncols)
            throw std::range_error("Columns specified in coef_recover_cols out of range.");
    }

    /* Checking attributes */ 
    if (coefs_infect.size() != (coef_infect_cols.size() + 1u))
        throw std::logic_error(
            "The number of coefficients (infection) doesn't match the number of features. It must be as many features of the agents plus 1 (exposure.)"
            );

    if (coefs_recover.size() != coef_recover_cols.size())
        throw std::logic_error(
            "The number of coefficients (recovery) doesn't match the number of features. It must be as many features of the agents."
            );
    
    Model::reset();

    return;

}

/**
 * @brief A Susceptible-Infected-Removed (SIR) model
 * 
 * @param model A Model object where to set up the SIR.
 * @param vname std::string Name of the virus
 * @param prevalence Initial prevalence (proportion)
 * @param contact_rate Average number of contacts (interactions) per step.
 * @param prob_transmission Probability of transmission
 * @param prob_recovery Probability of recovery
 */
inline ModelSIRLogit::ModelSIRLogit(
    const std::string & vname,
    epiworld_fast_uint n,
    double * data,
    size_t ncols,
    std::vector< double > coefs_infect,
    std::vector< double > coefs_recover,
    std::vector< size_t > coef_infect_cols,
    std::vector< size_t > coef_recover_cols,
    epiworld_double transmission_prob,
    epiworld_double recovery_rate,
    epiworld_double prevalence
    )
{

    this->set_population_size(n);

    if (coef_infect_cols.size() == 0u)
        throw std::logic_error("No columns specified for coef_infect_cols.");

    if (coef_recover_cols.size() == 0u)
        throw std::logic_error("No columns specified for coef_recover_cols.");

    // Saving the variables
    this->set_agents_data(
        data, ncols
    );

    this->coefs_infect = coefs_infect;
    this->coefs_recover = coefs_recover;
    this->coef_infect_cols = coef_infect_cols;
    this->coef_recover_cols = coef_recover_cols;

    this->add_param(transmission_prob, "Transmission probability");
    this->add_param(recovery_rate, "Recovery rate");

    // Infection: each infectious neighbor is an exposure with probability
    // plogis(baseline + p * coef_exposure), where the baseline comes from the
    // agent's features and p is the usual per-contact probability
    const Value p_contact("Transmission probability");
    InfectionSampler sampler = [p_contact](
        Agent & p, Model & m
        ) -> Virus *
        {

            ModelSIRLogit * _m = model_cast<ModelSIRLogit>(&m);

            const double coef_exposure = _m->coefs_infect[0u];

            double baseline = 0.0;
            for (size_t k = 0u; k < _m->coef_infect_cols.size(); ++k)
                baseline += p(k, m) * _m->coefs_infect[k + 1u];

            size_t nviruses_tmp = 0u;

            // Every network layer, in the order the layers were added
            for (auto * net : m.get_network_layers())
            for (auto * neighbor : net->neighbors_view(static_cast< size_t >(p.get_id())))
            {

                if (neighbor->get_virus() == nullptr)
                    continue;

                auto & v = neighbor->get_virus();

                m.reserve_tmp(nviruses_tmp + 1u);

                const double x = baseline +
                    (1.0 - p.get_susceptibility_reduction(v, m)) *
                    p_contact.eval(p, v.get(), m) *
                    (1.0 - neighbor->get_transmission_reduction(v, m)) *
                    coef_exposure;

                m.array_double_tmp[nviruses_tmp] = 1.0 / (1.0 + std::exp(-x));
                m.array_virus_tmp[nviruses_tmp++] = &(*v);

            }

            if (nviruses_tmp == 0u)
                return nullptr;

            const int which = roulette(nviruses_tmp, &m);
            return (which < 0) ? nullptr : m.array_virus_tmp[which];

        };

    // Recovery: plogis of the agent's features
    ValueFun p_recover = [](Agent & p, Model & m) -> epiworld_double
    {

        ModelSIRLogit * _m = model_cast<ModelSIRLogit>(&m);

        double prob = 0.0;
        for (size_t i = 0u; i < _m->coefs_recover.size(); ++i)
            prob += p(i, m) * _m->coefs_recover[i];

        return 1.0 / (1.0 + std::exp(-prob));

    };

    Virus virus(vname);
    virus.add_stage("Susceptible");
    virus.add_stage("Infected", stage::infectious);
    virus.add_stage("Recovered");
    virus.infection(SUSCEPTIBLE, INFECTED, "Transmission probability").sampler(sampler);
    virus.rate(INFECTED, RECOVERED, p_recover);

    this->add_virus(virus, assign::share(prevalence));

    this->set_name("Susceptible-Infected-Removed (SIR) (logit)");

}


#endif
/*//////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

 End of -include/epiworld/models/sirlogit.hpp-

////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////*/


/*//////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

 Start of -include/epiworld/models/diffnet.hpp-

////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////*/


#ifndef EPIWORLD_DIFFNET_H 
#define EPIWORLD_DIFFNET_H

#include <string>
#include <vector>
// (already included include/epiworld/models/../config.hpp)
// (already included include/epiworld/models/../model-bones.hpp)

/**
 * @brief A Network Diffusion Model
 * @ingroup special_models
 * 
 * @param model A Model object where to set up the SIR.
 * @param vname std::string Name of the virus
 * @param n Number of agents.
 * @param initial_prevalence epiworld_double Initial prevalence
 * @param initial_efficacy epiworld_double Initial susceptibility_reduction of the immune system
 * @param initial_recovery epiworld_double Initial recovery rate of the immune system
 * 
 */
class ModelDiffNet : public Model
{
private:
public:

    // Statuses
    static constexpr int NONADOPTER = 0;
    static constexpr int ADOPTER    = 1;

    ModelDiffNet() = delete;

    ModelDiffNet(
        const std::string & innovation_name,
        epiworld_fast_uint n,
        epiworld_double prevalence,
        epiworld_double prob_adopt,
        bool normalize_exposure = true,
        double * agents_data = nullptr,
        size_t data_ncols = 0u,
        std::vector< size_t > data_cols = {},
        std::vector< double > params = {}
    );

    bool normalize_exposure = true;
    std::vector< size_t > data_cols;
    std::vector< double > params;
};

inline ModelDiffNet::ModelDiffNet(
    const std::string & innovation_name,
    epiworld_fast_uint n,
    epiworld_double prevalence,
    epiworld_double prob_adopt,
    bool normalize_exposure,
    double * agents_data,
    size_t data_ncols,
    std::vector< size_t > data_cols,
    std::vector< double > params
    )
{

    this->set_population_size(n);

    // Adding additional parameters
    this->normalize_exposure = normalize_exposure;
    this->data_cols = data_cols;
    this->params = params;

    std::string parname = std::string("Prob. Adopting ") + innovation_name;
    this->add_param(prob_adopt, parname);

    // Adoption: for each innovation, a logit of the exposure (the share of
    // neighbors who adopted it, optionally normalized) plus the agent's
    // features and the baseline probability; one draw picks one, or none
    InfectionSampler sampler = [](
        Agent & agent, Model & m
    ) -> Virus * {

        const size_t nviruses = m.get_n_viruses();
        std::vector< Virus* > innovations(nviruses, nullptr);
        std::vector< double > exposure(nviruses, 0.0);

        ModelDiffNet * diffmodel = model_cast<ModelDiffNet>(&m);

        size_t n_neighbors = 0u;
        // Every network layer, in the order the layers were added
        for (auto * net : m.get_network_layers())
        for (auto * neighbor : net->neighbors_view(static_cast< size_t >(agent.get_id())))
        {

            ++n_neighbors;

            auto & v = neighbor->get_virus();
            if (v == nullptr)
                continue;

            const size_t vid = static_cast< size_t >(v->get_id());
            if (innovations[vid] == nullptr)
                innovations[vid] = &(*v);

            exposure[vid] +=
                (1.0 - agent.get_susceptibility_reduction(v, m)) *
                (1.0 - agent.get_transmission_reduction(v, m));

        }

        for (size_t i = 0u; i < nviruses; ++i)
        {

            if (diffmodel->normalize_exposure && (n_neighbors > 0u))
                exposure[i] /= static_cast< double >(n_neighbors);

            for (auto & j: diffmodel->data_cols)
                exposure[i] += agent(j, m) * diffmodel->params.at(j);

            // Baseline probability of adoption
            const double p = m.par(
                std::string("Prob. Adopting ") + m.get_viruses()[i]->get_name()
            );
            exposure[i] += std::log(p) - std::log(1.0 - p);

            exposure[i] = 1.0 / (1.0 + std::exp(-exposure[i]));

        }

        const int which = roulette(exposure, &m);
        if (which < 0)
            return nullptr;

        // An innovation no neighbor has is adopted from the model's own
        Virus * v = innovations[static_cast< size_t >(which)];
        return (v != nullptr) ? v : m.get_viruses()[static_cast< size_t >(which)].get();

    };

    this->set_agents_data(agents_data, data_ncols);

    Virus virus(innovation_name);
    virus.add_stage("Non adopters");
    virus.add_stage("Adopters", stage::infectious);
    virus.infection(NONADOPTER, ADOPTER, parname).sampler(sampler);

    this->add_virus(virus, assign::share(prevalence));

    this->set_name(
        std::string("Diffusion of Innovations - ") + innovation_name);

    return;
   
}

#endif
/*//////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

 End of -include/epiworld/models/diffnet.hpp-

////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////*/


/*//////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

 Start of -include/epiworld/models/seirquarantine.hpp-

////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////*/


#ifndef EPIWORLD_MODELS_SEIRQUARANTINE_HPP
#define EPIWORLD_MODELS_SEIRQUARANTINE_HPP

// (already included include/epiworld/models/../model-bones.hpp)

/**
 * @file seirquarantine.hpp
 * @brief A Susceptible-Exposed-Infected-Removed (SEIR) model
 *        with quarantine, isolation, and contact tracing
 */

/**
 * @brief SEIR model with quarantine, isolation, and contact tracing
 *
 * ![Model Diagram](../assets/img/seirquarantine.png)
 *
 * @details An SEIR model whose contacts come from its layers, of any kind
 * (see `Model::add_layer()`): networks, mixing by group, or both. Each agent
 * has a stage of the disease and a status:
 *
 * - Stages (the virus's natural history): Susceptible, Exposed (latent),
 *   Infected (infectious), Severe (needs hospital care), Recovered. Infected
 *   agents recover or become severe (competing daily rates); severe ones
 *   recover at rate 1 / "Hospitalization period".
 * - Statuses (the model's): Free, Quarantined, Isolated, Hospitalized. The
 *   last three take the agent out of every layer.
 *
 * Every day, the quarantine process (a global event):
 *
 * 1. detects each free infected agent with probability 1 / "Detection delay",
 *    from the day after its onset (a negative delay disables detection, 0
 *    detects on the first day possible); a detected
 *    agent willing to isolate is isolated ("Isolated", if "Isolation period"
 *    is not negative);
 * 2. traces the contacts of the detected agents in every layer's log (the
 *    model turns tracing on at reset), within "Contact tracing window" days,
 *    each found with probability "Contact tracing probability". A free
 *    contact without tools who is willing to quarantine is quarantined if
 *    susceptible or exposed ("Quarantined"), or isolated if infected and
 *    willing to isolate ("Isolated").
 *
 * Declared rules:
 *
 * - Quarantine ends "Quarantine period" days after it starts.
 * - Isolation ends "Isolation period" days after the onset (entering
 *   Infected), whatever the stage then.
 * - An agent quarantined at the start of the day of its onset is isolated
 *   that day ("Onset in quarantine").
 * - Entering Severe admits the agent ("Admitted", recorded as a
 *   hospitalization); leaving it discharges the agent ("Discharged").
 *
 * **Implementation details:**
 * <a href="../impl/quarantine-isolation-and-contact-tracing.md">Quarantine, Isolation, and Contact Tracing</a>
 *
 * @ingroup basic_compartmental
 */
class ModelSEIRQuarantine : public Model
{
private:

    // Data about the quarantine process
    std::vector< bool > quarantine_willingness; ///< Indicator
    std::vector< bool > isolation_willingness; ///< Indicator for isolation willingness
    std::vector< size_t > agent_quarantine_triggered; ///< Whether the quarantine process has started

    static void _quarantine_process(Model * m);

public:

    // Stages
    static constexpr int SUSCEPTIBLE = 0;
    static constexpr int EXPOSED     = 1;
    static constexpr int INFECTED    = 2;
    static constexpr int SEVERE      = 3;
    static constexpr int RECOVERED   = 4;

    // Statuses
    static constexpr int FREE         = 0;
    static constexpr int QUARANTINED  = 1;
    static constexpr int ISOLATED     = 2;
    static constexpr int HOSPITALIZED = 3;

    static const size_t QUARANTINE_PROCESS_INACTIVE = 0u;
    static const size_t QUARANTINE_PROCESS_ACTIVE   = 1u;
    static const size_t QUARANTINE_PROCESS_DONE     = 2u;

    ModelSEIRQuarantine() = delete;

    /**
     * @param vname Name of the virus.
     * @param n Number of agents.
     * @param prevalence Initial share of infected (exposed) agents.
     * @param transmission_prob Probability of transmission per contact.
     * @param latent_period Mean latent period (days; geometric).
     * @param recovery_rate Daily probability of recovery while infected.
     * @param hospitalization_rate Daily probability of becoming severe.
     * @param hospitalization_period Mean days severe (in hospital).
     * @param detection_delay Mean days until an infected agent is detected
     * (negative: never; 0: at once).
     * @param quarantine_period Days of quarantine (negative: no quarantine).
     * @param quarantine_willingness Share of agents willing to quarantine.
     * @param isolation_willingness Share of agents willing to isolate.
     * @param isolation_period Days of isolation from the onset (negative: no
     * isolation).
     * @param contact_tracing_prob Probability that a contact is traced.
     * @param contact_tracing_window Days back the contacts are traced.
     */
    ModelSEIRQuarantine(
        const std::string & vname,
        epiworld_fast_uint n,
        epiworld_double prevalence,
        epiworld_double transmission_prob,
        epiworld_double latent_period,
        epiworld_double recovery_rate,
        epiworld_double hospitalization_rate,
        epiworld_double hospitalization_period,
        // Policy parameters
        epiworld_double detection_delay,
        epiworld_fast_int quarantine_period,
        epiworld_double quarantine_willingness,
        epiworld_double isolation_willingness,
        epiworld_fast_int isolation_period,
        epiworld_double contact_tracing_prob = 1.0,
        epiworld_fast_uint contact_tracing_window = 4u
    );

    void reset() override;
    std::unique_ptr< Model > clone_ptr() override;

    std::vector< size_t > get_agent_quarantine_triggered() const
    {
        return agent_quarantine_triggered;
    };

    std::vector< bool > get_quarantine_willingness() const
    {
        return quarantine_willingness;
    };

    std::vector< bool > get_isolation_willingness() const
    {
        return isolation_willingness;
    };


    /// This model's entry in a `ModelRegistry`: arguments and builder
    static ModelInfo model_info();
};

inline void ModelSEIRQuarantine::reset()
{

    // The quarantine process reads the contacts of every layer
    for (size_t i = 0u; i < this->get_n_layers(); ++i)
        if (!this->get_layer(i).is_tracing())
            this->get_layer(i).enable_tracing();

    Model::reset();

    // Who is willing to quarantine, and to isolate (drawn once per run)
    quarantine_willingness.assign(this->size(), false);
    isolation_willingness.assign(this->size(), false);
    for (size_t idx = 0; idx < quarantine_willingness.size(); ++idx)
    {
        quarantine_willingness[idx] =
            this->runif() < EPI_PAR(this, "Quarantine willingness");
        isolation_willingness[idx] =
            this->runif() < EPI_PAR(this, "Isolation willingness");
    }

    agent_quarantine_triggered.assign(this->size(), 0u);

    return;

}

inline std::unique_ptr<Model> ModelSEIRQuarantine::clone_ptr()
{
    return std::make_unique<ModelSEIRQuarantine>(*this);
}

inline void ModelSEIRQuarantine::_quarantine_process(Model * m)
{

    auto * model = model_cast<ModelSEIRQuarantine>(m);
    const int isolated = m->transition_id("Isolated");
    const int quarantined = m->transition_id("Quarantined");
    const bool isolation_on = EPI_PAR(m, "Isolation period") >= 0;

    // 1. Detection of the free infected agents
    const epiworld_double delay = EPI_PAR(m, "Detection delay");
    if (delay >= 0.0)
    {

        for (auto & p : m->get_agents())
        {

            // From the day after the onset: its contacts as an infectious
            // agent are in the logs by then
            const size_t id = static_cast< size_t >(p.get_id());
            if (
                (m->get_stage(p) != INFECTED) || (m->get_status(p) != FREE) ||
                (model->agent_quarantine_triggered[id] != QUARANTINE_PROCESS_INACTIVE) ||
                (m->get_stage_entered(id, INFECTED) >= m->today())
            )
                continue;

            if ((delay > 0.0) && (m->runif() >= 1.0 / delay))
                continue;

            model->agent_quarantine_triggered[p.get_id()] = QUARANTINE_PROCESS_ACTIVE;
            if (isolation_on && model->isolation_willingness[p.get_id()])
                m->fire(isolated, p);

        }

    }

    // 2. Contact tracing from the detected agents
    for (size_t agent_i = 0u; agent_i < m->size(); ++agent_i)
    {

        if (model->agent_quarantine_triggered[agent_i] != QUARANTINE_PROCESS_ACTIVE)
            continue;

        model->agent_quarantine_triggered[agent_i] = QUARANTINE_PROCESS_DONE;

        if (EPI_PAR(m, "Quarantine period") < 0)
            continue;

        const auto success_rate = EPI_PAR(m, "Contact tracing probability");
        const auto days_prior = EPI_PAR(m, "Contact tracing window");

        // The contacts in the log of every traced layer, in the order the
        // layers were added (a log keeps each agent's last contacts only)
        for (size_t layer_i = 0u; layer_i < m->get_n_layers(); ++layer_i)
        {

            auto & layer = m->get_layer(layer_i);
            if (!layer.is_tracing())
                continue;

            auto & ct = layer.get_tracing();
            const size_t n_contacts = std::min(
                ct.get_n_contacts(agent_i), ct.get_max_contacts()
            );

            for (size_t contact_i = 0u; contact_i < n_contacts; ++contact_i)
            {

                if (m->runif() > success_rate)
                    continue;

                auto [contact_id, contact_date] = ct.get_contact(agent_i, contact_i);

                if ((static_cast< double >(m->today()) -
                    static_cast< double >(contact_date)) > days_prior)
                    continue;

                auto & agent = m->get_agent(contact_id);
                const int s = m->get_stage(agent);
                if ((m->get_status(agent) != FREE) || (s > INFECTED))
                    continue;

                // Agents with some tool are not quarantined
                if (agent.get_n_tools() != 0u)
                    continue;

                if (!model->quarantine_willingness[contact_id])
                    continue;

                if (s != INFECTED)
                    m->fire(quarantined, agent);
                else if (isolation_on && model->isolation_willingness[contact_id])
                    m->fire(isolated, agent);

            }

        }

    }

}

inline ModelSEIRQuarantine::ModelSEIRQuarantine(
    const std::string & vname,
    epiworld_fast_uint n,
    epiworld_double prevalence,
    epiworld_double transmission_prob,
    epiworld_double latent_period,
    epiworld_double recovery_rate,
    epiworld_double hospitalization_rate,
    epiworld_double hospitalization_period,
    // Policy parameters
    epiworld_double detection_delay,
    epiworld_fast_int quarantine_period,
    epiworld_double quarantine_willingness,
    epiworld_double isolation_willingness,
    epiworld_fast_int isolation_period,
    epiworld_double contact_tracing_prob,
    epiworld_fast_uint contact_tracing_window
    )
{

    this->set_population_size(n);

    this->add_param(transmission_prob, "Transmission probability");
    this->add_param(recovery_rate, "Recovery rate");
    this->add_param(latent_period, "Latent period");
    this->add_param(hospitalization_rate, "Hospitalization rate");
    this->add_param(hospitalization_period, "Hospitalization period");
    this->add_param(detection_delay, "Detection delay");
    this->add_param(quarantine_period, "Quarantine period");
    this->add_param(quarantine_willingness, "Quarantine willingness");
    this->add_param(isolation_willingness, "Isolation willingness");
    this->add_param(isolation_period, "Isolation period");
    this->add_param(contact_tracing_prob, "Contact tracing probability");
    this->add_param(contact_tracing_window, "Contact tracing window");

    // The disease
    Virus virus(vname);
    virus.add_stage("Susceptible");
    virus.add_stage("Exposed", stage::carrier);
    virus.add_stage("Infected", stage::infectious);
    virus.add_stage("Severe", stage::carrier);
    virus.add_stage("Recovered");
    virus.infection(SUSCEPTIBLE, EXPOSED, "Transmission probability");
    virus.after(EXPOSED, INFECTED, Duration::geometric("Latent period"));
    virus.rate(INFECTED, RECOVERED, "Recovery rate").effect(Effect::recovery);
    virus.rate(INFECTED, SEVERE, "Hospitalization rate");
    const int out_severe =
        virus.rate(SEVERE, RECOVERED, Value::inverse("Hospitalization period"));
    this->add_virus(virus, assign::share(prevalence));

    // Where agents are
    Statuses s;
    s.add_status("Free");
    s.add_status("Quarantined");
    s.add_status("Isolated");
    s.add_status("Hospitalized");
    s.set_contact(QUARANTINED, 0.0);
    s.set_contact(ISOLATED, 0.0);
    s.set_contact(HOSPITALIZED, 0.0);

    s.external(FREE, QUARANTINED, "Quarantined");
    s.after(QUARANTINED, FREE, Duration::fixed("Quarantine period"))
        .name("Quarantine ends");
    s.external(FREE, ISOLATED, "Isolated");
    const int onset_q = s.external({FREE, QUARANTINED}, ISOLATED, "Onset in quarantine");
    s.after(
        ISOLATED, FREE,
        Duration::fixed("Isolation period").since(INFECTED)
    ).name("Isolation ends");
    const int admit = s.external({FREE, QUARANTINED, ISOLATED}, HOSPITALIZED, "Admitted");
    const int discharge = s.external(HOSPITALIZED, FREE, "Discharged");
    this->set_statuses(std::move(s));

    // How the two are linked
    this->react(Trigger::entered(INFECTED).was({QUARANTINED}), onset_q);
    this->react(Trigger::entered(SEVERE), admit);
    this->react(
        Trigger::entered(SEVERE),
        [](Agent & p, Model & m) { m.record_hospitalization(p); }
    );
    this->react(Trigger::fired(out_severe), discharge);

    this->add_globalevent(_quarantine_process, "Quarantine process");

    this->set_name("SEIR with Quarantine");

}

inline ModelInfo ModelSEIRQuarantine::model_info()
{

    using uint_t = epiworld_fast_uint;
    using int_t  = epiworld_fast_int;
    const double inf = std::numeric_limits< double >::infinity();

    return {"SEIRQuarantine",
        "SEIR with hospitalization, isolation and quarantine",
        "SEIR where detected cases are isolated and their contacts, traced "
        "through the layers that trace, quarantined.",
        "Disease",
        {
            ParamInfo::agents(10000),
            ParamInfo::prevalence(0.01),
            ParamInfo::probability("transmission_prob", "Transmission probability",
                0.05, "Probability that a contact with an infected agent infects."),
            ParamInfo::duration("latent_period", "Latent period", 7.0,
                "Mean days in the exposed state (infected, not yet infectious)."),
            ParamInfo::probability("recovery_rate", "Recovery rate", 0.14,
                "Daily probability that an infected agent recovers."),
            ParamInfo::probability("hospitalization_rate", "Hospitalization rate",
                0.05, "Daily probability that an infected agent is hospitalized."),
            ParamInfo::duration("hospitalization_period", "Hospitalization period",
                7.0, "Average number of days in the hospital."),
            {"detection_delay", "Detection delay", 3.0, -1.0, inf, false,
                "Mean days before an infected agent is detected (0: at once; "
                "negative: never)."},
            ParamInfo::period("quarantine_period", "Quarantine period", 14.0,
                "Days in quarantine for the contacts of a detected case "
                "(negative: no quarantine)."),
            ParamInfo::probability("quarantine_willingness",
                "Quarantine willingness", 0.8,
                "Probability that a contact complies with quarantine."),
            ParamInfo::probability("isolation_willingness",
                "Isolation willingness", 0.5,
                "Probability that a detected case complies with isolation."),
            ParamInfo::period("isolation_period", "Isolation period", 7.0,
                "Days in isolation for a detected case (negative: no "
                "isolation)."),
            ParamInfo::probability("contact_tracing_prob",
                "Contact tracing probability", 1.0,
                "Probability that a contact is traced."),
            ParamInfo::count("contact_tracing_window", "Contact tracing window",
                4.0, "Days before detection whose contacts are traced.")
        },
        [](const ModelArgs & a) -> std::unique_ptr< Model > {
            return std::make_unique< ModelSEIRQuarantine >(
                a.virus_name(), static_cast< uint_t >(a("n")), a("prevalence"),
                a("transmission_prob"), a("latent_period"), a("recovery_rate"),
                a("hospitalization_rate"), a("hospitalization_period"),
                a("detection_delay"),
                static_cast< int_t >(a("quarantine_period")),
                a("quarantine_willingness"), a("isolation_willingness"),
                static_cast< int_t >(a("isolation_period")),
                a("contact_tracing_prob"),
                static_cast< uint_t >(a("contact_tracing_window"))
            );
        }};
}

#endif
/*//////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

 End of -include/epiworld/models/seirquarantine.hpp-

////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////*/



    /**
     * @brief Adds the core models to a model registry.
     * @ingroup epi_models
     *
     * @details Each model's entry is its `model_info()`. `ModelSIRLogit` and
     * `ModelDiffNet` take data arrays and are built directly.
     */
    inline ModelRegistry & register_models(ModelRegistry & registry)
    {
        return registry
            .add(ModelSIS::model_info())
            .add(ModelSIR::model_info())
            .add(ModelSEIR::model_info())
            .add(ModelSISD::model_info())
            .add(ModelSIRD::model_info())
            .add(ModelSEIRD::model_info())
            .add(ModelSEIRQuarantine::model_info())
            .add(ModelSURV::model_info());
    }
}

#endif
/*//////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

 End of -./include/epiworld/models/models.hpp-

////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////*/



}

// ---------------------------------------------------------------------------
// Specializations of std::generate_canonical for epiworld::epi_xoshiro256ss.
//
// This is the agnostic performance fix: by specializing generate_canonical for
// our custom engine, ALL std distributions (normal, gamma, binomial, etc.)
// that call generate_canonical internally will automatically bypass the
// long-double arithmetic that causes severe slowdowns on platforms where
// long double is software-emulated 128-bit (e.g., Clang+libstdc++ on ARM).
//
// The specialisation converts the 64-bit engine output to a floating-point
// value in [0, 1) using only the precision of the target type:
//   - double (53 mantissa bits): top 53 bits of the 64-bit word, × 2^{-53}
//   - float  (24 mantissa bits): top 24 bits,                      × 2^{-24}
//
// The result is strictly less than 1.0 by construction (max value is
// (2^N - 1) / 2^N), so no nextafter clamp is required.
// ---------------------------------------------------------------------------
namespace std {
    template<>
    inline double generate_canonical<
        double,
        numeric_limits<double>::digits,
        epiworld::epi_xoshiro256ss
    >(epiworld::epi_xoshiro256ss& eng) {
        return static_cast<double>(eng() >> 11) * 0x1.0p-53;
    }

    template<>
    inline float generate_canonical<
        float,
        numeric_limits<float>::digits,
        epiworld::epi_xoshiro256ss
    >(epiworld::epi_xoshiro256ss& eng) {
        return static_cast<float>(eng() >> 40) * 0x1.0p-24f;
    }
}

#endif
