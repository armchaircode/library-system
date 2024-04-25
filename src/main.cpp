#include "App.hpp"
#include "Librarydb.hpp"

#include <iostream>
#include <memory>
#include <vector>

int main(int argc, char** argv) {
    std::vector<std::string> args;
    for(int i = 0; i<argc; ++i) {
        args.push_back(argv[i]);
    }
    for(auto arg : args)
        std::cout<<arg<<std::endl;

    ap = std::make_unique<App>();
    db = std::make_unique<Librarydb>();
    db->db_path = "";
    db->init();

    return ap->run();
}
