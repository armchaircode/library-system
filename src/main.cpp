#include "App.hpp"
#include "Librarydb.hpp"

#include <iostream> // cerr
#include <cstdlib> // EXIT_FAILURE
#include <exception> // exception
#include <filesystem> // create_directory, canonical, is_regular_file
#include <iterator> // next
#include <memory> // make_unique
#include <stdexcept> // invalid_argument
#include <vector> // vector
#include <string> // string
#include <fstream> // ofstream

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


    if(not db_path.empty()) {
        try{
            auto path = std::filesystem::canonical(db_path);
            if (not std::filesystem::is_regular_file(path)){
                std::cerr<<"Error: Can't open database file "<<path<<" : Not regular file\n";
                return EXIT_FAILURE;
            }
        }
        catch(const std::exception& e){
            std::cerr<<"Error: Database file "<<db_path<<" doesn't exist!\n";
            return EXIT_FAILURE;
        }
    }
    else {
        std::string path = data_dir / "library.db";
        if(not std::filesystem::is_regular_file(path)){
            auto fs = std::ofstream(path.c_str());
        }
        db_path = path;
    }

    try {
        db = std::make_unique<Librarydb>(db_path);
    }
    catch(const std::invalid_argument& e) {
        std::cerr<<"[ERROR] Failed to initialize database: <"<<e.what()<<">"<<std::endl;
        return EXIT_FAILURE;
    }

    ap = std::make_unique<App>();
    ap->session_file = data_dir / "session.txt";
    if(new_session){
        ap->newSession = true;
    }

    return ap->run();
}
