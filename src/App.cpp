#include "App.hpp"
#include "Librarydb.hpp"

#include "User.hpp"
#include "ftxui/component/screen_interactive.hpp"
#include "ftxui/component/component.hpp"

#include <chrono>
#include <cstdlib>
#include <fstream>
#include <ftxui/dom/elements.hpp>
#include <future>
#include <memory>
#include <exception>
#include <thread>
#include <utility>
#include <vector>

class Exit : public std::exception {};

int App::run() {
    try {
        while(true) {
            if(not active_user) {
                login();
            }
            else {
                home();
            }
        }
    }
    catch(Exit& e){
        // cleanup
    }
    //
    return EXIT_SUCCESS;
}

void App::attemptRestore() {
    //TODO: attempt to restore the session
    auto usr = db->restoreSession(session);
    if(usr.username.empty()){
        // restore failed, session expired
        return;
    }
    else{
        active_user = std::make_unique<User>(usr);
    }
}

void flip(bool& flag) {
    auto th = std::thread([&flag] {
        flag = true;
        std::this_thread::sleep_for(std::chrono::seconds{1});;
        flag = false;
    });
    th.detach();
}

void App::writeSession() {
    // Shouldn't be called when there is no active user
    if (not active_user)
        throw std::exception();
    // TODO: Derive session informatin from active_user and write it to session_file
    auto ofs = std::ofstream(session_file.c_str());
    // do the actual writing
    ofs.close();
}

void App::login() {
    using namespace ftxui;

    if(not session.empty()) {
        attemptRestore();
    }
    if (active_user) {
        home();
    }

    bool signup_ok = false;
    std::string login_username, signup_username, signup_password, login_password, email, signup_error_message;
    auto signup_status = Renderer([&] {
        return text(signup_error_message) | color(Color::Red);
    }) | Maybe([&] {
            bool show_error = true;
            if ( !signup_username.empty() && signup_username.size() < 4) {
                signup_error_message = "Username too short!";
            }
            else if ( !signup_username.empty() && db->usernameExists(signup_username)) {
                signup_error_message = "Username is taken!";
            }
            else if ( !signup_password.empty() && signup_password.size() < 4) {
                signup_error_message = "Password is too short!";
            }
            else if (!email.empty() && db->emailIsUsed(email)) {
                signup_error_message = "Email is used before!";
            }
            else {
                show_error = false;
            }
            signup_ok = !(show_error || signup_username.empty() || signup_password.empty() || email.empty());
            return show_error;
        });

    bool show_failed_authentication = false;
    auto failed_login = Renderer([&] {
        return text("Login failed. Wrong credentials!") | color(Color::Red);
    }) | Maybe(&show_failed_authentication);

    auto signup_action = [&] {
        if (! signup_ok)
            return;

        // save sign-up info and set active_user
        auto usr = User{email, signup_username, UserClass::NORMAL};
        db->addUser(usr, signup_password);
        // signed up
        active_user = std::make_unique<User>(usr);
        signup_password.clear();
        signup_username.clear();
        email.clear();
        home();
    };

    auto login_action = [&] {
        //read button data and set active_user
        auto usr = db->authenticate(login_username, login_password);
        if (not usr) {
            login_username.clear();
            login_password.clear();
            flip(show_failed_authentication);
            }
        else {
            // loged in
            login_password.clear();
            login_username.clear();
            active_user = std::make_unique<User>(usr.value());
            home();
        }
    };

    // input boxes
    ftxui::InputOption password_option;
    password_option.password = true;

    auto login_screen_container = Container::Vertical({
        Input(&login_username, "Username") | size(WIDTH, ftxui::EQUAL, 40) | border,
        Input(&login_password, "Password", password_option) | size(WIDTH, ftxui::EQUAL, 40) | border,
        failed_login,
        Container::Horizontal({
            Button("Login", login_action, ButtonOption::Ascii()),
            Button("Quit", [] { throw Exit(); }, ButtonOption::Ascii())
        })
    });

    auto signup_screen_container = Container::Vertical({
        Input(&email, "Email") | size(WIDTH, ftxui::EQUAL, 40) | border,
        Input(&signup_username, "Username") | size(WIDTH, ftxui::EQUAL, 40) | border,
        Input(&signup_password, "Password", password_option) | size(WIDTH, ftxui::EQUAL, 40) | ftxui::border,
        signup_status,
        Container::Horizontal({
            Button("Sign Up", signup_action, ButtonOption::Ascii()),
            Button("Quit", [&] { throw Exit(); }, ButtonOption::Ascii())
       })
    });

    std::vector<std::string> toggle_labels{"Login", "Signup"};
    int login_signup_selected = 0;

    auto login_signup_screen = Container::Vertical({
        Toggle(&toggle_labels, &login_signup_selected),
        Container::Tab({login_screen_container, signup_screen_container },
            &login_signup_selected)
    });


    auto login_signup_renderer = Renderer(login_signup_screen, [&] {
        return vbox({
            hbox(
                filler(),
                text("Welcome to Library Management System") | bold,
                filler()
            ),
            separator(),
            filler(),
            hbox(
                filler(),
                login_signup_screen->Render(),
                filler()
            ),
            filler()
        }) | border;
    });

    screen.Loop(login_signup_renderer);
}

void App::home() {
    // TODO: home screen implemntation
    std::string username = active_user->username;
    auto home_screen = ftxui::Container::Vertical({
        ftxui::Button("Logout", [&]{
            active_user = nullptr;
            screen.Exit();
        }, ftxui::ButtonOption::Ascii())
    });

    auto home_renderer = ftxui::Renderer(home_screen, [&] {
        return ftxui::vbox(
            ftxui::text(std::string("Welcome home: ") + username),
            home_screen->Render()
        );
    });
    screen.Loop(home_renderer);
}
