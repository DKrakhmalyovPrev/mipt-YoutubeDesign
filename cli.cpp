#pragma once

#include <ostream>
#include <algorithm>
#include <functional>

#include "client.cpp"

class YoutubeCLI {
private:
    using CLIAcceptor = std::function<bool(const std::vector<std::string> &)>;
    using CLICommand = const std::vector<std::string>;

    std::istream &input;
    std::ostream &output;
    std::vector<CLIAcceptor> processingChain;
    std::ostringstream helpString;

    youtube::client::YoutubeClient client;

public:
    YoutubeCLI(std::istream &input, std::ostream &output, youtube::client::YoutubeClient&& client)
            : input(input), output(output), client(std::move(client)) {
        initProcessing();
    }

    void handleNextCommand(CLICommand &command) {
        try {
            for (CLIAcceptor &acceptor : processingChain)
                if (acceptor(command))
                    break;
        } catch (const std::exception &exception) {
            output << exception.what() << std::endl;
        }
    }

    void initProcessing() {
        acceptWithHelp("register", 2, [this](CLICommand &cmd) {
            client.registerUser(cmd[1], cmd[2]);
            output << "Success" << std::endl;
            return true;
        }, "username password - register new user");

        acceptWithHelp("auth", 2, [this](CLICommand &cmd) {
            client.auth(cmd[1], cmd[2]);
            output << "Success" << std::endl;
            return true;
        }, "username password - authorize");

        acceptWithHelp("upload-video", 1, [this](CLICommand &cmd) {
            output << "Type your video here :)" << std::endl;
            std::string video;
            std::getline(input, video);
            client.uploadVideo(cmd[1], video);
            return true;
        }, "title - upload new video");

        acceptWithHelp("search-video", 1, 1000, [this](CLICommand &cmd) {
            std::vector<std::string> searchLine = std::vector<std::string>(cmd.begin() + 1, cmd.end());
            std::vector<std::shared_ptr<youtube::Video>> result = client.searchVideos(searchLine);
            for (const auto video : result) {
                output << video->id << ' ' << video->title << '\n';
            }
            output.flush();
            return true;
        }, "title - upload new video");

        acceptWithHelp("help", 0, [this](CLICommand &cmd) {
            output << helpString.str();
            output.flush();
            return true;
        }, "- prints help");

        processingChain.emplace_back([this](CLICommand &cmd) {
            output << "Wrong command! Print 'help' for help." << std::endl;
            return true;
        });
    }


    void acceptWithHelp(std::string &&firstLexeme, int numArgs, CLIAcceptor &&executor, const std::string &help) {
        helpString << firstLexeme << ' ' << help << '\n';
        accept(std::move(firstLexeme), numArgs, std::move(executor));
    }

    void acceptWithHelp(std::string &&firstLexeme, int minArgs, int maxArgs,
            CLIAcceptor &&executor, const std::string &help) {
        helpString << firstLexeme << ' ' << help << '\n';
        accept(std::move(firstLexeme), minArgs, maxArgs, std::move(executor));
    }

    void accept(std::string &&firstLexeme, int numArgs, CLIAcceptor &&executor) {
        accept(std::move(firstLexeme), numArgs, numArgs, std::move(executor));
    }

    void accept(std::string &&firstLexeme, int minArgs, int maxArgs, CLIAcceptor &&executor) {
        std::transform(firstLexeme.begin(), firstLexeme.end(), firstLexeme.begin(), ::tolower);
        processingChain.emplace_back(
                [minArgs, maxArgs, firstLexeme, executor = std::move(executor)](CLICommand &cmd) {
                    if (cmd.size() - 1 < minArgs || cmd.size() - 1 > maxArgs)
                        return false;

                    std::string first = cmd[0];
                    std::transform(first.begin(), first.end(), first.begin(), ::tolower);

                    if (first != firstLexeme)
                        return false;

                    return executor(cmd);
                }
        );
    }
};