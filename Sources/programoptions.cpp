//
//  programoptions.cpp
//  kssutil
//
//  Created by Steven W. Klassen on 2014-05-01.
//  Copyright (c) 2014 Klassen Software Solutions. All rights reserved.
//  Licensing follows the MIT License.
//

#include <algorithm>
#include <cassert>
#include <cctype>
#include <cerrno>
#include <cstdlib>
#include <iostream>
#include <iterator>
#include <limits>
#include <memory>
#include <unordered_map>
#include <vector>

#include <getopt.h>

#include "_contract.hpp"
#include "programoptions.hpp"

using namespace std;
using namespace kss::util::po;
namespace contract = kss::util::_private::contract;


////
//// MARK: architecture specific settings
////

#if defined(__linux)
#else
# define HAS_OPTRESET 1
#endif


////
//// MARK: Impl
////


namespace {
	using results_map_t = unordered_map<string, string>;

    int toHasArg(HasArgument hasArg) {
        switch (hasArg) {
            case HasArgument::none:        return no_argument;
            case HasArgument::required:    return required_argument;
            case HasArgument::optional:    return optional_argument;
            default:
                // It should not be possible to get here. If we do we terminate.
                contract::conditions({ KSS_EXPR(false) });
                return -1;
        }
    }

	// Create a duplicate of argv that is not const.
	vector<char*> duplicateArgV(int argc, const char* const* argv) {
        contract::preconditions({
            KSS_EXPR(argc > 0),
            KSS_EXPR(argv != nullptr)
        });

		vector<char*> ret;
		ret.reserve((size_t)argc);
		for (int i = 0; i < argc; ++i) {
			ret.push_back((char*)argv[i]);
		}
		return ret;
	}

    // Returns true if the string is a valid argument (i.e. a single word).
    bool isValidArgumentName(const string& s) noexcept {
        if (s.empty()) {
            return false;
        }

        for (const auto& ch : s) {
            if (isspace(ch) || !isprint(ch)) {
                return false;
            }
        }
        return true;
    }

    // Verify the given option to see if it can be added. Throws an exception if
    // it cannot.
    void verifyOption(const vector<Option>& options, const Option& o) {
        if (!isValidArgumentName(o.name)) {
            throw invalid_argument("the name '" + o.name + "' is not a valid option name");
        }

        for (const auto& opt : options) {
            if (opt.name == o.name) {
                throw invalid_argument("already have an option named " + o.name);
            }

            if (o.shortOption == noShortOption) {
                continue;
            }

            const auto so = (o.shortOption == autoShortOption ? o.name[0] : o.shortOption);
            if (so == opt.shortOption) {
                throw invalid_argument("already have a short option of '"
                                       + to_string(o.shortOption)
                                       + "', (option named "
                                       + opt.name + ")");
            }
        }
    }
}

struct ProgramOptions::Impl {
	vector<Option>          poptions;
	results_map_t	        results;
	string			        programName;
};



////
//// MARK: ProgramOptions
////

ProgramOptions::ProgramOptions(initializer_list<Option> options) : _impl(new Impl()) {
    for (const auto& o : options) {
        add(o);
    }

    contract::postconditions({
        KSS_EXPR(bool(_impl) == true),
        KSS_EXPR(_impl->poptions.size() == options.size()),
        KSS_EXPR(_impl->results.empty()),
        KSS_EXPR(_impl->programName.empty())
    });
}

ProgramOptions::~ProgramOptions() = default;
ProgramOptions::ProgramOptions(ProgramOptions&&) = default;
ProgramOptions& ProgramOptions::operator=(ProgramOptions &&) noexcept = default;

void ProgramOptions::add(const Option &o) {
    Option o2(o);
    add(move(o2));
}

void ProgramOptions::add(Option&& o) {
    contract::preconditions({
        KSS_EXPR(bool(_impl) == true)
    });

    const auto numOptions = _impl->poptions.size();
    verifyOption(_impl->poptions, o);
    _impl->poptions.push_back(move(o));

    auto& newOpt = _impl->poptions.back();
    if (newOpt.shortOption == autoShortOption) {
        newOpt.shortOption = newOpt.name[0];
    }

    contract::postconditions({
        KSS_EXPR(_impl->poptions.size() == (numOptions+1))
    });
}

void ProgramOptions::add(initializer_list<Option> options) {
    contract::preconditions({
        KSS_EXPR(bool(_impl) == true)
    });
    contract::parameters({
        KSS_EXPR(options.size() > 0)
    });

    add(options.begin(), options.end());
}


namespace {
    void initWithDefaultValues(const vector<Option>& poptions, results_map_t& results) {
        results.clear();
        for (const auto& o : poptions) {
            if (o.hasArg == HasArgument::optional) {
                results[o.name] = o.defaultValue;
            }
        }
    }

    vector<struct option> createGetOptLongStructOptions(const vector<Option>& poptions) {
        size_t n = poptions.size();
        vector<struct option> longopts(n+1);

        for (size_t i = 0; i < n; ++i) {
            const auto& popt = poptions[i];
            struct option& opt = longopts.at(i);
            opt.name = popt.name.c_str();
            opt.has_arg = toHasArg(popt.hasArg);
            opt.flag = nullptr;
            opt.val = 0;
        }

        struct option& opt = longopts.at(n);
        opt.name = nullptr;
        opt.has_arg = 0;
        opt.flag = nullptr;
        opt.val = 0;

        return longopts;
    }

    string createGetOptLongOptString(const vector<Option>& poptions) {
        string optstring = "";
        for (const auto& popt : poptions) {
            if (isprint(popt.shortOption) && !isspace(popt.shortOption)) {
                optstring += popt.shortOption;
                if (popt.hasArg == HasArgument::required
                    || popt.hasArg == HasArgument::optional)
                {
                    optstring += ':';
                }
            }
        }
        return optstring;
    }

    // This will return nullptr if the character should be ignored and the processing
    // should continue.
    const Option* getOptionFromCharacter(int ch, int idx,
                                         const vector<Option>& poptions,
                                         bool ignoreUnknownOptions)
    {
        if (ch == '?') {
            if (ignoreUnknownOptions) {
                return nullptr;
            }
            throw invalid_argument("Unknown or ambiguous option");
        }

        if (ch == ':') {
            throw invalid_argument("Missing required argument");
        }

        const Option* po = nullptr;
        if (ch == 0) {
            // Search for the long name using idx.
            assert(poptions.size() < numeric_limits<int>::max());
            assert(idx >= 0 && idx < (int)poptions.size());
            const auto& popt = poptions[(size_t)idx];
            po = &popt;
        }
        else {
            // Search for the short name using ch.
            auto start = begin(poptions);
            const auto stop = end(poptions);
            const auto it = find_if(start, stop, [&ch](const Option& popt){
                return popt.shortOption == ch;
            });

            // If the item is not found then there is a bug in either add_option or
            // getopt_long.
            assert(it != stop);
            if (it != stop) {
                po = &(*it);
            }
        }

        return po;
    }

    string getResultFromOption(const Option& po, int argc) {
        string ret;
        switch(po.hasArg) {
            case HasArgument::none:
                break;
            case HasArgument::required:
                // On the mac I occasionally get the case (for missing arguments) that
                // optarg is pointing to a bad location. This always seems to be
                // accompanied by optind growing larger than the number of arguments.
                // So we check for that case explicitly to avoid the bad memory access.
                if (optind > argc || optarg == nullptr || optarg[0] == 0) {
                    throw invalid_argument("Missing requirement argument for " + po.name + ".");
                }
                ret = string(optarg);
                break;
            case HasArgument::optional:
                ret = string(optarg == nullptr ? "" : optarg);
                break;
            default:
                // It should not be possible to get here.
                contract::conditions({
                    KSS_EXPR(false)
                });
        }
        return ret;
    }
}

void ProgramOptions::parse(int argc, const char *const *argv, bool ignoreUnknownOptions) {
    contract::preconditions({
        KSS_EXPR(bool(_impl) == true)
    });
    contract::parameters({
        KSS_EXPR(argc > 0),
        KSS_EXPR(argv != nullptr)
    });

    // Setup for the getopt_long_only calls.
    initWithDefaultValues(_impl->poptions, _impl->results);
    auto newargv = duplicateArgV(argc, argv);
    const auto longopts = createGetOptLongStructOptions(_impl->poptions);
    const auto optstring = createGetOptLongOptString(_impl->poptions);

    // Now parse the command line.
    int ch = 0;
    int idx = -1;
    _impl->programName = string(argv[0]);
    opterr = 0;
    optind = 1;
#if defined(HAS_OPTRESET)
    optreset = 1;
#endif
    while ((ch = getopt_long_only(argc,
                                  newargv.data(),
                                  optstring.c_str(),
                                  longopts.data(),
                                  &idx)) != -1)
    {
        const Option* po = getOptionFromCharacter(ch, idx, _impl->poptions,
                                                  ignoreUnknownOptions);
        assert(po != nullptr || ignoreUnknownOptions == true);
        if (po) {
            _impl->results[po->name] = getResultFromOption(*po, argc);
        }
    }
}

string ProgramOptions::usage() const {
    contract::preconditions({
        KSS_EXPR(bool(_impl) == true)
    });

    if (_impl->programName.empty()) {
		return "";
	}

	stringstream s;
    s << "usage: " << _impl->programName << " <options>" << endl;
	s << "  where options are:" << endl;

    for (const auto& popt : _impl->poptions) {
		s << "    ";
		s << "--" << popt.name;

		if (popt.hasArg == HasArgument::required) {
			s << "=<value>";
		}
		else if (popt.hasArg == HasArgument::optional) {
			s << "[=<value>]";
		}
        else {
            // no value to add to s
        }

		if (popt.shortOption) {
			s << " (or -" << popt.shortOption;

			if (popt.hasArg == HasArgument::required) {
				s << " <value>";
			}
			else if (popt.hasArg == HasArgument::optional) {
				s << " [<value>]";
			}
            else {
                // no value to add to s
            }
            
			s << ")";
		}

		if (popt.hasArg == HasArgument::optional) {
			s << ", default=" << popt.defaultValue;
		}

		if (!popt.description.empty()) {
			s << ": " << popt.description;
		}

		s << endl;
	}

	return s.str();
}


bool ProgramOptions::hasOption(const string& name) const {
    contract::preconditions({
        KSS_EXPR(bool(_impl) == true)
    });
    contract::parameters({
        KSS_EXPR(!name.empty())
    });

	const auto it = _impl->results.find(name);
	return (it != _impl->results.end());
}

string ProgramOptions::rawOptionValue(const string& name) const {
    contract::preconditions({
        KSS_EXPR(bool(_impl) == true)
    });
    contract::parameters({
        KSS_EXPR(!name.empty())
    });
    
	const auto it = _impl->results.find(name);
	if (it != _impl->results.end()) {
		return it->second;
	}
    throw invalid_argument("Could not find the option '" + name + "'");
}
