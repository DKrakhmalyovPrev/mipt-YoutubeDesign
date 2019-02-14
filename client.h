#pragma once

#include <utility>

#include "common-data.h"

namespace youtube {
    namespace client {
        class YoutubeClient {
        private:
            const std::shared_ptr<Backend> backend;
            std::vector<std::shared_ptr<Notification>> receivedNotifications;
            std::shared_ptr<ClientCallback> callback;
            std::string authToken;

        public:
            explicit YoutubeClient(std::shared_ptr<Backend> backend)
                    : backend(std::move(backend)) {
            }

            void auth(const std::string &name, const std::string &password) {
                authToken = backend->auth(name, password);
                backend->setClientCallback(authToken, callback = std::make_shared<ClientCallback>(
                        [this](const std::shared_ptr<Notification> notification) {
                            receivedNotifications.push_back(notification);
                        }
                ));
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

            void leaveComment(const std::string &videoId, const std::string &comment, const size_t replyToIndex) {
                backend->leaveComment(authToken, videoId, comment, replyToIndex);
            }

            void likeVideo(const std::string &videoId) {
                backend->leaveLike(authToken, videoId);
            }

            void likeComment(const std::string &videoId, size_t commentId) {
                backend->leaveLike(authToken, videoId, commentId);
            }

            void subscribeFor(const std::string& userName) {
                backend->subscribeFor(authToken, userName);
            }

            const std::vector<std::shared_ptr<Notification>> getAndReleaseNotifications() {
                backend->releasePendingNotifications(authToken);

                std::vector<std::shared_ptr<Notification>> result;
                result.swap(receivedNotifications);
                return result;
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
