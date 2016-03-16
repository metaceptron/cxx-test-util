#ifndef TEST_TOOLS_UTIL_HXX
#define TEST_TOOLS_UTIL_HXX

#include <memory>
#include <functional>
#include <iostream>
#include <iomanip>
#include <string>
#include <map>
#include <exception>
#include <stdexcept>
#include <vector>

#include "util_options.hxx"


namespace test {

void crash_handler(int sig);

void initialize_test_env(const Options& options);

class AssertionFailed : public std::exception {};


struct Configuration
{
	bool print_exceptions = true;
	bool verbose = false;
};

class TestCase
{
public:
	typedef std::function<void(TestCase&)> TestFunction_t;
	typedef std::function<void()> Callable_t;


	TestCase() = delete;
	TestCase(const Configuration& config, const std::string& unit_name)
		: config_(config),
		  unit_name_(unit_name)
	{
	}

	TestCase(const std::string& unit_name = std::string())
		: config_(Configuration()),
		  unit_name_(unit_name)
	{
	}

	virtual ~TestCase() = default;

	size_t failures_count() const
	{
		size_t failures = 0;
		for(auto& it : results_) {
			if(!it.second) {
				failures++;
			}
		}
		return failures;
	}

	template <class T>
	bool equal(const T& result,
				const T& expected,
				const std::string& name = "equal")
	{
		print_name(name);

		bool ok = (result == expected);
		if(!ok) {
			print_diff(result, expected, name);
		}
		store_result(ok, name);
		return ok;
	}

	template <class T>
	bool equal(const std::string& result,
			   const std::string& expected,
			   const std::string& name = "equal")
	{
		print_name(name);

		int different = (result.compare(expected));
		if(different) {
			print_diff(result, expected, name);
		}
		store_result(!different, name);
		return !different;
	}

	template <class T>
	bool not_equal(const T& result,
				   const T& expected,
				   const std::string& name = "not_equal")
	{
		print_name(name);

		bool ok = (result != expected);
		if(!ok) {
			print_diff(result, expected, name);
		}
		store_result(ok, name);
		return ok;
	}

	template <class T>
	bool not_equal(const std::string& result,
			   const std::string& expected,
			   const std::string& name = "equal")
	{
		print_name(name);

		bool ok = (result.compare(expected) != 0);
		store_result(ok, name);
		return ok;
	}

	bool no_throw(Callable_t callable, const std::string& name = "no_throw")
	{
		return no_throw([&](TestCase&) { callable(); }, name);
	}

	bool no_throw(TestFunction_t code, const std::string& name = "no_throw")
	{
		print_name(name);

		bool passed = false;
		try {
			code(*this);
		}
		catch(const std::exception& e) {
			passed = false;
			store_result(true, name);
			print_error(
				name,
				"Threw: " + std::string(typeid(e).name()) + "\n"
				+ std::string(e.what())
			);
		}

		return passed;
	}

	bool throws(Callable_t callable,
				const std::string& name = "throws")
	{
		return throws([&](TestCase&) { callable(); }, name);
	}

	bool throws(TestFunction_t code,
				const std::string& name = "throws")
	{
		print_name(name);

		bool threw = false;

		try {
			code(*this);
		}
		catch(const std::exception& e) {
			threw = true;
		}

		store_result(threw, name);
		print_error(name, "Did not throw");

		return threw;
	}

	template <class Exception_t>
	bool throws(Callable_t callable, const std::string& name = "throws")
	{
		return throws<Exception_t>([&](TestCase&) { callable(); }, name);
	}

	template <class Exception_t>
	bool throws(TestFunction_t code, const std::string& name = "throws")
	{
		print_name(name);

		bool passed = false;
		bool threw = false;
		std::string wrong_exception_type;
		std::string wrong_exception_msg;

		try {
			code(*this);
		}
		catch(const Exception_t& e) {
			passed = true;
		}
		catch(...) {
			threw = true;
			std::exception_ptr eptr = std::current_exception(); 
			try {
				std::rethrow_exception(eptr);
			}
			catch(const std::exception& e) {
				wrong_exception_type = typeid(e).name();
				wrong_exception_msg = e.what();
			}
		}

		std::string excpt_type(typeid(Exception_t).name());
		std::string case_name("throws<" + excpt_type + ">");
		store_result(passed, case_name);
		if(!passed) {
			if(!threw) {
				print_error(case_name, "Did not throw " + excpt_type);
			}
			else {
				print_error(
					case_name,
					"\n\tThrew: " + wrong_exception_type + ", what():\n"
					+ wrong_exception_msg
				);
			}
		}

		return passed;
	}

	template <class T>
	void assert_equal(const T& result,
					   const T& expected,
					   const std::string& name = "assert_equal")
	{
		if(!equal(result, expected, name)) {
			throw AssertionFailed();
		}
	}

	template <class T>
	void assert_equal(const std::string& result,
					  const std::string& expected,
					  const std::string& name = "assert_equal")
	{
		if(!equal(result, expected, name)) {
			throw AssertionFailed();
		}
	}

	template <class T>
	void assert_not_equal(const T& result,
						  const T& expected,
						  const std::string& name = "assert_not_equal")
	{
		if(!not_equal(result, expected, name)) {
			throw AssertionFailed();
		}
	}

	void assert_no_throw(Callable_t callable, const std::string& name = "assert_no_throw")
	{
		if(!no_throw([&](TestCase&) { callable(); }, name)) {
			throw AssertionFailed();
		}
	}

	void assert_no_throw(TestFunction_t code, const std::string& name = "assert_no_throw")
	{
		if(!no_throw(code, name)) {
			throw AssertionFailed();
		}
	}


	void assert_throws(Callable_t callable,
					   const std::string& name = "assert_throws")
	{
		if(!throws([&](TestCase&) { callable(); }, name)) {
			throw AssertionFailed();
		}
	}

	void assert_throws(TestFunction_t code,
					   const std::string& name = "assert_throws")
	{
		if(!throws(code, name)) {
			throw AssertionFailed();
		}
	}

	template <class Exception_t>
	void assert_throws(Callable_t callable, const std::string& name = "assert_throws")
	{
		if(!throws<Exception_t>([&](TestCase&) { callable(); }, name)) {
			throw AssertionFailed();
		}
	}

	template <class Exception_t>
	void assert_throws(TestFunction_t code, const std::string& name = "assert_throws")
	{
		if(!throws<Exception_t>(code, name)) {
			throw AssertionFailed();
		}
	}

	void assert_true(bool v, const std::string& name = "assert_true")
	{
		print_name(name);

		store_result(v, name);
		if(!v) {
			print_error(name, "Did not return true");
			throw AssertionFailed();
		}
	}

	void assert_false(bool v, const std::string& name = "assert_true")
	{
		print_name(name);

		store_result(!v, name);
		if(v) {
			print_error(name, "Did not return true");
			throw AssertionFailed();
		}
	}

protected:
	const Configuration config_;
	const std::string unit_name_;
	std::map<std::string, int> results_;

	void  print_name(const std::string& name)
	{
		if(config_.verbose) {
			std::cout << "\n\t... " << name;
		}
	}

	template<class T>
	void  print_diff(const T& result,
					 const T& expected,
					 const std::string& name)
	{
		if(!config_.verbose) {
			return;
		}

		std::cout << "\n\t> Failed case: '"
				  << (name.empty() ? "..." : name) << "'"
				  << "\n\tRESULT: " << result
				  << "\n\tEXPECT: " << expected
				  << std::endl;
	}

	void  print_error(const std::string& name, const std::string& error)
	{
		if(!config_.verbose) {
			return;
		}

		std::cout << "\n\t> Failed case: '"
				  << (name.empty() ? "..." : name) << "'"
				  << "\n\t> Reason: " << error
				  << std::endl;
	}

	void store_result(bool passed, const std::string& name = std::string())
	{
		static int result_id = 0;
		result_id++;
		std::string test_name(name.empty() ? unit_name_ + "test" : name);
		test_name.append("_" + std::to_string(result_id));
		results_[test_name] = passed;
	}
};



class UnitTest
{
public:
	UnitTest(const Configuration& config = Configuration())
		: config_(config)
	{
	}

	void test_case(const std::string& name,
				   TestCase::TestFunction_t code)
	{
		cases_.push_back(std::make_pair(name, code));
	}

	int run(int argc, char** argv)
	{
		Options options;
		if(!options.parse(argc, argv)) {
			return -1;
		}

		if(options.has_help()) {
			options.display_help();
			return 0;
		}

		return run(options);
	}

	int run(const Options& options)
	{
		config_.verbose = options.get<bool>("verbose");

		initialize_test_env(options);

		int i = 0;
		for(auto& it : cases_) {
			try	{
				i++;
				std::cout << std::setw(3) <<  std::left << i << " - "
						  << std::setw(59) << it.first << " - ";

				TestCase tcase(config_, it.first);
				try {
					it.second(tcase);
				}
				catch(const AssertionFailed& e) {
					failed_.push_back(it.first);
					std::cout << "ASSERTION FAILED" << std::endl;
					break;
				}

				size_t failures = tcase.failures_count();
				if(failures) {
					failed_.push_back(it.first);
				}
				
				std::cout << (config_.verbose ? "\n" : "")
						  << (failures ? "FAIL" : "PASS") << std::endl;
			}
			catch(const std::exception& e) {
				failed_.push_back(it.first);
				std::cout << "FAIL: " << it.first << "\n"
						  << "\t" << e.what() << std::endl;
			}
		}

		return failed_.size();
	}

protected:
	Configuration config_;
	std::vector<std::pair<std::string, TestCase::TestFunction_t>> cases_;
	std::vector<std::string> failed_;
};


class TestDaemon
{
public:
	virtual void set_arguments(std::vector<std::string>& args) = 0;
	virtual bool is_ready() const { return true; }
	virtual pid_t get_pid() const = 0;
};

class ProcessTest
{
public:
	ProcessTest() = delete;
	ProcessTest(std::unique_ptr<TestDaemon> daemon,
				UnitTest& unit_test);
	~ProcessTest() = default;

	int run(int argc, char** argv);
private:
	std::unique_ptr<TestDaemon> daemon_;
	UnitTest unit_test_;
};


} // test

#endif
