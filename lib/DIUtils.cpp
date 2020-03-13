#include "DIUtils.h"

#include <llvm/Support/raw_ostream.h>

using llvm::cast;
using llvm::cast_or_null;
using llvm::dyn_cast;
using llvm::dyn_cast_or_null;
using llvm::isa;

namespace lb {

namespace DebugInfo {

static bool
get_full_name(const llvm::DISubprogram*, bool, llvm::raw_string_ostream&);
static bool
get_full_name(const llvm::DIScope*, bool, llvm::raw_string_ostream&);
static bool
get_full_name(const llvm::DICompositeType*, bool, llvm::raw_string_ostream&);
static bool
get_full_name(const llvm::DINamespace*, bool, llvm::raw_string_ostream&);
static bool
get_full_name(const llvm::DICompileUnit*, bool, llvm::raw_string_ostream&);

// Format the templates in the C++11 style i.e. without spaces between 
// angle brackets indiciating nested ends of template specifications
static std::string
format_templates(llvm::StringRef s) {
  std::string buf;
  llvm::raw_string_ostream ss(buf);

  if(s.size() > 1) {
    for(size_t i = 0; i < s.size() - 1; i++) {
      // If the current character is a space and the next is the end of a
      // template, then get rid of the space. This is the C++11 style of
      // templates
      if(s[i] == ' ') {
        if(s[i + 1] != '>')
          ss << s[i];
      } else {
        ss << s[i];
      }
    }
  }
  if(s.size())
    ss << s[s.size() - 1];
  return ss.str();
}

// This is meant to be called only with the source
// names. The main source of ambiguity when stripping templates is
// distinguishing between 	angle brackets used to demarcate templates and
// overload functions for operators <, <<, >, >> and ->
// This is meant to be called only with the "source" names as input, so to
// find whether or not this is an operator overload, we only have to check
// the start of the string
static std::string
strip_templates(llvm::StringRef s) {
  std::string buf;
  llvm::raw_string_ostream ss(buf);

  size_t start = 0;
  if(s.startswith("operator")) {
    start = 8;
    if(s.size() > 9) {
      if((s[8] == '<') and (s[9] == '<')) // operator<<
        start = 10;
      else if((s[8] == '>') and (s[9] == '>')) // operator>>
        start = 10;
      else if((s[8] == '-') and (s[9] == '>')) // operator->
        start = 10;
      else if((s[8] == '<') or (s[8] == '>')) // operator< or operator>
        start = 9;
    }
  }

  // If we have to start checking for templates at some later point in the
  // string, then just copy everything before that point
  for(size_t i = 0; i < start; i++)
    ss << s[i];

  unsigned nesting = 0;
  for(size_t i = start; i < s.size(); i++) {
  	if(s[i] == '<')
  		nesting++;
  	else if(s[i] == '>')
  		nesting--;
  	else if(nesting == 0)
  		ss << s[i];
  }

  return ss.str();
}

static bool
get_full_name(const llvm::DICompileUnit*, bool, llvm::raw_string_ostream&) {
  return false;
}

static bool
get_full_name(const llvm::DINamespace* di,
              bool keep_templates,
              llvm::raw_string_ostream& ss) {
  if(const llvm::DIScope* scope = di->getScope()) {
    if(get_full_name(scope, keep_templates, ss))
      ss << "::";
  }
  ss << get_name(di, keep_templates);
  return true;
}

static bool
get_full_name(const llvm::DICompositeType* di,
              bool keep_templates,
              llvm::raw_string_ostream& ss) {
  if(const auto* scope = cast_or_null<llvm::DIScope>(di->getScope())) {
    if(get_full_name(scope, keep_templates, ss))
      ss << "::";
  }
  ss << get_name(di, keep_templates);
  return true;
}

static bool
get_full_name(const llvm::DIScope* di,
              bool keep_templates,
              llvm::raw_string_ostream& ss) {
  if(const auto* compile = dyn_cast<llvm::DICompileUnit>(di))
    return get_full_name(compile, keep_templates, ss);
  else if(const auto* ns = dyn_cast<llvm::DINamespace>(di))
    return get_full_name(ns, keep_templates, ss);
  else if(const auto* composite = dyn_cast<llvm::DICompositeType>(di))
    return get_full_name(composite, keep_templates, ss);
  return false;
}

static bool
get_full_name(const llvm::DISubprogram* di,
              bool keep_templates,
              llvm::raw_string_ostream& ss) {
  if(const auto* scope = cast_or_null<llvm::DIScope>(di->getScope())) {
    if(get_full_name(scope, keep_templates, ss))
      ss << "::";
  }
  ss << get_name(di, keep_templates);
  return true;
}

static bool
get_full_name(const llvm::DIGlobalVariable* di,
              bool keep_templates,
              llvm::raw_string_ostream& ss) {
  if(llvm::DIScope* scope = di->getScope()) {
    if(get_full_name(scope, keep_templates, ss))
      ss << "::";
  }
  ss << get_name(di, keep_templates);
  return true;
}

static std::string
format_name(llvm::StringRef s, bool keep_templates) {
	if(keep_templates)
		return format_templates(s);
	else 
		return strip_templates(s);
}

std::string
get_name(const llvm::DINamespace* di, bool keep_templates) {
	return format_name(di->getName(), keep_templates);
}

std::string
get_name(const llvm::DICompositeType* di, bool keep_templates) {
  return format_name(di->getName(), keep_templates);
}

std::string
get_name(const llvm::DILocalVariable* di, bool keep_templates) {
  return format_name(di->getName(), keep_templates);
}

std::string
get_name(const llvm::DIGlobalVariable* di, bool keep_templates) {
  return format_name(di->getName(), keep_templates);
}

std::string
get_name(const llvm::DISubprogram* di, bool keep_templates) {
  return format_name(di->getName(), keep_templates);
}

std::string
get_full_name(const llvm::DISubprogram* di) {
  std::string buf;
  llvm::raw_string_ostream ss(buf);
  get_full_name(di, true, ss);

  return ss.str();
}

std::string
get_full_name(const llvm::DIGlobalVariable* di) {
  std::string buf;
  llvm::raw_string_ostream ss(buf);
  get_full_name(di, true, ss);

  return ss.str();
}

std::string
get_qualified_name(const llvm::DISubprogram* di) {
  std::string buf;
  llvm::raw_string_ostream ss(buf);
  get_full_name(di, false, ss);

  return ss.str();
}

std::string
get_qualified_name(const llvm::DIGlobalVariable* di) {
  std::string buf;
  llvm::raw_string_ostream ss(buf);
  get_full_name(di, false, ss);

  return ss.str();
}

} // namespace DebugInfo

} // namespace lb