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
        void emplace_front(Args &&...args) {
            THROW_LENGTH_ERROR_IF(size_ > max_size() - 1, "list<T>'s size too big");
            auto link_node = create_node(mystl::forward<Args>(args)...);
            // 下面这一步是完成了什么？
            link_nodes_at_front(link_node->as_base(), link_node->as_base());
            ++size_;
        }

        template<class ...Args>
        void emplace_back(Args &&...args) {
            THROW_LENGTH_ERROR_IF(size_ > max_size() - 1, "list<T>'s size too big");
            auto link_node = create_node(mystl::forward<Args>(args)...);
            // 这一步做了什么？
            link_nodes_at_back(link_node->as_base(), link_node->as_base());
            ++size_;
        }

        template<class ...Args>
        iterator emplace(const_iterator pos, Args &&...args) {
            THROW_LENGTH_ERROR_IF(size_ > max_size() - 1, "list<T>'s size too big");
            auto link_node = create_node(mystl::forward<Args>(args)...);
            link_nodes(pos.node_, link_node->as_base(), link_node->as_base());
            ++size_;
            return iterator(link_node);
        }

        // insert
        iterator insert(const_iterator pos, const value_type &value) {
            THROW_LENGTH_ERROR_IF(size_ > max_size() - 1, "list<T>'s size too big");
            auto link_node = create_node(mystl::move(value));
            ++size_;
            return link_iter_node(pos, link_node->as_base());
        }

        iterator insert(const_iterator pos, value_type &&value) {
            THROW_LENGTH_ERROR_IF(size_ > max_size() - 1, "list<T>'s size too big");
            auto link_node = create_node(mystl::move(value));
            ++size_;
            return link_iter_node(pos, link_node->as_base());
        }

        iterator insert(const_iterator pos, value_type &&value) {
            THROW_LENGTH_ERROR_IF(size_ > max_size() - 1, "list<T>'s size too big");
            return fill_insert(pos, n, value);
        }

        template<class Iter, typename std::enable_if<
                mystl::is_input_iterator<Iter>::value, int>::value = 0>
        iterator insert(const_iterator pos, Iter first, Iter last) {
            size_type n = mystl::distance(first, last);
            THROW_LENGTH_ERROR_IF(size_ > max_size() - 1, "list<T>'s size too big");
            return copy_insert(pos, n, first);
        }

        // push_front / push_back
        void push_front(const value_type &value) {
            THROW_LENGTH_ERROR_IF(size_ > max_size() - 1, "list<T>'s size too big");
            auto link_node = create_node(value);
            link_node_at_front(link_node->as_base(), link_node->as_base());
            ++size_;
        }

        void push_front(value_type &&value) {
            emplace_front(mystl::move(value));
        }

        void push_back(const value_type &value) {
            THROW_LENGTH_ERROR_IF(size_ > max_size() - 1, "list<T>'s size too big");
            auto link_node = create_node(value);
            link_node_at_back(link_node->as_base(), link_node->as_base());
            ++size_;
        }

        void push_back(value_type &&value) {
            emplace_back(mystl::move(value));
        }

        // pop_front / pop_back
        // 弹出头元素
        void pop_front() {
            MYSTL_DEBUG(!empty());
            auto n = node_->next;
            unlink_nodes(n, n);
            destroy_node(n->as_node());
            --size_;
        }

        // 弹出尾元素
        void pop_back() {
            MYSTL_DEBUG(!empty());
            auto n = node_->prev;
            // 下面这个是干嘛的？
            unlink_nodes(n, n);
            destroy_node(n->as_node());
            --size_;
        }

        // erase / clear
        iterator erase(const_iterator pos);

        iterator erase(const_iterator first, const_iterator last);

        void clear();

        // resize
        void resize(size_type new_size) { resize(new_size, value_type()); }

        void resize(size_type new_size, const value_type &value);

        void swap(list &rhs) noexcept {
            mystl::swap(node_, rhs.node_);
            mystl::swap(size_, rhs.size_);
        }

        // list相关操作

        // 实现list拼接
        // 在pos后将other所有元素拼接到要操作的list对象上
        void splice(const_iterator pos, list &other);

        // 只把指定的it值剪接到要操作的list上
        void splice(const_iterator pos, list &other, const_iterator it);

        // 把first到last剪接到要操作的list对象中
        void splice(const_iterator pos, list &other, const_iterator first, const_iterator last);

        void remove(const value_type &value) {
            // 这里的[&]是什么含义
            // 将另一元操作 pred 为 true 的所有元素移除
            // 没搞懂这个函数的含义？
            remove_if([&](const value_type &v) {
                return v == value;
            });
        }

        template<class UnaryPredicate>
        void remove_if(UnaryPredicate pred);

        void unique() {
            unique(mystl::equal_to<T>());
        }

        template<class BinaryPredicate>
        void unique(BinaryPredicate pred);

        void merge(list &x) {
            merge(x, mystl::less<T>());
        }

        template<class Compare>
        void merge(list &x, Compare comp);

        void sort() {
            list_sort(begin(), end(), size(), mystl::less<T>());
        }

        template<class Compared>
        void sort(Compared comp) {
            list_sort(begin(), end(), comp);
        }

        void reverse();

    private:
        // helper functions
        // 一些函数的声明

        // create / destroy node
        template<class ...Args>
        node_ptr create_node(Args &&...args);

        void destroy_node(node_ptr p);

        // initialize
        void fill_init(size_type n, const value_type &value);

        template<class Iter>
        void copy_init(Iter first, Iter last);

        // link / unlink
        iterator link_iter_node(const_iterator pos, base_ptr node);

        void link_nodes(base_ptr p, base_ptr first, base_ptr last);

        void link_nodes_at_front(base_ptr first, base_ptr last);

        void link_nodes_at_back(base_ptr first, base_ptr last);

        void unlink_nodes(base_ptr f, base_ptr l);

        // assign
        void fill_assign(size_type n, const value_type &value);

        template<class Iter>
        void copy_assign(Iter first, Iter last);

        // insert
        iterator fill_insert(const_iterator pos, size_type n, const value_type &value);

        template<class Iter>
        iterator copy_insert(const_iterator pos, size_type n, Iter first);

        // sort
        template<class Compared>
        iterator list_sort(iterator first, iterator last, size_type n, Compared comp);

    };

/************************************************************************************************/

// 删除pos处的元素
    template<class T>
    typename list<T>::iterator
    list<T>::erase(const_iterator pos) {
        // cend是const迭代器，返回最后位置的节点
        MYSTL_DEBUG(pos != cend());
        auto n = pos.node_;
        auto next = n->next;
        // 这个是什么功能？
        unlink_nodes(n, n);
        destroy_node(n->as_node());
        --size_;
        // 返回删除后的下一个位置的迭代器
        return iterator(next);
    }

// 删除[first, last)内的元素
    template<class T>
    typename list<T>::iterator
    list<T>::erase(const_iterator first, const_iterator last) {
        if (first != last) {
            // 断开连接？
            // last指向最后一个节点的后一个，所以要prev
            unlink_nodes(first.node_, last.node_->prev);
            while (first != last) {
                auto cur = first.node_;
                ++first;
                destroy_node(cur->as_node());
                --size_;
            }
        }
        return iterator(last.node_);
    }

// 清空list
    template<class T>
    void list<T>::clear() {
        if (size_ != 0) {
            auto cur = node_->next;
            for (base_ptr next = cur->next; cur != node_; cur = next, next = cur->next) {
                destroy_node(cur->as_node());
            }
            node_->unlink();
            size_ = 0;
        }
    }

// 重置容器大小
    template<class T>
    void list<T>::resize(size_type new_size, const value_type &value) {
        auto i = begin();
        size_type len = 0;
        // 将指针移动到new_size的位置
        while (i != end() && len < new_size) {
            ++i;
            ++len;
        }
        if (len == new_size) {
            // 将多余位置擦除
            erase(i, node_);
        } else {
            // 若不够长度，则插入指定值在后面填充
            insert(node_, new_size - len, value);
        }
    }

// 将 list x 接合于 pos 之前
    template<class T>
    void list<T>::splice(const_iterator pos, list &x) {
        MYSTL_DEBUG(this != &x);
        if (!x.empty()) {
            THROW_LENGTH_ERROR_IF(size_ > max_size() - x.size_, "list<T>'s size too big");

            // 找到 x 的起始和结束位置
            auto f = x.node_->next;
            auto l = x.node_.prev;

            // 断开连接？
            x.unlink_nodes(f, l);
            // pos.node_ 指向 pos 的当前节点
            // 将f到l之间的元素连接到 pos 的当前节点后面
            link_nodes(pos.node_, f, l);

            size_ += x.size_;
            x.size_ = 0;
        }
    }

// 将 list x 中 it 所指的节点接合于 pos 之前
// 这个函数的作用没有搞懂？
    template<class T>
    void list<T>::splice(const_iterator pos, list &x, const_iterator it) {
        if (pos.node_ != it.node_ && pos.node_ != it.node_->next) {
            THROW_LENGTH_ERROR_IF(size_ > max_size() - x.size_, "list<T>'s size too big");

            auto f = it.node_;
            // 断开节点
            x.unlink_nodes(f, f);
            // 连接节点
            link_nodes(pos.node_, f, f);

            ++size_;
            // 将x中的一个节点移走所以--
            --x.size_;
        }
    }

// 将list x的[first, last) 内的节点接合于pos之前
    template<class T>
    void list<T>::splice(const_iterator pos, list<T> &x, const_iterator first, const_iterator last) {
        if (first != last && this != &x) {
            size_type n = mystl::distance(first, last);
            THROW_LENGTH_ERROR_IF(size_ > max_size() - x.size_, "list<T>'s size too big");
            auto f = first.node_;
            auto l = last.node_->prev;

            x.unlink_nodes(f, l);
            link_nodes(pos.node_, f, l);

            size_ += n;
            x.size_ -= n;
        }
    }

// 将另一元操作 pred 为 true 的所有元素移除
    template<class T>
    template<class UnaryPredicate>
    void list<T>::remove_if(UnaryPredicate pred) {
        auto f = begin();  // list的开头
        auto l = end();   // list的结尾
        for (auto next = f; f != l; f = next) {
            ++next;
            // 将符合判断条件的元素移除
            if (pred(*f)) {
                erase(f);
            }
        }
    }

// 移除list中满足perd为true的重复元素
    template<class T>
    template<class BinaryPredicate>
    void list<T>::unique(BinaryPredicate pred) {
        auto i = begin();
        auto e = end();
        auto j = i;
        ++j;
        while (j != e) {
            if (pred(*i, *j)) {
                erase(j);
            } else {
                i = j;
            }
            j = i;
            j++;
        }
    }

// 与另一个list合并，按照comp为true的顺序
    template<class T>
    template<class Compare>
    void list<T>::merge(list<T> &x, Compare comp) {
        if (this != &x) {
            THROW_LENGTH_ERROR_IF(size_ > max_size() - x.size_, "list<T>'s size too big");

            // 当前的begin和end
            auto f1 = begin();
            auto l1 = end();
            // 另一个list的begin和end
            auto f2 = x.begin();
            auto l2 = x.end();

            while (f1 != l1 && f2 != l2) {
                // 根据comp条件进行判断
                if (comp(*f2, *f1)) {
                    // 使comp为true的一段区间
                    auto next = f2;
                    ++next;
                    // 一直移动到不满足comp判断条件为止
                    for (; next != l2 && comp(*next, *f1); ++next);
                    auto f = f2.node_;
                    auto l = next.node_->prev;
                    f2 = next;

                    // 断开并重新连接节点
                    x.unlink_nodes(f, l);
                    link_nodes(f1.node_, f, l);
                    ++f1;
                } else {
                    ++f1;
                }
            }
            // 连接剩余部分
            if (f2 != l2) {
                auto f = f2.node_;
                auto l = l2.node_->prev;
                x.unlink_nodes(f, l);
                link_nodes(l1.node_, f, l);
            }

            size_ += x.size_;
            x.size_ = 0;
        }
    }

// 将 list 反转
    template<class T>
    void list<T>::reverse() {
        if (size_ <= 1) {
            return;
        }
        auto i = begin();
        auto e = end();
        while (i.node_ != e.node_) {
            // 交换当前节点的前后节点
            mystl::swap(i.node_->prev, i.node_->next);
            i.node_ = i.node_->prev;
        }
        mystl::swap(e.node_->prev, e.node_->next);
    }

/*****************************************************************************************/
// helper function



// 容器与 [first, last] 结点断开连接
    template<class T>
    void list<T>::unlink_nodes(base_ptr first, base_ptr last) {
        first->prev->next = last->next;
        last->next->prev = first->prev;
    }

} // namespace mystl
#endif // !MYTINYSTL_LIST_H_

