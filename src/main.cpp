#include "App.hpp"
#include "Librarydb.hpp"

#include <iostream>
#include <cstdlib>
#include <exception>
#include <filesystem>
#include <iterator>
#include <memory>
#include <vector>
#include <string>
#include <fstream>

void print_usage() {
std::cerr<<
R"#(
Library Management System

Usage: library [-n] [-d dbfile]
    -n          Start new session
    -d FILE     Open database file FILE
)#";
}

int main(int argc, char** argv) {
    std::vector<std::string> args{argv+1, argv+argc};
    bool new_session = false;
    std::string db_path;

    for(auto it = args.begin(); it != args.end(); ++it) {
        if(*it == "-n")
            new_session = true;
        else if (*it == "-d") {
            if(std::next(it) == args.end()){
                print_usage();
                return EXIT_FAILURE;
            }
            db_path = *std::next(it);
            ++it;
        }
        else {
            print_usage();
            return EXIT_FAILURE;
        }
    }
    std::filesystem::path data_dir;

    #ifdef WINDOWS_TARGET_H
    if(auto dir = std::getenv("APPDATA")){
        data_dir = dir;
    }
    else {
        data_dir = data_dir / getenv("USERPROFILE") / "AppData" / "Roaming";
    }
    #else
    if(auto dir = std::getenv("XDG_DATA_HOME")){
        data_dir = dir;
    }
    else {
        data_dir = data_dir / getenv("HOME") / ".local" / "share";
    }
    #endif // WINDOWS_TARGET_H

    data_dir /= "library-system";
    std::filesystem::create_directory(data_dir);

    db = std::make_unique<Librarydb>();

    if(not db_path.empty()) {
        try{
            auto path = std::filesystem::canonical(db_path);
            if (not std::filesystem::is_regular_file(path)){
                std::cerr<<"Error: Can't open database file "<<path<<" : Not regular file\n";
                return EXIT_FAILURE;
            }
        }
        catch(std::exception& e){
            std::cerr<<"Error: Database file "<<db_path<<" doesn't exist!\n";
            return EXIT_FAILURE;
        }
        db->db_path = db_path.c_str();
    }
    else {
        std::string path = data_dir / "library.db";
        if(not std::filesystem::is_regular_file(path)){
            auto fs = std::ofstream(path.c_str());
        }
        db->db_path = path.c_str();
    }
    db->init();

    ap = std::make_unique<App>();
    ap->session_file = data_dir / "session.txt";
    if(!new_session){
        auto session_path = data_dir / "session.txt";
        if(std::filesystem::is_regular_file(session_path)){
            std::string session;
            auto ifs = std::ifstream(session_path.c_str());
            //TODO: session file contents may vary. Proper reading is requisite.
            ifs>>session;
            ap->session = session;
        }
    }

    return ap->run();
}
