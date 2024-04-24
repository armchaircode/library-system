#include "SQLiteCpp/Database.h"
#include <memory>

class Librarydb{
    public:
        void init();
    //
};

inline std::unique_ptr<Librarydb> db;
