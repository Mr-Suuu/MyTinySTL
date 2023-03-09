#ifndef MYTINYSTL_VECTOR_H_
#define MYTINYSTL_VECTOR_H_

#include <initializer_list>

#include "iterator.h"
#include "memory.h"
#include "util.h"
#include "exceptdef.h"
#include "algo.h"

namespace mystl {

#ifdef max
#pragma message("#undefing marco max")
#undef max
#endif // max

#ifdef min
#pragma message("#undefing marco min")
#undef min
#endif // min

// 模板类: vector 
// 模板参数 T 代表类型
    template<class T>
    class vector {
        // 静态断言，static_assert(常量表达式，提示字符串)
        // 若常量表达式为true则跳过，若为false则产生一条编译错误，错误提示为后面的提示字符串
        // is_name 判断两个类型是否相同  ::value 取值
        static_assert(!std::is_same<bool, T>::value, "vector<bool> is abandoned in mystl");
    public:
        // vector 的嵌套型别定义
        typedef mystl::allocator<T> allocator_type;
        typedef mystl::allocator<T> data_allocator;

        typedef typename allocator_type::value_type value_type;
        typedef typename allocator_type::pointer pointer;
        typedef typename allocator_type::const_pointer const_pointer;
        typedef typename allocator_type::reference reference;
        typedef typename allocator_type::const_reference const_reference;
        typedef typename allocator_type::size_type size_type;
        typedef typename allocator_type::difference_type difference_type;

        typedef value_type *iterator;
        typedef const value_type *const_iterator;
        typedef mystl::reverse_iterator<iterator> reverse_iterator;
        typedef mystl::reverse_iterator<const_iterator> const_reverse_iterator;

        allocator_type get_allocator() { return data_allocator(); }

    private:
        // 私有变量命名以下划线结尾
        iterator begin_;  // 表示目前使用空间的头部
        iterator end_;    // 表示目前使用空间的尾部
        iterator cap_;    // 表示目前储存空间的尾部

    public:
        // 构造、复制、移动、析构函数
        // noexcept表示不抛出异常，效果上与throw相同
        vector() noexcept { try_init(); }

        explicit vector(size_type n) { fill_init(n, value_type()); }

        vector(size_type n, const value_type &value) { fill_init(n, value); }

        // std::enable_if：满足条件时类型有效
        // 这里有问题没看懂
        template<class Iter, typename std::enable_if<
                mystl::is_input_iterator<Iter>::value, int>::type = 0>
        vector(Iter first, Iter last) {
            MYSTL_DEBUG(!(last < first));
            range_init(first, last);
        }

        vector(const vector &rhs) {
            range_init(rhs.begin_, rhs.end_);
        }

        vector(vector &&rhs) noexcept
                : begin_(rhs.begin_),
                  end_(rhs.end_),
                  cap_(rhs.cap_) {
            rhs.begin_ = nullptr;
            rhs.end_ = nullptr;
            rhs.cap_ = nullptr;
        }

        vector(std::initializer_list<value_type> ilist) {
            range_init(ilist.begin(), ilist.end());
        }

        vector &operator=(const vector &rhs);

        vector &operator=(vector &&rhs) noexcept;

        vector &operator=(std::initializer_list<value_type> ilist) {
            vector tmp(ilist.begin(), ilist.end());
            swap(tmp);
            return *this;
        }
        // 上面到这里都是有问题没看懂

        ~vector() {
            destroy_and_recover(begin_, end_, cap_ - begin_);
            begin_ = end_ = cap_ = nullptr;
        }

    public:

        // 迭代器相关操作
        iterator begin() noexcept { return begin_; }

        // const_iterator：只能用于读取容器内的元素，但不能改变其值
        const_iterator begin() const noexcept { return begin_; }

        iterator end() noexcept { return end_; }

        const_iterator end() const noexcept { return end_; }

        // reverse_iterator：反向迭代器适配器，常用来对容器进行反向遍历，即从容器中存储的最后一个元素开始，一直遍历到第一个元素
        // 反向开始就是从结尾开始，即调用end()
        reverse_iterator rbegin() noexcept { return reverse_iterator(end()); }

        const_reverse_iterator rbegin() const noexcept { return const_reverse_iterator(end()); }

        // 从结尾反向开始即从开头开始，即调用begin()
        reverse_iterator rend() noexcept { return reverse_iterator(begin()); }

        const_reverse_iterator rend() const noexcept { return const_reverse_iterator(begin()); }

        // 这几个c是干嘛？
        const_iterator cbegin() const noexcept { return begin(); }

        const_iterator cend() const noexcept { return end(); }

        const_reverse_iterator crbegin() const noexcept { return rbegin(); }

        const_reverse_iterator crend() const noexcept { return rend(); }

        // 容量相关操作
        bool empty() const noexcept { return begin_ == end_; }

        // static_cast：显示转换
        // 返回使用空间容量大小
        size_type size() const noexcept { return static_cast<size_type>(end_ - begin_); }

        // max_size？
        size_type max_size() const noexcept { return static_cast<size_type>(-1) / sizeof(T); }

        // 返回存储空间容量大小
        size_type capacity() const noexcept { return static_cast<size_type>(cap_ - begin_); }

        void reserve(size_type n);

        void shrink_to_fit();

        // 访问元素相关操作
        // reference、const_reference：自己定义的，作用？
        // 重载[]，即访问数组中的元素
        reference operator[](size_type n) {
            MYSTL_DEBUG(n < size());
            return *(begin_ + n);
        }

        const_reference operator[](size_type n) const {
            MYSTL_DEBUG(n < size());
            return *(begin_ + n);
        }

        reference at(size_type n) {
            THROW_OUT_OF_RANGE_IF(!(n < size()), "vector<T>::at() subscript out of range");
            return (*this)[n];
        }

        const_reference at(size_type n) const {
            THROW_OUT_OF_RANGE_IF(!(n < size()), "vector<T>::at() subscript out of range");
            return (*this)[n];
        }

        reference front() {
            MYSTL_DEBUG(!empty());
            return *begin_;
        }

        const_reference front() const {
            MYSTL_DEBUG(!empty());
            return *begin_;
        }

        reference back() {
            MYSTL_DEBUG(!empty());
            return *(end_ - 1);
        }

        const_reference back() const {
            MYSTL_DEBUG(!empty());
            return *(end_ - 1);
        }

        // 这两个point？
        pointer data() noexcept { return begin_; }

        const_pointer data() const noexcept { return begin_; }

        // 修改容器相关操作

        // assign？

        void assign(size_type n, const value_type &value) { fill_assign(n, value); }

        template<class Iter, typename std::enable_if<
                mystl::is_input_iterator<Iter>::value, int>::type = 0>
        void assign(Iter first, Iter last) {
            MYSTL_DEBUG(!(last < first));
            copy_assign(first, last, iterator_category(first));
        }

        void assign(std::initializer_list<value_type> il) {
            copy_assign(il.begin(), il.end(), mystl::forward_iterator_tag{});
        }

        // emplace / emplace_back
        // emplace：在 vector 容器指定位置之前插入一个新的元素
        template<class... Args>
        iterator emplace(const_iterator pos, Args &&...args);

        // emplace_back：在 vector 容器的尾部添加一个元素
        template<class... Args>
        void emplace_back(Args &&...args);

        // push_back / pop_back
        // push_back：在 vector 容器尾部添加一个元素
        void push_back(const value_type &value);

        void push_back(value_type &&value) { emplace_back(mystl::move(value)); }

        // pop_back：弹出容器尾部元素
        void pop_back();

        // insert
        iterator insert(const_iterator pos, const value_type &value);

        iterator insert(const_iterator pos, value_type &&value) { return emplace(pos, mystl::move(value)); }

        iterator insert(const_iterator pos, size_type n, const value_type &value) {
            MYSTL_DEBUG(pos >= begin() && pos <= end());
            // const_cast：擦去const属性
            return fill_insert(const_cast<iterator>(pos), n, value);
        }

        template<class Iter, typename std::enable_if<
                mystl::is_input_iterator<Iter>::value, int>::type = 0>
        void insert(const_iterator pos, Iter first, Iter last) {
            MYSTL_DEBUG(pos >= begin() && pos <= end() && !(last < first));
            copy_insert(const_cast<iterator>(pos), first, last);
        }

        // erase / clear
        iterator erase(const_iterator pos);

        iterator erase(const_iterator first, const_iterator last);

        void clear() { erase(begin(), end()); }

        // resize / reverse
        void resize(size_type new_size) { return resize(new_size, value_type); }

        void resize(size_type new_size, const value_type &value);

        void reverse() { mystl::reverse(begin(), end()); }

        // swap
        void swap(vector &rhs) noexcept;

    private:
        // helper functions

        // initialize / destroy
        void try_init() noexcept;

        void init_space(size_type size, size_type cap);

        void fill_init(size_type n, const value_type &value);

        template<class Iter>
        void range_init(Iter first, Iter last);

        void destroy_and_recover(iterator first, iterator last, size_type n);

        // 计算增长规模
        size_type get_new_cap(size_type add_size);

        // assign
        void fill_assign(size_type n, const value_type &value);

        template<class IIter>
        void copy_assign(IIter first, IIter last, input_iterator_tag);

        template<class FIter>
        void copy_assign(FIter first, FIter last, forward_iterator_tag);

        // reallocate
        template<class... Args>
        void reallocate_emplace(iterator pos, Args &&...args);

        void reallocate_insert(iterator pos, const value_type &value);

        // insert
        iterator fill_insert(iterator pos, size_type n, const value_type &value);

        template<class IIter>
        void copy_insert(iterator pos, IIter first, IIter last);

        // shrink_to_fit
        void reinsert(size_type size);
    };
/**********************************************************************************************/

// 复制赋值操作符
    template<class T>
    vector<T> &vector<T>::operator=(const vector &&rhs) noexcept {
        if (this != &rhs) {
            // auto即自动类型推导，由于auto默认自动推导后不带const，因此要带上const
            const auto len = rhs.size();
            // capacity() 返回存储空间容量大小
            if (len > capacity()) {
                // 原空间较小则新建一个vector，然后复制里面的内容到新空间
                // 但是为什么原空间不要在此时释放呢？难道要等到程序运行结束调用析构函数的时候释放吗
                vector tmp(rhs.begin(), rhs.end()); // 定义一个新的vector
                // swap？ 原理还没搞懂
                swap(tmp);  // 与另一个 vector 交换
            } else if (size() >= len) {
                // size() 是指原空间的大小
                // 原空间较大则复制到原空间，然后释放多余的空间
                // 将rhs复制到当前begin()的位置，并返回rhs？
                auto i = mystl::copy(rhs.begin(), rhs.end(), begin());
                data_allocator::destroy(i, end_);  // 释放空间
                end_ = begin_ + len;
            } else {
                // 这一部分是对应什么情况？
                mystl::copy(rhs.begin(), rhs.begin() + size(), begin_);
                mystl::uninitialized_copy(rhs.begin() + size(), rhs.end(), end_);  // 这个是什么复制？
                cap_ = end_ = begin_ + len;
            }
        }
        return *this;
    }

// 移动赋值操作符
    template<class T>
    vector<T> &vector<T>::operator=(vector<T> &&rhs) noexcept {
        // 先销毁再恢复？
        destroy_and_recover(begin_, end_, cap_ - begin_);
        begin_ = rhs.begin_;
        end_ = rhs.end_;
        cap_ = rhs.cap_;
        rhs.begin_ = nullptr;
        rhs.end_ = nullptr;
        rhs.cap_ = nullptr;
        return *this;
    }

// 预留空间大小，当原容量小于要求大小时，才会重新分配
// reserve的作用是更改vector的容量（capacity），使vector至少可以容纳n个元素
    template<class T>
    void vector<T>::reserve(size_type n) {
        if (capacity() < n) {
            THROW_LENGTH_ERROR_IF(n > max_size(), "n can not larger than maxsize() in vector<T>::reserve(n)");
            const auto old_size = size();
            auto tmp = data_allocator::allocate(n);  // 分配未初始化的存储
            mystl::uninitialized_move(begin_, end_, tmp);  // 移动一定数量对象到未初始化内存区域
            data_allocator::deallocate(begin_, cap_ - begin_);  // 用分配器解分配存储
            begin_ = tmp;
            end_ = tmp + old_size;
            cap_ = begin_ + n;
        }
    }

// 放弃多余的容量
    template<class T>
    void vector<T>::shrink_to_fit() {
        if (end_ < cap_) {
            reinsert(size()); // ？
        }
    }

// 在 pos 位置就地构造元素，避免额外的复制或移动开销
    template<class T>
    template<class ...Args>
    typename vector<T>::iterator
    vector<T>::emplace(const_iterator pos, Args &&...args) {
        MYSTL_DEBUG(pos >= begin() && pos <= end());
        iterator xpos = const_cast<iterator>(pos);  // 获取位置 xpos
        const size_type n = xpos - begin_;  // 计算大小
        if (end_ != cap_ && xpos == end_) {
            // 还有剩余空间且插入位置在队尾
            // ？
            data_allocator::construct(mystl::address_of(*end_), mystl::forward<Args>(args)...);
            ++end_;
        } else if (end_ != cap_) {
            // 还有剩余空间，且插入位置不在队尾
            auto new_end = end_;
            data_allocator::construct(mystl::address_of(*end_), *(end_ - 1));
            ++new_end;
            mystl::copy_backward(xpos, end_ - 1, end_);
            *xpos = value_type(mystl::forward<Args>(args)...);  // value_type是指特定的类型？
            end_ = new_end;
        } else {
            // 若没有剩余空间，则重新分配
            reallocate_emplace(xpos, mystl::forward<Args>(args)...);
        }
        return begin() + n; // 返回添加到的位置
    }

// 在尾部就地构造元素，避免额外的复制或移动开销
    template<class T>
    template<class ...Args>
    void vector<T>::emplace_back(Args &&...args) {
        if (end_ < cap_) {
            // 还有剩余空间
            data_allocator::construct(mystl::address_of(*end_), mystl::forward<Args>(args)...);
            ++end_;
        } else {
            // 若没有剩余空间，则重新分配
            reallocate_emplace(end_, mystl::forward<Args>(args)...);
        }
    }

// 在尾部插入元素
    template<class T>
    void vector<T>::push_back(const value_type &&value) {
        if (end_ != cap_) {
            // 有剩余空间则直接插入
            data_allocator::construct(mystl::address_of(*end), value);
            ++end_;
        } else {
            // 没有剩余空间则重新开辟一片空间并插入
            reallocate_insert(end_, value);
        }
    }

// 弹出尾部元素
    template<class T>
    void vector<T>::pop_back() {
        MYSTL_DEBUG(!empty());
        data_allocator::destroy(end_ - 1);  // 直接销毁最后一个元素
        --end_;
    }

// 在pos出插入元素
    template<class T>
    typename vector<T>::iterator
    vector<T>::insert(const_iterator pos, const value_type &&value) {
        MYSTL_DEBUG(pos >= begin() && pos <= end());
        iterator xpos = const_cast<iterator>(pos);  // const_cast 用于消除const属性
        const size_type n = pos - begin_;
        if (end_ != cap_ && xpos == end_) {
            // 还有剩余空间且在当前队尾插入
            data_allocator::construct(mystl::address_of(*end_), value);
            ++end_;
        } else if (end_ != cap_) {
            // 还有剩余空间，但不在队尾插入
            auto new_end = end_;
            data_allocator::construct(mystl::address_of(*end_), *(end_ - 1));
            ++new_end;
            auto value_copy = value;  // 避免元素因以下复制操作而被改变
            mystl::copy_backward(xpos, end_ - 1, end_);  // 将 xpos - (end_-1) 的元素从后到前依次往后复制
            *xpos = mystl::move(value_copy);  // 插入元素
            end_ = new_end;
        } else {
            // 没有剩余空间
            reallocate_insert(xpos, value);
        }
        return begin_ + n;
    }

// 删除 pos 位置上的元素
    template<class T>
    typename vector<T>::iterator
    vector<T>::erase(const_iterator pos) {
        MYSTL_DEBUG(pos >= begin() && pos < end());
        iterator xpos = begin_ + (pos - begin());
        mystl::move(xpos + 1, end_, xpos);  // 将xpos移至队尾
        data_allocator::destroy(end_ - 1);  // 销毁元素
        --end_;
        return xpos;
    }

// 删除[first, last)上的元素
    template<class T>
    typename vector<T>::iterator
    vector<T>::erase(const_iterator first, const_iterator last) {
        MYSTL_DEBUG(first >= begin() && last <= end() && !(last < first));
        const auto n = first - begin();
        iterator r = begin_ + (first - begin());  // 要删除的起始位置
        data_allocator::destroy(mystl::move(r + (last - first), end_, r), end_);  // 先将要删除区域移动至队尾，再删除
        end_ = end_ - (last - first);
        return begin_ + n; // 返回删除的起始位置
    }

// 重置容器大小
    template<class T>
    void vector<T>::resize(size_type new_size, const value_type &value) {
        if (new_size < size()) {
            // 若新尺寸小于当前尺寸，则删除多余元素
            erase(begin() + new_size, end());
        } else {
            // 若新尺寸大于当前尺寸，则在后面添加value填充
            insert(end(), new_size - size(), value);
        }
    }

// 与另一个vector交换
    template<class T>
    void vector<T>::swap(vector<T> &rhs) noexcept {
        if (this != &rhs) {
            mystl::swap(begin_, rhs.begin_);
            mystl::swap(end_, rhs.end_);
            mystl::swap(cap_, rhs.cap_);
        }
    }
/*********************************************************************************************/
// helper function

// try_init函数，若分配失败则忽略，不抛出异常
    template<class T>
    void vector<T>::try_init() noexcept {
        try {
            begin_ = data_allocator::allocate(16);  // 分配空间
            end_ = begin_;
            cap_ = begin_ + 16;
        }
        catch (...) {
            // 若分配失败则置为空
            begin_ = nullptr;
            end_ = nullptr;
            cap_ = nullptr;
        }
    }

// init_space 函数
    template<class T>
    void vector<T>::init_space(size_type size, size_type cap) {
        try {
            // 初始化指定容量 cap 的空间，并设置当前队尾至 size
            begin_ = data_allocator::allocate(cap);
            end_ = begin_ + size;
            cap_ = begin_ + cap;
        }
        catch (...) {
            begin_ = nullptr;
            end_ = nullptr;
            cap_ = nullptr;
            throw;
        }
    }

// fill_init函数
    template<class T>
    void vector<T>::
    fill_init(size_type n, const value_type &value) {
        // 初始化并全部填入value
        const size_type init_size = mystl::max(static_cast<size_type>(16), n);
        init_space(n, init_size);
        mystl::uninitialized_fill_n(begin_, n, value);
    }

// range_init 函数
    template<class T>
    template<class Iter>
    void vector<T>::
    range_init(Iter first, Iter last) {
        // 初始化一个区间
        const size_type init_size = mystl::max(static_cast<size_type>(last - first),
                                               static_cast<size_type>(16));
        init_space(static_cast<size_type>(last - first), init_size);
        mystl::uninitialized_copy(first, last, begin_);
    }

// destroy_and_recover 函数
    template<class T>
    void vector<T>::
    destroy_and_recover(iterator first, iterator last, size_type n) {
        data_allocator::destroy(first, last);  // 先销毁
        data_allocator::deallocate(first, n);  // 后恢复
    }

// get_new_cap 函数
// 在旧空间的基础上添加空间
    template<class T>
    typename vector<T>::size_type
    vector<T>::
    get_new_cap(size_type add_size) {
        const auto old_size = capacity(); // 获取旧的存储空间容量大小
        // 判断要增加的长度与当前存储空间大小相加是否超出最大值范围
        THROW_LENGTH_ERROR_IF(old_size > max_size() - add_size, "vector<T>'s size too big");
        // 判断当前旧空间是否超过最大空间的2/3，若超过则进入判断
        if (old_size > max_size() - old_size / 2) {
            // 判断是否能在add_size的基础上再加16，若不超出最大值范围则在add_size基础上+16
            return old_size + add_size > max_size() - 16 ? old_size + add_size : old_size + add_size + 16;
        }
        // 若旧空间不超过最大空间的2/3
        // 若旧空间为0则返回add_size和16的最大值，否则返回在旧空间基础上加 旧空间大小的一半 或 add_size 的最大值
        const size_type new_size =
                old_size == 0 ? mystl::max(add_size, static_cast<size_type>(16)) : mystl::max(old_size + old_size / 2,
                                                                                              old_size + add_size);
        return new_size;
    }

// fill_assign 函数
// 分配新的内容到vector中，以代替现在的内容并相应的修改size
    template<class T>
    void vector<T>::
    fill_assign(size_type n, const value_type &value) {
        if (n > capacity()) {
            // 若 n 大于 存储空间大小，则重新创建一个新的vector并全部填充为value值
            vector tmp(n, value);
            swap(tmp);  // swap原理没懂？
        } else if (n > size()) {
            // 如果 n 大于 使用空间容量大小，则直接将整个vector填充为value值
            mystl::fill(begin(), end(), value);
            // uninitialized_fill_n：
            //      作用：将 end_ --- n - size() 的值填充为value
            //      返回值：指向最后复制的元素后一位置元素的迭代器
            end_ = mystl::uninitialized_fill_n(end_, n - size(), value);
        } else {
            // 若 n 小于当前使用空间大小，则先给begin_到n之间的元素复制为value，再将n-end_的元素擦除
            erase(mystl::fill_n(begin_, n, value), end_);
        }
    }

// copy_assign 函数
// 作用？
    template<class T>
    template<class IIter>
    void vector<T>::
    copy_assign(IIter first, IIter last, input_iterator_tag) {
        // input_iterator_tag是干嘛的？
        auto cur = begin_;
        for (; first != last && cur != end_; ++first, ++cur) {
            // 直到cur到end_ 或 first到last结束for循环
            *cur = *first;  // 将cur逐个后移
        }
        if (first == last) {
            // 若到达last，则擦去last到end_的元素（此时last < end）
            erase(cur, end_);
        } else {
            // 若到达end_，则插入元素到后面？（此时end_ < last）
            insert(end_, first, last);
        }
    }

// 用 [first, last) 为容器赋值
    template<class T>
    template<class FIter>
    void vector<T>::
    copy_assign(FIter first, FIter last, forward_iterator_tag) {
        // first和last是输入序列的迭代器，其中包含len个元素
        const size_type len = mystl::distance(first, last);  // 获取first - last 长度
        if (len > capacity()) {
            // 若长度超过存储容量大小，则扩容
            vector tmp(first, last);
            swap(tmp);
        } else if (size() >= len) {
            // 若使用空间容量大小 大于 first到last，则将first到end之间的元素拷贝到从begin_开始的地方
            auto new_end = mystl::copy(first, last, begin_);  // 返回最新的结尾地址
            data_allocator::destroy(new_end, end_);  // 销毁多余的空间
            end_ = new_end;  // 更新结尾地址
        } else {
            // len < capacity() 且 len < size()，即要复制的元素比当前存在的元素多，但长度仍在存储容量范围内
            auto mid = first;
            mystl::advance(mid, size());  // 使迭代器往后走 size() 步
            mystl::copy(first, mid, begin_);  // 将first到mid的元素复制到begin_为起点的位置
            // uninitialized_copy：复制来自范围 [mid, last) 的元素到始于 end_ 的未初始化内存
            auto new_end = mystl::uninitialized_copy(mid, last, end_);
            end_ = new_end;  // 更新end_
        }
    }

// 重新分配空间并在 pos 处就地构造元素
    template<class T>
    template<class ...Args>
    void vector<T>::
    reallocate_emplace(iterator pos, Args &&args...) {
        const auto new_size = get_new_cap(1);  // 在旧空间的基础上添加空间  增加1
        auto new_begin = data_allocator::allocate(new_size);  // 获得新空间的起始位置
        auto new_end = new_begin;  // 将新的终止位置设为上面新的开始位置
        try {
            // uninitialized_move：从范围 [begin_, pos) 移动元素到始于 new_begin 的未初始化内存区域
            new_end = mystl::uninitialized_move(begin_, pos, new_begin);
            // address_of：return &new_end   forward：return static_cast<Args>(args)
            // 在 p 所指的未初始化存储中构造 T 类型对象 ？
            data_allocator::construct(mystl::address_of(*new_end), mystl::forward<Args>(args)...);
            ++new_end;  // 终止位置后移一位
            // uninitialized_move：从范围 [pos, end_) 移动元素到始于 new_end 的未初始化内存区域
            new_end = mystl::uninitialized_move(pos, end_, new_end);
        }
        catch (...) {
            // 若出现异常则恢复并抛出
            // 从指针 new_begin 所引用的存储解分配，其中指针必须是通过先前对 allocate() 获得的指针
            data_allocator::deallocate(new_begin, new_size);
            throw;
        }
        // 销毁并对引用的存储解分配
        destroy_and_recover(begin_, end_, cap_ - begin_);
        // 更新各指针位置
        begin_ = new_begin;
        end_ = new_end;
        cap_ = new_begin + new_size;
    }

// 重新分配空间并在pos处插入元素
    template<class T>
    void vector<T>::
    reallocate_insert(iterator pos, const value_type &value) {
        const auto new_size = get_new_cap(1);  // 在旧空间的基础上添加空间  增加1
        auto new_begin = data_allocator::allocate(new_size);  // 获得新的起始点
        auto new_end = new_begin;  // 定义新的结束点
        const value_type &value_copy = value;  // 要插入的值
        try {
            new_end = mystl::uninitialized_move(begin_, pos, new_begin);
            // 相当于原地构造元素，但是元素给定为value_copy
            data_allocator::construct(mystl::address_of(*new_end), value_copy);
            ++new_end;  // 将结尾点后移一位
            // uninitialized_move：从范围 [pos, end_) 移动元素到始于 new_end 的未初始化内存区域
            new_end = mystl::uninitialized_move(pos, end_, new_end);
        }
        catch (...) {
            // 若出现异常则恢复并抛出
            // 从指针 new_begin 所引用的存储解分配，其中指针必须是通过先前对 allocate() 获得的指针
            data_allocator::deallocate(new_begin, new_size);
            throw;
        }
        // 销毁并对引用的存储解分配
        destroy_and_recover(begin_, end_, cap_ - begin_);
        // 更新各指针位置
        begin_ = new_begin;
        end_ = new_end;
        cap_ = new_begin + new_size;
    }

// fill_insert 函数
// 在指定位置插入n个value，并返回插入的位置
    template<class T>
    typename vector<T>::iterator
    vector<T>::
    fill_insert(iterator pos, size_type n, const value_type &value) {
        if (n == 0) return pos;
        const size_type xpos = pos - begin_;  // 插入位置前面有几个元素
        const value_type value_copy = value;  // 避免被覆盖
        if (static_cast<size_type>(cap_ - end_) >= n) {
            // 若备用空间大于等于增加的空间
            const size_type after_elems = end_ + pos;  // 初始化后移后的结尾位置
            auto old_end = end_;  // 保存旧的结尾位置
            if (after_elems > n) {
                // 若假定的结尾位置比要插入的个数大，这样判断有什么用处？
                // uninitialized_copy：复制来自范围 [end_ - n, end_) 的元素到始于 end_ 的未初始化内存
                mystl::uninitialized_copy(end_ - n, end_, end_);
                end_ += n;  // 结尾位置后移n位
                // 移动来自范围 [pos, old_end - n) 的元素到终于 old_end 的另一范围
                // 可能出现pos > old_end的情况，此时即不用移动元素
                mystl::move_backward(pos, old_end - n, old_end);  // 移出n个位置，用于插入元素
                // 将 [pos, n) 的值填充为value_copy
                mystl::uninitialized_fill_n(pos, n, value_copy);
            } else {
                // 假定的结尾位置比插入个数小或等于，这样判断有什么用处？
                // 这里没搞懂？
                // 将 [end_, n - after_elems) 的值填充为 value_copy
                end_ = mystl::uninitialized_fill_n(end_, n - after_elems, value_copy);
                // 从范围 [pos, old_end) 移动元素到始于 end_ 的未初始化内存区域
                end_ = mystl::uninitialized_move(pos, old_end, end_);
                // 将 [pos, after_elems) 的值填充为 value_copy
                mystl::uninitialized_fill_n(pos, after_elems, value_copy);
            }
        } else {
            // 若备用空间不足
            const auto new_size = get_new_cap(n);  // 在旧空间的基础上添加空间  增加n
            auto new_begin = data_allocator::allocate(new_size);  // 获得新的起始点
            auto new_end = new_begin;  // 初始化新的结束位置
            try {
                // 先将begin_到pos的元素移动到新的地址空间
                new_end = mystl::uninitialized_move(begin_, pos, new_begin);
                // 再填充n个value
                new_end = mystl::uninitialized_fill_n(new_end, n, value);
                // 最后将pos到end_的元素移动过去即可
                new_end = mystl::uninitialized_move(pos, end_, new_end);
            }
            catch (...) {
                // 若出现异常则恢复并抛出
                // 从指针 new_begin 所引用的存储解分配，其中指针必须是通过先前对 allocate() 获得的指针
                destroy_and_recover(new_begin, new_end, new_size);
                throw;
            }
            // 销毁旧的存储空间
            data_allocator::deallocate(begin_, cap_ - begin_);
            // 更新位置
            begin_ = new_begin;
            end_ = new_end;
            cap_ = begin_ + new_size;
        }
        return begin_ + xpos;
    }

// copy_insert 函数
// 复制迭代器中的元素到pos起始的位置
    template<class T>
    template<class IIter>
    void vector<T>::
    copy_insert(iterator pos, IIter first, IIter last) {
        // first和last是输入序列的迭代器，其中包含len个元素
        if (first == last) return;
        const auto n = mystl::distance(first, last);  // 获得first - last的长度
        if ((cap_ - end_) >= n) {
            // 若备用空间大小足够
            const auto after_elems = end_ - pos;
            auto old_end = end_;
            if (after_elems > n) {
                // 若假定的结尾位置比要插入的个数大，这样判断有什么用处？
                // 复制来自范围 [end_ - n, end_) 的元素到始于 end_ 的未初始化内存
                end_ = mystl::uninitialized_copy(end_ - n, end_, end_);
                // 移动来自范围 [pos, old_end - n) 的元素到终于 old_end 的另一范围
                mystl::move_backward(pos, old_end - n, old_end);
                // 复制来自范围 [first, last) 的元素到始于 pos 的未初始化内存
                mystl::uninitialized_copy(first, last, pos);
            } else {
                // 若假定的结尾位置比要插入的个数小，这样判断有什么用处？
                auto mid = first;
                mystl::advance(mid, after_elems);  // 将mid后移after_elems位
                // 复制来自范围 [mid, last) 的元素到始于 end_ 的未初始化内存
                end_ = mystl::uninitialized_copy(mid, last, end_);
                // 移动来自范围 [pos, old_end) 的元素到终于 end_ 的另一范围
                end_ = mystl::uninitialized_move(pos, old_end, end_);
                // 复制来自范围 [first, mid) 的元素到始于 pos 的未初始化内存
                mystl::uninitialized_copy(first, mid, pos);
            }
        } else {
            // 备用空间不足
            const auto new_size = get_new_cap(n);  // 在旧空间的基础上添加空间  增加n
            auto new_begin = data_allocator::allocate(new_size);  // 获取新的起始位置
            auto new_end = new_begin;  // 初始化新的结束位置
            try {
                // 先将begin_到pos的元素移动到新的地址空间
                new_end = mystl::uninitialized_move(begin_, pos, new_begin);
                // 再复制来自范围 [first, last) 的元素到始于 new_end 的未初始化内存
                new_end = mystl::uninitialized_copy(first, last, new_end);
                // 最后将pos到end_的元素移动过去即可
                new_end = mystl::uninitialized_move(pos, end_, new_end);
            }
            catch (...) {
                // 若出现异常则恢复并抛出
                // 从指针 new_begin 所引用的存储解分配，其中指针必须是通过先前对 allocate() 获得的指针
                destroy_and_recover(new_begin, new_end, new_size);
                throw;
            }
            // 销毁旧的存储空间
            data_allocator::deallocate(begin_, cap_ - begin_);
            // 更新位置
            begin_ = new_begin;
            end_ = new_end;
            cap_ = begin_ + new_size;
        }
    }

// reinsert 函数
// 创建新的大小为size的空间，并将旧的空间中的数据移动到新的存储空间中
    template<class T>
    void vector<T>::reinsert(size_type size) {
        auto new_begin = data_allocator::allocate(size);  // 获取大小为size的新空间的起始位置
        try {
            // 复制来自范围 [begin_, end_) 的元素到始于 new_begin 的位置
            mystl::uninitialized_move(begin_, end_, new_begin);
        }
        catch (...) {
            // 若出现异常则恢复并抛出
            // 从指针 new_begin 所引用的存储解分配，其中指针必须是通过先前对 allocate() 获得的指针
            data_allocator::deallocate(new_begin, size);
            throw;
        }
        // 销毁旧的存储空间
        data_allocator::deallocate(begin_, cap_ - begin_);
        // 更新位置
        begin_ = new_begin;
        end_ = begin_ + size;
        cap_ = begin_ + size;
    }

} // namespace mystl
#endif // !MYTINYSTL_VECTOR_H_

