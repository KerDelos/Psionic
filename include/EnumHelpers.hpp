#pragma once

#include <string>
#include <optional>

using namespace std;

struct ci_less //code from https://stackoverflow.com/questions/1801892/how-can-i-make-the-mapfind-operation-case-insensitive
{
    // case-independent (ci) compare_less binary function
    struct nocase_compare
    {
        bool operator() (const unsigned char& c1, const unsigned char& c2) const {
            return tolower (c1) < tolower (c2); 
        }
    };
    bool operator() (const std::string & s1, const std::string & s2) const {
        return std::lexicographical_compare 
        (s1.begin (), s1.end (),   // source range
        s2.begin (), s2.end (),   // dest range
        nocase_compare ());  // comparison
    }
};

struct ci_equal
{
	bool operator() (const std::string & s1, const std::string & s2)
	{
		ci_less comparator;
		return !comparator(s1,s2) && !comparator(s2,s1);
	}
};


template <class E>
std::optional<E> str_to_enum(string p_str, map<string,E,ci_less> p_mapping)
{
	auto it = p_mapping.find(p_str);
	if(it != p_mapping.end())
	{
		return optional<E>(it->second);
	}
	else
	{
		return std::nullopt;
	}
	
}

template <class E> //should throw exception instead ?
optional<string> enum_to_str(E p_enum_value, map<string,E,ci_less> p_mapping)
{
	for(auto pair : p_mapping)
	{
		if(pair.second == p_enum_value)
		{
			return optional<string>(pair.first);
		}
	}

	return std::nullopt;
}