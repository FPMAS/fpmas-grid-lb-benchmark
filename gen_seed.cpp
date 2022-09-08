#include <random>
#include <iostream>

int main(int argc, char** argv)
{
	std::size_t n = std::stoul({argv[1]});
	std::vector<std::seed_seq::result_type> seeds(n);
	for(std::size_t i = 0; i < n; i++)
		seeds[i] = i;

	std::seed_seq seq(seeds.begin(), seeds.end());
	seq.generate(seeds.begin(), seeds.end());
	
	for(auto seed : seeds)
		std::cout << seed << " ";
}
