#pragma once

#include <map>
#include <vector>
#include <utility>

namespace opustags {

    struct Tag final
    {
        std::string key;
        std::string value;
    };

    // A std::map adapter that keeps the order of insertion.
    class Tags final
    {
    public:
        Tags();
        Tags(const std::vector<Tag> &tags);

        const std::vector<Tag> get_all() const;

        std::string get(const std::string &key) const;
        void add(const Tag &tag);
        void add(const std::string &key, const std::string &value);
        void remove(const std::string &key);
        bool contains(const std::string &key) const;
        void clear();

        // Additional fields are required to match the specs:
        // https://tools.ietf.org/html/draft-ietf-codec-oggopus-14#section-5.2

        std::string vendor;
        std::string extra;

    private:
        std::vector<Tag> tags;
    };

    Tag parse_tag(const std::string &assoc); // KEY=value

}
