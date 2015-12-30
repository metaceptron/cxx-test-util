#ifndef TEST_TOOLS_UTIL_OPTIONS_HXX
#define TEST_TOOLS_UTIL_OPTIONS_HXX

#include <boost/program_options.hpp>
#include <string>


namespace test {


namespace po = boost::program_options;

class Options
{
public:
	Options() = default;
	virtual ~Options() = default;

	template <class T>
	const T get(const std::string& k) const
	{
		try {
			return vm_[k].as<T>();
		}
		catch(boost::bad_any_cast& e) {
			std::cerr << "ERROR: Options::get(): " << k << std::endl;
			throw e;
		}
	}

	void dump_variables_map() const;

	bool parse(int argc, char** argv);
	bool has_help() const;
	void display_help() const;

	inline bool is_verbose() const
	{
		return (vm_["debug"].as<bool>() || vm_["verbose"].as<bool>());
	}

protected:
	po::variables_map vm_;
	po::options_description all_;

};


} // test

#endif
