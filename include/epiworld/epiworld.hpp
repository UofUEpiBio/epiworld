#include <vector>
#include <functional>
#include <memory>
#include <stdexcept>
#include <random>
#include <fstream>
#include <string>
#include <map>
#include <unordered_map>
#include <chrono>
#include <climits>
#include <cstdint>

#ifndef EPIWORLD_HPP
#define EPIWORLD_HPP

namespace epiworld {

    #include "config.hpp"
    #include "epiworld-macros.hpp"

    #include "misc.hpp"
    #include "progress.hpp"

    #include "userdata-bones.hpp"
    #include "userdata-meat.hpp"

    #include "seq_processing.hpp"

    #include "database-bones.hpp"
    #include "database-meat.hpp"
    #include "adjlist-bones.hpp"
    #include "adjlist-meat.hpp"

    #include "randgraph.hpp"

    #include "queue-bones.hpp"

    #include "model-bones.hpp"
    #include "model-meat.hpp"
    #include "virus-bones.hpp"
    #include "virus-meat.hpp"
    #include "personviruses-bones.hpp"
    #include "personviruses-meat.hpp"
    #include "tools-bones.hpp"
    #include "tools-meat.hpp"
    #include "persontools-bones.hpp"
    #include "persontools-meat.hpp"
    
    #include "person-meat-status.hpp"
    #include "person-bones.hpp"
    #include "person-meat.hpp"

}

#endif 