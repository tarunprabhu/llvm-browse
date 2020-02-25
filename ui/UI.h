#ifndef LLVM_BROWSE_UI_H
#define LLVM_BROWSE_UI_H

#include <gtkmm.h>

namespace lb {

class UI {
protected:
  Glib::RefPtr<Gtk::Builder> builder;

protected:
  template<typename T>
  T& get(const Glib::ustring& name) {
    T* widget = nullptr;
    builder->get_widget(name, widget);
    return *widget;
  }
  
public:
  UI();
  virtual ~UI() = default;
};

} // namespace lb

#endif // LLVM_BROWSE_UI_H
