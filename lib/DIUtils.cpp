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
get_full_name(const llvm::DISubprogram*, llvm::raw_string_ostream&);
static bool
get_full_name(const llvm::DIScope*, llvm::raw_string_ostream&);
static bool
get_full_name(const llvm::DICompositeType*, llvm::raw_string_ostream&);
static bool
get_full_name(const llvm::DINamespace*, llvm::raw_string_ostream&);
static bool
get_full_name(const llvm::DICompileUnit*, llvm::raw_string_ostream&);

static std::string
format_name(llvm::StringRef s) {
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

static bool
get_full_name(const llvm::DICompileUnit*, llvm::raw_string_ostream&) {
  return false;
}

static bool
get_full_name(const llvm::DINamespace* di, llvm::raw_string_ostream& ss) {
  if(const llvm::DIScope* scope = di->getScope()) {
    if(get_full_name(scope, ss))
      ss << "::";
  }
  ss << get_name(di);
  return true;
}

static bool
get_full_name(const llvm::DICompositeType* di, llvm::raw_string_ostream& ss) {
  if(const auto* scope = cast_or_null<llvm::DIScope>(di->getScope())) {
    if(get_full_name(scope, ss))
      ss << "::";
  }
  ss << get_name(di);
  return true;
}

static bool
get_full_name(const llvm::DIScope* di, llvm::raw_string_ostream& ss) {
  if(const auto* compile = dyn_cast<llvm::DICompileUnit>(di))
    return get_full_name(compile, ss);
  else if(const auto* ns = dyn_cast<llvm::DINamespace>(di))
    return get_full_name(ns, ss);
  else if(const auto* composite = dyn_cast<llvm::DICompositeType>(di))
    return get_full_name(composite, ss);
  return false;
}

static bool
get_full_name(const llvm::DISubprogram* di, llvm::raw_string_ostream& ss) {
  if(const auto* scope = cast_or_null<llvm::DIScope>(di->getScope())) {
    if(get_full_name(scope, ss))
      ss << "::";
  }
  ss << get_name(di);
  return true;
}

static bool
get_full_name(const llvm::DIGlobalVariable* di, llvm::raw_string_ostream& ss) {
  if(llvm::DIScope* scope = di->getScope()) {
    if(get_full_name(scope, ss))
      ss << "::";
  }
  ss << get_name(di);
  return true;
}

std::string
get_name(const llvm::DINamespace* di) {
  return format_name(di->getName());
}

std::string
get_name(const llvm::DICompositeType* di) {
  return format_name(di->getName());
}

std::string
get_name(const llvm::DILocalVariable* di) {
  return format_name(di->getName());
}

std::string
get_name(const llvm::DIGlobalVariable* di) {
  return format_name(di->getName());
}

std::string
get_name(const llvm::DISubprogram* di) {
  return format_name(di->getName());
}

std::string
get_full_name(const llvm::DISubprogram* di) {
  std::string buf;
  llvm::raw_string_ostream ss(buf);
  get_full_name(di, ss);

  return ss.str();
}

std::string
get_full_name(const llvm::DIGlobalVariable* di) {
  std::string buf;
  llvm::raw_string_ostream ss(buf);
  get_full_name(di, ss);

  return ss.str();
}

} // namespace DebugInfo

} // namespace lb