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
    template<class T>
    class deque {
    public:
        // deque 的型别定义
        typedef mystl::allocator<T> allocator_type;
        typedef mystl::allocator<T> data_allocator;
        typedef mystl::allocator<T *> map_allocator;

        typedef typename allocator_type::value_type value_type;
        typedef typename allocator_type::pointer pointer;
        typedef typename allocator_type::const_pointer const_pointer;
        typedef typename allocator_type::reference reference;
        typedef typename allocator_type::const_reference const_reference;
        typedef typename allocator_type::size_type size_type;
        typedef typename allocator_type::difference_type difference_type;
        typedef pointer *map_pointer;
        typedef const_pointer *const_map_pointer;

        typedef deque_iterator<T, T &, T *> iterator;
        typedef deque_iterator<T, const T &, const T *> const_iterator;
        typedef mystl::reverse_iterator<iterator> reverse_iterator;
        typedef mystl::reverse_iterator<const_iterator> const_reverse_iterator;

        allocator_type get_allocator() { return allocator_type(); }

        static const size_type buffer_size = deque_buf_size<T>::value;

    private:
        // 用以下四个数据来表现一个deque
        iterator begin_;  // 指向第一个节点
        iterator end_;  // 指向最后一个节点
        map_pointer map_;  // 指向一块map，map中的每个元素都是一个指针，指向一个缓冲区
        size_type map_size_;  // map 内指针的数目

    public:
        // 构造、复制、移动、析构函数
        deque() { fill_init(0, value_type()); }

        // explicit：禁止隐式转换
        explicit deque(size_type n) { fill_init(n, value_type()); }

        deque(size_type n, const value_type &value) { fill_init(n, value); }

        // typename和class作用类似，定义普通的类型可以使用class，但定义带::的用typename
        template<class IIter, typename std::enable_if<
                mystl::is_input_iterator<IIter>::value, int>::type = 0>
        deque(IIter first, IIter last) { copy_init(first, last, iterator_category(first)); }

        deque(std::initializer_list<value_type> ilist) {
            copy_init(ilist.begin(), ilist.end(), mystl::forward_iterator_tag());
        }

        deque(const deque &rhs) {
            copy_init(rhs.begin(), rhs.end(), mystl::forward_iterator_tag());
        }

        // noexcept不报错
        deque(deque &&rhs) noexcept
                : begin_(mystl::move(rhs.begin_)),
                  end_(mystl::move(rhs.end_)),
                  map_(rhs.map_),
                  map_size_(rhs.map_size_) {
            rhs.map_ = nullptr;
            rhs.map_size_ = 0;
        }

        deque &operator=(const deque &rhs);

        deque &operator=(deque &&rhs);

        // 可将列表初始化作为参数传入
        deque &operator=(std::initializer_list<value_type> ilist) {
            deque tmp(ilist);
            swap(tmp);
            return *this;
        }

        // 析构函数
        ~deque() {
            if (map_ != nullptr) {
                clear();
                data_allocator::deallocate(*begin_.node, buffer_size);
                *begin_.node = nullptr;
                map_allocator::deallocate(map_, map_size_);
                map_ = nullptr;
            }
        }

    public:
        // 迭代器相关操作
        iterator begin() noexcept { return begin_; }

        // const_iterator：只能读取，不能修改
        const_iterator begin() const noexcept { return begin_; }

        iterator end() noexcept { return end_; }

        const_iterator end() const noexcept { return end_; }

        // 反向迭代
        reverse_iterator rbegin() noexcept { return reverse_iterator(end()); }

        const_reverse_iterator rbegin() const noexcept { return reverse_iterator(end()); }

        reverse_iterator rend() noexcept { return reverse_iterator(begin()); }

        const_reverse_iterator rend() const noexcept { return reverse_iterator(begin()); }

        // cbegin：返回const的begin
        const_iterator cbegin() const noexcept { return begin(); }

        const_iterator cend() const noexcept { return end(); }

        // crbegin：返回const的rbegin
        const_reverse_iterator crbegin() const noexcept { return rbegin(); }

        const_reverse_iterator crend() const noexcept { return rend(); }

        // 容量相关操作
        bool empty() const noexcept { return begin() == end(); }

        size_type size() const noexcept { return end_ - begin_; }

        size_type max_size() const noexcept { return static_cast<size_type>(-1); }

        void resize(size_type new_size) { resize(new_size, value_type()); }

        void resize(size_type new_size, const value_type &value);

        // shrink_to_fit：减少容器的容量以适应其大小并销毁超出容量的所有元素
        void shrink_to_fit() noexcept;

        // 访问元素的相关操作
        reference operator[](size_type n) {
            MYSTL_DEBUG(n < size());
            return begin_[n];
        }

        const_reference operator[](size_type n) const {
            MYSTL_DEBUG(n < size());
            return begin_[n];
        }

        // at：返回对出现在双端队列中位置 n 的元素的引用
        reference at(size_type n) {
            THROW_OUT_OF_RANGE_IF(!(n < size()), "deque<T>::at() subscript out of range");
            return (*this)[n];
        }

        const_reference at(size_type n) const {
            THROW_OUT_OF_RANGE_IF(!(n < size()), "deque<T>::at() subscript out of range");
            return (*this)[n];
        }

        // 返回开头的引用
        reference front() {
            MYSTL_DEBUG(!empty());
            return *begin();
        }

        const_reference front() const {
            MYSTL_DEBUG(!empty());
            return *begin();
        }

        // 返回结尾的引用
        reference back() {
            MYSTL_DEBUG(!empty());
            return *(end() - 1);
        }

        const_reference back() const {
            MYSTL_DEBUG(!empty());
            return *(end() - 1);
        }

        // 修改容器相关操作
        // assign：为 deque 容器分配新的内容，并相应地修改容器的大小
        // 如：assign(5, 6)  -> 6 6 6 6 6
        void assign(size_type n, const value_type &value) {
            fill_assign(n, value);
        }

        // 功能同上，传参不同，这里传入的是两个迭代器对象
        template<class IIter, typename std::enable_if<
                mystl::is_input_iterator<IIter>::value, int>::type = 0>
        void assign(IIter first, IIter last) {
            // iterator_category？写完这个写iterator.h
            copy_assign(first, last, iterator_category(first));
        }

        // 功能同上，传参不同，这里传入的是一个初始化好的列表
        void assign(std::initializer_list<value_type> ilist) {
            copy_assign(ilist.begin(), ilist.end(), mystl::forward_iterator_tag{});
        }

        // emplace_front / emplace_back / emplace

        // emplace_front：在双端队列的前面插入新元素并将双端队列的大小增加一
        template<class ...Args>
        void emplace_front(Args &&...args);

        // emplace_back：在双端队列的后面插入新元素并将双端队列的大小增加一
        template<class ...Args>
        void emplace_back(Args &&...args);

        // emplace：在双端队列指定位置插入新元素并将双端队列的大小增加一
        template<class ...Args>
        void emplace(iterator pos, Args &&...args);

        // push_front / push_back
        // emplace效率比push高，因为不用调用移动构造函数和拷贝构造函数
        // push_front：在开始处插入元素
        void push_front(const value_type &value);

        // push_back：在结束位置插入元素
        void push_back(const value_type &value);

        // 在开始位置插入元素，这里的push调用了移动构造函数
        void push_back(value_type &&value) { emplace_back(mystl::move(value)); }

        // 在结束位置插入元素，这里的push调用了移动构造函数
        void push_front(value_type &&value) { emplace_front(mystl::move(value)); }

        // pop_back / pop_front
        // 弹出开头元素
        void pop_front();

        // 弹出结尾元素
        void pop_back();

        // insert
        // 在指定位置插入元素
        iterator insert(iterator position, const value_type &value);

        // 在指定位置插入元素
        iterator insert(iterator position, value_type &&value);

        // 在指定位置插入n个元素
        void insert(iterator position, size_type n, const value_type &value);

        // 在指定位置插入迭代器first到last之间的元素
        template<class IIter, typename std::enable_if<
                mystl::is_input_iterator<IIter>::value, int>::type = 0>
        void insert(iterator position, IIter first, IIter last) {
            insert_dispatch(position, first, last, iterator_category(first));
        }

        // erase /clear
        // 删除指定位置的元素
        iterator erase(iterator position);

        // 删除迭代器first到last之间的元素
        iterator erase(iterator first, iterator last);

        // 清空整个队列
        void clear();

        // swap
        // 交换队列
        void swap(deque &rhs) noexcept;

    private:
        // helper functions

        // 创建节点 / 销毁节点
        // create node / destroy node
        map_pointer create_map(size_type size);

        void create_buffer(map_pointer nstart, map_pointer nfinish);

        void destroy_buffer(map_pointer nstart, map_pointer nfinish);

        // initialize  初始化
        void map_init(size_type nElem);

        void fill_init(size_type n, const value_type &value);

        template<class IIter>
        void copy_init(IIter first, IIter last, input_iterator_tag);

        template<class FIter>
        void copy_init(FIter, FIter, forward_iterator_tag);

        // assign  为 deque 容器分配新的内容，并相应地修改容器的大小
        void fill_assign(size_type n, const value_type &value);

        template<class IIter>
        void copy_assign(IIter first, IIter last, input_iterator_tag);

        template<class FIter>
        void copy_assign(FIter first, FIter last, forward_iterator_tag);

        // insert  插入
        template<class... Args>
        iterator insert_aux(iterator position, Args &&...args);

        void fill_insert(iterator position, size_type n, const value_type &value);

        template<class FIter>
        void copy_insert(iterator, FIter, FIter, size_type);

        template<class IIter>
        void insert_dispatch(iterator, IIter, IIter, input_iterator_tag);

        template<class FIter>
        void insert_dispatch(iterator, FIter, FIter, forward_iterator_tag);

        // reallocate  重新分配
        void require_capacity(size_type n, bool front);

        void reallocate_map_at_front(size_type need);

        void reallocate_map_at_back(size_type need)

    };

/*****************************************************************************************/

// 复制赋值运算符
    template<class T>
    deque<T> &deque<T>::operator=(const deque &rhs) {
        if (this != &rhs) {
            // 若当前地址不等于rhs的地址
            const auto len = size();  // 记录当前空间大小
            if (len >= rhs.size()) {
                // 若当前空间大小大于等于rhs的空间大小
                // 复制rhs到当前begin_开始的位置，并将复制后的结束位置到当前的end_的元素擦除
                erase(mystl::copy(rhs.begin_, rhs.end_, begin_), end_);
            } else {
                // 若当前空间大小小于rhs的空间大小
                // 记录rhs与当前空间大小一样长的位置
                iterator mid = rhs.begin() + static_cast<difference_type>(len);
                // 先将和当前空间大小长度一致的位置复制到从当前空间begin_开始的地方
                mystl::copy(rhs.begin_, mid, begin_);
                // 剩余元素通过insert插入在后面
                insert(end_, mid, rhs.end_);
            }
        }
        return *this;
    }

// 移动赋值运算符
// 将rhs移动到当前deque地址下
    template<class T>
    deque<T> &deque<T>::operator=(deque<T> &&rhs) {
        clear(); // 清空当前deque
        // 更新各参数
        begin_ = mystl::move(rhs.begin_);
        end_ = mystl::move(rhs.end_);
        map_ = rhs.map_;
        map_size_ = rhs.map_size_;
        // 将原rhs置为空
        rhs.map_ = nullptr;
        rhs.map_size_ = 0;
        return *this;
    }

// 重置容器大小
    template<class T>
    void deque<T>::resize(size_type new_size, const value_type &value) {
        const auto len = size();  // 获取当前容量
        if (new_size < len) {
            // 若当前容量大于新容量
            // 擦除新容量后的元素
            erase(begin_ + new_size, end_);
        } else {
            // 若当前容量小于新容量
            // 在结尾后面插入new_size - len个value进行填充至新容量大小
            insert(end_, new_size - len, value);
        }
    }

// 减小容器容量
    template<class T>
    void deque<T>::shrink_to_fit() noexcept {
        // 至少会留下头部缓冲区
        // map_：指向一块map，map中的每个元素都是一个指针，指向一个缓冲区
        for (auto cur = map_; cur < begin_.node; ++cur) {
            // 将头节点之前的空间全部清空并置为nullptr
            data_allocator::deallocate(*cur, buffer_size);
            *cur = nullptr;
        }
        for (auto cur = end_.node; cur < map_ + map_size_; ++cur) {
            // 将 尾结点之后 且 小于map_指向的map的最后一个位置 内的空间清空并置为nullptr
            data_allocator::deallocate(*cur, buffer_size);
            *cur = nullptr;
        }
    }

// 在头部就地构建元素
    template<class T>
    template<class ...Args>
    void deque<T>::emplace_front(Args &&...args) {
        if (begin_.cur != begin_.first) {
            // 若当前节点不等于头节点
            // cur 指向所在缓冲区的当前元素
            // first 指向所在缓冲区的头部
            // 在当前元素前一个位置构造元素
            data_allocator::construct(begin_.cur - 1, mystl::forward<Args>(args)...);
            --begin_.cur; // 将指向的当前元素前移一位指向新构建的元素
        } else {
            // 若当前节点等于头节点
            // 请求一个新的空间
            require_capacity(1, true);
            try {
                --begin_; // 将起始节点前移一位
                // 在新空间上构建新的元素
                data_allocator::construct(begin_.cur, mystl::forward<Args>(args)...);
            }
            catch (...) {
                // 若报错则将begin_恢复并抛出
                ++begin_;
                throw;
            }
        }
    }

// 在尾部就地构造元素
    template<class T>
    template<class ...Args>
    void deque<T>::emplace_back(Args &&args...) {
        if (end_.cur != end_.last - 1) {
            // 若当前结尾不等于空间结尾位置，则直接插入
            data_allocator::construct(end_.cur, mystl::forward<Args>(args)...);
            ++end_.cur;
        } else {
            // 若当前结尾等于空间结尾位置，即空间不足
            // 则请求一个空间并构建元素插入
            require_capacity(1, false);  // 这个false是干嘛的？
            data_allocator::construct(end_.cur, mystl::forward<Args>(args)...);
            ++end_;
        }
    }

// 在pos位置就地构建元素
    template<class T>
    template<class ...Args>
    typename deque<T>::iterator deque<T>::emplace(iterator pos, Args &&args...) {
        if (pos.cur == begin_.cur) {
            // 若插入位置等于起始位置，则在头部就地构建元素
            emplace_front(mystl::forward<Args>(args)...);
            return begin_;
        } else if (pos.cur == end_.cur) {
            // 若插入位置等于结尾位置，则在尾部就地构建元素
            emplace_back(mystl::forward<Args>(args)...);
            return end_ - 1;
        }
        // 否则则进行对应位置的插入
        return insert_aux(pos, mystl::forward<Args>(args)...);
    }

// 在头部插入元素
// 和emplace_front的区别是这里传入的直接是一个value，不用自己构建
    template<class T>
    void deque<T>::push_front(const value_type &value) {
        if (begin_.cur != begin_.first) {
            // 若begin_的当前位置不等于begin_缓冲区的开始位置，则在头部插入并将begin_前移一位
            data_allocator::construct(begin_.cur - 1, value);
            --begin_.cur;
        } else {
            // 若begin_的当前位置等于begin_缓冲区的开始位置，即空间不足，则请求一个空间并插入
            require_capacity(1, true);
            try {
                --begin_;
                data_allocator::construct(begin_.cur, value);
            } catch (...) {
                // 若出错则恢复begin_并抛出错误
                ++begin_;
                throw;
            }
        }
    }

// 在尾部插入元素
    template<class T>
    void deque<T>::push_back(const value_type &value) {
        if (end_.cur != end_.last - 1) {
            // 若空间充足，则直接插入
            data_allocator::construct(end_.cur, value);
            ++end_.cur;
        } else {
            // 若空间不足，则请求空间并插入
            require_capacity(1, false);
            data_allocator::construct(end_.cur, value);
            ++end_;
        }
    }

// 弹出头部元素
    template<class T>
    void deque<T>::pop_front() {
        MYSTL_DEBUG(!empty());
        if (begin_.cur != begin_.last - 1) {
            // 若begin缓冲区当前位置不等于begin缓冲区的结尾位置
            // 则销毁头部元素，并令begin缓冲区当前位置后移一位，即弹出头部元素
            data_allocator::destroy(begin_.cur);
            ++begin_.cur;
        } else {
            // 若begin缓冲区当前位置等于begin缓冲区的结尾位置
            // 则销毁头部元素当前区域，并令begin缓冲区整个后移一位，并销毁原begin缓冲区
            data_allocator::destroy(begin_.cur);
            ++begin_;
            destroy_buffer(begin_.node - 1, begin_.node - 1);
        }
    }

// 弹出尾部元素
    template<class T>
    void deque<T>::pop_back() {
        MYSTL_DEBUG(!empty());
        if (end_.cur != end_.first) {
            // 若end缓冲区当前位置不等于end缓冲区起始位置
            // 则先将end缓冲区的cur进行前移（因为end的cur指向的是最后一个元素的后一位？），再销毁
            --end_.cur;
            data_allocator::destroy(end_.cur);
        } else {
            // 若end缓冲区当前位置等于end缓冲区起始位置
            // 则将整个end缓冲区进行前移（因为end的cur指向的是最后一个元素的后一位？），再销毁
            // 最后将原end缓冲区整个销毁
            --end_;
            data_allocator::destroy(end_.cur);
            destroy_buffer(end_.node + 1, end_.node + 1);
        }
    }

// 在position处插入元素
    template<class T>
    typename deque<T>::iterator
    deque<T>::insert(iterator position, const value_type &value) {
        if (position.cur == begin_.cur) {
            // 若position的当前位置等于begin缓冲区的当前位置
            // 则直接进行头插法
            push_front(value);
            return begin_;  // 返回插入的位置
        } else if (position.cur == end_.cur) {
            // 若position的当前位置等于end缓冲区的当前位置
            // 则直接进行尾插法
            push_back(value);
            auto tmp = end_;
            --tmp;
            return tmp;  // 返回插入的位置
        } else {
            // 若不在首尾则通过aux插入
            return insert_aux(position, value);
        }
    }

    template<class T>
    typename deque<T>::iterator
    deque<T>::insert(iterator position, value_type &&value) {
        // 这个和上面有什么区别？为什么要通过就地构建元素完成？
        if (position.cur == begin_.cur) {
            emplace_front(mystl::move(value));
            return begin_;
        } else if (position.cur == end_.cur) {
            emplace_back(mystl::move(value));
            auto tmp = end_;
            --tmp;
            return tmp;
        } else {
            return insert_aux(position, mystl::move(value));
        }
    }

// 在position位置插入n个元素
    template<class T>
    void deque<T>::insert(iterator position, size_type n, const value_type &value) {
        if (position.cur == begin_.cur) {
            // 若插入位置在头部
            // 则在头部申请n个空间
            require_capacity(n, true);
            // 更新新的begin
            auto new_begin = begin_ - n;
            // 初始化新加入的空间为value
            mystl::uninitialized_fill_n(new_begin, n, value);
            begin_ = new_begin;
        } else if (position.cur == end_.cur) {
            // 若插入位置在尾部
            // 则在尾部申请n个空间
            require_capacity(n, false);
            // 更新新的end
            auto new_end = end_ + n;
            // 初始化新加入的空间为value
            mystl::uninitialized_fill_n(end_, n, value);
            end_ = new_end;
        } else {
            // 若在中间插入，则通过fill_insert完成
            fill_insert(position, n, value);
        }
    }

// 删除position处的元素
    template<class T>
    typename deque<T>::iterator
    deque<T>::erase(iterator position) {
        auto next = position;  // 记录position位置
        ++next;  // 移动到position后一位
        const size_type elems_before = position - begin_;  // 记录在position之前有几个元素
        if (elems_before < (size() / 2)) {
            // 若position前元素个数小于整体元素个数的一半
            // 将[begin, position)之间的元素复制到以next结尾的地方
            mystl::copy_backward(begin_, position, next);
            // 弹出首元素（因为多余，首元素在第二个位置复制得到过）
            pop_front();
        } else {
            // 若position前的元素大于整体元素的一半
            // 则复制[next, end_)之间的元素到以position开头的地方
            mystl::copy(next, end_, position);
            // 弹出为元素
            pop_back();
        }
        // 返回弹出的位置
        return begin_ + elems_before;
    }

// 删除[first, last)上的元素
    template<class T>
    typename deque<T>::iterator
    deque<T>::erase(iterator first, iterator last) {
        if (first == begin_ && last == end_) {
            // 若清除的范围刚好是整个空间，则直接clear并返回结束位置
            clear();
            return end_;
        } else {
            // 若清除的范围不是整个空间
            // 计算删除的长度
            const size_type len = last - first;
            // 计算第一个删除的元素距离起始位置的距离
            const size_type elems_before = first - begin_;
            // 这个if else的目的是为了移动更少的元素
            if (elems_before < ((size() - len) / 2)) {
                // 若[begin_, first)之间的元素少于[last, end_]之间的元素
                // 复制[begin_, first)之间的元素到以last结尾的位置
                mystl::copy_backward(begin_, first, last);
                // 定义新的起始位置
                auto new_begin = begin_ + len;
                // 销毁[begin, new_begin]之间的数据
                data_allocator::destroy(begin_.cur, new_begin.cur);
                // 更新起始位置
                begin_ = new_begin;
            } else {
                // 若 第一个删除的元素距离起始位置的距离 大于
                // 复制[last, end_]之间的元素到以first起始的位置
                mystl::copy(last, end_, first);
                // 定义新的结束位置
                auto new_end = end_ - len;
                // 销毁新的结束位置到旧的结束位置之间的数据
                data_allocator::destroy(new_end.cur, end_.cur);
                // 更新结束位置
                end_ = new_end;
            }
            return begin_ + elems_before;
        }
    }

// 清空 deque
    template<class T>
    void deque<T>::clear() {
        // clear 会保留头部的缓冲区
        for (map_pointer cur = begin_.node + 1; cur < end_.node; ++cur) {
            // 从头部下一个开始销毁到结束缓冲区之前
            data_allocator::destroy(*cur, *cur + buffer_size);
        }
        if (begin_.node != end_.node) {
            // 有两个以上的缓冲区
            // 分别销毁
            mystl::destroy(begin_.cur, begin_.last);
            mystl::destroy(end_.first, end_.cur);
        } else {
            // 若头部等于尾部，则一起销毁
            mystl::destroy(begin_.cur, end_.cur);
        }
        // 减小容器容量
        shrink_to_fit();
        // 更新结束位置
        end_ = begin_;
    }

// 交换两个deque
    template<class T>
    void deque<T>::swap(deque<T> &rhs) noexcept {
        if (this != &rhs) {
            // 交换两个不相等的deuqe的全部信息
            mystl::swap(begin_, rhs.begin_);
            mystl::swap(end_, rhs.end_);
            mystl::swap(map_, rhs.map_);
            mystl::swap(map_size_, rhs.map_size_);
        }
    }

/**************************************************************************************************/
// helper function

// create_map 函数
    template<class T>
    typename deque<T>::map_pointer
    deque<T>::create_map(size_type size) {
        map_pointer mp = nullptr;  // 创建一个map指针
        mp = map_allocator::allocate(size);  // mp指向构建为size大小的map
        for (size_type i = 0; i < size; ++i) {
            // map中全部指向空
            *(mp + i) = nullptr;
        }
        return mp;
    }

// create_buffer 函数
    template<class T>
    void deque<T>::
    create_buffer(map_pointer nstart, map_pointer nfinish) {
        map_pointer cur;
        try {
            for (cur = nstart; cur <= nfinish; ++cur) {
                // 在每一个map指针中构建buffer
                *cur = data_allocator::allocate(buffer_size);
            }
        } catch (...) {
            // 若出现异常则全部操作销毁
            while (cur != nstart) {
                --cur;
                data_allocator::deallocate(*cur, buffer_size);
                *cur = nullptr;
            }
            throw;
        }
    }

// destroy_buffer 函数
    template<class T>
    void deque<T>::
    destroy_buffer(map_pointer nstart, map_pointer nfinish) {
        for (map_pointer n = nstart; n <= nfinish; ++n) {
            // 释放当前buffer的内存空间
            data_allocator::deallocate(*n, buffer_size);
            // 将指针置为空
            *n = nullptr;
        }
    }

// map_init 函数
    template<class T>
    void deque<T>::
    map_init(size_type nElem) {
        const size_type nNode = nElem / buffer_size + 1; // 需要分配的缓冲区个数
        // 设置 map_size_为固定的初始化大小 和 当前要求分配的缓冲区个数+2 中取最大
        map_size_ = mystl::max(static_cast<size_type>(DEQUE_MAP_INIT_SIZE), nNode + 2);
        try {
            // 创建map
            map_ = create_map(map_size_);
        } catch (...) {
            // 若出错则销毁map并抛出异常
            map_ = nullptr;
            map_size_ = 0;
            throw;
        }

        // 让 nstart 和 nfinish 都指向 map_ 最中央的区域，方便向头尾扩充
        map_pointer nstart = map_ + (map_size_ - nNode) / 2;
        map_pointer nfinish = nstart + nNode - 1;
        try {
            // 创建buffer
            create_buffer(nstart, nfinish);
        } catch (...) {
            // 若出错则销毁
            map_allocator::deallocate(map_, map_size_);
            map_ = nullptr;
            map_size_ = 0;
            throw;
        }
        // 更新各个参数
        begin_.set_node(nstart);
        end_.set_node(nfinish);
        begin_.cur = begin_.first;
        end_.cur = end_.first + (nElem % buffer_size);
    }

// fill_init 函数
// 初始化并填充n个value
    template<class T>
    void deque<T>::
    fill_init(size_type n, const value_type &value) {
        map_init(n);  // 初始化n个空间
        if (n != 0) {
            for (auto cur = begin_.node; cur < end_.node; ++cur) {
                // 将每一个buffer填充为value
                mystl::uninitialized_fill(*cur, *cur + buffer_size, value);
            }
            // 清空end_中的数据？
            mystl::uninitialized_fill(end_.first, end_.cur, value);
        }
    }

// copy_init 函数
// 初始化并复制[first, last)之间的元素
    template<class T>
    template<class IIter>
    void deque<T>::
    copy_init(IIter first, IIter last, input_iterator_tag) {
        const size_type n = mystl::distance(first, last);  // 计算有多少个元素
        map_init(n);  // 初始化n个空间
        for (; first != last; ++first) {
            // 使用尾插法原地构建元素
            emplace_back(*first);
        }
    }

    template<class T>
    template<class FIter>
    void deque<T>::
    copy_init(IIter first, IIter last, forward_iterator_tag) {
        const size_type n = mystl::distance(first, last);  // 计算距离
        map_init(n);  // 初始化map
        for (auto cur = begin_.node; cur < end_.node; ++cur) {
            auto next = first;
            mystl::advance(next, buffer_size);  // 前进buffer_size
            mystl::uninitialized_copy(first, next, *cur);  // 赋值初始化
            first = next;  // 将first后移
        }
        mystl::uninitialized_copy(first, last, end_.first);
    }

// fill_assign 函数
    template<class T>
    void deque<T>::
    fill_assign(size_type n, const value_type &value) {
        if (n > size()) {
            // 若n大于当前空间大小
            // 则先全部填充
            mystl::fill(begin(), end(), value);
            // 再在后面插入
            insert(end(), n - size(), value);
        } else {
            // 否则擦去多余空间
            erase(begin() + n, end());
            // 填充value
            mystl::fill(begin(), end(), value);
        }
    }

// copy_assign 函数
    template<class T>
    template<class IIter>
    void deque<T>::
    copy_assign(IIter first, IIter last, input_iterator_tag) {
        // 记录下当前空间的begin和end的位置
        auto first1 = begin();
        auto last1 = end();
        for (; first != last && first1 != last1; ++first, ++first1) {
            // 移动first1
            *first1 = *first;
        }
        if (first1 != last1) {
            // 擦去[first1, last1]即[first1, end]的元素
            erase(first1, last1);
        } else {
            // 这是干嘛的？
            insert_dispatch(end_, first, last, input_iterator_tag{});
        }
    }

    template<class T>
    template<class FIter>
    void deque<T>::
    copy_assign(FIter first, FIter last, forward_iterator_tag) {
        const size_type len1 = size();  // 当前尺寸
        const size_type len2 = mystl::distance(first, last);  // 计算距离
        if (len1 < len2) {
            // 若当前空间小于要复制的空间
            auto next = first;
            mystl::advance(next, len1);  // 后移len1位
            mystl::copy(first, next, begin_);  // 复制[first, next]到begin_开头的位置
            // 这个dispatch是干嘛的？
            insert_dispatch(end_, next, last, forward_iterator_tag{});
        } else {
            // 若当前空间更大
            // 则直接复制到以begin开头的位置并将last后的元素删除
            erase(mystl::copy(first, last, begin_), end_);
        }
    }

// insert_aux 函数
    template<class T>
    template<class... Args>
    typename deque<T>::iterator
    deque<T>::
    insert_aux(iterator position, Args &&args...) {
        const size_type elems_before = position - begin_;  // 计算插入位置前有几个元素
        value_type value_copy = value_type(mystl::forward<Args>(args)...);
        if (elems_before < (size() / 2)) {
            // 在前半段插入
            // 为什么都要++？
            emplace_front(front());  // 在开头原地构造元素
            auto front1 = begin_;
            ++front1;
            auto front2 = front1;
            ++front2;
            position = begin_ + elems_before;
            auto pos = position;
            ++pos;
            // 复制[front2, pos)到以front1开头的位置
            mystl::copy(front2, pos, front1);
        } else {
            // 在后半段插入
            // 与上面原理类似
            emplace_back(back());
            auto back1 = end_;
            --back1;
            auto back2 = back1;
            --back2;
            popsition = begin_ + elems_before;
            mystl::copy_backward(position, back2, back1);
        }
        *position = mystl::move(value_copy);
        return position;
    }

// fill_insert 函数
    template<class T>
    void deque<T>::
    fill_insert(iterator position, size_type n, const value_type &value) {
        const size_type elems_before = position - begin_;
        const size_type len = size();
        auto value_copy = value;
        if (elems_before < (len < 2)) {
            require_capacity(n, true);  // 在开头请求空间
            // 原来的迭代器可能会失效
            auto old_begin = begin_;
            auto new_begin = begin_ + n;
            position = begin_ + elems_before;
            try {
                if (elems_before >= n) {
                    auto begin_n = begin_ + n;
                    mystl::uninitialized_copy(begin_, begin_n, new_begin);
                    begin_ = new_begin;
                    mystl::copy(begin_n, position, old_begin);
                    mystl::fill(position - n, position, value_copy);
                } else {
                    mystl::uninitialized_fill(mystl::uninitialized_copy(begin_, position, new_begin), begin_,
                                              value_copy);
                    begin_ = new_begin;
                    mystl::fill(old_begin, position, value_copy);
                }
            } catch (...) {
                if (new_begin.node != begin_.node) {
                    destroy_buffer(new_begin.node, begin_.node - 1);
                    throw;
                }
            }
        } else {
            require_capacity(n, false);
            // 原来的迭代器可能会失效
            auto old_end = end_;
            auto new_end = end_ + n;
            const size_type elems_after = len - elems_before;
            position = end_ - elems_after;
            try {
                if (elems_after > n) {
                    auto end_n = end_ - n;
                    mystl::uninitialized_copy(end_n, end_, end_);
                    end_ = new_end;
                    mystl::copy_backward(position, end_n, end_);
                    end_ = new_end;
                    mystl::copy_backward(position, end_n, old_end);
                    mystl::fill(position, position + n, value_copy);
                }
            } catch (...) {
                if (new_end.node != end_.node) {
                    destroy_buffer(end_.node + 1, new_end.node);
                }
                throw;
            }
        }
    }

// copy_insert 函数
    template<class T>
    template<class FIter>
    void deque<T>::
    copy_insert(iterator position, FIter first, FIter last, size_type n) {
        const size_type elems_before = position - begin_;
        auto len = size();
        if (elems_before < (len / 2)) {
            require_capacity(n, true);
            // 原来的迭代器可能失效
            auto old_begin = begin_;
            auto new_begin = begin_ - n;
            position = begin_ + elems_before;
            try {
                if (elems_before >= n) {
                    auto begin_n = begin_ + n;
                    mystl::uninitialized_copy(begin_, begin_n, new_begin);
                    begin_ = new_begin;
                    mystl::copy(begin_n, position, old_begin);
                    mystl::copy(first, last, position - n);
                } else {
                    auto mid = first;
                    mystl::advance(mid, n - elems_before);
                    mystl::uninitialized_copy(first, mid, mystl::uninitialized_copy(begin_, position, new_begin));
                    begin_ = new_begin;
                    mystl::copy(mid, last, old_begin);
                }
            } catch (...) {
                if (new_begin.node != begin_.node) {
                    destroy_buffer(new_begin.node, begin_.node - 1);
                }
                throw;
            }
        } else {
            require_capacity(n, false);
            // 原来的迭代器可能会失效
            auto old_end = end_;
            auto new_end = end_ + n;
            const auto elems_after = len - elems_before;
            position = end_ - elems_after;
            try {
                if (elems_after > n) {
                    auto end_n = end_ - n;
                    mystl::uninitialized_copy(end_n, end_, end_);
                    end_ = new_end;
                    mystl::copy_backward(position, end_n, old_end);
                    mystl::copy(first, last, position);
                } else {
                    auto mid = first;
                    mystl::advance(mid, elems_after);
                    mystl::uninitialized_copy(position, end_, mystl::uninitialized_copy(mid, last, end_));
                    end_ = new_end;
                    mystl::copy(first, mid, position);
                }
            } catch (...) {
                if (new_end.node != end_.node) {
                    destroy_buffer(end_.node + 1, new_end.node);
                }
                throw;
            }
        }
    }

// insert_dispatch 函数
// 通过调用insert函数实现
    template<class T>
    template<class IIter>
    void deque<T>::
    insert_dispatch(iterator position, IIter first, IIter last, input_iterator_tag) {
        if (last <= first) return;
        const size_type n = mystl::distance(first, last);
        const size_type elems_before = position - begin_;
        if (elems_before < (size() / 2)) {
            require_capacity(n, true);  // 在头部申请空间
        } else {
            require_capacity(n, false); // 在结尾申请空间
        }
        position = begin_ + elems_before;
        auto cur = --last;
        for (size_type i = 0; i < n; ++i, --cur) {
            // 从后往前将first-last之间的数据插入
            insert(position, *cur);
        }
    }

    template<class T>
    template<class FIter>
    void deque<T>::
    insert_dispatch(iterator position, IIter first, IIter last, forward_iterator_tag) {
        if (last <= first) return;
        const size_type n = mystl::distance(first, last);
        if (position.cur == begin_.cur) {
            // 若在开头插入则在开头请求空间
            require_capacity(n, true);
            auto new_begin = begin_ - n;
            try {
                // 将[first, last)复制到new_begin起始的位置
                mystl::uninitialized_copy(first, last, new_begin);
                begin_ = new_begin;
            } catch (...) {
                // 若出错则销毁新增的空间
                if (new_begin.node != begin_.node) {
                    destroy_buffer(new_begin.node, begin_.node - 1);
                }
                throw;
            }
        } else if (position.cur == end_.cur) {
            // 若在结尾处插入则在结尾申请空间
            require_capacity(n, false);
            auto new_end = end_ + n;
            try {
                // 将[first, last)复制到end_起始的位置
                mystl::uninitialized_copy(first, last, end_);
                end_ = new_end;
            } catch (...) {
                if (new_end.node != end_.node) {
                    destroy_buffer(end_.node + 1, new_end.node);
                }
                throw;
            }
        } else {
            // 若在中间则通过copy_insert函数实现
            copy_insert(position, first, last, n);
        }
    }

// require_capacity 函数
    template<class T>
    void deque<T>::require_capacity(size_type n, bool front) {
        if (front && (static_cast<size_type>(begin_.cur - begin_.first) < n)) {
            // 头插并且begin_空间的当前位置和起始位置之间的空间小于n
            // 计算需要多少空间
            const size_type need_buffer = (n - (begin_.cur - begin_.first)) / buffer_size + 1;
            if (need_buffer > static_cast<size_type>(begin_.node - map_)) {
                // 这里没太搞懂，是需要的空间更多？
                // 在前端再分配更大的空间
                reallocate_map_at_front(need_buffer);
                return;
            }
            // 创建buffer
            create_buffer(begin_.node - need_buffer, begin_.node - 1);
        } else if (!front && (static_cast<size_type>(end_.last - end_.cur - 1) < n)) {
            // 若实在尾部插入则同理
            const size_type need_buffer = (n - (end_.last - end_.cur - 1)) / buffer_size + 1;
            if (need_buffer > static_cast<size_type>((map_ + map_size_) - end_.node - 1)) {
                reallocate_map_at_back(need_buffer);
                return;
            }
            create_buffer(end_.node + 1, end_.node + need_buffer);
        }
    }

// reallocate_map_at_front 函数
    template<class T>
    void deque<T>::reallocate_map_at_front(size_type need_buffer) {
        // 判断并获得需要增加的map数量
        const size_type new_map_size = mystl::max(map_size_ << 1, map_size_ + need_buffer + DEQUE_MAP_INIT_SIZE);
        // 创建map
        map_pointer new_map = create_map(new_map_size);
        // 记录旧的buffer大小
        const size_type old_buffer = end_.node - begin_.node + 1;
        // 计算新的buffer大小
        const size_type new_buffer = old_buffer + need_buffer;

        // 令新的map中的指针指向原来的buffer，并开辟新的buffer
        auto begin = new_map + (new_map_size - new_buffer) / 2;  // 指向新的起始位置
        auto mid = begin + new_buffer;  // 指向原buffer的位置
        auto end = mid + old_buffer;  // 指向整个buffer的结尾
        // 在mid前记原空间数据前创建buffer
        create_buffer(begin, mid - 1);
        for (auto begin1 = mid, begin2 = begin_.node; begin1 != end; ++begin1, ++begin2) {
            // 将旧的buffer中的数据转移到新的buffer中，以mid为起始位置
            *begin1 = *begin2;
        }

        // 更新数据
        map_allocator::deallocate(map_, map_size_);
        map_ = new_map;
        map_size_ = new_map_size;
        begin_ = iterator(*mid + (begin_.cur - begin_.first), mid);
        end_ = iterator(*(end - 1) + (end_.cur - end_.first), end - 1);
    }

// reallocate_map_at_back 函数
    template<class T>
    void deque<T>::reallocate_map_at_back(size_type need_buffer) {
        // 获得应该增加的map大小
        const size_type new_map_size = mystl::max(map_size_ << 1, map_size_ + need_buffer + DEQUE_MAP_INIT_SIZE);
        // 创建新的map
        map_pointer new_map = create_map(new_map_size);
        // 记录旧的buffer空间大小
        const size_type old_buffer = end_.node - begin_.node + 1;
        // 计算新的buffer大小
        const size_type new_buffer = old_buffer + need_buffer;

        // 令新的map中的指针指向原来的buffer，并开辟新的buffer
        // 计算新的起始位置
        auto begin = new_map + ((new_map_size - new_buffer) / 2);
        // 计算旧的空间的起始位置
        auto mid = begin + old_buffer;
        // 计算结尾位置
        auto end = mid + need_buffer;
        for (auto begin1 = begin, begin2 = begin_.node; begin1 != mid; ++begin1, ++begin2) {
            // 将旧空间的数据赋值给新空间对应位置，以mid为结束位置
            *begin1 = *begin2;
        }

        // 在mid后创建buffer
        create_buffer(mid, end - 1);

        // 更新数据
        map_allocator::deallocate(map_, map_size_);
        map_ = new_map;
        map_size_ = new_map_size;
        begin_ = iterator(*begin + (begin_.cur - begin_.first), begin);
        end_ = iterator(*(mid - 1) + (end_.cur - end_.first), mid - 1);
    }

// 重载比较操作符
    template<class T>
    bool operator==(const deque<T> &lhs, const deque<T> &rhs) {
        // 比较空间大小和值是否相等
        return lhs.size() == rhs.size() &&
               mystl::equal(lhs.begin(), lhs.end(), rhs.begin());
    }

    template<class T>
    bool operator<(const deque<T> &lhs, const deque<T> &rhs) {
        //     检查第一个范围 [lhs.begin(), lhs.end()) 是否按字典序小于
        //        第二个范围 [rhs.begin(), rhs.begin() + (lhs.end() - lhs.begin()))
        return mystl::lexicographical_compare(lhs.begin(), lhs.end(), rhs.begin(), rhs.end());
    }

    template<class T>
    bool operator!=(const deque<T> &lhs, const deque<T> &rhs) {
        // 利用重载的==实现
        return !(lhs == rhs);
    }

    template<class T>
    bool operator>(const deque<T> &lhs, const deque<T> &rhs) {
        return rhs < lhs;
    }

    template<class T>
    bool operator<=(const deque<T> &lhs, const deque<T> &rhs) {
        return !(rhs < lhs);
    }

    template<class T>
    bool operator>=(const deque<T> &lhs, const deque<T> &rhs) {
        return !(lhs < rhs);
    }

// 重载 mystl 的 swap 函数
    template<class T>
    void swap(deque<T> &lhs, deque<T> &rhs) {
        lhs.swap(rhs);
    }

} // namespace mystl
#endif // !MYTINYSTL_DEQUE_H_

