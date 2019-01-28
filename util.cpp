#include <random>

class RandomSequenceGenerator {
private:
    RandomSequenceGenerator() = default;

    std::uniform_int_distribution<> distribution{'a', 'z'};
    std::mt19937 gen{std::random_device{}()};

public:
    static RandomSequenceGenerator& instance() {
        static RandomSequenceGenerator generator;
        return generator;
    }

    const std::string nextRandomString(const size_t length) {
        std::string result(length, '\u0000');
        for (size_t i = 0; i < length; ++i) {
            result[i] = static_cast<char>(distribution(gen));
        }
        return result;
    }
};