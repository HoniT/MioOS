// =======================================================================
// Copyright Ioane Baidoshvili 2024.
// Distributed under the terms of the MIT License.
// =======================================================================

#pragma once

#ifndef TREE_HPP
#define TREE_HPP

void test_tree_print();

// Struct for tree node
struct TreeNode {
    // Data
    char* data;
    TreeNode* firstChild;
    TreeNode* nextSibling;

    // Constructor and destructor equivalent without using the 'new' or 'delete' operators
    void init(const char* value);
    void clean();
};

// Tree class
class Tree {
private:
    TreeNode* root;

    // Helper function to recursively delete nodes
    void deleteTree(TreeNode* node);

    // Helper function to print tree
    void printTree(TreeNode* node, int depth) const;

public:
    // Constructor
    Tree(const char* rootData);

    // Destructor
    ~Tree();

    // Add a child to a parent node
    TreeNode* addChild(TreeNode* parent, const char* childData);

    // Get the root node
    TreeNode* getRoot() const;

    // Print the tree
    void print() const;
};

#endif // TREE_HPP
