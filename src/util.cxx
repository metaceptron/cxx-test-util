#include "include/util.hxx"
#include "include/util_options.hxx"

#include <iostream>
#include <sys/wait.h>
#include <errno.h>
#include <cstdlib>
#include <signal.h>
#include <unistd.h>

#if defined(EASYLOGGINGPP_H)
#undef ELPP_STACKTRACE_ON_CRASH

INITIALIZE_EASYLOGGINGPP

namespace test {

void crash_handler(int sig)
{
	LOG(ERROR) << "Crashed";
	el::Helpers::logCrashReason(sig, true);
	el::Helpers::crashAbort(sig);
}

void initialize_test_env(const Options& options)
{
	el::Configurations conf;

	bool is_verbose = options.is_verbose();

	conf.setGlobally(
		el::ConfigurationType::ToStandardOutput,
		(is_verbose ? "true" : "false"));

	conf.setGlobally(
		el::ConfigurationType::Enabled,
		(is_verbose ? "true" : "false"));

	std::string logfile = options.get<std::string>("logfile");
	if(!logfile.empty()) {
		conf.setGlobally(el::ConfigurationType::Filename, logfile);
	}

	el::Loggers::addFlag(el::LoggingFlag::AutoSpacing);
	el::Loggers::addFlag(el::LoggingFlag::ImmediateFlush);
	el::Helpers::setCrashHandler(crash_handler);
	el::Loggers::reconfigureAllLoggers(conf);

};

} // test

#else

namespace test {
void crash_handler(int sig)
{
}


void initialize_test_env(const Options&)
{
}


} // namespace test

#endif



// test util continues
namespace test {

TestDaemon::TestDaemon(bool verbose)
	: is_verbose_(verbose)
{
}

bool TestDaemon::is_verbose() const
{
	return is_verbose_;
}

void TestDaemon::set_no_cleanup(bool state)
{
	no_cleanup_ = state;
}

ProcessTest::ProcessTest(std::unique_ptr<TestDaemon> daemon,
						 UnitTest& unit_test)
	: daemon_(std::move(daemon)),
	  unit_test_(unit_test)
{
}

int ProcessTest::run(int argc, char** argv)
{
	Options options;
	if(!options.parse(argc, argv)) {
		return -1;
	}

	if(options.has_help()) {
		options.display_help();
		return 0;
	}

	daemon_->set_no_cleanup(options.get<bool>("no-cleanup"));

	std::vector<std::string> args;
	daemon_->set_arguments(args);

	bool verbose_run = (options.is_verbose() || daemon_->is_verbose());

	int test_result = -1;
	int status;
	pid_t pid = fork();

	if(pid == -1) { 
		std::cerr << "ProcessTest::run(): fork() failed" << std::endl;
		return -1;
	}
	else if(pid == 0) { // child
		char** arguments = new char* [args.size() + 1];

		for(size_t i = 0; i < args.size(); i++) {
			arguments[i] = (char*) args[i].c_str();
		}
		arguments[args.size()] = (char*)NULL;

		std::string program = args[0];

		std::cout << "## Starting daemon" << std::endl;
		if(options.is_debug() || daemon_->is_verbose()) {
			for(auto& it :args) {
				std::cout << it << "\n";
			}
			std::cout << std::endl;
		}
		if(execv(program.c_str(), arguments) == -1) {
			std::cerr << "execv() failed: " << strerror(errno) << std::endl;
			return false;
		}
		_exit(0);
	}
	else { // parent
		// wait for our own child
		pid_t wpid = wait(&status);

		if(daemon_->is_ready()) {
			std::cout << "## Daemon ready, running test cases" << std::endl;
			test_result = unit_test_.run(options);
		}
		else {
			std::cerr << "## ERROR: Daemon not ready, "
					  << "test cases will not be run" << std::endl;
		}

		pid_t daemon_pid = daemon_->get_pid();

		if(daemon_pid) {
			std::cout << "## Stopping daemon";
			int kill_status = kill(daemon_pid, SIGTERM);
			if(kill_status == -1) {
				std::cerr << "\nkill(" << daemon_pid << ", TERM) failed: "
						  << strerror(errno) << std::endl;
			}

			int wait_loops = 50;

			while(kill(daemon_pid, 0) == 0 && wait_loops) {
				if(verbose_run) {
					std::cout << ".";
				}
				usleep(100 * 1000);
				wait_loops--;
			}

			std::cout << std::endl;

			if(kill(daemon_pid, 0) == 0) {
				std::cerr << "Resorting to SIGKILL..." << std::endl;
				kill(daemon_pid, SIGKILL);
			}
		}
	}

	return test_result;
}

UnitTest::UnitTest(const Configuration& config)
	: config_(config)
{
}

void UnitTest::test_case(const std::string& name,
						 TestCase::TestFunction_t code)
{
	cases_.push_back(std::make_pair(name, code));
}

int UnitTest::run(int argc, char** argv)
{
	Options options;
	if(!options.parse(argc, argv)) {
		return -1;
	}

	if(options.has_help()) {
		options.display_help();
		return 0;
	}

	return run(options_);
}

int UnitTest::run(const Options& options)
{
	config_.verbose = options.is_verbose();

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

} // test
