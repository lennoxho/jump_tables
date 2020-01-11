#include <tuple>
#include <boost/variant.hpp>

// https://godbolt.org/z/rsAybz

namespace detail {

    template <std::size_t I, typename T>
    struct nth_element_ {};

    template <std::size_t I, template <typename...> class L, typename... T>
    struct nth_element_<I, L<T...>> {
        using type = std::tuple_element_t<I, std::tuple<T...>>;
    };

} // namespace detail

template <std::size_t I, typename T>
using nth_element = typename detail::nth_element_<I, T>::type;

namespace detail {

    template <typename T>
    struct type_list_size_ {};

    template <template <typename...> class L, typename... T>
    struct type_list_size_<L<T...>> {
        static constexpr std::size_t value = sizeof...(T);
    };

} // namespace detail

template <typename T>
using type_list_size = detail::type_list_size_<T>;

namespace detail {

    template <typename T, typename... L>
    struct count__ {
        static constexpr std::size_t value = 0;
    };

    template <typename T, typename F, typename... R>
    struct count__<T, F, R...> {
        static constexpr std::size_t value = (std::is_same<T, F>::value ? 1 : 0) + count__<T, R...>::value;
    };

    template <typename T, typename L>
    struct count_ {};

    template <typename T, template <typename...> class L, typename... R>
    struct count_<T, L<R...>> {
        static constexpr std::size_t value = count__<T, R...>::value;
    };

} // namespace detail

template <typename T, typename L>
using count = detail::count_<T, L>;

namespace detail {

    template <typename ReturnType, typename Visitor, typename Variant, typename VariantType>
    inline ReturnType visit_inner(Visitor visitor, Variant variant) {
        return visitor(boost::get<VariantType>(variant));
    }

    template <typename L>
    struct visit_impl {};

    template <std::size_t... I>
    struct visit_impl<std::index_sequence<I...>> {
        template <typename Visitor, typename Variant>
        decltype(auto) operator()(Visitor &&visitor, Variant &&variant) const {
            using return_type = decltype(visitor(boost::get<nth_element<0, std::decay_t<Variant>>>(variant)));
            using callback_type = decltype(&visit_inner<return_type, decltype(visitor), decltype(variant), nth_element<0, std::decay_t<Variant>>>);
            static constexpr callback_type jmp_table[] = { visit_inner<return_type, decltype(visitor), decltype(variant), nth_element<I, std::decay_t<Variant>>>... };

            return jmp_table[variant.which()](std::forward<Visitor>(visitor), std::forward<Variant>(variant));
        }
    };

    template <>
    struct visit_impl<std::index_sequence<>> {};

} // namespace detail

template <typename Visitor, typename Variant>
inline decltype(auto) visit(Visitor &&visitor, Variant &&variant) {
    static constexpr std::size_t N = type_list_size<std::decay_t<Variant>>::value -
        count<boost::detail::variant::void_, std::decay_t<Variant>>::value;
    return detail::visit_impl<std::make_index_sequence<N>>()(std::forward<Visitor>(visitor),
        std::forward<Variant>(variant));
}

struct A {};
struct B {};
struct C {};

struct func {
    int operator()(A) const { return 0; }
    int operator()(B) const { return 1; }
    int operator()(C) const { return 2; }
};

#include <iostream>

int main(int argc, char** argv) {
    boost::variant<A, B, C> var;
    if (argc == 2) {
        var = B{};
    }
    else if (argc == 3) {
        var = C{};
    }

    std::cout << visit(func(), var) << "\n";;
}