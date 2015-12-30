#include "util_options.hxx"
#include <exception>
#include <iomanip>


namespace test {


void Options::dump_variables_map() const
{
	for (const auto& it : vm_) {
		std::cout << ": "  << std::setw(48) << std::left << it.first;
		std::cout.width(0);
		auto& raw = it.second.value();
		std::string v;
		if (auto v = boost::any_cast<int>(&raw)) {
			std::cout << *v;
		}
		else if (auto v = boost::any_cast<std::string>(&raw)) {
			std::cout << *v;
		}
		else if (auto v = boost::any_cast<bool>(&raw)) {
			std::cout << *v;
		}

		std::cout << std::endl;
	}
}

bool Options::parse(int argc, char** argv)
{
	using namespace std;

	try {
		po::options_description options("Generic");
		string stropt;
		int intopt;
		bool boolopt;

		options.add_options()
			("help,h", "display this help")
			("verbose,v",
			 po::value<bool>()->implicit_value(true)->zero_tokens()->default_value(false),
			 "verbose run")
			;

		all_.add(options);
		po::store(po::command_line_parser(argc, argv).options(all_).run(), vm_);
		po::notify(vm_);
	}
	catch(exception& e) {
		cerr << "error: " << e.what() << "\n";
		return false;
	}
	return true;
}

bool Options::has_help() const
{
	return vm_.count("help");
}

void Options::display_help() const
{
	std::cout << all_ << std::endl;
}

} // test
