#include "tags.h"
#include <algorithm>

using namespace opustags;

// ASCII only, but for tag keys it's good enough
static bool iequals(const std::string &a, const std::string &b)
{
    if (a.size() != b.size())
        return false;
    for (size_t i = 0; i < a.size(); i++)
        if (std::tolower(a[i]) != std::tolower(b[i]))
            return false;
    return true;
}

const std::vector<Tag> Tags::get_all() const
{
    return tags;
}

std::string Tags::get(const std::string &key) const
{
    for (auto &tag : tags)
        if (iequals(tag.key, key))
            return tag.value;
    throw std::runtime_error("Tag '" + key + "' not found.");
}

void Tags::add(const Tag &tag)
{
    tags.push_back(tag);
}

void Tags::add(const std::string &key, const std::string &value)
{
    tags.push_back({key, value});
}

void Tags::clear()
{
    tags.clear();
}

void Tags::remove(const std::string &key)
{
    std::vector<Tag> new_tags;
    std::copy_if(
        tags.begin(),
        tags.end(),
        std::back_inserter(new_tags),
        [&](const Tag &tag) { return !iequals(tag.key, key); });
    tags = new_tags;
}

bool Tags::contains(const std::string &key) const
{
    return std::count_if(
        tags.begin(),
        tags.end(),
        [&](const Tag &tag) { return iequals(tag.key, key); }) > 0;
}

Tag opustags::parse_tag(const std::string &assoc)
{
    size_t eq = assoc.find_first_of('=');
    if (eq == std::string::npos)
        throw std::runtime_error("misconstructed tag");
    std::string name = assoc.substr(0, eq);
    std::string value = assoc.substr(eq + 1);
    return { name, value };
}
