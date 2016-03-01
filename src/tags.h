#pragma once

#include <map>
#include <vector>
#include <utility>

namespace opustags {

    // A std::map adapter that keeps the order of insertion.
    class Tags final
    {
    public:
        Tags();

        const std::vector<std::pair<std::string, std::string>> get_all() const;

        std::string get(const std::string &key) const;
        void set(const std::string &key, const std::string &value);
        void set(const std::string &assoc); // KEY=value
        void remove(const std::string &key);
        bool contains(const std::string &key) const;

        // Additional fields are required to match the specs:
        // https://tools.ietf.org/html/draft-ietf-codec-oggopus-14#section-5.2

        std::string vendor;
        std::string extra;

    private:
        std::map<std::string, std::string> key_to_value;
        std::map<std::string, size_t> key_to_index;
        size_t max_index;
    };

}
