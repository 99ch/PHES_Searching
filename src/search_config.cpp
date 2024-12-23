#include "search_config.hpp"
#include <algorithm>

// Global instance of SearchConfig
SearchConfig search_config;

// Function to format a string for use as a filename
string format_for_filename(string s){
	// Replace spaces with underscores
	replace(s.begin(), s.end(), ' ' , '_');
	// Remove double quotes from the string
	s.erase(remove(s.begin(), s.end(), '"'), s.end());
	return s;
}
