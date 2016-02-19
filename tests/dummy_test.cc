#include "dummy.h"
#include "catch.h"

TEST_CASE("A dummy test")
{
    REQUIRE(opustags::return_one() == 1);
}
