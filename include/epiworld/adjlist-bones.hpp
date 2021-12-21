#ifndef EPIWORLD_ADJLIST_BONES_HPP
#define EPIWORLD_ADJLIST_BONES_HPP

class AdjList {
private:

    std::string filename;
    std::map<int,std::map<int,int>> dat;

    int id_min,id_max;
    int N = 0;
    int E = 0;

public:

    void read_edgelist(std::string fn, int skip = 0, bool directed = true);
    const std::map<int,int> & operator()(int i) const;
        
    void print(unsigned int limit = 20u) const;
    int get_id_max() const;
    int get_id_min() const;
    size_t vcount() const;
    size_t ecount() const;
    
    const std::map<int,std::map<int,int>> & get_dat() const {
        return dat;
    };

};


#endif

