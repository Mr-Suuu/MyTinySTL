#ifndef MYTINYSTL_DEQUE_H_
#define MYTINYSTL_DEQUE_H_

// 这个头文件包含了一个模板类 deque
// deque: 双端队列

// notes:
//
// 异常保证：
// mystl::deque<T> 满足基本异常保证，部分函数无异常保证，并对以下等函数做强异常安全保证：
//   * emplace_front
//   * emplace_back
//   * emplace
//   * push_front
//   * push_back
//   * insert

#include <initializer_list>

#include "iterator.h"
#include "memory.h"
#include "util.h"
#include "exceptdef.h"

namespace mystl {

#ifdef max
#pragma message("#undefing marco max")
#undef max
#endif // max

#ifdef min
#pragma message("#undefing marco min")
#undef min
#endif // min

// deque map 初始化的大小
#ifndef DEQUE_MAP_INIT_SIZE
#define DEQUE_MAP_INIT_SIZE 8
#endif

    template<class T>
    struct deque_buf_size {
        // static 静态变量，该变量属于这个类而不是该类的某个对象，即类共享一个变量
        // constexpr表达式是指值不会改变并且在编译过程就能得到计算结果的表达式
        // 若T的大小小于16，则buf大小取 4096 / sizeof(T)，否则取16
        static constexpr size_t value = sizeof(T) < 256 ? 4096 / sizeof(T) : 16;
    };

// deque 的迭代器设计
    template<class T, class Ref, class Ptr>
    struct deque_iterator : public iterator<random_access_iterator_tag, T> {
        typedef deque_iterator<T, T &, T *> iterator;
        typedef deque_iterator<T, const T &, const T *> const_iterator;
        typedef deque_iterator self;

        typedef T value_type;
        typedef Ptr pointer;
        typedef Ref reference;
        typedef size_t size_type;
        typedef ptrdiff_t difference_type;
        typedef T *value_pointer;
        typedef T **map_pointer;

        static const size_type buffer_size = deque_buf_size<T>::value;  // deque中一个buffer的大小

        // 迭代器所含成员数据
        value_pointer cur;    // 指向所在缓冲区的当前元素
        value_pointer first;  // 指向所在缓冲区的头部
        value_pointer last;   // 指向所在缓冲区的尾部
        map_pointer node;   // 缓冲区所在节点

        // 构造、复制、移动函数
        // noexcept表示不抛出异常，效果上与throw相同
        // : 后表示赋值
        // 无参构造
        deque_iterator() noexcept: cur(nullptr), first(nullptr), last(nullptr), node(nullptr) {}

        // 有参构造
        deque_iterator(value_pointer v, map_pointer n) : cur(v), first(*n), last(*n + buffer_size), node(n) {}

        deque_iterator(const iterator &rhs) : cur(rhs.cur), first(rhs.first), last(rhs.last), node(rhs.node) {}

        deque_iterator(iterator &&rhs) noexcept: cur(rhs.cur), first(rhs.first), last(rhs.last), node(rhs.node) {
            rhs.cur = nullptr;
            rhs.first = nullptr;
            rhs.last = nullptr;
            rhs.node = nullptr;
        }

        deque_iterator(const const_iterator &rhs) : cur(rhs.cur), first(rhs.first), last(rhs.last), node(rhs.node) {}

        // rhs 指右操作数
        self &operator=(const iterator &rhs) {
            if (this != &rhs) {
                // 若当前不是rhs，则进行赋值
                cur = rhs.cur;
                first = rhs.first;
                last = rhs.last;
                node = rhs.node;
            }
            return *this; // 返回更新后的值
        }

        // 转到另一个缓冲区
        void set_node(map_pointer new_node) {
            node = new_node;
            first = *new_node;  // 更新新的起始位置
            last = first + buffer_size;  // 初始化last为first + 一个buffer的大小
        }

        // 重载运算符
        // *取值，取出cur地址中的值
        reference operator*() const { return *cur; }

        // -> 返回cur的地址？
        pointer operator->() const { return cur; }

        difference_type operator-(const self &x) const {
            return static_cast<difference_type>(buffer_size) * (node - x.node)
                   + (cur - first) - (x.cur - x.first);
        }

        // 当前位置后移一位
        self &operator++() { // 地址后移
            ++cur;
            if (cur == last) {
                // 若到达缓冲区的尾
                // node：缓冲区所在节点
                set_node(node + 1);  // 转到另一个缓冲区
                cur = first;  // 将当前指向节点指向first
            }
            return *this;
        }

        // 值++
        self operator++(int) {
            self tmp = *this;
            ++*this;
            return tmp;
        }

        // 地址向前退一位
        self &operator--() {
            if (cur == first) {
                // 若到达缓冲区的头
                // node：缓冲区所在节点
                set_node(node - 1);  // 转到另一个缓冲区
                cur = last;  // 将当前节点指向移动到结尾
            }
            --cur;  // 节点前移
            return *this;
        }

        // 值--
        self operator--(int) {
            self tmp = *this;
            --*this;
            return tmp;
        }

        // 地址后移n位
        self &operator+=(difference_type n) {
            const auto offset = n + (cur - first);
            if (offset >= 0 && offset < static_cast<difference_type>(buffer_size)) {
                // 仍在当前缓冲区
                cur += n;  // 直接加
            } else {
                // 要跳到其他缓冲区
                const auto node_offset = offset > 0 ? offset / static_cast<difference_type>(buffer_size) :
                                         -static_cast<difference_type>((-offset - 1) / buffer_size) - 1;
                set_node(node + node_offset);  // 重置一下位置
                cur = first + (offset - node_offset * static_cast<difference_type>(buffer_size));
            }
            return *this;
        }

        // 值加n
        self operator+(difference_type n) const {
            self tmp = *this;
            return tmp += n;
        }

        // 地址前移n位
        self &operator-=(difference_type n) {
            // 通过重载的+=实现
            return *this += -n;
        }

        // 值减n
        self operator-(difference_type n) const {
            self tmp = *this;
            return tmp -= n;
        }

        // 重载[]取值
        reference operator[](difference_type n) const { return *(*this + n); }

        // 重载比较操作符
        bool operator==(const self &rhs) const { return cur == rhs.cur; }

        bool operator<(const self &rhs) const { return node == rhs.node ? (cur < rhs.cur) : (node < rhs.node); }

        bool operator!=(const self &rhs) const { return !(*this == rhs); }

        bool operator>(const self &rhs) const { return rhs < *this; }

        bool operator<=(const self &rhs) const { return !(rhs < *this); }

        bool operator>=(const self &rhs) const { return !(*this < rhs); }
    };

// 模板类deque
// 模板参数代表数据类型


} // namespace mystl
#endif // !MYTINYSTL_DEQUE_H_

