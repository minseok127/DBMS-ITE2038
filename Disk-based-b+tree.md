Disk-based b+tree
=================
For implement Disk-based b+tree, we must know about operation of b+tree. 

Features
========
1. Call path of the insert/delete operation on b+tree
2. Detail flow of the structure modification
3. Designs or required changes for building on-disk b+tree

Call path of the insert/delete operation on b+tree
==================================================
Before explain about specific call path of insert/delete operation, I will explain about 
Insert
------
<pre>
<code>
node * insert( node * root, int key, int value ) {
    record * pointer;
    node * leaf;

    if (find(root, key, false) != NULL)
        return root;

    pointer = make_record(value);

    if (root == NULL) 
        return start_new_tree(key, pointer);

    leaf = find_leaf(root, key, false);

    if (leaf->num_keys < order - 1) {
        leaf = insert_into_leaf(leaf, key, pointer);
        return root;
    }

    return insert_into_leaf_after_splitting(root, leaf, key, pointer);
}
</code>
<pre>