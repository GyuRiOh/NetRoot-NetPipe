
#ifndef __RED__BLACK__TREE__
#define __RED__BLACK__TREE__

#include "MemTLS.h"
#include <functional>

using namespace std;

namespace server_baby
{
    class NetPipe;

    enum color
    {
        RED = 0,
        BLACK
    };

    template<typename DATA, typename Value>
    struct TreeNode
    {
        TreeNode<DATA, Value>* parent;
        TreeNode<DATA, Value>* left, * right;
        DATA key;
        Value value;
        bool color = RED;
    };

    template<typename DATA, typename Value>
    class MyRedBlackTree
    {
    public:
        MyRedBlackTree() : root_(nullptr), size_(NULL)
        {
            NIL_node_ = treePool_->Alloc();
            NIL_node_->color = BLACK;
            NIL_node_->left = nullptr;
            NIL_node_->right = nullptr;
            NIL_node_->parent = nullptr;
            NIL_node_->key = -208;
            NIL_node_->value = NULL;

            root_ = NIL_node_;

        }

        virtual ~MyRedBlackTree()
        {
            if (size_ != NULL)
                CrashDump::Crash();

            treePool_->Free(NIL_node_);
        }

        static void GarbageCollectNodePool()
        {
            treePool_->GarbageCollect();
        }

        bool Insert(DATA key, Value value)
        {
            TreeNode<DATA, Value>* endY = NIL_node_;
            TreeNode<DATA, Value>* startX = root_;
            TreeNode<DATA, Value>* node = nullptr;

            if (startX == NIL_node_)
            {
                NewRoot(key, value);
                size_++;
                return true;
            }

            while (startX != NIL_node_)
            {
                endY = startX;
                if (key < startX->key)
                    startX = startX->left;
                else if (key > startX->key)
                    startX = startX->right;
                else
                    return false;
            }

            if (key < endY->key)
            {
                endY->left = NewNode(key, value, endY);
                node = endY->left;
            }
            else if (key > endY->key)
            {
                endY->right = NewNode(key, value, endY);
                node = endY->right;
            }

            InsertFixup(node);

            size_++;
            return true;
        }

        bool Delete(DATA key, Value* value = nullptr)
        {
            TreeNode<DATA, Value>* deleteNode = IterativeSearchFromRoot(key);
            if (deleteNode == NIL_node_)
                return false;

            TreeNode<DATA, Value>* successorNode = deleteNode;
            bool s_originalColor = successorNode->color;
            TreeNode<DATA, Value>* s_replaceNode = nullptr;

            if (deleteNode->left == NIL_node_)
            {
                s_replaceNode = deleteNode->right;
                RB_Transplant(deleteNode, deleteNode->right);

            }
            else if (deleteNode->right == NIL_node_)
            {
                s_replaceNode = deleteNode->left;
                RB_Transplant(deleteNode, deleteNode->left);
            }
            else
            {
                successorNode = MinValueNode(deleteNode->right);
                s_originalColor = successorNode->color;
                s_replaceNode = successorNode->right;


                if (successorNode->parent == deleteNode)
                {
                    s_replaceNode->parent = successorNode;

                }
                else
                {
                    RB_Transplant(successorNode, successorNode->right);
                    successorNode->right = deleteNode->right;
                    successorNode->right->parent = successorNode;
                }

                RB_Transplant(deleteNode, successorNode);
                successorNode->left = deleteNode->left;
                successorNode->left->parent = successorNode;
                successorNode->color = deleteNode->color;

            }

            if (s_originalColor == BLACK)
                DeleteFixup(s_replaceNode);

            if(value)
                *value = deleteNode->value;
    
            treePool_->Free(deleteNode);

            size_--;
            return true;
        }

        Value Find(DATA key)
        {
            TreeNode<DATA, Value>* retNode = IterativeSearchFromRoot(key);
            return retNode->value;
        }

        bool ExchangeValue(DATA key, Value value) 
        {
            TreeNode<DATA, Value>* retNode = IterativeSearchFromRoot(key);
            if (retNode != NIL_node_)
            {
                retNode->value = value;
                return true;
            }
            else
                return false;
        }

        void Foreach(function<void(DATA, Value)> func)
        {
            InorderTreeWalk(func, root_);
        }

        static long long GetTreeNodeUsed()
        {
            return treePool_->GetUseCount();
        }

        static long long GetTreeNodeCapacity()
        {
            return treePool_->GetCapacityCount();
        }

        static void DeleteTLS()
        {
            treePool_->RemoveRemainders();
        }

        int Size()
        {
            return size_;
        }

    private:
        TreeNode<DATA, Value>* Search(TreeNode<DATA, Value>* node, DATA key)
        {
            TreeNode<DATA, Value>* nodeX = node;
            if (nodeX == NIL_node_ || key == nodeX->key)
                return nodeX;

            if (key < nodeX->key)
                return Search(nodeX->left, key);
            else
                return Search(nodeX->right, key);
        }

        TreeNode<DATA, Value>* IterativeSearch(TreeNode<DATA, Value>* node, DATA key)
        {
            TreeNode<DATA, Value>* nodeX = node;
            while (nodeX != NIL_node_ && key != nodeX->key)
            {
                if (key < nodeX->key)
                    nodeX = nodeX->left;
                else
                    nodeX = nodeX->right;
            }
            return nodeX;
        }


        TreeNode<DATA, Value>* SearchFromRoot(DATA key)
        {
            TreeNode<DATA, Value>* nodeX = root_;
            if (nodeX == NIL_node_ || key == nodeX->key)
                return nodeX;

            if (key < nodeX->key)
                return Search(nodeX->left, key);
            else
                return Search(nodeX->right, key);
        }

        TreeNode<DATA, Value>* IterativeSearchFromRoot(DATA key)
        {
            TreeNode<DATA, Value>* nodeX = root_;
            while (nodeX != NIL_node_ && key != nodeX->key)
            {
                if (key < nodeX->key)
                    nodeX = nodeX->left;
                else
                    nodeX = nodeX->right;
            }
            return nodeX;
        }


        TreeNode<DATA, Value>* MinValueNode(TreeNode<DATA, Value>* node)
        {
            TreeNode<DATA, Value>* current = node;
            //맨 왼쪽 단말 노드를 찾으러 내려감
            while (current->left != NIL_node_)
                current = current->left;

            return current;
        }

        TreeNode<DATA, Value>* MaxValueNode(TreeNode<DATA, Value>* node)
        {
            TreeNode<DATA, Value>* current = node;
            //맨 왼쪽 단말 노드를 찾으러 내려감
            while (current->right != NIL_node_)
                current = current->right;

            return current;
        }


        TreeNode<DATA, Value>* Successor(TreeNode<DATA, Value>* node)
        {
            TreeNode<DATA, Value>* nodeX = node;
            if (nodeX->right != NIL_node_)
                return MinValueNode(nodeX->right);

            TreeNode<DATA, Value>* nodeParentY = nodeX->parent;
            while (nodeParentY != NIL_node_ && nodeX == nodeParentY->right)
            {
                nodeX = nodeParentY;
                nodeParentY = nodeParentY->parent;
            }
            return nodeParentY;
        }

        TreeNode<DATA, Value>* Predecessor(TreeNode<DATA, Value>* node)
        {
            TreeNode<DATA, Value>* nodeX = node;
            if (nodeX->left != NIL_node_)
                return MaxValueNode(nodeX->left);

            TreeNode<DATA, Value>* nodeParentY = nodeX->parent;
            while (nodeParentY != NIL_node_ && nodeX == nodeParentY->left)
            {
                nodeX = nodeParentY;
                nodeParentY = nodeParentY->parent;
            }
            return nodeParentY;

        }

        void LeftRotate(TreeNode<DATA, Value>* nodeX)
        {
            TreeNode<DATA, Value>* nodeRightY = nodeX->right; //set node_right
            nodeX->right = nodeRightY->left; //turn node_right's left subtree into node's right subtree

            if (nodeRightY->left != NIL_node_)
                nodeRightY->left->parent = nodeX;

            nodeRightY->parent = nodeX->parent;

            if (nodeX->parent == NIL_node_)
                root_ = nodeRightY;
            else if (nodeX == nodeX->parent->left)
                nodeX->parent->left = nodeRightY;
            else
                nodeX->parent->right = nodeRightY;

            nodeRightY->left = nodeX;
            nodeX->parent = nodeRightY;
        }

        void RightRotate(TreeNode<DATA, Value>* nodeY)
        {
            TreeNode<DATA, Value>* nodeLeftX = nodeY->left; //set left_x
            nodeY->left = nodeLeftX->right;

            if (nodeLeftX->right != NIL_node_)
                nodeLeftX->right->parent = nodeY;

            nodeLeftX->parent = nodeY->parent;

            if (nodeY->parent == NIL_node_)
                root_ = nodeLeftX;
            else if (nodeY == nodeY->parent->right)
                nodeY->parent->right = nodeLeftX;
            else
                nodeY->parent->left = nodeLeftX;

            nodeLeftX->right = nodeY;
            nodeY->parent = nodeLeftX;
        }

        TreeNode<DATA, Value>* NewNode(DATA key, Value value, TreeNode<DATA, Value>* parent)
        {
            TreeNode<DATA, Value>* newNode = treePool_->Alloc();
            newNode->color = RED;
            newNode->key = key;
            newNode->value = value;
            newNode->parent = parent;
            newNode->left = NIL_node_;
            newNode->right = NIL_node_;

            return newNode;
        }

        TreeNode<DATA, Value>* NewRoot(DATA key, Value value)
        {
            TreeNode<DATA, Value>* newRoot = treePool_->Alloc();
            newRoot->color = BLACK;
            newRoot->left = NIL_node_;
            newRoot->right = NIL_node_;
            newRoot->parent = NIL_node_;
            newRoot->key = key;
            newRoot->value = value;

            root_ = newRoot;
            return root_;
        }

        void InsertFixup(TreeNode<DATA, Value>* node)
        {
            TreeNode<DATA, Value>* nodeZ = node;
            while (nodeZ->parent->color == RED)
            {
                if (nodeZ->parent == nodeZ->parent->parent->left)
                {
                    TreeNode<DATA, Value>* uncle = nodeZ->parent->parent->right;
                    if (uncle->color == RED)
                    {
                        nodeZ->parent->color = BLACK;
                        uncle->color = BLACK;
                        nodeZ->parent->parent->color = RED;
                        nodeZ = nodeZ->parent->parent;

                    }
                    else
                    {
                        if (nodeZ == nodeZ->parent->right)
                        {
                            nodeZ = nodeZ->parent;
                            LeftRotate(nodeZ);

                        }
                        nodeZ->parent->color = BLACK;
                        nodeZ->parent->parent->color = RED;
                        RightRotate(nodeZ->parent->parent);
                    }
                }
                else
                {
                    TreeNode<DATA, Value>* uncle = nodeZ->parent->parent->left;
                    if (uncle->color == RED)
                    {
                        nodeZ->parent->color = BLACK;
                        uncle->color = BLACK;
                        nodeZ->parent->parent->color = RED;
                        nodeZ = nodeZ->parent->parent;

                    }
                    else
                    {
                        if (nodeZ == nodeZ->parent->left)
                        {
                            nodeZ = nodeZ->parent;
                            RightRotate(nodeZ);

                        }
                        nodeZ->parent->color = BLACK;
                        nodeZ->parent->parent->color = RED;
                        LeftRotate(nodeZ->parent->parent);

                    }
                }
            }

            root_->color = BLACK;
        }

        void RB_Transplant(TreeNode<DATA, Value>* originalNode, TreeNode<DATA, Value>* replaceNode)
        {
            if (originalNode->parent == NIL_node_)
                root_ = replaceNode;
            else if (originalNode == originalNode->parent->left)
                originalNode->parent->left = replaceNode;
            else
                originalNode->parent->right = replaceNode;

            //assign to replaceNode.parent when replaceNode NIL_node
            replaceNode->parent = originalNode->parent;
        }

        void DeleteFixup(TreeNode<DATA, Value>* replaceNode)
        {
            while (replaceNode != root_ && replaceNode->color == BLACK)
            {
                TreeNode<DATA, Value>* siblingNode;
                if (replaceNode == replaceNode->parent->left)
                {
                    siblingNode = replaceNode->parent->right;
                    if (siblingNode->color == RED)
                    {
                        //case 1
                        siblingNode->color = BLACK;
                        replaceNode->parent->color = RED;
                        LeftRotate(replaceNode->parent);
                        siblingNode = replaceNode->parent->right;
                    }

                    if (siblingNode->left->color == BLACK && siblingNode->right->color == BLACK)
                    {
                        //case 2
                        siblingNode->color = RED;
                        replaceNode = replaceNode->parent;
                    }
                    else
                    {
                        if (siblingNode->right->color == BLACK)
                        {
                            //case 3
                            siblingNode->left->color = BLACK;
                            siblingNode->color = RED;
                            RightRotate(siblingNode);
                            siblingNode = replaceNode->parent->right;

                        }

                        //case 4
                        siblingNode->color = replaceNode->parent->color;
                        replaceNode->parent->color = BLACK;
                        siblingNode->right->color = BLACK;
                        LeftRotate(replaceNode->parent);
                        replaceNode = root_;

                    }

                }
                else
                {
                    siblingNode = replaceNode->parent->left;
                    if (siblingNode->color == RED)
                    {
                        //case 1
                        siblingNode->color = BLACK;
                        replaceNode->parent->color = RED;
                        RightRotate(replaceNode->parent);
                        siblingNode = replaceNode->parent->left;
                    }

                    if (siblingNode->right->color == BLACK && siblingNode->left->color == BLACK)
                    {
                        //case 2
                        siblingNode->color = RED;
                        replaceNode = replaceNode->parent;
                    }
                    else
                    {
                        if (siblingNode->left->color == BLACK)
                        {
                            //case 3
                            siblingNode->right->color = BLACK;
                            siblingNode->color = RED;
                            LeftRotate(siblingNode);
                            siblingNode = replaceNode->parent->left;
                        }

                        //case 4
                        siblingNode->color = replaceNode->parent->color;
                        replaceNode->parent->color = BLACK;
                        siblingNode->left->color = BLACK;
                        RightRotate(replaceNode->parent);
                        replaceNode = root_;
                    }

                }

            }

            replaceNode->color = BLACK;
        }

        void InorderTreeWalk(function<void(DATA, Value)> func, TreeNode<DATA, Value>* root)
        {
            if (root != NIL_node_)
            {
                InorderTreeWalk(func, root->left);
                func(root->key, root->value);
                InorderTreeWalk(func, root->right);
            }
        }

    private:
        TreeNode<DATA, Value>* root_;
        TreeNode<DATA, Value>* NIL_node_;
        ULONG size_;

        static MemTLS<TreeNode<DATA, Value>>* treePool_; //어떻게 삭제할까?
    };

    template<typename DATA, typename Value>
    MemTLS<TreeNode<DATA, Value>>* MyRedBlackTree<DATA, Value>::treePool_ = new MemTLS<TreeNode<DATA, Value>>(100, 1, eRED_BLACK_TREE_NODE_CODE);

}

#endif