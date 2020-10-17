# Disk based b+tree
Disk-based b+tree를 구현하기 위하여 b+tree의 기본적인 연산과 구현에 대한 대략적인 디자인에 대해서 설명하겠습니다.    


# Features
1. Call path of the insert/delete operation on b+tree
2. Detail flow of the structure modification
3. Designs or required changes for building on-disk b+tree


### 1. Call path of the insert/delete operation on b+tree
+ insert/delete 함수에 사용되는 함수 및 변수
+ insert 함수의 Call path
+ delete 함수의 Call path


> ### insert/delete 함수에 사용되는 함수 및 변수
* #### order
b+tree를 구성하는 노드에 들어갈 수 있는 pointers의 크기를 나타내는 변수입니다.        
예를 들어 order가 4라면 node 안에는 pointer가 최대 4개, key가 최대 3개 들어갈 수 있습니다.    

* #### record * find( node * root, int key, bool verbose )   
인자로 받은 key가 b+tree에 존재하는 key인지 확인하는 함수입니다.   
find_leaf함수를 호출하여 key가 위치해야할 leaf 노드로 접근한 후   
해당 노드가 존재하지 않거나 존재하더라도 key가 없다면 NULL을 반환하고,
key를 찾았다면 그에 대응하는 record를 반환합니다.   

* #### node * find_leaf( node * root, int key, bool verbose )   
인자로 받은 key가 위치해야할 leaf 노드를 반환합니다.   
만약 b+tree에 노드가 없다면 NULL을 반환합니다.

* #### record * make_record(int value)
인자로 받은 value를 갖고 있는 record를 생성하고 그 주소값을 반환합니다.

* #### node * start_new_tree(int key, record * pointer)
인자로 받은 key와 record를 갖고 있는 루트노드를 생성하고 그 주소값을 반환합니다.   

* #### node * make_leaf( void )
leaf 노드를 생성하고 그 주소값을 반환합니다.

* #### node * make_node( void )
비어있는 노드를 동적할당하여 생성한 후 그 주소값을 반환합니다.

* #### int cut( int length )
인자로 들어온 length가 짝수라면 length/2를 반환하고, 홀수라면 length/2의 upper bound를 반환합니다.    
예를 들어 length가 4라면 2를 반환하고, 5라면 2.5의 반올림인 3을 반환합니다.


> ### Call path of Insert   

다음은 insert함수의 코드입니다.
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
</pre>
1. find함수를 호출하여 인자로 들어온 key와 중복되는 값이 있는 지 확인하고 있다면 insert함수호출을 종료합니다.   
2. 중복되는 값이 없을 시에 value를 담고있는 record를 make_reocrd함수를 호출하여 할당받습니다.   
3. b+tree가 비어있다면 start_new_tree함수를 호출하여 새로운 트리를 만들어냅니다.   
4. key가 위치해야할 노드를 find_leaf함수를 호출하여 찾아내고 그 노드에 새로운 값을 입력합니다.   
    4-1. 자리가 있다면 insert_into_leaf함수를 호출하여 값을 입력 후 루트노드를 반환합니다.   
    4-2. 자리가 없다면 반환값으로 insert_into_leaf_after_splitting함수를 호출하여 노드를 분리하여 입력합니다.   

아래는 insert함수의 Call path에서 사용된 함수들에 대한 설명입니다.   
* #### node * insert_into_leaf( node * leaf, int key, record * pointer )
인자로 받은 leaf노드에 key와 pointer가 들어올 위치를 찾고,    
그 위치 이후의 키값과 포인터들을 오른쪽으로 한칸 씩 이동시킨 후 인자로 받은 key, pointer를 해당 위치에 저장합니다.    
해당 leaf노드의 주소값을 반환합니다.

* #### node * insert_into_leaf_after_splitting(node * root, node * leaf, int key, record * pointer)
인자로 받은 leaf노드의 key와 pointer가 order-1개만큼 있어서 더이상 그 노드에 새로운 key, record를 저장하지 못할 때,       
해당 노드를 두 개의 노드로 분리하여 값들을 저장하는 함수입니다.    
   
예를 들어 order = 4와 같이 짝수일 떄    
leaf노드가
<pre>
     1     2     4
  a     b     d       
</pre>
이와 같고, 여기에 key = 3, pointer = c인 값을 넣으면
<pre>
     1     2  |     3     4
  a     b     |  c     d 
</pre>
이렇게 두 개의 노드로 분리합니다.
혹은 order = 5처럼 홀수라면
<pre>
     1     2     4     5 
  a     b     d     e       
</pre>
이와 같고, 여기에 key = 3, pointer = c인 값을 넣으면
<pre>
     1     2  |     3      4     5
  a     b     |  c     d     e 
</pre>
이렇게 왼쪽이 하나 적게 분리가 됩니다.   
   
또한 b+tree의 leaf노드가 가진 특징인 '두 개의 leaf노드를 구별하는 부모노드의 key는 우측 노드의 첫번째 key다'를 만족시키기 위해       
우측 노드의 첫번쨰 값인 3을 부모노드의 key에 입력해야합니다. 이 때 insert_into_parent함수를 호출합니다.    
leaf노드의 또다른 특성인 왼쪽 노드의 마지막 포인터는 오른쪽 노드를 가리킨다를 만족하기 위하여 각 노드의 마지막 포인터를 재설정합니다.
이 떄 맨 우측노드의 마지막 포인터는 NULL을 가리킵니다.  

* #### node * insert_into_parent(node * root, node * left, int key, node * right)
left노드와 right노드를 구별하는 key값을 부모노드에 삽입하는 함수입니다.    
만약, 부모노드가 없다면 insert_into_new_root함수를 호출하여 새로운 루트노드를 만든 후 값을 삽입합니다.
부모노드가 이미 존재한다면 get_left_index함수를 호출하여 부모노드에서 left노드를 가리키는 포인터의 위치를 찾습니다.    
부모노드에 새로운 key를 넣을 자리가 있다면 부모노드를 대상으로 insert_into_node함수를 호출합니다.   
이 함수의 인자로 left노드의 위치가 사용됩니다.   
그러나 자리가 없다면 부모노드를 대상으로 insert_into_node_after_splitting함수를 호출합니다.

* #### int get_left_index(node * parent, node * left)
parent노드의 pointer 중 left를 가리키는 포인터의 위치값을 반환합니다.

* #### node * insert_into_node(node * root, node * n, int left_index, int key, node * right)
인자로 들어온 n노드의 배열에서 left_index위치보다 우측에 있는 key와 pointer들을 한칸씩 우측으로 옮깁니다.   
   
예를 들어 key = 3, right = d, left_index = 2이고
<pre>
     1     2     4
  a     b     c     e   
</pre>
와 같이 n노드가 형성이 되어있을 때, left_index인 c의 우측에 d를 넣고 이를 구별할 key로 3을 넣기 위하여   
<pre>
     1     2     ㅁ     4   
  a     b     c     ㅁ     e    
</pre>
와 같이 2번 인덱스 이후의 값들을 한칸씩 우측으로 옮기고 비어있는 위치에 3, d를 입력하여 결과적으로   
<pre>
    1     2     3     4   
  a     b    c     d     e   
</pre>
이와 같이 n노드를 재구성한 뒤 n노드의 주소값을 반환합니다.   

* #### node * insert_into_node_after_splitting(node * root, node * old_node, int left_index, int key, node * right)
인자로 받은 old_node의 key가 order-1개만큼, pointer가 order개 만큼 있어서 더이상 그 노드에 새로운 key, record를 저장하지 못할 때,       
해당 노드를 두 개의 노드로 분리하여 값들을 저장하는 함수입니다.    
   
예를 들어 order = 4와 같이 짝수일 떄    
olde_node가
<pre>
     1     2     4
  a     b     d     e  
</pre>
이와 같고, 여기에 key = 3, pointer = c인 값을 넣으면
<pre>
     1     |     3     4
  a     b  |  c     d     e 
</pre>
이렇게 두 개의 노드로 분리되고, 왼쪽이 적게 분리됩니다.
혹은 order = 5처럼 홀수라면
<pre>
     1     2     4     5 
  a     b     d     e     f       
</pre>
이와 같고, 여기에 key = 3, pointer = c인 값을 넣으면
<pre>
     1     2     |     4     5
  a     b     c  |  d     e     f
</pre>   
이와 같이 분리됩니다.
마지막 반환에서 old_node의 부모노드로 올라간 key에 대해서도 insert_into_parent함수를 호출합니다.
> 주의할 점은 leaf노드와 일반적인 internal노드의 splitting방식이 다르다는 것입니다.   
> leaf노드의 경우 새로 들어온 값을 포함해서 양쪽 모두 값이 유지되고 노드를 구별할 key값이 복사되어 부모노드로 올라갔다면,   
> internal노드의 경우 노드를 구별할 key값이 복사되지 않고 부모노드로 이동되었습니다.   
  

> ### Call path of Delete

다음은 delete함수의 코드입니다. 
<pre>
<code>
node * delete(node * root, int key) {

    node * key_leaf;
    record * key_record;

    key_record = find(root, key, false);
    key_leaf = find_leaf(root, key, false);
    if (key_record != NULL && key_leaf != NULL) {
        root = delete_entry(root, key_leaf, key, key_record);
        free(key_record);
    }
    return root;
}
</code>
</pre>
1. 인자로 받은 key에 해당하는 record를 찾고, 그 key를 갖고 있는 leaf노드를 찾습니다.   
2. 만약 해당하는 record와 leaf노드가 존재하지 않는다면 함수를 종료합니다.   
3. 그러나 존재한다면, 그 leaf노드와 key, record를 인자로 받는 delete_entry함수를 호출하고 free함수로 해당 record를 소멸시킵니다.   

아래는 delete함수의 Call path에서 사용된 함수들에 대한 설명입니다.   
* #### node * delete_entry( node * root, node * n, int key, void * pointer )
지우고자 하는 key, pointer를 가지고 있는 노드 n에 대하여 remove_entry_from_node함수를 호출하여 해당 entry를 제거합니다.  
    
만약, n이 루트였다면 adjust_root함수를 호출하여 루트노드에 대한 연산을 진행합니다.   
n이 루트가 아니라면 b+tree가 가진 특성인 key, pointer의 최소 개수라는 개념을 만족시키기 위하여   
해당 order에서의 min_keys 값을 구합니다.   
> 루트와 그렇지 않은 경우를 나누는 이유는 루트노드와 그렇지 않은 노드가 다른 제한 조건을 가지기 때문입니다.
> 루트노드를 제외한 각 노드들은 order/2의 upper bound만큼의 포인터를 가져야 하기 때문에   
> internal 노드들은 cut(order) - 1개의 key를 가져야 하고, leaf 노드들은 cut(order - 1)개의 key를 가져야 한다는 특징이 있습니다.  
> 반면에 루트노드는 order에 관계없이 최소 1개의 key만 가지면 된다는 특징이 있습니다. 

key, pointer를 제거한 후에도 n의 key의 개수가 min_key 이상이면 함수호출을 종료하지만, 그 값보다 작다면   
coalescence_nodes함수 혹은 redistribute_nodes함수를 호출합니다.   
   
만약 값이 지워진 노드가 leaf노드라면 해당 노드의 key개수와 이웃노드의 key개수의 합이 order보다 작을 때     
둘을 병합하는 coalescence함수를 호출하고, 그 외의 경우에 이웃노드 key, pointer의 일부를 옮기는 redistribute_nodes함수를 호출합니다.   
만약 leaf노드가 아니라면 해당 노드의 key개수와 이웃노드의 key개수의 합이 order - 1보다 작을 때   
coalescence함수를 호출하고, 그 외의 경우에 이웃노드 key, pointer의 일부를 옮기는 redistribute_nodes함수를 호출합니다.   
> leaf노드와 internal노드의 병합기준이 다른 이유는 leaf노드는 부모노드에서 값을 가져오지 않고 이웃노드와 병합하지만   
> internal노드는 이웃노드와 해당 노드를 구별하는 부모노드의 key도 가져오기 때문에 차이가 발생합니다.

* #### node * remove_entry_from_node(node * n, int key, node * pointer)
노드 n에 들어있는 key, pointer를 제거한 후 그 위치 뒤에 있는 값들을 앞으로 옮겨서 배열을 재구성합니다.   
n을 다시 반환합니다.   

* #### node * adjust_root(node * root)
루트노드의 key 개수가 1개 이상이라면 문제가 없으므로 함수호출을 종료하고 루트노드의 주소값을 반환합니다.   
그러나 0개가 되었을 떄   
만약 루트노드가 leaf노드인 경우, 즉 b+tree에 루트노드만 존재하는 경우 문제가 없기 때문에 함수호출을 종료하고 NULL을 반환합니다.
leaf노드가 아니라면 루트노드의 첫 번쨰 포인터가 가리키는 노드를 루트노드로 설정하고 그 주소값을 반환합니다.   
> 루트노드가 leaf노드가 아니면서 key의 개수가 0이 되는 경우는 b+tree에서는 coalescence함수가 호출되었을 떄입니다.   
> 그렇기에 새롭게 루트노드가 되는 노드는 sibling이 없는 상태로 생각할 수 있습니다.   

* #### int get_neighbor_index( node * n )
coalescence함수와 redistribute함수에서 사용되는 이웃노드의 위치를 알기 위한 함수입니다.   
만약 타겟이 되는 노드가 맨 왼쪽에 있다면 -1을 반환하고,   
그 외의 경우에는 타겟이 되는 노드의 왼쪽 인덱스를 반환합니다.   

* #### coalescence함수와 redistribute함수
2번 항목에 서술 되어있습니다.


### 2. Detail flow of the structure modification
+ Merge(coalescence_nodes & redistribute_nodes)
+ Split(insert_into_leaf_after_splitting & insert_into_node_after_splitting)


> ### Merge(coalescence_nodes & redistribute_nodes)

노드가 가져야하는 최소 key의 개수보다 낮은 개수의 key를 가진 노드가 생겼을 때, 이웃한 노드와 합치는 함수입니다.
다음은 coalescence_nodes의 코드입니다.
<pre>
<code>
node * coalesce_nodes(node * root, node * n, node * neighbor, int neighbor_index, int k_prime) {

    int i, j, neighbor_insertion_index, n_end;
    node * tmp;

    if (neighbor_index == -1) {
        tmp = n;
        n = neighbor;
        neighbor = tmp;
    }

    neighbor_insertion_index = neighbor->num_keys;

    if (!n->is_leaf) {

        neighbor->keys[neighbor_insertion_index] = k_prime;
        neighbor->num_keys++;

        n_end = n->num_keys;

        for (i = neighbor_insertion_index + 1, j = 0; j < n_end; i++, j++) {
            neighbor->keys[i] = n->keys[j];
            neighbor->pointers[i] = n->pointers[j];
            neighbor->num_keys++;
            n->num_keys--;
        }

        neighbor->pointers[i] = n->pointers[j];

        for (i = 0; i < neighbor->num_keys + 1; i++) {
            tmp = (node *)neighbor->pointers[i];
            tmp->parent = neighbor;
        }
    }
    else {
        for (i = neighbor_insertion_index, j = 0; j < n->num_keys; i++, j++) {
            neighbor->keys[i] = n->keys[j];
            neighbor->pointers[i] = n->pointers[j];
            neighbor->num_keys++;
        }
        neighbor->pointers[order - 1] = n->pointers[order - 1];
    }

    root = delete_entry(root, n->parent, k_prime, n);
    free(n->keys);
    free(n->pointers);
    free(n); 
    return root;
}
</code>
</pre>
타겟노드가 부모노드의 제일 왼쪽 포인터가 가리키는 노드라면 neighbor_index는 -1이고, 그렇지 않다면 타겟노드의 왼쪽 인덱스값을 가집니다.      
명칭의 단순화를 위해 이웃노드와 타겟노드의 명칭을 바꿔줍니다. 즉, 오른쪽이 타겟노드이고 왼쪽이 이웃노드가 되도록 설정합니다.   
인자로 오는 또다른 값인 k_prime은 타겟노드와 이웃노드를 구별하는 부모노드의 key를 가리킵니다.   
   
이 함수는 오른쪽 노드의 key, pointer들을 왼쪽노드로 옮기고 오른쪽 노드를 제거하는 연산을 실행합니다.   
이 때, leaf노드와 internal노드에 차이가 발생합니다.
   
예를 들어 leaf노드의 경우   
<pre>
   1    |      4     5     6   
a       |   d     e     f 
</pre>
이와 같이 A노드가 제한조건을 만족시키지 못하는 상황에서 이웃노드 B노드가 있다는 가정하에, 
<pre>
   1     4     5     6   |      4     5     6
a     d     e     f      |   d     e     f
</pre>
이처럼 B노드의 key, pointer들이 A노드로 옮겨지고 B노드는 여전히 남아있습니다.   
그리고 k_prime자리의 왼쪽포인터가 병합된 노드를 가리키고, k_prime의 오른쪽 포인터는 B노드를 가리키고 있습니다.    
A노드와 B노드를 구별하던 k_prime인 4를 부모노드에서 지워주고 B노드를 가리키는 포인터를 없애기 위해   
k_prime과 n을 인자로 delete_entry함수를 호출합니다.     
그리하면 k_prime의 key와 오른쪽 포인터는 소멸하여 그 뒤에 있던 값들이 그 자리를 채우게 됩니다.   
이후 B노드르 free를 이용하여 소멸시킵니다.  

그러나 internal노드의 경우 k_prime도 함께 내려온다는 점을 반영해야합니다.
<pre>
               3                     7
       x                  y                      z
     /                 /
   1     |      4     5     6   
a     b  |   d     e     f     g
</pre>
이와 같이 3,7 key와 x,y,z pointer를 가지는 부모노드와 internal A,B가 있고, A가 제한조건을 만족시키지 못했을 떄,    
coalescence_nodes함수를 실행한다면   
<pre>
                                           3                                        7
                 x                                               y                                    z
                /                                               /
   1     3      4     5     6              |         4     5     6                  | 
a     b     d     e     f     e            |     d     e     f       g               |
</pre>
이와 같이 k_prime인 3을 A노드로 내리고, B노드의 key, pointer들을 A노드로 이동시킵니다.   
또한 x포인터는 병합된 노드를 가리키고, y포인터는 기존의 B노드를 가리키고 있습니다.   
leaf노드와 마찬가지로 k_prime을 부모노드에서 지워주도록   
k_prime값과 B노드를 가리키는 포인터를 인자로 delete_entry함수를 실행하면   
<pre>
                                 7                                          
                x                                   z                                            
               /                                   /
   1     3      4     5     6   
a     b     d     e     f     e
</pre>
이와 같은 모습이 되는 것을 확인할 수 있습니다.   
마지막으로 루트의 주소를 반환합니다.

다음은 redistribute_nodes의 코드입니다.
<pre>
<code>
node * redistribute_nodes(node * root, node * n, node * neighbor, int neighbor_index, int k_prime_index, int k_prime) {  

    int i;
    node * tmp;

    if (neighbor_index != -1) {
        if (!n->is_leaf)
            n->pointers[n->num_keys + 1] = n->pointers[n->num_keys];
        
        for (i = n->num_keys; i > 0; i--) {
            n->keys[i] = n->keys[i - 1];
            n->pointers[i] = n->pointers[i - 1];
        }

        if (!n->is_leaf) {
            n->pointers[0] = neighbor->pointers[neighbor->num_keys];
            tmp = (node *)n->pointers[0];
            tmp->parent = n;
            neighbor->pointers[neighbor->num_keys] = NULL;
            n->keys[0] = k_prime;
            n->parent->keys[k_prime_index] = neighbor->keys[neighbor->num_keys - 1];
        }
        else {
            n->pointers[0] = neighbor->pointers[neighbor->num_keys - 1];
            neighbor->pointers[neighbor->num_keys - 1] = NULL;
            n->keys[0] = neighbor->keys[neighbor->num_keys - 1];
            n->parent->keys[k_prime_index] = n->keys[0];
        }
    }

    else {  
        if (n->is_leaf) {
            n->keys[n->num_keys] = neighbor->keys[0];
            n->pointers[n->num_keys] = neighbor->pointers[0];
            n->parent->keys[k_prime_index] = neighbor->keys[1];
        }
        else {
            n->keys[n->num_keys] = k_prime;
            n->pointers[n->num_keys + 1] = neighbor->pointers[0];
            tmp = (node *)n->pointers[n->num_keys + 1];
            tmp->parent = n;
            n->parent->keys[k_prime_index] = neighbor->keys[0];
        }
        for (i = 0; i < neighbor->num_keys - 1; i++) {
            neighbor->keys[i] = neighbor->keys[i + 1];
            neighbor->pointers[i] = neighbor->pointers[i + 1];
        }
        if (!n->is_leaf)
            neighbor->pointers[i] = neighbor->pointers[i + 1];
    }

    n->num_keys++;
    neighbor->num_keys--;

    return root;
}
</code>
</pre>
먼저 leaf노드의 경우를 예제를 통하여 살펴보겠습니다.   
기준에 미달이 되는 노드가 맨 왼쪽이 아닌 경우입니다.    
<pre>
                       6
         x                          y
         /                         /
   1     3     4               6      7
  a     b     c               d       e
</pre>
leaf노드인 A,B노드가 있다는 가정하에, 6이라는 key값에 대한 delete연산을 진행하게 되면
6과 d가 사라지고 그 자리를 오른쪽에 있던 key와 pointer들이 채우게 되면서
<pre>
                       6
         x                          y
         /                         /
   1     3     4                  7
  a     b     c                  e
</pre>
이와 같이 변하게 됩니다.   
이 때, B노드가 기준에 미달하게 되었고, redistribute를 실행하게 됩니다.   
먼저 B노드의 key와 pointer들을 한칸씩 우측으로 이동시키고, 맨 앞의 key와 pointer에 A노드의 끝에 있는 4와 c를 가져오게 됩니다.   
<pre>
                       6
         x                          y
         /                         /
   1        3                  4       7
  a        b                  c        e
</pre>
그 다음 leaf노드의 성질을 만족시키기 위해 B노드의 제일 왼쪽에 있는 key값인 4를 A노드와 B노드의 k_prime으로 설정하여    
결과적으로는
<pre>
                       4
         x                          y
         /                         /
   1        3                  4       7
  a        b                  c        e
</pre>
이와 같은 모습을 하게 됩니다.   
만약 타겟노드가 제일 왼쪽의 노드라면 기준이 미달되는 노드는 A가 되고 이미 A노드의 끝에 자리가 있기 떄문에 B노드의 맨 앞에 있는 key와 pointer를 A로 옮긴 후, B노드의 각 key, pointer들을 한칸씩 앞으로 이동시키고 k_prime을 재설정합니다.

두 번쨰 상황인 internal노드에 관한 예시입니다.   
<pre>
                       5
         x                          y
         /                         /
    1     3     4               6     7
  a    b     c    d          e     f     g
</pre>
이와 같은 상황에서 B노드의 key 6이 사라지는 상황이 나오려면, coalesce_nodes함수가 호출이되어야합니다.    
왜냐하면 redistribute라면 6이 사라져도 다른 값으로 대체되어 개수가 부족한 현상이 일어나지 않기 때문입니다.    
그리고 coalesce_nodes함수가 호출되어 6이 사라졌다면, 그에 맞춰 사라지는 포인터는 f입니다.    
그래서 이러한 상황에 6이 delete되었다면    
<pre>
                       5
         x                          y
         /                         /
    1     3     4               7    
  a    b     c    d          e     g    
</pre>
이러한 형태로 바뀌게 되고, B노드가 기준에 못미치게 되었습니다.
그렇다면 leaf노드처럼 B노드의 key, pointer들을 오른쪽으로 한칸씩 이동시키고    
k_prime인 5를 B의 맨 앞으로, A의 끝인 4를 k_prime으로, A의 끝에 있는 pointer인 d를 B의 맨 앞으로 이동시킵니다.
<pre>
                       4
         x                          y
         /                         /
    1     3                    5     7    
  a    b     c              d      e     g    
</pre>
만약 타겟노드가 제일 왼쪽에 있다면 반대방향으로 이동시키게 됩니다.

internal노드와 leaf노드의 경우 모두 루트를 반환합니다.


> ### Split(insert_into_leaf_after_splitting & insert_into_node_after_splitting)
1번 항목에 서술 되어있습니다.   

### 3. Designs or required changes for building on-disk b+tree
+ Introduce
+ Pages
+ Required changes

> ### Introduce
record의 크기는 고정되어있습니다. 페이지 내부의 레코드들은 key값을 기준으로 오름차순으로 정렬됩니다.   
record의 크기가 고정되어있어도 key값의 순서가 존재하기 때문에 중간에 있는 hole에 바로 insert를 할 수 없습니다.    
그리하여 페이지 내에 hole이 있다면 packing을 실행하겠습니다.    
각 페이지의 index는 해당 페이지 내에서 가장 작은 key값으로 설정하겠습니다.

> ### Page의 구성
1. 페이지의 크기는 4096바이트입니다.   
2. Header 페이지, Free 페이지, Leaf 페이지, Internal 페이지로 구성되어 있습니다.
3. 각 레코드들이 가진 key는 8바이트, value는 120바이트로 크기가 고정되어있습니다.
4. 페이지의 넘버는 해당 페이지가 어디에 위치하였는가를 기준으로 하겠습니다.

+ #### Header 페이지
  + 첫 번째 Free 페이지를 가리키는 포인터를 가지고 있습니다.   
  + 루트 페이지를 가리키는 포인터를 가지고 있습니다.
  + 전체 페이지의 개수를 나타내는 값을 담고 있습니다   
  + 페이지 번호는 0번입니다.
+ #### Free 페이지
  + 다음 Free 페이지를 가리키는 포인터를 가지고 있습니다.
  + Linked List형태로 관리할 것이고 stack개념을 사용하겠습니다.
+ #### Leaf 페이지
  + 헤더를 가지고 있습니다.
    + 해당 leaf페이지를 가리키는 부모페이지의 페이지 넘버를 가리키는 값을 담고 있습니다.
    + Leaf 여부를 구별하는 값을 담고 있습니다.
    + Key의 개수에 대한 값을 담고 있습니다.
    + 우측 leaf페이지를 가리키는 포인터를 담고 있습니다.
  + key와 value를 가지고 있습니다.
    + 합쳐서 128바이트의 크기를 가지고 하나의 leaf페이지당 최대 31개까지 존재할 수 있습니다.
+ #### Internal 페이지
  + 헤더를 가지고 있습니다.
    + leaf페이지의 헤더와 거의 동일한 구성요소를 가집니다.
    + leaf페이지의 해더와 다른 점은 leaf페이지는 우측 페이지, 즉 sibling을 가리키는 포인터를 가졌다면, internal 페이지에서는 자식 페이지 
      를 가리키게 됩니다.
  + key와 그에 대응하는 페이지 넘버를 가지고 있습니다.
    + 합쳐서 8바이트의 크기를 가지고 하나의 internal페이지당 최대 248개 존재합니다.
> internal 페이지가 가리키는 자식 페이지는 최대 249개(헤더의 자식페이지 포인터 + key의 자식페이지 포인터) 존재합니다.

> ### Required changes
+ 디스크의 데이터 입출력이 느리다는 특성을 보완하기 위해 delayed merge를 사용합니다.
  기존의 b+tree는 요구되는 최소한의 key개수 이하로 떨어지면 merge를 실행했지만 여기서는 key의 개수가 0이 되었을 때만 merge를 실행합니다.
+ merge를 실행 후 남게된 빈 페이지는 없애는 것이 아니라 내용을 초기화한 후 free 페이지의 첫 번째 페이지로 이동하겠습니다.
+ insert에서 페이지 공간이 부족하여 새로운 페이지가 요구된다면 첫 번쨰 free 페이지를 가져옵니다.


