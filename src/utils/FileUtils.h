/*
 * FileUtils.h
 *
 * @Date: 26.08.2010
 * @Author: Wolfgang Eckhardt
 */

#ifndef FILEUTILS_H_
#define FILEUTILS_H_

#include <string>
#include <cstddef>

/**
 * Check if a file exists.
 */
bool fileExists(const char* fileName);

/*
 * Get the file name extension marked by the last '.' in the filename.
 */
std::string getFileExtension(const char* fileName);

/**
 * Delete a file from the system.
 */
void removeFile(const char* fileName);

/**
 * Retrieve the size of a file.
 *
 * @return 0 if an error occurred retrieving the size, so the user has to check
 *         if the file exists.
 */
unsigned int getFileSize(const char* fileName);

/**
 * Adding string of numbers with leading zeros to stream (e.g. simstep in output filename)
 */
struct fill_width
{
	fill_width( char f, uint8_t w )
		: fill(f), width(w) {}
	char fill;
	int width;
};

std::ostream& operator<<( std::ostream& o, const fill_width& a );

/*
 * Split / Tokenize a string
 * copied from here: http://www.cplusplus.com/faq/sequences/strings/split/
 *
 * call e.g. like this:
 *
 * std::vector<string> fields;
 * std::string str = "split:this:string";
 * fields = split( fields, str, ":", split_type::no_empties );
 *
 */

struct split_type
{
	enum empties_t { empties_ok, no_empties };
};

template <typename Container>
Container& split(
	Container&                            result,
	const typename Container::value_type& s,
	const typename Container::value_type& delimiters,
	split_type::empties_t                      empties = split_type::empties_ok )
	{
	result.clear();
	size_t current;
	size_t next = -1;
	do
	{
		if (empties == split_type::no_empties)
		{
			next = s.find_first_not_of( delimiters, next + 1 );
			if (next == Container::value_type::npos) break;
			next -= 1;
		}
		current = next + 1;
		next = s.find_first_of( delimiters, current );
		result.push_back( s.substr( current, next - current ) );
	}
	while (next != Container::value_type::npos);
	return result;
}

#endif /* FILEUTILS_H_ */
