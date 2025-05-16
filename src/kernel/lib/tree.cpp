// =======================================================================
// Copyright Ioane Baidoshvili 2025.
// Distributed under the terms of the MIT License.
// (See accompanying file LICENSE or copy at
// http://www.opensource.org/licenses/MIT_1_0.txt)
//
// fs.cpp contains the tree data structure logic
// =======================================================================

#include <lib/tree.hpp>
#include <mm/heap.hpp>
#include <drivers/vga_print.hpp>
#include <lib/string_util.hpp>

void test_tree_print() {
    Tree myTree("root");

    TreeNode* root = myTree.getRoot();

    TreeNode* child1 = myTree.addChild(root, "child1");
    TreeNode* child2 = myTree.addChild(root, "child2");
    TreeNode* child3 = myTree.addChild(root, "child3");

    myTree.addChild(child1, "child1.1");
    myTree.addChild(child1, "child1.2");

    myTree.addChild(child2, "child2.1");

    myTree.addChild(child3, "child3.1");
    myTree.addChild(child3, "child3.2");
    myTree.addChild(child3, "child3.3");

    vga::printf("=== Tree Structure ===\n");
    myTree.print();
}


// Constructor and destructor for tree node
void TreeNode::init(const char* value) {
    // Allocate memory for the node's data
    this->data = (char*)kmalloc(strlen(value) + 1);
    if (this->data) {
        strcpy(this->data, value);
    } else {
        // Handle allocation failure (optional, depending on your OS design)
        vga::printf("Error: Memory allocation failed for TreeNode data\n");
    }

    // Initialize pointers
    this->firstChild = nullptr;
    this->nextSibling = nullptr;
}

void TreeNode::clean() {
    // Free allocated memory for data
    if (this->data) {
        kfree(this->data);
        this->data = nullptr;
    }
}

// Tree constructor
Tree::Tree(const char* rootData) {
    // Allocate memory for the root node
    root = (TreeNode*)kmalloc(sizeof(TreeNode));
    if (root) {
        root->init(rootData);
    } else {
        vga::printf("Error: Memory allocation failed for Tree root\n");
    }
}

// Tree destructor
Tree::~Tree() {
    deleteTree(root);
    root = nullptr; // Nullify root pointer to prevent dangling reference
}

// Recursively delete nodes
void Tree::deleteTree(TreeNode* node) {
    if (!node) return;

    deleteTree(node->firstChild);
    deleteTree(node->nextSibling);

    node->clean(); // Clean the node
    kfree(node);    // Free the memory for the node itself
}

// Add a child to a parent node
TreeNode* Tree::addChild(TreeNode* parent, const char* childData) {
    if (!parent) return nullptr;

    // Allocate memory for a new TreeNode
    TreeNode* newChild = (TreeNode*)kmalloc(sizeof(TreeNode));
    if (newChild) {
        newChild->init(childData); // Initialize the new node
    } else {
        vga::printf("Error: Memory allocation failed for TreeNode\n");
        return nullptr;
    }

    // Add as the first child or to the sibling list
    if (!parent->firstChild) {
        parent->firstChild = newChild;
    } else {
        TreeNode* sibling = parent->firstChild;
        while (sibling->nextSibling) {
            sibling = sibling->nextSibling;
        }
        sibling->nextSibling = newChild;
    }

    return newChild;
}

// Get the root node
TreeNode* Tree::getRoot() const {
    return root;
}

// Print the tree
void Tree::print() const {
    printTree(root, 0);
}

// Helper function to print the tree
void Tree::printTree(TreeNode* node, int depth) const {
    if (!node) return;

    // Indent based on depth
    for (int i = 0; i < depth; i++) {
        vga::printf(" "); // Print spaces for indentation
    }

    // Print node data
    if (node->data) {
        vga::printf(node->data);
    }
    vga::printf("\n");

    // Recursively print children and siblings
    printTree(node->firstChild, depth + 1);
    printTree(node->nextSibling, depth);
}
