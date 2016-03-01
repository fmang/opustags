#include "tags.h"
#include <algorithm>

using namespace opustags;

Tags::Tags() : max_index(0)
{
}

const std::vector<std::pair<std::string, std::string>> Tags::get_all() const
{
    std::vector<std::string> keys;
    for (const auto &kv : key_to_value)
        keys.push_back(kv.first);

    std::sort(
        keys.begin(),
        keys.end(),
        [&](const std::string &a, const std::string &b)
        {
            return key_to_index.at(a) < key_to_index.at(b);
        });

    std::vector<std::pair<std::string, std::string>> result;
    for (const auto &key : keys) {
        result.push_back(
            std::make_pair(
                key, key_to_value.at(key)));
    }

    return result;
}

std::string Tags::get(const std::string &key) const
{
    return key_to_value.at(key);
}

void Tags::set(const std::string &key, const std::string &value)
{
    key_to_value[key] = value;
    key_to_index[key] = max_index;
    max_index++;
}

void Tags::set(const std::string &assoc)
{
    size_t eq = assoc.find_first_of('=');
    if (eq == std::string::npos)
        throw std::runtime_error("misconstructed tag");
    std::string name = assoc.substr(0, eq);
    std::string value = assoc.substr(eq + 1);
    set(name, value);
}

void Tags::remove(const std::string &key)
{
    key_to_value.erase(key);
    key_to_index.erase(key);
}

bool Tags::contains(const std::string &key) const
{
    return key_to_value.find(key) != key_to_value.end();
}
