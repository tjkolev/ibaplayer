#ifdef HAVE_CONFIG_H
#	include <config.h>
#endif

#include <file_t.h>

namespace filesystem
	{
		file_t::file_t (const string& name)
			: name (name)
			{
			}

		const string&
		file_t::getName () const
			{
				return name ;
			}

		bool
		file_t::operator== (const file_t& other) const
			{
				return (name == other.name) ;
			}

		std::ostream& operator<< (std::ostream& out, const file_t& f)
			{
				out << f.getName () ;
				return out ;
			}
	}	// namespace filesystem
