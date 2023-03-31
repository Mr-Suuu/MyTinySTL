#ifndef MYTINYSTL_RB_TREE_H_
#define MYTINYSTL_RB_TREE_H_

// 这个头文件包含一个模板类 rb_tree
// rb_tree : 红黑树
// 插入删除频繁则用红黑树

/**
 * 红黑树：
 *      1.节点是红色或黑色
 *      2.根节点是黑色
 *      3.每个叶子节点都是黑色的空节点（NIL节点）
 *      4.每个红色节点的两个子节点都是黑色。也就是说从每个叶子到根的所有路径上不能有两个连续的红色节点
 *      5.从任一节点到其每个叶子的所有路径都包含相同数目的黑色节点
 *
 *      红黑树，读取略逊于AVL，维护强于AVL，每次插入和删除的平均旋转次数应该是远小于平衡树
 *      如果插入的数据很乱AVL就不太行了，所以这里使用红黑树
 * **/

#include <initializer_list>
#include <cassert>
#include "functional.h"
#include "iterator.h"
#include "memory.h"
#include "type_traits.h"
#include "exceptdef.h"

namespace mystl {
    // rb tree 节点颜色类型
    typedef bool rb_tree_color_type;

    static constexpr rb_tree_color_type rb_tree_red = false;
    static constexpr rb_tree_color_type rb_tree_black = true;

    // 前置声明
    template<class T>
    struct rb_tree_node_base;
    template<class T>
    struct rb_tree_node;

    template<class T>
    struct rb_tree_iterator;
    template<class T>
    struct rb_tree_const_iterator;

    // rb tree value traits
    // 红黑树值的萃取器
    template<class T, bool>
    struct rb_tree_value_traits_imp {
        typedef T key_type;
        typedef T mapped_type;
        typedef T value_type;

        // 返回值
        template<class Ty>
        static const key_type &get_key(const Ty &value) {
            return value;
        }

        template<class Ty>
        static const value_type &get_value(const Ty &value) {
            return value;
        }
    };

    template<class T>
    struct rb_tree_value_traits_imp<T, true> {
        typedef typename std::remove_cv<typename T::first_type>::type key_type;
        typedef typename T::second_type mapped_type;
        typedef T value_type;

        template<class Ty>
        static const value_type &get_value(const Ty &value) {
            return value;
        }
    };

    template<class T>
    struct rb_tree_value_traits {
        static constexpr bool is_map = mystl::is_pair<T>::value;

        typedef rb_tree_value_traits_imp<T, is_map> value_traits_type;
        typedef typename value_traits_type::key_type key_type;
        typedef typename value_traits_type::mapped_type mapped_type;
        typedef typename value_traits_type::value_type value_type;

        template<class Ty>
        static const key_type &get_key(const Ty &value) {
            return value_traits_type::get_key(value);
        }

        template<class Ty>
        static const value_type &get_value(const Ty &value) {
            return value_traits_type::get_value(value);
        }
    };

    // rb tree node traits
    template<class T>
    struct rb_tree_value_traits {
        typedef rb_tree_color_type color_type;

        typedef rb_tree_value_traits<T> value_traits;
        typedef typename value_traits::key_type key_type;
        typedef typename value_traits::mapped_type mapped_type;
        typedef typename value_traits::value_type value_type;

        typedef rb_tree_node_base<T> *base_ptr;
        typedef rb_tree_node<T> *node_ptr;
    };

    // rb tree 的节点设计
    // 基类
    template<class T>
    struct rb_tree_node_base {
        typedef rb_tree_color_type color_type;
        typedef rb_tree_node_base<T> *base_ptr;
        typedef rb_tree_node<T> *node_ptr;

        base_ptr parent;  // 父节点
        base_ptr left;    // 左子节点
        base_ptr right;   // 右子节点
        color_type color;   // 节点颜色

        base_ptr get_base_ptr() {
            return &*this;
        }

        // 获得当前节点的指针
        node_ptr get_node_ptr() {
            return reinterpret_cast<node_ptr>(&*this);
        }

        // 获得当前节点的引用
        node_ptr &get_node_ref() {
            return reinterpret_cast<node_ptr &>(*this);
        }
    };

    // 派生类
    template<class T>
    struct rb_tree_node : public rb_tree_node_base<T> {
        typedef rb_tree_node_base<T> *base_ptr;  // 基类指针
        typedef rb_tree_node<T> *node_ptr;  // 派生类指针

        T value;  // 节点值

        base_ptr get_base_ptr() {
            return static_cast<base_ptr>(&*this);
        }

        node_ptr get_node_ptr() {
            return &*this;
        }
    };

    // rb tree traits
    template<class T>
    struct rb_tree_traits {
        typedef rb_tree_value_traits<T> value_traits;

        typedef typename value_traits::key_type key_type;
        typedef typename value_traits::mapped_type mapped_type;
        typedef typename value_traits::value_type value_type;

        typedef value_type *pointer;
        typedef value_type &reference;
        typedef const value_type *const_pointer;
        typedef const value_type &const_reference;

        typedef rb_tree_node_base<T> base_type;
        typedef rb_tree_node<T> node_type;

        typedef base_type *base_ptr;
        typedef node_type *node_ptr;
    };

    // rb tree 的迭代器设计
    // 迭代器的基类
    template<class T>
    struct rb_tree_iterator_base : public mystl::iterator<mystl::bidirectional_iterator_tag, T> {
        typedef typename rb_tree_traits<T>::base_ptr base_ptr;

        base_ptr node;  // 指向节点本身

        rb_tree_iterator_base() : node(nullptr) {}

        // 使迭代器前进
        //
        void inc() {
            if (node->right != nullptr) {
                // 若有右子树，则找到右子树上最小的节点
                node = rb_tree_min(node->right);
            } else {
                // 如果没有右节点
                // 若是父节点的左节点，则返回父节点
                // 若是父节点的右节点，则迭代父节点根据其所在的父父节点的左右位置重复上一条和这一条，直至返回
                auto y = node->parent;
                while (y->right == node) {
                    node = y;
                    y = y->parent;
                }
                if (node->right != y) {
                    // 应对“寻找根节点的下一节点，而根节点没有右子节点”的特殊情况
                    node = y;
                }
            }
        }

        // 使迭代器后退
        void dec() {
            if (node->parent->parent == node && rb_tree_is_red(node)) {
                // 这个判断条件不是很懂？
                // 如果node为header
                node = node->right;  // 指向整棵树的 max 节点
            } else if (node->left != nullptr) {
                // 找到左子树的最大节点
                node = rb_tree_max(node->left);
            } else {
                // 非header节点，也无左节点
                // 与前进类似，找到父节点的左边最大元素，或继续进行迭代
                auto y = node->parent;
                while (node == y->left) {
                    node = y;
                    y = y->parent;
                }
                node = y;
            }
        }

        bool operator==(const rb_tree_iterator_base &rhs) { return node == rhs.node; }

        bool operator!=(const rb_tree_iterator_base &rhs) { return node != rhs.node; }
    };

    // 迭代器派生类
    template<class T>
    struct rb_tree_iterator : public rb_tree_iterator_base<T> {
        typedef rb_tree_traits<T> tree_traits;

        typedef typename tree_traits::value_type value_type;
        typedef typename tree_traits::pointer pointer;
        typedef typename tree_traits::reference reference;
        typedef typename tree_traits::base_ptr base_ptr;
        typedef typename tree_traits::node_ptr node_ptr;

        typedef rb_tree_iterator<T> iterator;
        typedef rb_tree_const_iterator<T> const_iterator;
        typedef iterator self;

        using rb_tree_iterator_base<T>::node;

        // 构造函数
        rb_tree_iterator() {}

        rb_tree_iterator(base_ptr x) { node = x; }

        rb_tree_iterator(node_ptr x) { node = x; }

        // ()赋值
        rb_tree_iterator(const iterator &rhs) { node = rhs.node; }

        rb_tree_iterator(const const_iterator &rhs) { node = rhs.node; }

        // 重载操作符
        // *取值
        reference operator*() const { return node->get_node_ptr()->value; }

        pointer operator->() const { return &(operator*()); }

        self &operator++() {
            this->inc();
            return *this;
        }

        self operator++(int) {
            self tmp(*this);
            this->inc();
            return tmp;
        }

        self &operator--() {
            this->dec();
            return *this;
        }

        self operator--(int) {
            self tmp(*this);
            this->dec();
            return tmp;
        }
    };


}

#endif