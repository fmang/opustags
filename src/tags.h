#pragma once

#include <map>
#include <vector>
#include <tuple>

namespace opustags {

    // A std::map adapter that keeps the order of insertion.
    class Tags final
    {
    public:
        Tags();

        const std::vector<std::tuple<std::string, std::string>> get_all() const;

        std::string get(const std::string &key) const;
        void set(const std::string &key, const std::string &value);
        void remove(const std::string &key);
        bool contains(const std::string &key) const;

    private:
        std::map<std::string, std::string> key_to_value;
        std::map<std::string, size_t> key_to_index;
        size_t max_index;
    };

}
