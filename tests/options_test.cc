#include "options.h"
#include <memory>
#include "catch.h"
#include "tags_handlers/modification_tags_handler.h"
#include "tags_handlers/insertion_tags_handler.h"
#include "tags_handlers/removal_tags_handler.h"

using namespace opustags;

static std::unique_ptr<char[]> string_to_uptr(const std::string &str)
{
    auto ret = std::make_unique<char[]>(str.size() + 1);
    for (size_t i = 0; i < str.size(); i++)
        ret[i] = str[i];
    ret[str.size()] = 0;
    return ret;
}

static Options retrieve_options(
    std::vector<std::string> args, bool fake_input_path = true)
{
    // need to pass non-const char*, but we got const objects. make copies
    std::vector<std::unique_ptr<char[]>> arg_holders;
    arg_holders.push_back(string_to_uptr("fake/path/to/program"));
    for (size_t i = 0; i < args.size(); i++)
        arg_holders.push_back(string_to_uptr(args[i]));
    if (fake_input_path)
        arg_holders.push_back(string_to_uptr("fake/path/to/input"));

    auto plain_args = std::make_unique<char*[]>(arg_holders.size());
    for (size_t i = 0; i < arg_holders.size(); i++)
        plain_args[i] = arg_holders[i].get();

    return parse_args(arg_holders.size(), plain_args.get());
}

// retrieve a specific TagsHandler from the CompositeTagsHandler in options
template<typename T> static T *get_handler(
    const Options &options, const size_t index)
{
    const auto handlers = options.tags_handler.get_handlers();
    const auto handler = dynamic_cast<T*>(handlers.at(index).get());
    REQUIRE(handler);
    return handler;
}

TEST_CASE("option parsing", "[options]")
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

    SECTION("--in-place") {
        REQUIRE(!retrieve_options({}).in_place);
        REQUIRE(retrieve_options({}).path_out.empty());
        REQUIRE(retrieve_options({"--in-place", "ABC"}, false).in_place);
        REQUIRE(
            retrieve_options({"--in-place", "ABC"}, false).path_out == ".otmp");
        REQUIRE(retrieve_options({"--in-place"}).in_place);
        REQUIRE(retrieve_options({"--in-place"}).path_out == ".otmp");
        REQUIRE(retrieve_options({"--in-place=ABC"}).in_place);
        REQUIRE(retrieve_options({"--in-place=ABC"}).path_out == "ABC");
        REQUIRE(retrieve_options({"-i", "ABC"}, false).in_place);
        REQUIRE(retrieve_options({"-i", "ABC"}, false).path_out == ".otmp");
        REQUIRE(retrieve_options({"-i"}).in_place);
        REQUIRE(retrieve_options({"-i"}).path_out == ".otmp");
        REQUIRE(retrieve_options({"-iABC"}).in_place);
        REQUIRE(retrieve_options({"-iABC"}).path_out == "ABC");
    }

    SECTION("input") {
        REQUIRE_THROWS(retrieve_options({}, false));
        REQUIRE_THROWS(retrieve_options({""}, false));
        REQUIRE(retrieve_options({"input"}, false).path_in == "input");
        REQUIRE_THROWS(retrieve_options({"input", "extra argument"}, false));
    }

    SECTION("--output") {
        REQUIRE(retrieve_options({"--output", "ABC"}).path_out == "ABC");
        REQUIRE(retrieve_options({"-o", "ABC"}).path_out == "ABC");
        REQUIRE_THROWS(retrieve_options({"--output", ""}));
        REQUIRE_THROWS(retrieve_options({"-o", ""}));
    }

    SECTION("--delete-all") {
        REQUIRE(get_handler<RemovalTagsHandler>(
            retrieve_options({"--delete-all"}), 0)->get_tag_key().empty());

        REQUIRE(get_handler<RemovalTagsHandler>(
            retrieve_options({"-D"}), 0)->get_tag_key().empty());
    }

    SECTION("--delete") {
        const auto opts = retrieve_options({"--delete", "A", "-d", "B"});
        REQUIRE(get_handler<RemovalTagsHandler>(opts, 0)->get_tag_key() == "A");
        REQUIRE(get_handler<RemovalTagsHandler>(opts, 1)->get_tag_key() == "B");
        REQUIRE_THROWS(retrieve_options({"--delete", "invalid="}));
    }

    SECTION("--add") {
        const auto opts = retrieve_options({"--add", "A=1", "-a", "B=2"});
        const auto handler1 = get_handler<InsertionTagsHandler>(opts, 0);
        const auto handler2 = get_handler<InsertionTagsHandler>(opts, 1);
        REQUIRE(handler1->get_tag_key() == "A");
        REQUIRE(handler1->get_tag_value() == "1");
        REQUIRE(handler2->get_tag_key() == "B");
        REQUIRE(handler2->get_tag_value() == "2");
        REQUIRE_THROWS(retrieve_options({"--add", "invalid"}));
    }

    SECTION("--set") {
        const auto opts = retrieve_options({"--set", "A=1", "-s", "B=2"});
        const auto handler1 = get_handler<ModificationTagsHandler>(opts, 0);
        const auto handler2 = get_handler<ModificationTagsHandler>(opts, 1);
        REQUIRE(handler1->get_tag_key() == "A");
        REQUIRE(handler1->get_tag_value() == "1");
        REQUIRE(handler2->get_tag_key() == "B");
        REQUIRE(handler2->get_tag_value() == "2");
        REQUIRE_THROWS(retrieve_options({"--set", "invalid"}));
    }

    SECTION("--stream") {
        // by default, use all the streams
        REQUIRE(
            get_handler<RemovalTagsHandler>(retrieve_options({"-d", "xyz"}), 0)
            ->get_streamno() == StreamTagsHandler::ALL_STREAMS);

        // ...unless the user supplies an option to use a specific stream
        REQUIRE(
            get_handler<RemovalTagsHandler>(
                retrieve_options({"--stream", "1", "-d", "xyz"}), 0)
            ->get_streamno() == 1);

        // ...which can be combined multiple times
        {
            const auto opts = retrieve_options({
                "--stream", "1",
                "-d", "xyz",
                "--stream", "2",
                "-d", "abc"});
            const auto handler1 = get_handler<RemovalTagsHandler>(opts, 0);
            const auto handler2 = get_handler<RemovalTagsHandler>(opts, 1);
            REQUIRE(handler1->get_streamno() == 1);
            REQUIRE(handler2->get_streamno() == 2);
        }
    }
}
