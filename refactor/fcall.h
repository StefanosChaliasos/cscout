/* 
 * (C) Copyright 2003 Diomidis Spinellis.
 *
 * Function call graph information
 *
 * $Id: fcall.h,v 1.6 2003/12/05 07:42:31 dds Exp $
 */

#ifndef FCALL_
#define FCALL_

/*
 * Function call information is always associated with Id objects
 * It is thus guaranteed to match the symbol table lookup operations
 */

// C function calling information
class FCall {
private:

	static FCall *current_fun;	// Function currently being parsed

	// Container for storing all declared functions
	typedef set <FCall *> fun_container;
	static fun_container all;	// Set of all functions

	string name;
	Tokid declaration;		// Function's first declaration 
					// (could also be reference if implicitly declared)
	Tokid definition;		// Function's definition
	Type type;			// Function's type
	bool defined;			// True if the function has been defined
	fun_container call;		// Functions this function calls
	fun_container caller;		// Functions that call this function
	bool visited;			// For calculating transitive closures
	void add_call(FCall *f) { call.insert(f); }
	void add_caller(FCall *f) { caller.insert(f); }
public:
	// Set the funciton currently being parsed
	static void set_current_fun(const Type &t);
	static void set_current_fun(const Id *id);
	// Called when outside a function body scope
	static void unset_current_fun() { current_fun = NULL; }
	// The current function makes a call to f
	static void register_call(FCall *f);

	// Clear the visit flags for all functions
	static void clear_visit_flags();

	// Interface for iterating through all functions
	typedef fun_container::const_iterator const_fiterator_type;
	static const_fiterator_type fbegin() { return all.begin(); }
	static const_fiterator_type fend() { return all.end(); }

	Tokid get_declaration() const { return declaration; }
	Tokid get_definition() const { return definition; }
	const string &get_name() const { return name; }
	bool contains(Eclass *e) const;

	// Interface for iterating through calls and callers
	const_fiterator_type call_begin() const { return call.begin(); }
	const_fiterator_type call_end() const { return call.end(); }
	const_fiterator_type caller_begin() const { return caller.begin(); }
	const_fiterator_type caller_end() const { return caller.end(); }

	int get_num_call() const { return call.size(); }
	int get_num_caller() const { return caller.size(); }

	bool is_defined() const { return defined; }
	void set_visited() { visited = true; }
	bool is_visited() const { return visited; }

	FCall(const Token& t, Type typ, const string &s);
};

#endif // FCALL_