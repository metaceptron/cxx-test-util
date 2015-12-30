#include "tools/util.hxx"
#include <string>
#include <vector>

int main(int argc, char** argv)
{
	using namespace test;
	Configuration config;
	UnitTest unit(config);

	unit.test_case(
		"Basic", 
		[](TestCase& test)
		{
			std::string payload("some text");
			std::string expected(payload);
			test.equal(payload, expected, "strings are equal");
			
			expected = "different text";
			test.not_equal(payload, expected, "strings are different");
		}
	);


	int i_value = 100;

	unit.test_case(
		"Integer", 
		[&i_value](TestCase& test)
		{
			std::vector<int> values {10, 20, 30};
			for(auto v : values) {
				test.not_equal(v, i_value);
			}
		}
	);

	unit.test_case(
		"Failing", 
		[&i_value](TestCase& test)
		{
			std::vector<int> values {10, 20, 30};
			for(auto v : values) {
				test.equal(v, i_value);
			}
		}
	);


	return unit.run(argc, argv);
}
