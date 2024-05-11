#include "App.hpp"
#include "Librarydb.hpp"

#include "User.hpp"
#include "ftxui/component/screen_interactive.hpp"
#include "ftxui/component/component.hpp"

#include <cstdlib>
#include <fstream>
#include <ftxui/dom/elements.hpp>
#include <memory>
#include <exception>
#include <vector>

class Exit : public std::exception {};

int App::run() {
    if(not session.empty()) {
        attemptRestore();
    }
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
    std::string login_username, signup_username, signup_password, login_password, email;

    auto signup_action = [&] {
        // save sign-up info and set active_user
        auto usr = User{email, signup_username, UserClass::NORMAL};
        db->addUser(usr, signup_password);
        // signed up
        active_user = std::make_unique<User>(usr);
        screen.Exit();
    };

    auto login_action = [&] {
        //read button data and set active_user
        auto usr = db->authenticate(login_username, login_password);
        if (not usr) {
            // TODO: authentication failed
        }
        else {
            // loged in
            active_user = std::make_unique<User>(usr.value());
            screen.Exit();
        }
    };

    // input boxes
    ftxui::InputOption password_option;
    password_option.password = true;

    auto login_screen_container = ftxui::Container::Vertical({
        ftxui::Input(&login_username, "Username") | ftxui::border,
        ftxui::Input(&login_password, "Password", password_option) | ftxui::border,
        ftxui::Container::Horizontal({
            ftxui::Button("Login", login_action, ftxui::ButtonOption::Ascii()),
            ftxui::Button("Quit", [] { throw Exit(); }, ftxui::ButtonOption::Ascii())
        })
    });

    auto signup_screen_container = ftxui::Container::Vertical({
        ftxui::Input(&email, "Email") | ftxui::border,
        ftxui::Input(&signup_username, "Username") | ftxui::border,
        ftxui::Input(&signup_password, "Password", password_option) | ftxui::border,
        ftxui::Container::Horizontal({
            ftxui::Button("Sign Up", signup_action, ftxui::ButtonOption::Ascii()),
            ftxui::Button("Quit", [&] { throw Exit(); }, ftxui::ButtonOption::Ascii())
        })
    });

    std::vector<std::string> toggle_labels{"Login", "Signup"};
    int login_signup_selected = 0;

    auto login_signup_screen = ftxui::Container::Vertical({
        ftxui::Toggle(&toggle_labels, &login_signup_selected),
        ftxui::Container::Tab({login_screen_container, signup_screen_container },
            &login_signup_selected)
    });

    auto login_signup_renderer = ftxui::Renderer(login_signup_screen, [&] {
        return ftxui::vbox({
            ftxui::hbox(
                ftxui::filler(),
                ftxui::text("Welcome to Library Management System") | ftxui::bold,
                ftxui::filler()
            ),
            ftxui::separator(),
            ftxui::filler(),
            ftxui::hbox(
                ftxui::filler(),
                login_signup_screen->Render(),
                ftxui::filler()
            ),
            ftxui::filler()
        }) | ftxui::border;
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
