/* 
 * (C) Copyright 2001 Diomidis Spinellis.
 *
 * For documentation read the corresponding .h file
 *
 * $Id: fileid.cpp,v 1.20 2002/09/17 16:20:37 dds Exp $
 */

#include <map>
#include <iostream>
#include <string>
#include <map>
#include <list>
#include <deque>
#include <set>
#include <vector>
#include <algorithm>
#ifdef unix
#include <unistd.h>		// access(2)
#else
#include <io.h>			// access(2)
#endif

#include "cpp.h"
#include "debug.h"
#include "metrics.h"
#include "attr.h"
#include "fileid.h"
#include "tokid.h"
#include "error.h"

int Fileid::counter;		// To generate ids
FI_uname_to_id Fileid::u2i;	// From unique name to id
FI_id_to_details Fileid::i2d;	// From id to file details
Fileid Fileid::anonymous = Fileid("ANONYMOUS", 0);
list <string> Fileid::ro_prefix;	// Read-only prefix

// Clear the maps
void
Fileid::clear()
{
	u2i.clear();
	i2d.clear(); 
	Fileid::anonymous = Fileid("ANONYMOUS", 0);
}

#ifdef WIN32
#include <windows.h>

// Return a Windows error code as a string
static string
werror(LONG err)
{
	LPVOID lpMsgBuf;

	FormatMessage( 
	    FORMAT_MESSAGE_ALLOCATE_BUFFER | 
	    FORMAT_MESSAGE_FROM_SYSTEM | 
	    FORMAT_MESSAGE_IGNORE_INSERTS,
	    NULL,
	    err,		// GetLastError() does not seem to work reliably
	    MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), // Default language
	    (LPTSTR) &lpMsgBuf,
	    0,
	    NULL 
	);
	string s((const char *)lpMsgBuf);
	LocalFree(lpMsgBuf);
	return s;
}

static const char *
get_uniq_fname_string(const char *name)
{
	static char buff[4096];
	LPTSTR nptr;

	int n = GetFullPathName(name, sizeof(buff), buff, &nptr);
	if (n > sizeof(buff) || n == 0) {	// No space or other error!
		string s = string(name) + ": " + werror(GetLastError());
		Error::error(E_FATAL, "Unable to get path of file " + s, false);
	}
	return (buff);
}

static const char *
get_full_path(const char *name)
{
	return (get_uniq_fname_string(name));
}
#endif /* WIN32 */

#ifdef unix
#include <sys/types.h>
#include <sys/stat.h>
#include <stdio.h>
#include <unistd.h>

static char *
get_uniq_fname_string(const char *name)
{
	static char buff[128];
	struct stat sb;

	if (stat(name, &sb) != 0) {
		string s(name);
		Error::error(E_FATAL, "Unable to stat file " + s, false);
	}
	sprintf(buff, "%ld:%ld", (long)sb.st_dev, (long)sb.st_ino);
	if (DP())
		cout << "uniq fname " << name << " = " << buff << "\n";
	return (buff);
}

static const char *
get_full_path(const char *name)
{
	static char buff[4096];
	char wd[4096];

	if (name[0] != '/') {
		// Relative path
		getcwd(wd, sizeof(wd));
		sprintf(buff, "%s/%s", wd, name);
		return (buff);
	} else
		return (name);
}
#endif /* unix */


// Return true if file fname is read-only
bool
Fileid::is_readonly(string fname)
{
	for (list <string>::const_iterator i = ro_prefix.begin(); i != ro_prefix.end(); i++)
		if(fname.compare((*i), 0, (*i).length()) == 0)
			return true;
	if (access(fname.c_str(), W_OK) != 0)
		return true;
	return false;
}

Fileid::Fileid(const string &name)
{
	// String identifier
	string sid(get_uniq_fname_string(name.c_str()));
	FI_uname_to_id::const_iterator i;

	if ((i = u2i.find(sid)) == u2i.end()) {
		// New filename; add a new fname/id pair in the map table
		string fpath(get_full_path(name.c_str()));
		u2i[sid] = counter;
		i2d.push_back(Filedetails(fpath, is_readonly(name)));
		id = counter++;
	} else
		// Filename exists; our id is the one from the map
		id = (*i).second;
}

// User for initialization and testing; not for real files
Fileid::Fileid(const string &name, int i)
{
	u2i[name] = i;
	i2d.resize(i + 1);
	i2d[i] = Filedetails(name, true);
	id = i;
	counter = i + 1;
}

const string&
Fileid::get_path() const
{
	return i2d[id].get_name();
}


bool 
Fileid::get_readonly() const
{
	return i2d[id].get_readonly();
}

void
Fileid::set_readonly(bool r)
{
	i2d[id].set_readonly(r);
}

// Return a sorted list of all filenames used
vector <Fileid> 
Fileid::sorted_files()
{
	vector <Fileid> r(i2d.size() - 1);

	for (int i = 0; i < r.size(); i++)
		r[i] = Fileid(i + 1);
	sort(r.begin(), r.end(), fname_order());
	return (r);
}

#ifdef UNIT_TEST
// cl -GX -DWIN32 -DUNIT_TEST fileid.cpp kernel32.lib

#include <iostream>

main()
{
	Fileid x1;
	Fileid x2;
	Fileid a("fileid.cpp");
	Fileid b("./fileid.cpp");
	Fileid c(".");
	Fileid d = b;
	Fileid e(c);

	cout << "a=" << a.get_path() << " (should be fileId.cpp)\n";
	cout << "b=" << b.get_path() << " (should be fileId.cpp)\n";
	cout << "c=" << c.get_path() << " (should be .)\n";
	cout << "d=" << d.get_path() << " (should be fileId.cpp)\n";
	cout << "e=" << e.get_path() << " (should be .)\n";
	cout << "a==b: " << (a == b) << " (should be 1)\n";
	cout << "size=" << sizeof(a) << " (it better be 4)\n";
	cout << "x2=" << x2.get_path() << " (should be ANONYMOUS)\n";
	cout << "x1==x2: " << (x1 == x2) << " (should be 1)\n";
	return (0);
}
#endif
