#include <iostream>
#include <sstream>
#include <memory>

#include "backend.cpp"
#include "client.cpp"
#include "cli.cpp"

std::vector<std::string> parseToLexemes(const std::string& s) {
    std::vector<std::string> result;

    char quotes = 0;
    bool lastSlash = false;
    std::ostringstream builder;

    auto finishLexeme = [&builder, &result]() mutable {
        std::string lexeme = builder.str();
        if (!lexeme.empty())
            result.push_back(std::move(lexeme));

        builder.str("");
        builder.clear();
    };

    for (char c : s) {
        if (c == '\\' && !lastSlash) {
            lastSlash = true;
            continue;
        }

        if (c == quotes && !lastSlash) {
            quotes = 0;
        } else if ((c == '\'' || c == '"') && !lastSlash) {
            quotes = c;
        } else if (c == ' ' && !quotes && !lastSlash) {
            // finish current lexeme
            finishLexeme();
        } else {
            // extend current lexeme
            builder << c;
        }

        lastSlash = false;
    }
    finishLexeme();

    return result;
}

int main() {
    std::cout << "Hello, Youtuber!" << std::endl;

    const std::shared_ptr<youtube::Backend> backend = std::make_shared<youtube::backend::BackendImpl>();
    youtube::client::StandardYoutubeClientFactory factory{backend};

    YoutubeCLI cli{std::cin, std::cout, factory.openConnection()};
    while (true) {
        std::cout << ">> ";
        std::cout.flush();

        std::string cmd;
        std::getline(std::cin, cmd);

        const std::vector<std::string> command = parseToLexemes(cmd);
        if (command.size() == 1 && command[0] == "stop")
            break;

        cli.handleNextCommand(command);
    }

    return 0;
}