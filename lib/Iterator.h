#ifndef LLVM_BROWSE_ITERATOR_H
#define LLVM_BROWSE_ITERATOR_H

namespace lb {

template<typename BaseIterator>
class DerefIterator : public BaseIterator {
public:
  using iterator_category = typename BaseIterator::iterator_category;
  using value_type        = typename BaseIterator::value_type::element_type;
  using difference_type   = typename BaseIterator::difference_type;
  using pointer           = value_type*;
  using reference         = value_type&;

  DerefIterator(const BaseIterator& other) : BaseIterator(other) {
    ;
  }

  reference operator*() const {
    return *(this->BaseIterator::operator*());
  }

  pointer operator->() const {
    return this->BaseIterator::operator*().get();
  }

  reference operator[](size_t n) const {
    return *(this->BaseOperator::operator[](n));
  }
};

} // namespace lb

#endif // LLVM_BROWSE_ITERATOR_H