#include "tags.h"
#include <algorithm>

using namespace opustags;

const std::vector<Tag> Tags::get_all() const
{
    return tags;
}

std::string Tags::get(const std::string &key) const
{
    for (auto &tag : tags)
        if (tag.key == key)
            return tag.value;
    throw std::runtime_error("Tag '" + key + "' not found.");
}

void Tags::add(const std::string &key, const std::string &value)
{
    tags.push_back({key, value});
}

void Tags::clear()
{
    tags.clear();
}

void Tags::add(const std::string &assoc)
{
    size_t eq = assoc.find_first_of('=');
    if (eq == std::string::npos)
        throw std::runtime_error("misconstructed tag");
    std::string name = assoc.substr(0, eq);
    std::string value = assoc.substr(eq + 1);
    add(name, value);
}

void Tags::remove(const std::string &key)
{
    std::vector<Tag> new_tags;
    std::copy_if(
        tags.begin(),
        tags.end(),
        std::back_inserter(new_tags),
        [&](const Tag &tag) { return tag.key != key; });
    tags = new_tags;
}

bool Tags::contains(const std::string &key) const
{
    return std::count_if(
        tags.begin(),
        tags.end(),
        [&](const Tag &tag) { return tag.key == key; }) > 0;
}
