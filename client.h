#pragma once

#include <utility>

#include "common-data.h"

namespace youtube {
    namespace client {
        class YoutubeClient {
        private:
            const std::shared_ptr<Backend> backend;
            std::string authToken;

        public:
            explicit YoutubeClient(std::shared_ptr<Backend> backend)
                    : backend(std::move(backend)) {}

            void auth(const std::string &name, const std::string &password) {
                authToken = backend->auth(name, password);
            }

            void registerUser(const std::string &name, const std::string &password) {
                backend->registerUser(name, password);
            }

            const std::vector<std::shared_ptr<Video>> searchVideos(const std::vector<std::string> &request) {
                return backend->searchVideos(request);
            }

            const std::shared_ptr<Video> getVideo(const std::string &id) {
                return backend->getVideo(id);
            }

            void uploadVideo(const std::string &name, const std::string &content) {
                backend->addVideo(authToken, name, content);
            }

            const std::string downloadVideo(const std::string &id) {
                return backend->downloadVideo(id);
            }

            void leaveComment(const std::string &videoId, const std::string &comment) {
                backend->leaveComment(authToken, videoId, comment);
            }
        };

        class YoutubeClientFactory {
        public:
            virtual YoutubeClient openConnection() = 0;
        };

        class StandardYoutubeClientFactory : public YoutubeClientFactory {
        private:
            const std::shared_ptr<Backend> backend;

        public:
            explicit StandardYoutubeClientFactory(std::shared_ptr<Backend> backend)
                    : backend(std::move(backend)) {}

            YoutubeClient openConnection() override {
                return YoutubeClient(backend);
            }
        };
    }
}
