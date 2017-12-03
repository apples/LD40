#ifndef LD40_MAINLOOP_HPP
#define LD40_MAINLOOP_HPP

#include <list>
#include <functional>
#include <memory>
#include <type_traits>

namespace mainloop {

namespace _detail {

class state {
private:
    struct state_interface {
        virtual void _call() = 0;
        virtual ~state_interface() = default;
    };
    template <typename T>
    struct state_impl : state_interface {
        T func;
        state_impl(T t) : func(std::move(t)) {}
        virtual void _call() final override {func();}
    };
public:
    state() = default;
    state(const state&) = default;
    state(state&&) = default;
    state(state&) = default;
    template <typename T>
    state(T&& t) : interface(std::make_shared<state_impl<std::decay_t<T>>>(std::forward<T>(t))) {}
    void operator()() {interface->_call();}
private:
    std::shared_ptr<state_interface> interface;
};

} //namespace _detail

extern std::list<_detail::state> states;
extern std::function<void()> swap_buffers;

void main_loop();

} //namespace mainloop

#endif //LD40_MAINLOOP_HPP
