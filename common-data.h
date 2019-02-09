#pragma once

#include <utility>
#include <vector>
#include <map>
#include <unordered_set>
#include <memory>


namespace youtube {
    namespace backend {
        class User;
    }

    class Video;

    class Comment;


    class Comment {
    private:
    public:
        const std::string userName;
        const std::string content;

        explicit Comment(std::string userName, std::string content)
                : userName(std::move(userName)), content(std::move(content)) {}
    };

    class Video {
    protected:
        std::vector<std::shared_ptr<Comment>> comments;

    public:
        const std::string id;
        const std::string title;

        explicit Video(std::string id, std::string title)
                : id(std::move(id)), title(std::move(title)) {}

        const std::vector<std::shared_ptr<Comment>> &getComments() {
            return comments;
        }

        virtual ~Video() = default;
    };


    class Backend {
    public:
        virtual const std::string auth(const std::string &name, const std::string &password) = 0;

        virtual const std::string downloadVideo(const std::string &id) = 0;

        virtual void registerUser(const std::string &name, const std::string &password) = 0;

        virtual const std::vector<std::shared_ptr<Video>> searchVideos(const std::vector<std::string> &request) = 0;

        virtual void addVideo(const std::string &authToken,
                              const std::string &name, const std::string &content) = 0;

        virtual const std::shared_ptr<Video> getVideo(const std::string &id) = 0;

        virtual void leaveComment(const std::string &authToken,
                                  const std::string &videoId, const std::string &comment) = 0;
    };
}
