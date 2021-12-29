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

#ifndef EPIWORLD_HPP
#define EPIWORLD_HPP

namespace epiworld {

    #include "epiworld-macros.hpp"

    #include "config.hpp"
    #include "misc.hpp"
    #include "progress.hpp"
    #include "database-bones.hpp"
    #include "database-meat.hpp"
    #include "adjlist-bones.hpp"
    #include "adjlist-meat.hpp"

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