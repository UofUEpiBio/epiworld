#include <iostream>
#include <vector>

int main()
{
	unsigned int i = 0u;
	std::vector< int > V({1,2,3});
	for (auto & v : V)
		std::cout << "[" << i++ << "] " << v << std::endl;

	return 0;

}
