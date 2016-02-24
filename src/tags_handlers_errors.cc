#include "tags_handlers_errors.h"

using namespace opustags;

TagAlreadyExistsError::TagAlreadyExistsError(const std::string &tag_key)
    : std::runtime_error("Tag already exists: " + tag_key)
{
}

TagDoesNotExistError::TagDoesNotExistError(const std::string &tag_key)
    : std::runtime_error("Tag does not exist: " + tag_key)
{
}
