/* 
 * (C) Copyright 2001 Diomidis Spinellis.
 *
 * For documentation read the corresponding .h file
 *
 * $Id: fileid.cpp,v 1.13 2002/09/04 17:50:36 dds Exp $
 */

#include <map>
#include <iostream>
#include <string>
#include <map>
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
#include "fileid.h"
#include "tokid.h"
#include "error.h"

int Fileid::counter;		// To generate ids
FI_uname_to_id Fileid::u2i;	// From unique name to id
FI_id_to_details Fileid::i2d;	// From id to file details
Fileid Fileid::anonymous = Fileid("ANONYMOUS", 0);

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

static const char *
get_uniq_fname_string(const char *name)
{
	static char buff[4096];
	LPTSTR nptr;

	if (GetFullPathName(name, sizeof(buff), buff, &nptr) != 0) {
		string s(name);
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


Fileid::Fileid(const string &name)
{
	// String identifier
	string sid(get_uniq_fname_string(name.c_str()));
	FI_uname_to_id::const_iterator i;

	if ((i = u2i.find(sid)) == u2i.end()) {
		// New filename; add a new fname/id pair in the map table
		string fpath(get_full_path(name.c_str()));
		u2i[sid] = counter;
		i2d[counter] = Filedetails(fpath, access(name.c_str(), W_OK) != 0);
		id = counter++;
	} else
		// Filename exists; our id is the one from the map
		id = (*i).second;
}

// User for initialization and testing; not for real files
Fileid::Fileid(const string &name, int i)
{
	u2i[name] = i;
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

struct fname_order {
      bool operator()(Fileid a, Fileid b) { 
	      return a.get_path() < b.get_path();
      }
};

// Return a sorted list of all filenames used
vector <Fileid> 
Fileid::sorted_files()
{
	vector <Fileid> r(i2d.size() - 1);
	FI_id_to_details::const_iterator i;
	int j;

	for (i = i2d.begin(), j = 0; i != i2d.end(); i++)
		if ((*i).first != 0)	// All but the anonymous entry
			r[j++] = ((*i).second.get_name());
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
