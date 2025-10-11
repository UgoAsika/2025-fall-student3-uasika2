#include <iostream>
#include <string>
#include <cstdint>


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

int main(int argc, char** argv) {
    //expect exactly 6 command line args plus program name
    // so argc should be 7 total
    if (argc != 7) {
        usage(argv[0]);
        return 1; // exit with error
    }

    try {
        //parse numerical arguments and store policies as __cpp_raw_strings
        uint64_t sets = std::stoull(argv[1]);
        uint64_t blocksPerSet = std::stoull(argv[2]);
        uint64_t bytesPerBlock = std::stoull(argv[3]);
        std::string alloc = argv[4];
        std::string write = argv[5];
        std::string evict = argv[6];

        bool ok = true; // flag to ctrack parameter validity

        //validate that set/block/byte counts are powers of two
        if (!isPowerOfTwo(sets) || !isPowerOfTwo(blocksPerSet) || !isPowerOfTwo(bytesPerBlock)) {
            std::cerr << "error: sets/blocks/bytes must each be positive powers of two.\n";
            ok = false;
        }
        //ensure minimum block size is 4 bytes
        if (bytesPerBlock < 4) {
            std::cerr << "error: bytes per block must be at least 4.\n";
            ok = false;
        }
        //validate write-allocate policy
        if (!(alloc == "write-allocate" || alloc == "no-write-allocate")) {
            std::cerr << "error: allocation policy must be write-allocate or no-write-allocate.\n";
            ok = false;
        }
        //validate write policy
        if (!(write == "write-through" || write == "write-back")) {
            std::cerr << "error: write policy must be write-through or write-back.\n";
            ok = false;
        }
        //validate eviction policy
        if (!(evict == "lru" || evict == "fifo")) {
            std::cerr << "error: eviction policy must be lru or fifo.\n";
            ok = false;
        }
        //ensure invalid policy combination is rejected
        if (alloc == "no-write-allocate" && write == "write-back") {
            std::cerr << "error: invalid combination: no-write-allocate with write-back.\n";
            ok = false;
        }

        //if any invalid parameter is detected return with error
        if (!ok) return 1;
        //this ois the end for milestone one for me
        return 0;
    } catch (...) {
        //catch conversion or runtime errors like non numeric inputs
        std::cerr << "error: invalid numeric parameter.\n";
        usage(argv[0]);
        return 1;
    }
}
