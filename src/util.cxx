#include "include/util.hxx"
#include "include/util_options.hxx"

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

	bool is_verbose = options.get<bool>("verbose");

	conf.setGlobally(
		el::ConfigurationType::ToStandardOutput,
		(is_verbose ? "true" : "false"));

	conf.setGlobally(
		el::ConfigurationType::Enabled,
		(is_verbose ? "true" : "false"));

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

	std::vector<std::string> args;
	daemon_->set_arguments(args);

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
			std::cout << "## WARNING: Daemon not ready, "
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

			int wait_loops = 20;

			while(kill(daemon_pid, 0) == 0 && wait_loops) {
				std::cout << ".";
				usleep(500 * 1000);
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


} // test
