#ifndef MYTINYSTL_LIST_H_
#define MYTINYSTL_LIST_H_

// 这个头文件包含了一个模板类 list
// list : 双向链表

// notes:
//
// 异常保证：
// mystl::list<T> 满足基本异常保证，部分函数无异常保证，并对以下等函数做强异常安全保证：
//   * emplace_front
//   * emplace_back
//   * emplace
//   * push_front
//   * push_back
//   * insert

#include <initializer_list>

#include "iterator.h"
#include "memory.h"
#include "functional.h"
#include "util.h"
#include "exceptdef.h"

namespace mystl {

    template<class T>
    struct list_node_base;
    template<class T>
    struct list_node;

    template<class T>
    struct node_traits {
        typedef list_node_base<T> *base_ptr;
        typedef list_node<T> *node_ptr;
    };

    // list 的节点结构
    // 类模板  结构体
    template<class T>
    struct list_node_base {
        typedef typename node_traits<T>::base_ptr base_ptr;  // 基本节点
        typedef typename node_traits<T>::node_ptr node_ptr;  // 数据节点

        base_ptr prev;  // 前一节点
        base_ptr next;  // 下一节点

        list_node_base() = default;

        node_ptr as_node() {
            // 作为节点返回
            return static_cast<node_ptr>(self());
        }

        void unlink() {
            prev = next = self();
        }

        base_ptr self() {
            return static_cast<base_ptr>(&*this);
        }
    };

    template<class T>
    struct list_node : public list_node_base<T> {
        // 萃取器
        typedef typename node_traits<T>::base_ptr base_ptr;
        typedef typename node_traits<T>::node_ptr node_ptr;

        T value;  // 数据域

        list_node() = default;

        list_node(const T &v) : value(v) {}

        // && 表示右值引用，当传入的不是变量而是一个值的时候将调用这个，比如list_node(10)而不是list_node(x)
        list_node(T &&v) : value(mystl::move(v)) {}

        base_ptr as_base() {
            return static_cast<base_ptr>(&*this);
        }

        node_ptr self() {
            return static_cast<node_ptr>(&*this);
        }
    };

// list的迭代器设计
// 继承了一个双向迭代器
    template<class T>
    struct list_iterator : public mystl::iterator<mystl::bidirectional_iterator_tag, T> {
        typedef T value_type;
        typedef T *pointer;
        typedef T &reference;
        typedef typename node_traits<T>::base_ptr base_ptr;
        typedef typename node_traits<T>::node_ptr node_ptr;
        typedef list_iterator<T> self;

        base_ptr node_;  // 指向当前节点

        // 构造函数
        list_iterator() = default;

        // : 是赋值，相当于在函数内部赋值
        list_iterator(base_ptr x) : node_(x) {}

        list_iterator(node_ptr x) : node_(x->as_base()) {}

        list_iterator(const list_iterator &rhs) : node_(rhs.node_) {}

        // 重载操作符
        reference operator*() const { return node_->as_node()->value; }

        pointer operator->() const { return &(operator*()); }

        // self关键字总是指向调用该方法的对象
        // &operator 返回的是地址
        // 若要进行操作符的连用要使用带&的
        self &operator++() {
            MYSTL_DEBUG(node_ != nullptr);
            node_ = node_->next;
            return *this;
        }

        // operator返回的是值，不能进行操作符的连用
        self operator++(int) {
            self tmp = *this;  // 取值？
            ++*this;  // 值++
            return tmp;  // 返回值
        }

        self &operator--() {
            MYSTL_DEBUG(node_ != nullptr);
            node_ = node_->prev;
            return *this;
        }

        self operator--(int) {
            self tmp = *this;
            --*this;
            return tmp;
        }

        // 重载比较操作符
        bool operator==(const self &rhs) const { return node_ == rhs.node_; }

        bool operator!=(const self &rhs) const { return node_ != rhs.node_; }
    };

    // const版本
    template<class T>
    struct list_const_iterator : public iterator<bidirectional_iterator_tag, T> {
        typedef T value_type;
        typedef const T *pointer;
        typedef const T &reference;
        typedef typename node_traits<T>::base_ptr base_ptr;
        typedef typename node_traits<T>::node_ptr node_ptr;
        typedef list_const_iterator<T> self;

        base_ptr node_;

        list_const_iterator() = default;

        list_const_iterator(base_ptr x)
                : node_(x) {}

        list_const_iterator(node_ptr x)
                : node_(x->as_base()) {}

        list_const_iterator(const list_iterator<T> &rhs)
                : node_(rhs.node_) {}

        list_const_iterator(const list_const_iterator &rhs)
                : node_(rhs.node_) {}

        reference operator*() const { return node_->as_node()->value; }

        pointer operator->() const { return &(operator*()); }

        self &operator++() {
            MYSTL_DEBUG(node_ != nullptr);
            node_ = node_->next;
            return *this;
        }

        self operator++(int) {
            self tmp = *this;
            ++*this;
            return tmp;
        }

        self &operator--() {
            MYSTL_DEBUG(node_ != nullptr);
            node_ = node_->prev;
            return *this;
        }

        self operator--(int) {
            self tmp = *this;
            --*this;
            return tmp;
        }

        // 重载比较操作符
        bool operator==(const self &rhs) const { return node_ == rhs.node_; }

        bool operator!=(const self &rhs) const { return node_ != rhs.node_; }
    };

// 模板类：list
// 模板参数 T 代表数据类型
    template<class T>
    class list {
    public:
        // list 的嵌套型别定义
        // 不同类型的allocator
        typedef mystl::allocator<T> allocator_type;
        typedef mystl::allocator<T> data_allocator;
        typedef mystl::allocator<list_node_base<T>> base_allocator;
        typedef mystl::allocator<list_node<T>> node_allocator;

        typedef typename allocator_type::value_type value_type;
        typedef typename allocator_type::pointer pointer;
        typedef typename allocator_type::const_pointer const_pointer;
        typedef typename allocator_type::reference reference;
        typedef typename allocator_type::const_reference const_reference;
        typedef typename allocator_type::size_type size_type;
        typedef typename allocator_type::difference_type difference_type;

        typedef list_iterator<T> iterator;
        typedef list_const_iterator<T> const_iterator;
        typedef mystl::reverse_iterator<iterator> reverse_iterator;
        typedef mystl::reverse_iterator<const_iterator> const_reverse_iterator;

        typedef typename node_traits<T>::base_ptr base_ptr;
        typedef typename node_traits<T>::node_ptr node_ptr;

        allocator_type get_allocator() { return node_allocator(); }

    private:
        base_ptr node_;  // 指向末尾节点
        size_type size_;  // 大小

    public:
        list() { fill_init(0, value_type()); }

        // 不能隐式转换
        explicit list(size_type n) { fill_init(n, value_type()); }

        list(size_type n, const T &value) { fill_init(n, value); }

        // 通过迭代器构造
        template<class Iter, typename std::enable_if<mystl::is_input_iterator<Iter>::value, int>::type = 0>
        list(Iter first, Iter last) { copy_init(first, last); }

        // 根据初始化的列表进行构造
        list(std::initializer_list<T> ilist) { copy_init(ilist.begin(), ilist.end()); }

        // 复制？
        list(const list &rhs) { copy_init(rhs.cbegin(), rhs.cend()); }

        // 这是干嘛？
        list(list &&rhs) noexcept
                : node_(rhs.node_), size_(rhs.size_) {
            rhs.node_ = nullptr;
            rhs.size_ = 0;
        }

        // 重载各种操作符
        list &operator=(const list &rhs) {
            if (this != &rhs) {
                assign(rhs.begin(), rhs.end());
            }
            return *this;
        }

        list &operator=(list &&rhs) noexcept {
            clear();
            splice(end(), rhs);
            return *this;
        }

        list &operator=(std::initializer_list<T> ilist) {
            list tmp(ilist.begin(), ilist.end());
            swap(tmp);
            return *this;
        }

        // 析构函数
        ~list() {
            if (node_) {
                // 若节点存在
                // 先清空
                clear();
                // 销毁空间
                base_allocator::deallocate(node_);
                // 置为空
                node_ = nullptr;
                size_ = 0;
            }
        }


    public:
        // 迭代器相关操作
        // 是循环？node_指向末尾节点，因此begin就是node_->next
        iterator begin() noexcept { return node_->next; }

        const_iterator begin() const noexcept { return node_->next; }

        iterator end() noexcept { return node_; }

        const_iterator end() const noexcept { return node_; }

        // reverse_iterator：反向迭代器适配器，常用来对容器进行反向遍历，即从容器中存储的最后一个元素开始，一直遍历到第一个元素
        // 反向调用end就是rbegin()
        reverse_iterator rbegin() noexcept { return reverse_iterator(end()); }

        const_reverse_iterator rbegin() const noexcept { return reverse_iterator(end()); }

        reverse_iterator rend() noexcept { return reverse_iterator(begin()); }

        const_reverse_iterator rend() const noexcept { return reverse_iterator(begin()); }

        const_iterator cbegin() const noexcept { return begin(); }

        const_iterator cend() const noexcept { return end(); }

        const_reverse_iterator crbegin() const noexcept { return rbegin(); }

        const_reverse_iterator crend() const noexcept { return rend(); }

        // 容量相关操作
        // 若当前节点的下一个还是当前节点即为空
        bool empty() const noexcept { return node_->next == node_; }

        size_type size() const noexcept { return size_; }

        // 返回最大容量，为什么是-1？
        size_type max_size() const noexcept { return static_cast<size_type>(-1); }

        // 访问元素相关操作
        reference front() {
            MYSTL_DEBUG(!empty());
            return *begin();
        }

        const_reference front() const {
            MYSTL_DEBUG(!empty());
            return *begin();
        }

        reference back() {
            MYSTL_DEBUG(!empty());
            return *(--end());
        }

        const_reference back() const {
            MYSTL_DEBUG(!empty());
            return *(--end());
        }

        // 调整容器相关操作
        // assign
        void assign(size_type n, const value_type &value) { fill_assign(n, value); }

        template<class Iter, typename std::enable_if<
                mystl::is_input_iterator<Iter>::value, int>::type = 0>
        void assign(Iter first, Iter last) { copy_assign(first, last); }

        void assign(std::initializer_list<T> ilist) { copy_assign(ilist.begin(), ilist.end()); }

        // emplace_front / emplace_back / emplace
        template<class ...Args>
        void emplace_front(Args&&...args) {
            THROW_LENGTH_ERROR_IF(size_ > max_size() - 1, "list<T>'s size too big");
            auto link_node = create_node(mystl::forward<Args>(args)...);
            link_nodes_at_front(link_node->as_base(), link_node->as_base());
            ++size_;
        }
    }


    } // namespace mystl
#endif // !MYTINYSTL_LIST_H_

