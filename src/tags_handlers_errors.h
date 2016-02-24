#pragma once

#include <stdexcept>

namespace opustags {

    struct TagAlreadyExistsError : std::runtime_error
    {
        TagAlreadyExistsError(const std::string &tag_key);
    };

}
