#include "Logging.h"

namespace lb {

const NewLineT endl;

thread_local static FormattedStream g_message("Message",
                                          llvm::raw_ostream::Colors::GREEN);
thread_local static FormattedStream g_warning("Warning",
                                              llvm::raw_ostream::Colors::YELLOW);
thread_local static FormattedStream
    g_critical("Critical", llvm::raw_ostream::Colors::MAGENTA);
thread_local static FormattedStream g_error("Error",
                                            llvm::raw_ostream::Colors::RED);


FormattedStream&
message(bool start) {
  if(start)
    g_message.start();
  return g_message;
}

FormattedStream&
warning(bool start) {
  if(start)
    g_warning.start();
  return g_warning;
}

FormattedStream&
critical(bool start) {
  if(start)
    g_critical.start();
  return g_critical;
}

FormattedStream&
error(bool start) {
  if(start)
    g_error.start();
  return g_error;
}

FormattedStream::FormattedStream(const std::string& label,
                                 llvm::raw_ostream::Colors color) :
    label(label),
    color(color) {
  ;
}

FormattedStream&
FormattedStream::start() {
  fs.changeColor(color, true, false);
  fs << label;
  fs.resetColor();
  fs << ": ";

  return *this;
}

FormattedStream&
FormattedStream::operator<<(const NewLineT&) {
	fs << "\n";
	fs.PadToColumn(label.size() + 2);
	return *this;
}

} // namespace label