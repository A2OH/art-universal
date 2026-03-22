// Shadow dchecked_vector.h - uses raw pointer iterators for GCC compatibility.
// The real dchecked_vector inherits std::vector iterators which encode the
// allocator type. This causes type mismatches when AOSP code returns
// ScopedArenaVector<>::iterator as ArenaVector<>::iterator (a known AOSP bug
// that clang's STL tolerates since both use raw pointers).
//
// Fix: override iterator/const_iterator to be raw pointers.
#ifndef ART_LIBARTBASE_BASE_DCHECKED_VECTOR_H_
#define ART_LIBARTBASE_BASE_DCHECKED_VECTOR_H_

#include <algorithm>
#include <type_traits>
#include <vector>

#include <android-base/logging.h>

namespace art {

template <typename T, typename Alloc = std::allocator<T>>
class dchecked_vector : private std::vector<T, Alloc> {
 private:
  static_assert(!std::is_same<T, bool>::value, "Not implemented for bool.");
  using Base = std::vector<T, Alloc>;

 public:
  using typename Base::value_type;
  using typename Base::allocator_type;
  using typename Base::reference;
  using typename Base::const_reference;
  using typename Base::pointer;
  using typename Base::const_pointer;
  // Use raw pointers as iterators so all dchecked_vector<T, *> share the same
  // iterator type regardless of allocator. This fixes the AOSP
  // ArenaVector/ScopedArenaVector iterator mismatch.
  using iterator = T*;
  using const_iterator = const T*;
  using reverse_iterator = std::reverse_iterator<iterator>;
  using const_reverse_iterator = std::reverse_iterator<const_iterator>;
  using typename Base::size_type;
  using typename Base::difference_type;

  // Construct/copy/destroy.
  dchecked_vector()
      : Base() { }
  explicit dchecked_vector(const allocator_type& alloc)
      : Base(alloc) { }
  explicit dchecked_vector(size_type n, const allocator_type& alloc = allocator_type())
      : Base(n, alloc) { }
  dchecked_vector(size_type n,
                  const value_type& value,
                  const allocator_type& alloc = allocator_type())
      : Base(n, value, alloc) { }
  template <typename InputIterator>
  dchecked_vector(InputIterator first,
                  InputIterator last,
                  const allocator_type& alloc = allocator_type())
      : Base(first, last, alloc) { }
  dchecked_vector(const dchecked_vector& src)
      : Base(src) { }
  dchecked_vector(const dchecked_vector& src, const allocator_type& alloc)
      : Base(src, alloc) { }
  dchecked_vector(dchecked_vector&& src)
      : Base(std::move(src)) { }
  dchecked_vector(dchecked_vector&& src, const allocator_type& alloc)
      : Base(std::move(src), alloc) { }
  dchecked_vector(std::initializer_list<value_type> il,
                  const allocator_type& alloc = allocator_type())
      : Base(il, alloc) { }
  ~dchecked_vector() = default;
  dchecked_vector& operator=(const dchecked_vector& src) {
    Base::operator=(src);
    return *this;
  }
  dchecked_vector& operator=(dchecked_vector&& src) {
    Base::operator=(std::move(src));
    return *this;
  }
  dchecked_vector& operator=(std::initializer_list<value_type> il) {
    Base::operator=(il);
    return *this;
  }

  // Iterators - use data() to get raw pointers.
  iterator begin() { return data(); }
  iterator end() { return data() + size(); }
  const_iterator begin() const { return data(); }
  const_iterator end() const { return data() + size(); }
  reverse_iterator rbegin() { return reverse_iterator(end()); }
  reverse_iterator rend() { return reverse_iterator(begin()); }
  const_reverse_iterator rbegin() const { return const_reverse_iterator(end()); }
  const_reverse_iterator rend() const { return const_reverse_iterator(begin()); }
  const_iterator cbegin() const { return begin(); }
  const_iterator cend() const { return end(); }
  const_reverse_iterator crbegin() const { return rbegin(); }
  const_reverse_iterator crend() const { return rend(); }

  // Capacity.
  using Base::size;
  using Base::max_size;
  using Base::capacity;
  using Base::empty;
  using Base::reserve;
  using Base::shrink_to_fit;
  void resize(size_type n) { Base::resize(n); }
  void resize(size_type n, const value_type& val) { Base::resize(n, val); }

  // Element access: inherited.
  using Base::data;

  // Element access: subscript operator. Check index.
  reference operator[](size_type n) {
    DCHECK_LT(n, size());
    return Base::operator[](n);
  }
  const_reference operator[](size_type n) const {
    DCHECK_LT(n, size());
    return Base::operator[](n);
  }

  // Element access: front(), back(). Check not empty.
  reference front() { DCHECK(!empty()); return Base::front(); }
  const_reference front() const { DCHECK(!empty()); return Base::front(); }
  reference back() { DCHECK(!empty()); return Base::back(); }
  const_reference back() const { DCHECK(!empty()); return Base::back(); }

  // Modifiers: inherited.
  using Base::push_back;
  using Base::clear;
  using Base::emplace_back;

  // assign - need wrappers for our iterator type
  void assign(size_type n, const value_type& val) { Base::assign(n, val); }
  template <typename InputIterator>
  void assign(InputIterator first, InputIterator last) { Base::assign(first, last); }
  void assign(std::initializer_list<value_type> il) { Base::assign(il); }

  // Modifiers: pop_back(). Check not empty.
  void pop_back() { DCHECK(!empty()); Base::pop_back(); }

  // Modifiers: swap().
  void swap(dchecked_vector& other) { Base::swap(other); }

  // Modifiers: insert(). Convert our raw-pointer iterators to Base iterators.
  iterator insert(const_iterator position, const value_type& value) {
    DCHECK(cbegin() <= position && position <= cend());
    auto base_it = Base::insert(to_base_const_iter(position), value);
    return data() + (base_it - Base::begin());
  }
  iterator insert(const_iterator position, size_type n, const value_type& value) {
    DCHECK(cbegin() <= position && position <= cend());
    auto base_it = Base::insert(to_base_const_iter(position), n, value);
    return data() + (base_it - Base::begin());
  }
  template <typename InputIterator>
  iterator insert(const_iterator position, InputIterator first, InputIterator last) {
    DCHECK(cbegin() <= position && position <= cend());
    auto base_it = Base::insert(to_base_const_iter(position), first, last);
    return data() + (base_it - Base::begin());
  }
  iterator insert(const_iterator position, value_type&& value) {
    DCHECK(cbegin() <= position && position <= cend());
    auto base_it = Base::insert(to_base_const_iter(position), std::move(value));
    return data() + (base_it - Base::begin());
  }
  iterator insert(const_iterator position, std::initializer_list<value_type> il) {
    DCHECK(cbegin() <= position && position <= cend());
    auto base_it = Base::insert(to_base_const_iter(position), il);
    return data() + (base_it - Base::begin());
  }

  // Modifiers: erase(). Convert our raw-pointer iterators to Base iterators.
  iterator erase(const_iterator position) {
    DCHECK(cbegin() <= position && position < cend());
    auto base_it = Base::erase(to_base_const_iter(position));
    return data() + (base_it - Base::begin());
  }
  iterator erase(const_iterator first, const_iterator last) {
    DCHECK(cbegin() <= first && first <= cend());
    DCHECK(first <= last && last <= cend());
    auto base_it = Base::erase(to_base_const_iter(first), to_base_const_iter(last));
    return data() + (base_it - Base::begin());
  }

  // Modifiers: emplace(). Check position.
  template <typename... Args>
  iterator emplace(const_iterator position, Args&&... args) {
    DCHECK(cbegin() <= position && position <= cend());
    auto base_it = Base::emplace(to_base_const_iter(position), std::forward<Args>(args)...);
    return data() + (base_it - Base::begin());
  }

  // Allocator.
  using Base::get_allocator;

 private:
  // Convert raw pointer const_iterator to Base's const_iterator
  typename Base::const_iterator to_base_const_iter(const_iterator pos) const {
    return Base::cbegin() + (pos - data());
  }
};

// Non-member swap()
template <typename T, typename Alloc>
void swap(dchecked_vector<T, Alloc>& lhs, dchecked_vector<T, Alloc>& rhs) {
  lhs.swap(rhs);
}

// Non-member relational operators.
template <typename T, typename Alloc>
bool operator==(const dchecked_vector<T, Alloc>& lhs, const dchecked_vector<T, Alloc>& rhs) {
  return lhs.size() == rhs.size() && std::equal(lhs.begin(), lhs.end(), rhs.begin());
}
template <typename T, typename Alloc>
bool operator!=(const dchecked_vector<T, Alloc>& lhs, const dchecked_vector<T, Alloc>& rhs) {
  return !(lhs == rhs);
}
template <typename T, typename Alloc>
bool operator<(const dchecked_vector<T, Alloc>& lhs, const dchecked_vector<T, Alloc>& rhs) {
  return std::lexicographical_compare(lhs.begin(), lhs.end(), rhs.begin(), rhs.end());
}
template <typename T, typename Alloc>
bool operator<=(const dchecked_vector<T, Alloc>& lhs, const dchecked_vector<T, Alloc>& rhs) {
  return !(rhs < lhs);
}
template <typename T, typename Alloc>
bool operator>(const dchecked_vector<T, Alloc>& lhs, const dchecked_vector<T, Alloc>& rhs) {
  return rhs < lhs;
}
template <typename T, typename Alloc>
bool operator>=(const dchecked_vector<T, Alloc>& lhs, const dchecked_vector<T, Alloc>& rhs) {
  return !(lhs < rhs);
}

}  // namespace art

#endif  // ART_LIBARTBASE_BASE_DCHECKED_VECTOR_H_
