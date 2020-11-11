Introduce
==========
여러 개의 스레드가 같은 레코드에 접근하고 수정하는 상황에서 스레드 간에 우선순위를 정하지 않는다면 올바른 정보가 유지되지 못할 수 있습니다.   
이를 해결하기 위해 Lock Table이라는 개념을 사용합니다.   
   
Lock이란 스레드가 레코드를 사용하기 위한 권한으로 볼 수 있습니다.   
어떤 스레드가 특정 레코드에 대하여 Lock을 보유했다면 해당 레코드에 접근 및 수정이 가능하고, 다른 스레드는 대기하게 됩니다.    
Lock을 보유하고 있는 스레드가 Lock을 release하게 된다면 순서에 맞춰 다른 스레드가 해당 Lock을 얻어 레코드에 접근하게 됩니다.   

Lock Table은 이러한 Lock을 관리하는 객체입니다.   
어떠한 테이블의 레코드에 대해서 스레드들이 어떠한 순서로 접근을 시도하였는지, 현재 어떤 스레드가 해당 레코드를 사용하는지를 관리합니다.   

Feature
========
1. Lock Table API

Lock Table API
==============
+ Lock Table의 구조
+ API

> ### Lock Table의 구조

그림넣어주고

* #### lock_t
<pre>
<code>
struct lock_t {
	lock_t* next = NULL;
	lock_t* prev = NULL;
	pthread_cond_t cond;
	table_entry* sentinel_ptr = NULL;
};
</code>
</pre>
Thread가 레코드에 접근하기 위해서 사용하는 Lock구조체입니다.   
Double Linked List형태를 취하고 있으며 Conditional variable을 가집니다.   
또한 lock이 어느 테이블의 레코드에 있는지를 알 수 있도록 table_entry라는 구조체에 대한 포인터를 가졌습니다.

* #### table_entry
<pre>
<code>
struct table_entry{
	lock_t* head = NULL;
	lock_t* tail = NULL;
	int table_id = -1;
	int64_t key = -1;

	table_entry* next = NULL;
};
</code>
</pre>
Lock Table의 구성요소입니다.   
lock_t 리스트의 head와 tail을 가리키는 포인터를 가졌습니다.   
해당 table_entry가 어떤 테이블 id인지, 어떠한 key인지를 알 수 있는 table_id, key 변수를 가졌습니다.    

Lock Table은 Chaining구조의 Hash Table입니다.   
그리하여 table_entry는 다음 table_entry를 가리키는 next포인터를 가집니다.   

* #### lock_table
<pre>
<code>
class lock_table{
private:
	int size_table;
	table_entry* hash;
public:
	lock_table();
	~lock_table();
	int find_pos(int table_id, int64_t key);
	table_entry* find_table_entry(int table_id, int64_t key);
};
</code>
</pre>
