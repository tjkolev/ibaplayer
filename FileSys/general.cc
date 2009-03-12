#ifdef HAVE_CONFIG_H
#	include <config.h>
#endif

#include <general.h>

#include <sys/stat.h>
#include <unistd.h>

namespace filesystem
	{
		bool
		isDirectory (const string& filename)
			{
				struct stat statbuf ;

				if (lstat (filename.c_str (), &statbuf) != 0)
					return false ;	// could not stat file

				return S_ISDIR(statbuf.st_mode) ;
			}

		const string
		getWorkingDirectory ()
			{
				string wd ;

#				ifdef _GNU_SOURCE

					char * wd_cstr = get_current_dir_name () ;

					if (wd_cstr)
						{
							wd = wd_cstr ;
							delete wd_cstr ;
						}
					else
						wd = "." ;

#				else

					const int maxbufsize = 2048 ;
					char buf[maxbufsize] ;

					if (getcwd (buf, maxbufsize) != 0)
						{
							wd = buf ;
						}
					else
						wd = "." ;

#				endif

				return wd ;
			}

		int
		getFileSize (const string& filename)
			{
				struct stat statbuf ;

				if (stat (filename.c_str (), &statbuf) != 0)
					return -1 ;	// could not stat file

				return statbuf.st_size ;
			}

		/**
		 * \todo Handle lines that are longer than 1023 characters.
		 */
		void
		getLine (std::istream& in, string& buf)
			{
				const int blocksize = 1024 ;
				char ibuf [blocksize] ;	// internal buffer

				buf = "" ;

				in.getline (ibuf, blocksize) ;

				buf += ibuf ;
			}

	}	// namespace filesystem
