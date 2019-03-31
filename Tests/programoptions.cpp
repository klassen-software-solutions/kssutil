//
//  programoptions.cpp
//  kssutil
//
//  Created by Steven W. Klassen on 2014-05-02.
//  Copyright (c) 2014 Klassen Software Solutions. All rights reserved.
//  Licensing follows the MIT License.
//

#include <cerrno>
#include <cstdio>
#include <cstring>
#include <ios>
#include <iostream>
#include <vector>

#include <kss/util/programoptions.hpp>

#include "ksstest.hpp"

using namespace std;
using namespace kss::util::po;
using namespace kss::test;

static char* invalid_command_line[] = { };
static const char* empty_command_line[] = { "/bin/someprog" };
static const char* const shortargs_command_line[] = { "/bin/someprog", "-h", "-q" };
static const char* const longargs_command_line[] = { "/bin/someprog", "--help", "--quiet" };
static const char* const combined_command_line[] = { "/bin/someprog", "-h", "--quiet" };
static const char* const complex_command_line[] = { "/bin/someprog", "-h", "--filename=/etc/somefile", "-c", "5", "extra", "argument" };
static const char* const incomplete_command_line[] = { "/bin/someprog", "-c", "5", "--filename" };
static const char* const optarg_command_line[] = { "/bin/someprog", "--filename=hi" };

namespace {
	void add_simple_options(ProgramOptions& po) {
        po.add(Option { "help", "Display a usage message" });
        po.add(Option { "quiet", "Show less information" });
	}

	void add_complex_options(ProgramOptions& po) {
        po.add(Option { "filename", "Input filename", noShortOption, HasArgument::required });
        po.add(Option { "Count", "Count", 'c', HasArgument::optional, "10" });
	}

    void testConstSettings(const ProgramOptions& po) {
        const auto s = po.option<string>("filename");
    }
}

#define SIZEOF(ar) sizeof(ar)/sizeof(*ar)


static void perform_simple_checks(int argc, const char* const* argv) {
    ProgramOptions opts {
        { "badValue", "", noShortOption, HasArgument::optional, "not an integer" }
    };
	add_simple_options(opts);
    opts.parse(argc, (char* const *)argv);
    KSS_ASSERT(opts.hasOption("help"));
    KSS_ASSERT(opts.hasOption("quiet"));
    KSS_ASSERT(!opts.hasOption("hi"));
    KSS_ASSERT(throwsException<invalid_argument>([&] {
        opts.option<int>("help");
    }));
    KSS_ASSERT(throwsException<system_error>([&] {
        opts.option<int>("badValue");
    }));
}


static TestSuite potests("po::programoptions", {
    make_pair("basic tests", [] {
        KSS_ASSERT(isTrue([] {
            ProgramOptions opts;
            return opts.usage().empty();
        }));

        KSS_ASSERT(throwsException<invalid_argument>([] {
            ProgramOptions opts;
            opts.parse(SIZEOF(invalid_command_line), invalid_command_line);
        }));

        KSS_ASSERT(isFalse([] {
            ProgramOptions opts;
            add_simple_options(opts);
            opts.parse(SIZEOF(empty_command_line), empty_command_line);
            return opts.usage().empty();
        }));

        // Various versions of the simple check.
        perform_simple_checks(SIZEOF(shortargs_command_line), shortargs_command_line);
        perform_simple_checks(SIZEOF(longargs_command_line), longargs_command_line);
        perform_simple_checks(SIZEOF(combined_command_line), combined_command_line);

        // An invalid line test.
        KSS_ASSERT(throwsException<invalid_argument>([] {
            ProgramOptions opts;
            add_complex_options(opts);
            opts.parse(SIZEOF(incomplete_command_line), incomplete_command_line);
        }));
    }),
    make_pair("more complex test", [] {
        ProgramOptions opts;
        add_simple_options(opts);
        add_complex_options(opts);
        opts.parse(SIZEOF(complex_command_line), complex_command_line);
        KSS_ASSERT(opts.hasOption("Count"));
        KSS_ASSERT(!opts.hasOption("extra"));
        KSS_ASSERT(opts.option<string>("filename") == "/etc/somefile");
        KSS_ASSERT(opts.option<int>("Count") == 5);
    }),
    make_pair("default value", [] {
        ProgramOptions opts;
        add_complex_options(opts);
        opts.parse(SIZEOF(optarg_command_line), optarg_command_line);
        KSS_ASSERT(opts.hasOption("Count"));
        KSS_ASSERT(opts.option<long>("Count") == 10L);
    }),
    make_pair("missing required argument", [] {
        ProgramOptions opts;
        add_complex_options(opts);
        opts.parse(SIZEOF(empty_command_line), empty_command_line);
        KSS_ASSERT(!opts.hasOption("filename"));
        KSS_ASSERT(throwsException<invalid_argument>([&] {
            opts.option<string>("filename");
        }));
    }),
    make_pair("error p45 - detect boolean options", [] {
        ProgramOptions opts;
        add_simple_options(opts);
        add_complex_options(opts);
        opts.parse(SIZEOF(empty_command_line), empty_command_line);
        KSS_ASSERT(!opts.hasOption("help"));
        KSS_ASSERT(!opts.hasOption("filename"));
    }),
    make_pair("ignoring unknown options", [] {
        ProgramOptions po {
            { "help", "Display a usage message" }
        };
        KSS_ASSERT(throwsException<invalid_argument>([&] {
            po.parse(SIZEOF(shortargs_command_line), shortargs_command_line);
        }));
        KSS_ASSERT(doesNotThrowException([&] {
            po.parse(SIZEOF(shortargs_command_line), shortargs_command_line, true);
        }));
    }),
    make_pair("bug 48 - option should be const", [] {
        ProgramOptions opts;
        add_simple_options(opts);
        add_complex_options(opts);
        opts.parse(SIZEOF(complex_command_line), complex_command_line);
        testConstSettings(opts);
        // There is nothing to check at runtime, this is a compiler test.
        KSS_ASSERT(true);
    }),
    make_pair("invalid adds", [] {
        ProgramOptions opts {{ "arg1" }};
        KSS_ASSERT(throwsException<invalid_argument>([&] {
            opts.add(Option { "" });            // empty args are not allowed
        }));
        KSS_ASSERT(throwsException<invalid_argument>([&] {
            opts.add(Option { "hello world" }); // args must be a single word
        }));
        KSS_ASSERT(throwsException<invalid_argument>([&] {
            opts.add(Option { "hi\002" });      // args must be printable characters
        }));
        KSS_ASSERT(throwsException<invalid_argument>([&] {
            opts.add(Option { "arg1" });        // arg1 already exists
        }));
        KSS_ASSERT(throwsException<invalid_argument>([&] {
            opts.add(Option { "anotherArg", }); // short arg 'a' already exists
        }));
        KSS_ASSERT(doesNotThrowException([&] {
            opts.add(Option { "Arg2" });        // args and short args are case sensitive
        }));
    }),
    make_pair("add", [] {
        Option o { "two" };
        vector<Option> v {
            { "five", "", noShortOption },
            { "six" },
            { "seven", "", noShortOption }
        };

        ProgramOptions opts;
        KSS_ASSERT(doesNotThrowException([&] {
            opts.add({ "one" });
            opts.add(o);
            opts.add({
                { "three", "", noShortOption },
                { "four" }
            });
            opts.add(v.begin(), v.end());
        }));

        KSS_ASSERT(throwsException<invalid_argument>([&] {
            opts.add({ });
        }));
        KSS_ASSERT(throwsException<invalid_argument>([&] {
            vector<Option> vv;
            opts.add(vv.begin(), vv.end());
        }));
    })
});
