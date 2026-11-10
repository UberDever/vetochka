/* Hand-derived target for the synthetic vf example, not compiler output. */
#include <assert.h>
#include <stdint.h>
#include <stdlib.h>

typedef struct Tree Tree;

enum Tree_tag {
    Tree_Leaf,
    Tree_Node
};

struct Tree {
    enum Tree_tag tag;
    union {
        struct {
            int64_t value;
        } Leaf;
        struct {
            Tree *left;
            int64_t value;
            Tree *right;
        } Node;
    } data;
};

static Tree Tree_Leaf_make(int64_t value)
{
    return (Tree){ .tag = Tree_Leaf, .data.Leaf = { .value = value } };
}

static Tree Tree_Node_make(Tree *left, int64_t value, Tree *right)
{
    return (Tree){
        .tag = Tree_Node,
        .data.Node = { .left = left, .value = value, .right = right }
    };
}

static int64_t sum(Tree *tree)
{
    Tree *vt_match_0 = tree;
    switch (vt_match_0->tag) {
    case Tree_Leaf: {
        int64_t x = vt_match_0->data.Leaf.value;
        return x;
    }
    case Tree_Node: {
        Tree *lhs = vt_match_0->data.Node.left;
        int64_t x = vt_match_0->data.Node.value;
        Tree *rhs = vt_match_0->data.Node.right;
        return sum(lhs) + x + sum(rhs);
    }
    }
    abort();
}

int main(void)
{
    Tree left = Tree_Leaf_make(2);
    Tree right = Tree_Leaf_make(3);
    Tree root = Tree_Node_make(&left, 5, &right);
    Tree deeper = Tree_Node_make(&root, 7, &right);

    assert(sum(&left) == 2);
    assert(sum(&root) == 10);
    assert(sum(&deeper) == 20);
    return 0;
}
