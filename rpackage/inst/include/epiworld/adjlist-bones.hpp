#ifndef EPIWORLD_ADJLIST_BONES_HPP
#define EPIWORLD_ADJLIST_BONES_HPP

class AdjList {
private:

    std::map<unsigned int,std::map<unsigned int, unsigned int>> dat;
    bool directed;
    unsigned int id_min,id_max;
    unsigned int N = 0;
    unsigned int E = 0;

public:

    AdjList() {};

    /**
     * @brief Construct a new Adj List object
     * 
     * @details 
     * It will create an adjacency list object with `maxid - minid + 1`
     * nodes. If min_id and max_id are not specified (both < 0), then the program will
     * try to figure them out automatically by looking at the range of the observed
     * ids.
     * 
     * @param source Unsigned int vector with the source
     * @param target Unsigned int vector with the target
     * @param directed Bool true if the network is directed
     * @param min_id int min id.
     * @param max_id int max id.
     */
    AdjList(
        const std::vector< unsigned int > & source,
        const std::vector< unsigned int > & target,
        bool directed,
        int min_id = -1,
        int max_id = -1
        );

    void read_edgelist(
        std::string fn,
        int skip = 0,
        bool directed = true,
        int min_id = -1,
        int max_id = -1
        );

    std::map<unsigned int, unsigned int> operator()(
        unsigned int i
        ) const;
        
    void print(unsigned int limit = 20u) const;
    unsigned int get_id_max() const;
    unsigned int get_id_min() const;
    size_t vcount() const;
    size_t ecount() const;
    
    std::map<unsigned int,std::map<unsigned int,unsigned int>> & get_dat() {
        return dat;
    };

    bool is_directed() const;

};


#endif

