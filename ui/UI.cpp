#include "UI.h"

namespace lb {

UI::UI() :
    builder(Gtk::Builder::create_from_file(
        "/home/tarun/code/llvm-browse/ui/llvm-browse.glade")) {
}

} // namespace lb
