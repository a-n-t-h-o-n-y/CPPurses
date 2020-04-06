#ifndef CPPURSES_WIDGET_ARRAY_HPP
#define CPPURSES_WIDGET_ARRAY_HPP
#include <array>
#include <cstddef>
#include <functional>
#include <utility>

namespace cppurses {

/// Homogeneous collection of Widgets within a Layout_t.
template <typename Layout_t, typename Widget_t, std::size_t N>
class Array : public Layout_t {
   public:
    /// \p args will be copied into each Widget's constructor call.
    template <typename... Args>
    Array(Args const&... args)
        : refs_{create_n_children(std::make_index_sequence<N>(), args...)}
    {}

    /// Get child by index.
    template <std::size_t Index>
    auto get() -> auto&
    {
        return std::get<Index>(refs_).get();
    }

    /// Get child by index.
    template <std::size_t Index>
    auto get() const -> auto const&
    {
        return std::get<Index>(refs_).get();
    }

   private:
    using References = std::array<std::reference_wrapper<Widget_t>, N>;
    References refs_;

   private:
    template <typename... Args>
    auto make_child_with_index(std::size_t, Args const&... args) -> Widget_t&
    {
        return this->template make_child<Widget_t>(args...);
    }

    template <std::size_t... I, typename... Args>
    auto create_n_children(std::index_sequence<I...>, Args const&... args)
        -> References
    {
        return {this->make_child_with_index(I, args...)...};
    }
};

}  // namespace cppurses
#endif  // CPPURSES_WIDGET_ARRAY_HPP
