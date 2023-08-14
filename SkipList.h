#pragma once

#include <iostream>
#include <string>
#include <set>
#include <time.h>
#include <climits>

using namespace std;

// 比较函数结构体，用于比较键的大小
template<typename T>
struct Less {
    bool operator () (const T & a , const T & b) const {
        return a < b;
    }
};

// 跳表类模板
template<typename K, typename V,typename Comp = Less<K>>
class skip_list {
private:
    // 跳表的节点结构体
    struct skip_list_node {
        int level;                // 节点的层级
        const K key;              // 键的值
        V value;                  // 值
        //forward 是指针数组，指向下一个节点的不同层
        skip_list_node** forward;

        // 默认节点构造函数
        skip_list_node() : key{ 0 }, value{ 0 }, level{ 0 }, forward{nullptr} {}

        // 带参节点构造函数
        skip_list_node(K k, V v, int l, skip_list_node* nxt = nullptr) :key(k), value(v), level(l) {
            forward = new skip_list_node * [level + 1];  // 创建指向下一层节点的指针数组
            for (int i = 0; i <= level; ++i) {
                forward[i] = nxt;  // 初始化指针数组
            }
        }

        // 节点析构函数
        ~skip_list_node() { delete[] forward; }
    };

    using node = skip_list_node;
    // 初始化头节点
    void init() {
        srand((uint32_t)time(NULL));  // 初始化随机数种子
        level = length = 0;
        head->forward = new node * [MAXL + 1];  // 创建头节点指向下一层节点的指针数组
        for (int i = 0; i <= MAXL; i++) {
            head->forward[i] = tail;  // 初始化头节点指向尾节点
        }
    }

    int randomLevel() {
        int lv = 1; while ((rand() & S) < PS) ++lv;  // 生成随机层级
        return MAXL > lv ? lv : MAXL;  // 返回最终层级，限制在最大层级范围内
    }

    int level;             // 跳表的当前层级
    int length;            // 跳表的长度
    static const int MAXL = 32;  // 最大层级
    static const int P = 4;      // 概率因子
    static const int S = 0xFFFF; // 随机数的范围
    static const int PS = S / P; // 随机数的概率，增加一层的概率默认为 0.25
    static const int INVALID = INT_MAX; // 无效值

    node *head, *tail;   // 头节点和尾节点
    Comp less;           // 比较函数对象

    // 根据键值查找节点。具体找到的是第一个键值不小于 key 的节点。
    // 在插入元素时，由于我们是在返回元素的前面添加，所以需要用 update 来记录每一层的前驱结点
    node* find(const K& key, node** update) {
        node* p = head;
        // 从最高层开始找
        for (int i = level; i >= 0; i--) {
            // 如果满足：没有到尾结点并且当前键值小于目标键值，则继续寻找
            while (p->forward[i] != tail && less(p->forward[i]->key, key)) {
                p = p->forward[i];
            }
            update[i] = p;  // 更新每层的前驱节点
        }
        p = p->forward[0];
        return p;
    }

public:
    // 迭代器结构体
    struct Iter {
        node* p;  // 指向节点的指针

        Iter() : p(nullptr) {};  // 默认构造函数

        Iter(node* rhs) : p(rhs) {}  // 带参构造函数

        node* operator ->()const { return (p); }  // 重载箭头运算符，返回指针本身

        node& operator *() const { return *p; }  // 重载解引用运算符，返回指针所指的节点

        bool operator == (const Iter& rhs) { return rhs.p == p; }  // 重载相等运算符

        bool operator != (const Iter& rhs) {return !(rhs.p == p); }  // 重载不等运算符

        void operator ++() {p = p->forward[0]; }  // 重载前置自增运算符

        void operator ++(int) { p = p->forward[0]; }  // 重载后置自增运算符
    };

    // 默认构造函数
    skip_list() : head(new node()), tail(new node()), less{Comp()} {
        init();  // 初始化跳表
    }

    // 带比较函数的构造函数
    skip_list(Comp _less) : head(new node()), tail(new node()),  less{_less} {
        init();  // 初始化跳表
    }

    // 插入操作
    void insert(const K& key, const V& value) {
        node * update[MAXL + 1];
        node* p = find(key,update);  // 查找插入位置

        if (p->key == key) {
            p->value = value;  // 如果已经存在相同的键，则更新值
            return;
        }

        int lv = randomLevel();  // 随机生成层级

        if (lv > level) {
            lv = ++level;  // 若生成的层级大于当前层级，则更新当前层级，并将头节点加入更新数组
            update[lv] = head;
        }

        node * newNode = new node(key, value, lv);  // 创建新节点
        for (int i = lv; i >= 0; --i) {
            p = update[i];
            newNode->forward[i] = p->forward[i];  // 更新新节点的指针
            p->forward[i] = newNode;
        }

        ++length;  // 更新长度
    }

    // 删除操作
    bool erase(const K& key) {
        node* update[MAXL + 1];
        node* p = find(key, update);  // 查找要删除的节点

        if (p->key != key) return false;  // 如果未找到对应键的节点，返回false

        for (int i = 0; i <= p->level; ++i) {
            update[i]->forward[i] = p->forward[i];  // 更新前后节点的指针
        }

        delete p;  // 删除节点
        while (level > 0 && head->forward[level] == tail) --level;  // 更新当前层级
        --length;  // 更新长度

        return true;
    }

    // 查找操作
    Iter find(const K&key) {
        node* update[MAXL + 1];
        node* p = find(key, update);  // 查找节点

        if (p == tail) return tail;  // 如果找到的是尾节点，表示未找到
        if (p->key != key) return tail;  // 如果找到的节点的键不等于目标键，表示未找到

        return Iter(p);  // 返回迭代器
    }

    // 判断键是否存在
    bool count(const K& key) {
        node* update[MAXL + 1];
        node* p = find(key, update);  // 查找节点

        if (p == tail) return false;  // 如果找到的是尾节点，表示未找到
        return key == p->key;  // 判断找到的节点的键是否等于目标键
    }

    // 返回尾节点的迭代器
    Iter end() {
        return Iter(tail);
    }   

    // 返回头节点的迭代器
    Iter begin() {
        return Iter(head->forward[0]);
    }
};
