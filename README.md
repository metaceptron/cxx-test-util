# minimal cxx-test-util

Depends on boost::program_options.


```cpp
using namespace test;
Configuration config;
UnitTest unit(config);

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
```

