//
//  ksstest.cpp
//  ksstest
//
//  Created by Steven W. Klassen on 2018-04-03.
//  Copyright © 2018 Klassen Software Solutions. All rights reserved.
//  Licensing follows the MIT License.
//

#include <algorithm>
#include <cassert>
#include <cstdlib>
#include <ctime>
#include <exception>
#include <fstream>
#include <future>
#include <iomanip>
#include <iostream>
#include <map>
#include <mutex>
#include <ostream>
#include <set>
#include <sstream>
#include <string>
#include <vector>

#include <cxxabi.h>
#include <getopt.h>
#include <signal.h>
#include <unistd.h>
#include <sys/wait.h>

#include "ksstest.hpp"

using namespace std;
using namespace std::chrono;
using namespace kss::test;


// MARK: Configuration constants.

namespace {
    constexpr size_t maxFailureReportLineLength = 100;
}

// MARK: Simple XML streaming "borrowed" from kssutil

namespace { namespace xml {

    /*!
     This namespace provides methods for efficiently creating XML output. The requirements
     for this project were as follows:

     1. Single-file header-only implementation suitable for embedding into other projects.
     2. No dependancies other than a modern C++ compiler (i.e. no third-party libraries).
     3. Ability to stream XML data without having to have the entire thing in memory.

     The intention is not a full-grown XML package, but just to be able to create XML
     output efficiently.
     */
    namespace simple_writer {

        using namespace std;

        struct node;

        /*!
         An XML child is represented by a node generator function. The function must return
         a node* with each call. The returned pointer must remain valid until the next call.
         When there are no more children to be produced, it should return nullptr. This
         design was chosen to allow the XML to be streamed without all being in memory at
         once.

         Note that the generator can be a lambda, or it could be a generator class that has
         a method "node* operator()() { ... }". In either case it will need to maintain
         its own state so that it will know what to return on each call.
         */
        using node_generator_fn = function<node*(void)>;

        /*!
         An XML node is represented by a key/value pair mapping (which become the attributes)
         combined with an optional child generator (which become the children).
         */
        struct node {
            string                               name;
            map<string, string>                  attributes;
            string                               text;
            mutable vector<node_generator_fn>    children;

            /*!
             Convenience access for setting attributes.
             */
            string& operator[](const string& key) {
                return attributes[key];
            }

            /*!
             Reset the node to an empty one.
             */
            void clear() noexcept {
                name.clear();
                attributes.clear();
                text.clear();
                children.clear();
            }
        };


        // Don't call anything in this "namespace" manually.
        struct _private {

            static ostream& indent(ostream& strm, int indentLevel) {
                for (auto i = 0; i < indentLevel; ++i) { strm << "  "; }
                return strm;
            }

            // Based on code found at
            // https://stackoverflow.com/questions/5665231/most-efficient-way-to-escape-xml-html-in-c-string
            static string encode(const string& data, int indentLevel = -1) {
                string buffer;
                buffer.reserve(data.size());
                for(size_t pos = 0; pos != data.size(); ++pos) {
                    switch(data[pos]) {
                        case '&':  buffer.append("&amp;");       break;
                        case '\"': buffer.append("&quot;");      break;
                        case '\'': buffer.append("&apos;");      break;
                        case '<':  buffer.append("&lt;");        break;
                        case '>':  buffer.append("&gt;");        break;
                        case '\n':
                            if (indentLevel > 0) {
                                buffer.append("\n");
                                while (indentLevel--) buffer.append("  ");
                            }
                            break;
                        default:   buffer.append(&data[pos], 1); break;
                    }
                }
                return buffer;
            }

            static ostream& write_with_indent(ostream& strm, const node& n, int indentLevel) {
                assert(!n.name.empty());
                assert(indentLevel >= 0);
                assert(n.text.empty() || n.children.empty());

                // Start the node.
                const bool singleLine = (n.text.empty() && n.children.empty());
                indent(strm, indentLevel);
                strm << '<' << n.name;

                // Write the attributes.
                for (auto& attr : n.attributes) {
                    assert(!attr.first.empty());
                    assert(!attr.second.empty());
                    strm << ' ' << attr.first << "=\"" << encode(attr.second) << '"';
                }
                strm << (singleLine ? "/>" : ">") << endl;

                if (!singleLine) {
                    // Write the contents.
                    if (!n.text.empty()) {
                        indent(strm, indentLevel+1);
                        strm << encode(n.text, indentLevel+1) << endl;
                    }

                    // Write the children.
                    for (auto fn : n.children) {
                        node* child = fn();
                        while (child) {
                            write_with_indent(strm, *child, indentLevel+1);
                            child = fn();
                        }
                    }

                    // End the node.
                    indent(strm, indentLevel);
                    strm << "</" << n.name << '>' << endl;
                }

                return strm;
            }
        };


        /*!
         Write an XML object to a stream.
         @returns the stream
         @throws any exceptions that the stream writing may throw.
         */
        inline ostream& write(ostream& strm, const node& root) {
            strm << "<?xml version=\"1.0\" encoding=\"UTF-8\"?>" << endl;
            return _private::write_with_indent(strm, root, 0);
        }
    }
}}


// MARK: Simple JSON streaming "borrowed" from kssutil.

namespace { namespace json {

    /*!
     This namespace provides methods for efficiently creating JSON output. The
     requirements for this project were as follows:

     1. Single-file header-only implementation suitable for embedding into other projects.
     2. No dependancies other than a modern C++ compiler (i.e. no third-party libraries).
     3. Ability to stream JSON data without having to have the entire thing in memory.
     4. Keys can be assumed to be strings.
     5. Values will be assumed to be numbers if they "look like" a number.
     6. Children will always be in arrays.

     The intention is not a full-grown JSON package (i.e. it is not the equivalent
     of kss::util::json::Document), but rather to be able to create JSON output
     efficiently.
     */
    namespace simple_writer {

        using namespace std;

        struct node;

        /*!
         A JSON child is represented by a key and a json_t generator function. The function
         should return a json_t* with each call. The pointer must remain valid until the
         next call. When there are no more children, it should return nullptr.

         Note that the generator can be a lambda, or it could be a generator class that
         has a method "json_t* operator()() { ... }". In either case it will need to maintain
         its own state so that it will know what to return on each call.
         */
        using node_generator_fn = function<node*(void)>;
        using array_child_t = pair<string, node_generator_fn>;

        /*!
         A JSON node is represented by a key/value pair mapping combined with an optional
         child generator. In the mapping values that appear to be numeric will not be
         quoted, all others will assumed to be strings and will be escaped and quoted.
         */
        struct node {
            map<string, string>              attributes;
            mutable vector<array_child_t>    arrays;

            /*!
             Convenience access for setting attributes.
             */
            string& operator[](const string& key) {
                return attributes[key];
            }

            /*!
             Reset the node to an empty one.
             */
            void clear() noexcept {
                attributes.clear();
                arrays.clear();
            }
        };


        // Don't call anything in this namespace manually.
        struct _private {

            // This is based on code found at
            // https://stackoverflow.com/questions/4654636/how-to-determine-if-a-string-is-a-number-with-c?utm_medium=organic&utm_source=google_rich_qa&utm_campaign=google_rich_qa
            static inline bool is_number(const string& s) {
                return !s.empty() && find_if(s.begin(), s.end(), [](char c) { return !(isdigit(c) || c == '.'); }) == s.end();
            }

            // The following is based on code found at
            // https://stackoverflow.com/questions/7724448/simple-json-string-escape-for-c/33799784#33799784
            static string encodeJson(const string &s) {
                // If it is a number we need no escapes or quotes.
                if (is_number(s)) {
                    return s;
                }

                // Otherwise we must escape it and quote it.
                ostringstream o;
                o << '"';
                for (auto c = s.cbegin(); c != s.cend(); c++) {
                    switch (*c) {
                        case '"': o << "\\\""; break;
                        case '\\': o << "\\\\"; break;
                        case '\b': o << "\\b"; break;
                        case '\f': o << "\\f"; break;
                        case '\n': o << "\\n"; break;
                        case '\r': o << "\\r"; break;
                        case '\t': o << "\\t"; break;
                        default:
                            if ('\x00' <= *c && *c <= '\x1f') {
                                o << "\\u"
                                << std::hex << std::setw(4) << std::setfill('0') << (int)*c;
                            } else {
                                o << *c;
                            }
                    }
                }
                o << '"';
                return o.str();
            }

            static ostream& indent(ostream& strm, int indentLevel, int extraSpaces = 0) {
                for (auto i = 0; i < (indentLevel*4)+extraSpaces; ++i) {
                    strm << ' ';
                }
                return strm;
            }

            static ostream& write_with_indent(ostream& strm, const node& json, int indentLevel,
                                              bool needTrailingComma)
            {
                indent(strm, indentLevel);
                strm << '{' << endl;

                auto lastIt = --json.attributes.end();
                for (auto it = json.attributes.begin(); it != json.attributes.end(); ++it) {
                    indent(strm, indentLevel, 2);
                    strm << '"' << it->first << "\": " << encodeJson(it->second);
                    if (it != lastIt || json.arrays.size() > 0) strm << ',';
                    strm << endl;
                }

                const auto last = --json.arrays.end();
                for (auto it = json.arrays.begin(); it != json.arrays.end(); ++it) {
                    write_child_in_array(strm, indentLevel, *it, it == last);
                }

                indent(strm, indentLevel);
                strm << '}';
                if (needTrailingComma) strm << ',';
                strm << endl;

                return strm;
            }

            static ostream& write_child_in_array(ostream& strm, int indentLevel, array_child_t& child, bool isLastChild) {
                indent(strm, indentLevel, 2);
                strm << '"' << child.first << "\": [" << endl;

                node* json = child.second();
                while (json) {
                    node tmp = *json;
                    node* next = child.second();
                    write_with_indent(strm, tmp, indentLevel+1, (next != nullptr));
                    json = next;
                }

                indent(strm, indentLevel, 2);
                strm << "]";
                if (!isLastChild) strm << ',';
                strm << endl;
                return strm;
            }
        };



        /*!
         Write a JSON object to a stream.
         @returns the stream
         @throws any exceptions that the stream writing may throw.
         */
        inline ostream& write(ostream& strm, const node& json) {
            return _private::write_with_indent(strm, json, 0, false);
        }
    }

} }


// MARK: State of the world

namespace {

    struct TestError {
        string errorType;           // The name of the exception.
        string errorMessage;        // The what() of the exception.

        operator string() const {
            return errorType + ": " + errorMessage;
        }

        static TestError makeError(const exception& e) {
            TestError ret;
            ret.errorType = _private::demangle(e);
            ret.errorMessage = e.what();
            return ret;
        }
    };

    using failures_t = vector<pair<string, string>>;

    struct TestCaseWrapper {
        string                  name;
        TestSuite*              owner = nullptr;
        TestSuite::test_case_fn fn;
        unsigned int            assertions = 0;
        vector<TestError>       errors;
        failures_t              failures;
        bool                    skipped = false;
        duration<double>        durationOfTest;
        string                  mostRecentDetails;

        bool operator<(const TestCaseWrapper& rhs) const noexcept {
            return name < rhs.name;
        }
    };

    struct TestSuiteWrapper {
        TestSuite*          suite;
        bool                filteredOut = false;
        string              timestamp;
        duration<double>    durationOfTestSuite;
        unsigned            numberOfErrors = 0;
        unsigned            numberOfFailedAssertions = 0;
        unsigned            numberOfSkippedTests = 0;
        unsigned            numberOfFailedTests = 0;

        bool operator<(const TestSuiteWrapper& rhs) const noexcept {
            return suite->name() < rhs.suite->name();
        }
    };

    thread_local static TestSuiteWrapper*   currentSuite = nullptr;
    thread_local static TestCaseWrapper*    currentTest = nullptr;
    static bool                             isQuietMode = false;
    static bool                             isVerboseMode = false;
    static bool                             isParallel = true;
    static string                           filter;
    static string                           xmlReportFilename;
    static string                           jsonReportFilename;

    // Lazy instantiation of the testSuites singleton.
    vector<TestSuiteWrapper>* testSuites() {
        static once_flag flag;
        static vector<TestSuiteWrapper>* suites = nullptr;
        call_once(flag, [&]{ suites = new vector<TestSuiteWrapper>(); });
        return suites;
    }

    // Summary of test results.
    struct TestResultSummary {
        mutex               lock;
        string              programName;
        string              nameOfTestRun;
        string              nameOfHost;
        string              timeOfTestRun;
        duration<double>    durationOfTestRun;
        unsigned            numberOfErrors = 0;
        unsigned            numberOfFailures = 0;
        unsigned            numberOfAssertions = 0;
        unsigned            numberOfTests = 0;
    };
    static TestResultSummary reportSummary;
}


// MARK: Internal Utilities

namespace {

    // Basename of a path.
    string basename(const string &path) {
        const auto dirSepPos = path.find_last_of('/');
        if (dirSepPos == string::npos) {
            return path;
        }
        else {
            return path.substr(dirSepPos+1);
        }
    }

    // Test for subclasses.
    template <class T, class Base>
    T* as(Base* obj) noexcept {
        if (!obj) return nullptr;
        return dynamic_cast<T*>(obj);
    }

    // Utility class to enforce RAII cleanup on C style calls.
    class finally {
    public:
        typedef std::function<void(void)> lambda;

        finally(lambda cleanupCode) : _cleanupCode(cleanupCode) {}
        ~finally() { _cleanupCode(); }
        finally& operator=(const finally&) = delete;

    private:
        lambda _cleanupCode;
    };

    // Exception that is thrown to skip a test case.
    class SkipTestCase {};

    // Signal handler to allow us to ignore SIGCHLD.
    void my_signal_handler(int sig) {
    }

    // Terminate handler allowing us to catch the terminate call.
    void my_terminate_handler() {
        _exit(0);           // Correct response.
    }

    // Parse the command line. Returns true if we should continue or false if we
    // should exit.
    static const struct option commandLineOptions[] = {
        { "help", no_argument, nullptr, 'h' },
        { "quiet", no_argument, nullptr, 'q' },
        { "verbose", no_argument, nullptr, 'v' },
        { "filter", required_argument, nullptr, 'f' },
        { "xml", required_argument, nullptr, 'X' },
        { "json", required_argument, nullptr, 'J' },
        { "no-parallel", no_argument, nullptr, 'N' },
        { nullptr, 0, nullptr, 0 }
    };

    static char** duplicateArgv(int argc, const char* const* argv) {
        // Create a duplicate of argv that is not const. This may seem dangerous but is needed
        // for the underlying C calls, even though they will not modify argv.
        int i;
        assert(argc > 0);
        char** newargv = (char**)malloc(sizeof(char*)*(size_t)argc);
        if (!newargv) throw bad_alloc();

        for (i = 0; i < argc; ++i) {
            newargv[i] = (char*)argv[i];
        }
        return newargv;
    }


    // Print the usage message to the given stream.
    void printUsageMessage(ostream& strm) {
        strm << "usage: " << reportSummary.programName << " [options]" << R"(

The following are the accepted command line options:
-h/--help displays this usage message
-q/--quiet suppress test result output (useful if all you want is the return value)
-v/--verbose displays more information (-q will override this if present. Specifying
    this option will also cause --no-parallel to be assumed.)
-f <testprefix>/--filter=<testprefix> only run tests that start with the prefix
---xml=<filename> writes a JUnit test compatible XML to the given filename
--json=<filename> writes a gUnit test compatible JSON to the given filename
--no-parallel will force all tests to be run in the same thread (This is assumed if
    the --verbose option is specified.)

The display options essentially run in three modes.

In the "quiet" mode (--quiet is specified) no output at all is written and the only
indication of the test results is the return code. This is useful for inclusion in scripts
where you only want a pass/fail result and do not care about the details. It is also
useful if you are outputting XML or JSON to the standard output device and don't want
to have to separate them in your script.

In the "normal" mode (neither --quiet nor --verbose is specified) the program will print a
header line when the tests begin, then will print one of the following characters for each
test suite, followed by a summary stating how many tests passed, failed, and skipped,
finishing with details of all the failed tests:
    . - all tests in the suite ran without error or failure
    E - one or more of the tests in the suite caused an error condition
    F - one or more of the tests in the suite failed an assertion
    S - one or more tests in the suite were skipped (due to use of the skip() method)

In the "verbose" mode (--verbose is specified) more details are written while the tests
are run. In particular you will see a header line for each test suite and an individual
line for each test case within the test suite. For each test case you will see one of
the following characters for each test (i.e. for each call to KSS_ASSERT):
    . - the assertion passed
    + - 10 consecutive assertions passed
    * - 100 consecutive assertions passed
    S - skip() was called (it will be the last report on the line)
    E - an error occurred while running the test (it will be the last report on the line)
    F - the test failed
If a tests has errors or failures, they will be written out on the following lines. When
the output for all the test cases in a suite is completed, a summary line for the test
suite will be output. Note that in order for this output to make sense, specifying --verbose
will also imply --no-parallel.

For --xml or --json you can specify "-" as the filename. In that case instead of writing
to a file the report will be written to the standard output device. If you decide to write
the reports to the standard output device, and you have not specified --quiet or you have
asked for both XML and JSON, the reports will be preceeded by a line containing at least the
text "==XML=REPORT==" and "==JSON=REPORT==" to help your scripts identify them in the
output. If you only send one of them to the standard output device, and you have specified
--quiet, then no such tag line will be output (hence the only output should be the report).

Filtering can be used to limit the tests that are run without having to add skip()
statements in your code. This is most useful when you are developing/debugging a particular
section and don't want to repeat all the other test until you have completed. It is also
generally useful to specify --verbose when you are filtering, but that is not assumed.

The return value, when all the tests are done, will be one of the following:
-1 (255 on some systems) if there was one or more error conditions raised,
0 if all tests completed with no errors or failures (although some may have skipped), or
>0 if some tests failed. The value will be the number of failures (i.e. the number of
times that KSS_ASSERT failed) in all the test cases in all the test suites.

    )" << endl;
    }

    // Obtain the required argument or print a usage message and exit if it does not exist.
    string getArgument() {
        if (!optarg) {
            printUsageMessage(cerr);
            exit(-1);
        }
        return string(optarg);
    }

    // Parse the command line and setup the global state of the world with the results.
    bool parseCommandLine(int argc, const char* const* argv) {
        if (argc > 0 && argv != nullptr) {
            char** newargv = duplicateArgv(argc, argv);
            finally cleanup([&]{ free(newargv); });

            int ch = 0;
            while ((ch = getopt_long(argc, newargv, "hqvf:", commandLineOptions, nullptr)) != -1) {
                switch (ch) {
                    case 'h':
                        printUsageMessage(cout);
                        return false;
                    case 'q':
                        isQuietMode = true;
                        break;
                    case 'v':
                        isVerboseMode = true;
                        break;
                    case 'N':
                        isParallel = false;
                        break;
                    case 'f':
                        filter = getArgument();
                        break;
                    case 'X':
                        xmlReportFilename = getArgument();
                        break;
                    case 'J':
                        jsonReportFilename = getArgument();
                        break;
                }
            }

            // Fix any command line dependances.
            if (isQuietMode) {
                isVerboseMode = false;
            }
            if (isVerboseMode) {
                isParallel = false;
            }
        }
        return true;
    }

    // Returns true if the given string starts with the given prefix and false otherwise.
    inline bool starts_with(const string& s, const string& prefix) noexcept {
        return (prefix == s.substr(0, prefix.size()));
    }

    // Returns true if the test case should pass the filter and false otherwise.
    bool passesFilter(const TestSuite& s) noexcept {
        if (!filter.empty()) {
            return starts_with(s.name(), filter);
        }
        return true;
    }

    // Write a file, throwing an exception if there is a problem.
    inline void throwProcessingError(const string& filename, const string& what_arg) {
        throw system_error(errno, system_category(), what_arg + " " + filename);
    }

    void write_file(const string& filename, function<void (ofstream&)> fn) {
        errno = 0;
        ofstream strm(filename);
        if (!strm.is_open()) throwProcessingError(filename, "Failed to open");
        fn(strm);
        if (strm.bad()) throwProcessingError(filename, "Failed while writing");
    }

    // Return the time that fn took to run.
    duration<double> timeOfExecution(function<void ()> fn) {
        const auto start = steady_clock::now();
        fn();
        return duration_cast<duration<double>>(steady_clock::now() - start);
    }

    // Return the current timestamp in ISO 8601 format.
    string now() {
        time_t now;
        ::time(&now);
        char buf[sizeof "9999-99-99T99:99:99Z "];
        strftime(buf, sizeof(buf), "%FT%TZ", ::gmtime(&now));
        return string(buf);
    }

    // Returns the hostname of the machine.
    string hostname() noexcept {
        char name[100];
        if (gethostname(name, sizeof(name)-1) == -1) {
            return "localhost";
        }
        return name;
    }
}


// MARK: TestSuite::Impl Implementation

struct TestSuite::Impl {
    TestSuite*                parent = nullptr;
    string                    name;
    vector<TestCaseWrapper>   tests;

    // Add the BeforeAll and AfterAll "tests" if appropriate.
    void addBeforeAndAfterAll() {
        if (auto* hba = as<HasBeforeAll>(parent)) {
            TestCaseWrapper wrapper;
            wrapper.name = "BeforeAll";
            wrapper.owner = parent;
            wrapper.fn = [hba] { hba->beforeAll(); };
            tests.insert(tests.begin(), move(wrapper));
        }
        if (auto* haa = as<HasAfterAll>(parent)) {
            TestCaseWrapper wrapper;
            wrapper.name = "AfterAll";
            wrapper.owner = parent;
            wrapper.fn = [haa] { haa->afterAll(); };
            tests.push_back(move(wrapper));
        }
    }

    // Run a test.
    void runTestCase(TestCaseWrapper& t) {
        currentTest = &t;
        try {
            t.durationOfTest = timeOfExecution([&]{
                if (auto* hbe = as<HasBeforeEach>(parent)) {
                    if (t.name != "BeforeAll" && t.name != "AfterAll") hbe->beforeEach();
                }
                t.fn();
                if (auto* haa = as<HasAfterEach>(parent)) {
                    if (t.name != "BeforeAll" && t.name != "AfterAll") haa->afterEach();
                }
            });
        }
        catch (const SkipTestCase&) {
            t.skipped = true;
            if (isVerboseMode) {
                cout << "SKIPPED";
            }
        }
        catch (const exception& e) {
            if (isVerboseMode) {
                cout << "E";
            }
            t.errors.push_back(TestError::makeError(e));
        }
        catch (...) {
            if (isVerboseMode) {
                cout << "E";
            }
            TestError err;
            err.errorMessage = "Unknown exception";
            t.errors.push_back(err);
        }

        // Update the counters.
        currentTest = nullptr;
        currentSuite->numberOfErrors += t.errors.size();
        currentSuite->numberOfFailedAssertions += t.failures.size();
        if (!t.failures.empty()) ++currentSuite->numberOfFailedTests;
        if (t.skipped) ++currentSuite->numberOfSkippedTests;

        {
            lock_guard<mutex> l(reportSummary.lock);
            reportSummary.numberOfErrors += t.errors.size();
            reportSummary.numberOfFailures += t.failures.size();
            reportSummary.numberOfAssertions += t.assertions;
            ++reportSummary.numberOfTests;
        }
    }

    // Returns the test suite result as one of the following:
    // '.' - all tests ran without problem
    // 'S' - one or more tests were skipped
    // 'E' - one or more tests caused an error condition
    // 'F' - one or more tests failed.
    char result() const noexcept {
        char res = '.';
        for (const auto& t : tests) {
            if (!t.errors.empty()) {
                res = 'E';
            }
            else if (!t.failures.empty()) {
                if (res != 'E') {
                    res = 'F';
                }
            }
            else if (t.skipped) {
                if (res == '.') {
                    res = 'S';
                }
            }
        }
        return res;
    }

    // Returns the number of failures (KSS_ASSERT failing) in all the test cases
    // for the suite.
    int numberOfFailures() const noexcept {
        int total = 0;
        for (const auto& t : tests) {
            total += t.failures.size();
        }
        return total;
    }
};


// MARK: Test reporting

namespace {
    void printTestRunHeader() {
        if (!isQuietMode) {
            cout << "Running test suites for " << reportSummary.nameOfTestRun << "..." << endl;;
            if (!isVerboseMode) {
                cout << "  ";
                flush(cout);    // Need flush before we start any threads.
            }
        }
    }

    void outputStandardSummary() {
        if (!isQuietMode) {
            if (isParallel) {
                flush(cout);    // Ensure all the thread output is flushed.
            }
            if (!isVerboseMode) {
                cout << endl;
            }

            const auto* suites = testSuites();
            const auto numberOfTestSuites = suites->size();
            unsigned numberOfErrors = 0, numberOfFailures = 0, numberOfSkips = 0, numberPassed = 0;
            unsigned numberFilteredOut = 0;
            for (const auto& ts : *suites) {
                if (ts.filteredOut) {
                    ++numberFilteredOut;
                }

                switch (ts.suite->_implementation()->result()) {
                    case '.':    ++numberPassed; break;
                    case 'S':    ++numberOfSkips; break;
                    case 'E':    ++numberOfErrors; break;
                    case 'F':    ++numberOfFailures; break;
                }
            }

            if (!numberOfFailures && !numberOfErrors && !numberOfSkips) {
                cout << "  PASSED all " << (numberOfTestSuites-numberFilteredOut) << " test suites.";
            }
            else {
                cout << "  Passed " << numberPassed << " of " << (numberOfTestSuites-numberFilteredOut) << " test suites";
                if (numberOfSkips > 0) {
                    cout << ", " << numberOfSkips << " skipped";
                }
                if (numberOfErrors > 0) {
                    cout << ", " << numberOfErrors << (numberOfErrors == 1 ? " error" : " errors");
                }
                if (numberOfFailures > 0) {
                    cout << ", " << numberOfFailures << " failed";
                }
                cout << ".";
            }

            if (numberFilteredOut > 0) {
                cout << "  (" << numberFilteredOut << " filtered out)";
            }
            cout << endl;

            if (!isVerboseMode) {
                // If we are not verbose we need to identify the errors and failures. If
                // we are verbose, then that information was displayed earlier.
                if (numberOfErrors > 0) {
                    cout << "  Errors:" << endl;
                    for (const auto& ts : *suites) {
                        for (const auto& t : ts.suite->_implementation()->tests) {
                            for (const auto& err : t.errors) {
                                cout << "    " << string(err) << endl;
                            }
                        }
                    }
                }
                if (numberOfFailures > 0) {
                    cout << "  Failures:" << endl;
                    for (const auto& ts : *suites) {
                        for (const auto& t : ts.suite->_implementation()->tests) {
                            for (const auto& f : t.failures) {
                                cout << "    " << f.first << endl;
                                if (!f.second.empty()) {
                                    cout << "       ↳" << f.second << endl;
                                }
                            }
                        }
                    }
                }
            }

            cout << "  Completed in " << reportSummary.durationOfTestRun.count() << "s." << endl;
        }
    }

    template <class T, class Node>
    struct AbstractGenerator {
        virtual ~AbstractGenerator() = default;
        AbstractGenerator(const AbstractGenerator&) = default;
        AbstractGenerator(AbstractGenerator&&) = default;
        AbstractGenerator& operator=(const AbstractGenerator&) = default;
        AbstractGenerator& operator=(AbstractGenerator&&) = default;

        Node* operator()() {
            if (_it == _items.end()) {
                return nullptr;
            }
            else {
                _n.clear();
                populate();
                ++_it;
                return &_n;
            }
        }

        virtual void populate() = 0;

    protected:
        AbstractGenerator(const vector<T>& items) : _items(items) {
            _it = _items.begin();
        }
        const vector<T>&                    _items;
        typename vector<T>::const_iterator  _it;
        Node                                _n;
    };

    struct FailureXmlGenerator
    : public AbstractGenerator<pair<string, string>, xml::simple_writer::node>
    {
        FailureXmlGenerator(const failures_t& failures) : AbstractGenerator(failures) {}
        virtual ~FailureXmlGenerator() = default;

        virtual void populate() override {
            _n.name = "failure";
            _n["message"] = _it->first;
            if (!_it->second.empty()) {
                _n.text = _it->second;
            }
        }
    };

    struct ErrorXmlGenerator : public AbstractGenerator<TestError, xml::simple_writer::node> {
        ErrorXmlGenerator(const vector<TestError>& errors) : AbstractGenerator(errors) {}
        virtual ~ErrorXmlGenerator() = default;

        virtual void populate() override {
            _n.name = "error";
            _n["message"] = _it->errorMessage;
            if (!_it->errorType.empty()) _n["type"] = _it->errorType;
        }
    };

    struct TestCaseXmlGenerator : public AbstractGenerator<TestCaseWrapper, xml::simple_writer::node> {
        TestCaseXmlGenerator(const vector<TestCaseWrapper>& testCases) : AbstractGenerator(testCases) {}
        virtual ~TestCaseXmlGenerator() = default;

        virtual void populate() override {
            _n.name = "testcase";
            _n["name"] = _it->name;
            _n["assertions"] = to_string(_it->assertions);
            _n["classname"] = (_it->owner ? _private::demangle(*(_it->owner)) : string("none"));
            _n["time"] = to_string(_it->durationOfTest.count());
            if (!_it->errors.empty() || !_it->failures.empty()) {
                _n.children = {
                    ErrorXmlGenerator(_it->errors),
                    FailureXmlGenerator(_it->failures)
                };
            }
        }
    };

    struct TestSuiteXmlGenerator : public AbstractGenerator<TestSuiteWrapper, xml::simple_writer::node> {
        TestSuiteXmlGenerator(const vector<TestSuiteWrapper>& suites) : AbstractGenerator(suites) {}
        virtual ~TestSuiteXmlGenerator() = default;

        virtual void populate() override {
            _n.name = "testsuite";
            _n["name"] = _it->suite->name();
            _n["tests"] = to_string(_it->suite->_implementation()->tests.size());
            _n["errors"] = to_string(_it->numberOfErrors);
            _n["failures"] = to_string(_it->numberOfFailedAssertions);
            _n["hostname"] = reportSummary.nameOfHost;
            _n["id"] = to_string(id++);
            _n["skipped"] = to_string(_it->numberOfSkippedTests);
            _n["time"] = to_string(_it->durationOfTestSuite.count());
            _n["timestamp"] = _it->timestamp;
            _n.children = { TestCaseXmlGenerator(_it->suite->_implementation()->tests) };
        }

    private:
        int id = 0;
    };

    void writeXmlReportToStream(ostream& strm) {
        xml::simple_writer::node root;
        root.name = "testsuites";
        root["errors"] = to_string(reportSummary.numberOfErrors);
        root["failures"] = to_string(reportSummary.numberOfFailures);
        root["name"] = reportSummary.nameOfTestRun;
        root["tests"] = to_string(reportSummary.numberOfAssertions - reportSummary.numberOfFailures);
        root["time"] = to_string(reportSummary.durationOfTestRun.count());
        const auto* suites = testSuites();
        root.children = { TestSuiteXmlGenerator(*suites) };
        xml::simple_writer::write(strm, root);
    }

    void printXmlReport() {
        if (xmlReportFilename == "-") {
            if (!isQuietMode || jsonReportFilename == "-") {
                cout << "==XML=REPORT===================================" << endl;
            }
            writeXmlReportToStream(cout);
        }
        else {
            write_file(xmlReportFilename, [&](ofstream& strm) {
                writeXmlReportToStream(strm);
            });
            if (!isQuietMode) {
                cout << "  Wrote XML report to " << xmlReportFilename << endl;
            }
        }
    }


    struct FailureJsonGenerator
    : public AbstractGenerator<pair<string, string>, json::simple_writer::node>
    {
        FailureJsonGenerator(const failures_t& failures) : AbstractGenerator(failures) {}
        virtual ~FailureJsonGenerator() = default;

        virtual void populate() override {
            _n["message"] = _it->first;
            if (!_it->second.empty()) {
                _n["message"] += " (" + _it->second + ")";
            }
        }
    };

    struct TestCaseJsonGenerator : public AbstractGenerator<TestCaseWrapper, json::simple_writer::node> {
        TestCaseJsonGenerator(const vector<TestCaseWrapper>& tests) : AbstractGenerator(tests) {}
        virtual ~TestCaseJsonGenerator() = default;

        virtual void populate() override {
            _n["name"] = _it->name;
            _n["status"] = (_it->skipped ? "NOTRUN" : "RUN");
            _n["time"] = to_string(_it->durationOfTest.count());
            _n["classname"] = (_it->owner ? _private::demangle(*(_it->owner)) : string("none"));
            if (!_it->failures.empty()) {
                _n.arrays = { make_pair("failures", FailureJsonGenerator(_it->failures)) };
            }
        }
    };

    struct TestSuiteJsonGenerator : public AbstractGenerator<TestSuiteWrapper, json::simple_writer::node> {
        TestSuiteJsonGenerator(const vector<TestSuiteWrapper>& suites) : AbstractGenerator(suites) {}
        virtual ~TestSuiteJsonGenerator() = default;

        virtual void populate() override {
            _n["name"] = _it->suite->name();
            _n["tests"] = to_string(_it->suite->_implementation()->tests.size());
            _n["failures"] = to_string(_it->numberOfFailedTests);
            _n["errors"] = to_string(_it->numberOfErrors);
            _n["time"] = to_string(_it->durationOfTestSuite.count());
            _n.arrays = { make_pair("testsuite", TestCaseJsonGenerator(_it->suite->_implementation()->tests)) };
        }
    };

    void writeJsonReportToStream(ostream& strm) {
        json::simple_writer::node n;
        n["tests"] = to_string(reportSummary.numberOfTests);
        n["failures"] = to_string(reportSummary.numberOfFailures);
        n["errors"] = to_string(reportSummary.numberOfErrors);
        n["time"] = to_string(reportSummary.durationOfTestRun.count());
        n["timestamp"] = reportSummary.timeOfTestRun;
        n["name"] = reportSummary.nameOfTestRun;
        const auto* suites = testSuites();
        n.arrays = { make_pair("testsuites", TestSuiteJsonGenerator(*suites)) };
        json::simple_writer::write(strm, n);
    }

    void printJsonReport() {
        if (jsonReportFilename == "-") {
            if (!isQuietMode || xmlReportFilename == "-") {
                cout << "==JSON=REPORT==================================" << endl;
            }
            writeJsonReportToStream(cout);
        }
        else {
            write_file(jsonReportFilename, [&](ofstream& strm) {
                writeJsonReportToStream(strm);
            });
            if (!isQuietMode) {
                cout << "  Wrote JSON report to " << jsonReportFilename << endl;
            }
        }
    }

    void printTestRunSummary() {
        if (!isQuietMode) {
            outputStandardSummary();
        }
        if (!xmlReportFilename.empty()) {
            printXmlReport();
        }
        if (!jsonReportFilename.empty()) {
            printJsonReport();
        }
    }

    void printTestSuiteHeader(const TestSuite& ts) {
        if (isVerboseMode) {
            cout << "  " << ts.name() << endl;
        }
    }

    void printTestSuiteSummary(const TestSuiteWrapper& w) {
        if (!isQuietMode) {
            const auto* impl = w.suite->_implementation();
            if (isVerboseMode) {
                unsigned numberOfAssertions = 0;
                for (const auto& t : impl->tests) {
                    numberOfAssertions += t.assertions;
                }

                if (!w.numberOfErrors && !w.numberOfFailedAssertions) {
                    cout << "    PASSED all " << numberOfAssertions << " checks";
                }
                else {
                    cout << "    Passed " << numberOfAssertions << " checks";
                }

                if (w.numberOfSkippedTests > 0) {
                    cout << ", " << w.numberOfSkippedTests << " test " << (w.numberOfSkippedTests == 1 ? "case" : "cases") << " SKIPPED";
                }
                if (w.numberOfErrors > 0) {
                    cout << ", " << w.numberOfErrors << (w.numberOfErrors == 1 ? " error" : " errors");
                }
                if (w.numberOfFailedAssertions > 0) {
                    cout << ", " << w.numberOfFailedAssertions << " FAILED";
                }
                cout << "." << endl;

                if (w.numberOfErrors > 0) {
                    cout << "    Errors:" << endl;
                    for (const auto& t : impl->tests) {
                        for (const auto& err : t.errors) {
                            cout << "      " << string(err) << endl;
                        }
                    }
                }
                if (w.numberOfFailedAssertions > 0) {
                    cout << "    Failures:" << endl;
                    for (const auto& t : impl->tests) {
                        for (const auto& f : t.failures) {
                            cout << "      " << f.first << endl;
                            if (!f.second.empty()) {
                                cout << "         ↳" << f.second << endl;
                            }
                        }
                    }
                }
            }
            else {
                cout << impl->result();
            }
        }
    }

    void printTestCaseHeader(const TestCaseWrapper& t) {
        if (isVerboseMode) {
            cout << "    " << t.name << " ";
        }
    }

    void printTestCaseSummary(const TestCaseWrapper& t) {
        if (isVerboseMode) {
            cout << endl;
        }
    }

    int testResultCode() {
        if (reportSummary.numberOfErrors > 0) {
            return -1;
        }
        return reportSummary.numberOfFailures;
    }

    void runTestSuite(TestSuiteWrapper* wrapper) {
        if (!passesFilter(*wrapper->suite)) {
            wrapper->filteredOut = true;
            return;
        }

        wrapper->timestamp = now();
        printTestSuiteHeader(*wrapper->suite);
        auto* impl = wrapper->suite->_implementation();
        impl->addBeforeAndAfterAll();
        currentSuite = wrapper;

        wrapper->durationOfTestSuite = timeOfExecution([&]{
            for (auto& t : impl->tests) {
                printTestCaseHeader(t);
                impl->runTestCase(t);
                printTestCaseSummary(t);
            }
        });

        currentSuite = nullptr;
        printTestSuiteSummary(*wrapper);
    }
}


namespace kss { namespace test {

    int run(const string& testRunName, int argc, const char *const *argv) {
        auto* suites = testSuites();
        reportSummary.programName = basename(argv[0]);
        reportSummary.nameOfTestRun = testRunName;
        reportSummary.nameOfHost = hostname();
        if (parseCommandLine(argc, argv)) {
            printTestRunHeader();

            sort(suites->begin(), suites->end());
            reportSummary.timeOfTestRun = now();
            reportSummary.durationOfTestRun = timeOfExecution([&]{
                vector<future<bool>> futures;

                for (auto& ts : *suites) {
                    if (!isParallel || as<MustNotBeParallel>(ts.suite)) {
                        runTestSuite(&ts);
                    }
                    else {
                        TestSuiteWrapper* tsw = &ts;
                        futures.push_back(async([tsw]{
                            runTestSuite(tsw);
                            return true;
                        }));
                    }
                }

                if (!futures.empty()) {
                    for (auto& fut : futures) {
                        fut.get();
                    }
                }
            });

            printTestRunSummary();
        }
        delete suites;
        return testResultCode();
    }

    void skip() {
        throw SkipTestCase();
    }

    bool isQuiet() noexcept {
        return isQuietMode;
    }

    bool isVerbose() noexcept {
        return isVerboseMode;
    }
}}


// MARK: Assertions

namespace kss { namespace test {

    bool doesNotThrowException(const function<void()>& fn) {
        bool caughtSomething = false;
        try {
            fn();
        }
        catch (const exception& e) {
            caughtSomething = true;
            _private::setFailureDetails("threw "
                                        + _private::demangle(e)
                                        + ", what=" + e.what());
        }
        return !caughtSomething;
    }

    bool throwsSystemErrorWithCategory(const error_category& cat, const function<void()>& fn) {
        bool caughtCorrectCategory = false;
        try {
            fn();
        }
        catch (const system_error& e) {
            caughtCorrectCategory = (e.code().category() == cat);
            if (!caughtCorrectCategory) {
                _private::setFailureDetails(string("actual category was ")
                                            + e.code().category().name());
            }
        }
        catch (const exception& e) {
            _private::setFailureDetails("actually exception was "
                                        + _private::demangle(e)
                                        + ", what=" + e.what());
        }
        return caughtCorrectCategory;
    }

    bool throwsSystemErrorWithCode(const error_code& code, const function<void()>& fn) {
        bool caughtCorrectCode = false;
        try {
            fn();
        }
        catch (const system_error& e) {
            caughtCorrectCode = (e.code() == code);
            if (!caughtCorrectCode) {
                _private::setFailureDetails(string("actual code was ")
                                            + to_string(e.code().value())
                                            + ", category "
                                            + e.code().category().name());
            }
        }
        catch (const exception& e) {
            _private::setFailureDetails("actually exception was "
                                        + _private::demangle(e)
                                        + ", what=" + e.what());
        }
        return caughtCorrectCode;
    }

    bool terminates(const function<void()>& fn) {
        // Need to ignore SIGCHLD for this test to work. We restore after the test. For some
        // reason we need to specify an empty signal handler and not just SIG_IGN.
        sig_t oldHandler = signal(SIGCHLD, my_signal_handler);

        // In the child process we set a terminate handler that will exit the process with a 0.
        pid_t pid = fork();
        if (pid == 0) {
            for (int i = 1; i <= 255; ++i)
                signal(i, my_signal_handler);
            set_terminate(my_terminate_handler);
            try {
                fn();
            }
            catch (...) {
                _exit(2);   // Error, an exception was thrown that did not cause a terminate().
            }
            _exit(1);       // Error, lambda exited without causing a terminate().
        }

        // In the parent process we wait for the child to exit, then see if it exited with
        // a 0 status.
        else {
            int status = -9;    // Any non-zero state.
            waitpid(pid, &status, 0);
            signal(SIGCHLD, oldHandler);
            if (WIFEXITED(status) && WEXITSTATUS(status) == 0)
                return true;   // success
            return false;       // failure
        }
    }
}}


// MARK: TestSuite Implementation

TestSuite::TestSuite(const string& testSuiteName,
                     initializer_list<pair<string, test_case_fn>> fns)
: _impl(new Impl())
{
    _impl->parent = this;
    _impl->name = testSuiteName;

    for (auto& p : fns) {
        TestCaseWrapper wrapper;
        wrapper.owner = this;
        wrapper.name = p.first;
        wrapper.fn = p.second;
        _impl->tests.push_back(move(wrapper));
    }
    sort(_impl->tests.begin(), _impl->tests.end());

    TestSuiteWrapper wrapper;
    wrapper.suite = this;
    testSuites()->push_back(wrapper);
}

TestSuite& TestSuite::operator=(TestSuite&& ts) {
    if (this != &ts) {
        _impl = move(ts._impl);
        ts._impl.reset();
    }
    return *this;
}

TestSuite::~TestSuite() noexcept {}
TestSuite::TestSuite(TestSuite&& ts) : _impl(move(ts._impl)) {}

const string& TestSuite::name() const noexcept {
    return _impl->name;
}

TestSuite& TestSuite::get() noexcept {
    assert(currentSuite != nullptr);     // Fails if called in a new thread.
    assert(currentSuite->suite != nullptr);
    return *(currentSuite->suite);
}

TestSuite::test_case_context_t TestSuite::testCaseContext() const noexcept {
    assert(currentTest != nullptr);     // Fails if called in a new thread.
    return currentTest;
}

void TestSuite::setTestCaseContext(test_case_context_t ctx) noexcept {
    assert(ctx != nullptr);             // User must not set a null value.
    currentTest = static_cast<TestCaseWrapper*>(ctx);
}


// MARK: _private Implementation

namespace kss { namespace test { namespace _private {

    void success(void) noexcept {
        assert(currentTest != nullptr);
        ++currentTest->assertions;
        if (isVerboseMode) {
            cout << ".";
        }
    }

    void failure(const char* expr, const char* filename, unsigned int line) noexcept {
        assert(currentTest != nullptr);
        ++currentTest->assertions;
        auto f = make_pair(basename(filename) + ": " + to_string(line) + ", " + expr,
                           currentTest->mostRecentDetails);
        if (f.first.size() > maxFailureReportLineLength) {
            f.first.resize(maxFailureReportLineLength);
            f.first.append("...");
        }
        currentTest->failures.push_back(move(f));
        currentTest->mostRecentDetails = "";
        if (isVerboseMode) {
            cout << "F";
        }
    }

    bool completesWithinSec(const duration<double>& d, const function<void()>& fn) {
        const auto dur = timeOfExecution(fn);
        const auto ret = (dur <= d);
        if (!ret) {
            _private::setFailureDetails("actual duration was " + to_string(dur.count()) + "s");
        }
        return (dur <= d);
    }

    void setFailureDetails(const string& d) {
        assert(currentTest != nullptr);
        currentTest->mostRecentDetails = d;
    }

    string demangleName(const char* mangledName) {
        int status;
        return abi::__cxa_demangle(mangledName, 0, 0, &status);
    }
    
}}}
