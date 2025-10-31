#include <iostream>
#include <string>
#include <cstdint>
#include <vector>
#include <sstream>
#include <stdexcept>
#include <limits>

//Im using the config struct to hold cache config parameters parsed from command line.

struct Config{
    uint64_t sets = 0; //number of our cache sets
    uint64_t ways = 0; //blocks per set basically associativity
    uint64_t blockBytes = 0; //these are our bytes per block
    bool writeAllocate = true; // if true write-allcoate else no write allocate
    bool writeThrough = false; // if true write through, else write back
    enum Evict {LRU, FIFO} evict = LRU; //this is our eviction policy
};


//Helper function to check if a number is a power of 2
// a number x is a power of two if x > 0 and x & (x-1) == 0
static bool isPowerOfTwo(uint64_t x) {
    return x && ((x & (x - 1)) == 0);
}

//prints usage instructions to standard error
//Called when incorrect arguments are provided
static void usage(const char* prog) {
    std::cerr
        << "Usage: " << prog << " <sets> <blocks_per_set> <bytes_per_block> "
        << "<write-allocate|no-write-allocate> <write-through|write-back> "
        << "<lru|fifo>\n";
}

//This will comput the log2(x) assuming x is a power of two
//We will use it to determine index and offset bit counts basically
static uint64_t log2u(uint64_t x){
    uint64_t n = 0;
    while ((1ull << n) < x) ++n;
    return n;
};

//Starting here I will write my cache data structures

//This struct represents one cache line, so a block
struct Line{
    bool valid = false; // this is true if block has valid data
    bool dirty = false; // this is true if modified (for write back)
    uint64_t tag = 0; // tag portion of the address
    uint64_t lastUsed = 0; // this is the access time stamp for the LRU
    uint64_t fifoOrdinal = 0; // The order inserted for FIFO
};

//Represents a cache set (contains multiple lines)

struct Set{
    std::vector<Line> lines; //vector of cache lines
    uint64_t nextFifoOrdinal = 0; // counter to assign FIFO order numbers
    explicit Set(size_t ways = 1) : lines(ways) {}
};

//This is the entire cache and its simulation logic
struct Cache{
    Config cfg; //cache config
    std::vector<Set> sets; //vector of all sets in the cache

    //These are the derived fields for bit manipulation
    uint64_t idxBits = 0; //number of index bits
    uint64_t offBits = 0; //number of block offset bits
    uint64_t idxMask = 0; //mask for extracting index bits

    //Statistics counters
    uint64_t totalLoads = 0;
    uint64_t totalStores = 0;
    uint64_t loadHits = 0;
    uint64_t loadMisses = 0;
    uint64_t storeHits = 0;
    uint64_t storeMisses = 0;
    uint64_t cycles = 0;         // total simulated cycles

    //Global counter for LRU recency tracking
    uint64_t accessTick = 0;

    //This constructor initialises sets and bit masks
    explicit Cache(const Config& c) : cfg(c), sets(c.sets, Set(static_cast<size_t>(c.ways))){
        offBits = log2u(cfg.blockBytes);
        idxBits = log2u(cfg.sets);
        idxMask = (cfg.sets - 1ull);
    }

    //Extracts index field from address
    inline uint64_t indexOf(uint64_t addr) const {
        return (addr >> offBits) & idxMask;
    }

    //This takes the tag field from the address
    inline uint64_t tagOf(uint64_t addr) const{
        return (addr >> (offBits + idxBits));
    }

    //These are the timing helpers each cache access costs 1 cycle
    inline void cacheAccessCost() { cycles += 1; }

    //Memory transfer cost would be 100 cycles per 4 bytes
    inline uint64_t memCost_bytes(uint64_t nbytes) const {
        return 100ull * (nbytes / 4ull);
    }
    inline uint64_t memCost_word() const {
        return 100ull;
    }

    //cache lookup and replacements

    //return index of matching tag if hit, or size() if miss.
    size_t findHit(Set& s, uint64_t tag) const{
        for(size_t i = 0; i<s.lines.size(); i++){
            if(s.lines[i].valid && s.lines[i].tag == tag){
                return i;
            }
        }
        return s.lines.size();
    }

    //Selects victim line to evict based on LRU or FIFO
    size_t chooseVictim(Set& s) const{
        //First try to find an invalid line (free slot)
        for(size_t i = 0; i < s.lines.size(); ++i){
            if(!s.lines[i].valid) return i;
        }
        //Other wise you should evict according to the policy!
        if(cfg.evict == Config::LRU){
            uint64_t bestTick = std::numeric_limits<uint64_t>::max();
            size_t bestIdx = 0;
            for(size_t i = 0; i < s.lines.size(); ++i){
                if(s.lines[i].lastUsed < bestTick){
                    bestTick = s.lines[i].lastUsed;
                    bestIdx = i;
                }
            }
            return bestIdx;
        } else{
            uint64_t oldest = std::numeric_limits<uint64_t>::max();
            size_t bestIdx = 0;
            for(size_t i = 0; i < s.lines.size(); ++i){
                if(s.lines[i].fifoOrdinal < oldest){
                    oldest = s.lines[i].fifoOrdinal;
                    bestIdx = i;
                }
            }
            return bestIdx;
        }
    }

    //Load a block from memory into cache, evicting if we need to
    size_t fillBlock(Set& s, uint64_t tag){
        size_t victim = chooseVictim(s);
        //if evicting a dirty line (write-back), write to memory first.
        if(s.lines[victim].valid && s.lines[victim].dirty && !cfg.writeThrough){
            cycles += memCost_bytes(cfg.blockBytes);
        }
        //This fetches the new block from memory into cache
        cycles += memCost_bytes(cfg.blockBytes);

        //This just updates metadata
        Line& ln = s.lines[victim];
        ln.valid = true;
        ln.dirty = false;
        ln.tag = tag;
        ln.lastUsed = 0; // we’ll update this after the access itself
        ln.fifoOrdinal = s.nextFifoOrdinal++;
        return victim;
    }
    
    //Load / Read operation
    void load(uint64_t addr){
        ++totalLoads;
        uint64_t idx = indexOf(addr);
        uint64_t tag = tagOf(addr);
        Set& s = sets[idx];

        size_t i = findHit(s, tag);
        if(i < s.lines.size()){
            //cache hit
            ++loadHits;
            cacheAccessCost();
            s.lines[i].lastUsed = ++accessTick;
            return;
        }

        //cache miss
        ++loadMisses;

        //fetch the block into the cache and then access it
        size_t filled = fillBlock(s,tag);
        cacheAccessCost();
        s.lines[filled].lastUsed = ++accessTick;
    }

    //Store (writing) operation
    void store(uint64_t addr){
        ++totalStores;
        uint64_t idx = indexOf(addr);
        uint64_t tag = tagOf(addr);
        Set& s = sets[idx];
        size_t i = findHit(s,tag);
        if(i<s.lines.size()){
            //Cache hit
            ++storeHits;
            cacheAccessCost();
            if(cfg.writeThrough){
                //Write immediately to memory (4 bytes)
                cycles += memCost_word();
            }else {
                //Write back: mark dirty, delay write until there is an eviction
                s.lines[i].dirty = true;
            }
            s.lines[i].lastUsed = ++accessTick;
            return;
        }

        //Cache miss
        ++storeMisses;
        if(cfg.writeAllocate){
            //Bring the block into your cache, then perform the write
            size_t filled = fillBlock(s, tag);
            cacheAccessCost();

            if(cfg.writeThrough){
                cycles += memCost_word();
            }
            else{
                sets[idx].lines[filled].dirty = true;
            }
            sets[idx].lines[filled].lastUsed = ++accessTick;
        } else{
            //No write allocate: write directly to the memory only with 4 bytes
            cycles += memCost_word();
        }
    }
};

//THis is the parse config arguments

static bool parseConfig(int argc, char** argv, Config& cfg){
    if(argc != 7) return false;
    try{
        cfg.sets = std::stoull(argv[1]);
        cfg.ways = std::stoull(argv[2]);
        cfg.blockBytes = std::stoull(argv[3]);

        std::string alloc = argv[4];
        std::string write = argv[5];
        std::string evict = argv[6];

        //These are the parse string options
        if (alloc == "write-allocate") cfg.writeAllocate = true;
        else if (alloc == "no-write-allocate") cfg.writeAllocate = false;
        else return false;

        if (write == "write-through") cfg.writeThrough = true;
        else if (write == "write-back") cfg.writeThrough = false;
        else return false;

        if (evict == "lru") cfg.evict = Config::LRU;
        else if (evict == "fifo") cfg.evict = Config::FIFO;
        else return false;

        // Validate numeric values
        if (!isPowerOfTwo(cfg.sets) || !isPowerOfTwo(cfg.ways) || !isPowerOfTwo(cfg.blockBytes))
            return false;
        if (cfg.blockBytes < 4) return false;

        // Invalid combo: no-write-allocate with write-back
        if (!cfg.writeAllocate && !cfg.writeThrough) return false;

        return true;
    } catch(...){
        return false;
    }
}

int main(int argc, char** argv) {
    std::ios::sync_with_stdio(false);
    std::cin.tie(nullptr);

    Config cfg;

    // Parse and validate configuration
    if (!parseConfig(argc, argv, cfg)) {
        std::cerr << "error: invalid parameters.\n";
        usage(argv[0]);
        return 1;
    }

    Cache cache(cfg);

    // Read each trace line from stdin
    std::string line;
    while (std::getline(std::cin, line)) {
        if (line.empty()) continue;

        std::istringstream iss(line);
        char op;
        std::string addrStr;
        std::string ignoreSize;

        // Example: "l 0x1fffff50 1"
        if (!(iss >> op >> addrStr)) continue;
        if (iss.good()) iss >> ignoreSize; // third field ignored

        // Convert hex string to integer address
        uint64_t addr = 0;
        try {
            addr = std::stoull(addrStr, nullptr, 16);
        } catch (...) {
            continue; // skip bad lines
        }

        // Execute corresponding cache operation
        if (op == 'l' || op == 'L') {
            cache.load(addr);
        } else if (op == 's' || op == 'S') {
            cache.store(addr);
        }
    }

    // Print results exactly as required
    std::cout << "Total loads: "  << cache.totalLoads   << "\n";
    std::cout << "Total stores: " << cache.totalStores  << "\n";
    std::cout << "Load hits: "    << cache.loadHits     << "\n";
    std::cout << "Load misses: "  << cache.loadMisses   << "\n";
    std::cout << "Store hits: "   << cache.storeHits    << "\n";
    std::cout << "Store misses: " << cache.storeMisses  << "\n";
    std::cout << "Total cycles: " << cache.cycles       << "\n";

    return 0;
}
