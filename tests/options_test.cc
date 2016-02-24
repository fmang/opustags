#include "options.h"
#include <memory>
#include "catch.h"

static std::unique_ptr<char[]> string_to_uptr(const std::string &str)
{
    auto ret = std::make_unique<char[]>(str.size() + 1);
    for (size_t i = 0; i < str.size(); i++)
        ret[i] = str[i];
    ret[str.size()] = 0;
    return ret;
}

static opustags::Options retrieve_options(std::vector<std::string> args)
{
    // need to pass non-const char*, but we got const objects. make copies
    std::vector<std::unique_ptr<char[]>> arg_holders;
    arg_holders.push_back(string_to_uptr("fake/path/to/program"));
    for (size_t i = 0; i < args.size(); i++)
        arg_holders.push_back(string_to_uptr(args[i]));

    auto plain_args = std::make_unique<char*[]>(arg_holders.size());
    for (size_t i = 0; i < arg_holders.size(); i++)
        plain_args[i] = arg_holders[i].get();

    return opustags::parse_args(arg_holders.size(), plain_args.get());
}

TEST_CASE("Options parsing test")
{
    SECTION("--help") {
        REQUIRE(retrieve_options({"--help"}).show_help);
        REQUIRE(retrieve_options({"--h"}).show_help);
        REQUIRE(!retrieve_options({}).show_help);
    }

    SECTION("--overwrite") {
        REQUIRE(retrieve_options({"--overwrite"}).overwrite);
        REQUIRE(retrieve_options({"-y"}).overwrite);
        REQUIRE(!retrieve_options({}).overwrite);
    }

    SECTION("--set-all") {
        REQUIRE(retrieve_options({"--set-all"}).set_all);
        REQUIRE(retrieve_options({"-S"}).set_all);
        REQUIRE(!retrieve_options({}).set_all);
    }

    SECTION("--delete-all") {
        REQUIRE(retrieve_options({"--delete-all"}).delete_all);
        REQUIRE(retrieve_options({"-D"}).delete_all);
        REQUIRE(!retrieve_options({}).delete_all);
    }

    SECTION("--in-place") {
        REQUIRE(retrieve_options({"-i"}).in_place == ".otmp");
        REQUIRE(retrieve_options({"--in-place"}).in_place == ".otmp");
        REQUIRE(retrieve_options({"--in-place=ABC"}).in_place == "ABC");
        REQUIRE(retrieve_options({"-iABC"}).in_place == "ABC");
        REQUIRE(retrieve_options({"--in-place", "ABC"}).in_place == ".otmp");
        REQUIRE(retrieve_options({"-i", "ABC"}).in_place == ".otmp");
        REQUIRE(retrieve_options({}).in_place.empty());
    }

    SECTION("--output") {
        REQUIRE(retrieve_options({"--output", "ABC"}).path_out == "ABC");
        REQUIRE(retrieve_options({"-o", "ABC"}).path_out == "ABC");
        REQUIRE_THROWS(retrieve_options({"--delete", "invalid="}));
    }

    SECTION("--delete") {
        REQUIRE(
            retrieve_options({"--delete", "ABC"}).to_delete
                == std::vector<std::string>{"ABC"});
        REQUIRE(
            retrieve_options({"-d", "ABC"}).to_delete
                == std::vector<std::string>{"ABC"});
        REQUIRE(
            retrieve_options({"-d", "ABC", "-d", "XYZ"}).to_delete
                == (std::vector<std::string>{"ABC", "XYZ"}));
        REQUIRE_THROWS(retrieve_options({"--delete", "invalid="}));
    }

    SECTION("--add") {
        const auto args1 = retrieve_options({"--add", "ABC=XYZ"});
        const auto args2 = retrieve_options({"-a", "ABC=XYZ"});
        REQUIRE(args1.to_add == args2.to_add);
        REQUIRE(args1.to_add
            == (std::map<std::string, std::string>{{"ABC", "XYZ"}}));
        REQUIRE(args1.to_delete.empty());
        REQUIRE(args2.to_delete.empty());

        const auto args3 = retrieve_options({"-a", "ABC=XYZ", "-a", "1=2"});
        REQUIRE(args3.to_add == (std::map<std::string, std::string>{
            {"ABC", "XYZ"},
            {"1", "2"}}));
        REQUIRE(args3.to_delete.empty());

        REQUIRE_THROWS(retrieve_options({"--add", "invalid"}));
    }

    SECTION("--set") {
        const auto args1 = retrieve_options({"--set", "ABC=XYZ"});
        const auto args2 = retrieve_options({"-s", "ABC=XYZ"});
        REQUIRE(args1.to_add == args2.to_add);
        REQUIRE(args1.to_add
            == (std::map<std::string, std::string>{{"ABC", "XYZ"}}));
        REQUIRE(args1.to_delete == args2.to_delete);
        REQUIRE(args1.to_delete == std::vector<std::string>{"ABC"});

        const auto args3 = retrieve_options({"-s", "ABC=XYZ", "-s", "1=2"});
        REQUIRE(args3.to_add == (std::map<std::string, std::string>{
            {"ABC", "XYZ"},
            {"1", "2"}}));
        REQUIRE(args3.to_delete == (std::vector<std::string>{"ABC", "1"}));

        REQUIRE_THROWS(retrieve_options({"--set", "invalid"}));
    }
}
