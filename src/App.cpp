#include "App.hpp"
#include "Librarydb.hpp"
#include "ftxui/component/screen_interactive.hpp"
#include "ftxui/component/component.hpp"

#include <cstdlib>
#include <fstream>
#include <memory>
#include <exception>

class Exit : public std::exception {};

int App::run() {
    try {
        if(session.empty()) {
            login();
        }
        else{
            //TODO: attempt to restore the session
            auto usr = db->restoreSession(session);
            if(usr.username.empty()){
                // restore failed, session expired
                login();
            }
            else{
                active_user = std::make_unique<User>(usr);
            }
        }
        // Now logged in
        auto screen = ftxui::ScreenInteractive::Fullscreen();

        // something else
    }
    catch(Exit& e){
        // cleanup
    }
    //
    return EXIT_SUCCESS;
}

void App::login() {
    auto login_screen_container = ftxui::Container::Vertical({
        ftxui::Container::Horizontal({
            ftxui::Button("Login", [](){}),
            ftxui::Button("Sign Up", [](){}),
        }),
        ftxui::Input("", "username"),
        ftxui::Input("", "password"),
        ftxui::Button("Quit", [](){ throw Exit{}; })
    });
    auto screen = ftxui::ScreenInteractive::Fullscreen();
    screen.Loop(login_screen_container);
    User usr;
    std::string password;
    usr.username = "root";
    usr.email = "root@library.com";
    usr.type = UserClass::ADMIN;

    // write session data
    auto ofs = std::ofstream(session_file.c_str());
    ofs<<usr.username+ ':' + password;
    ofs.close();

    active_user = std::make_unique<User>(usr);
}
