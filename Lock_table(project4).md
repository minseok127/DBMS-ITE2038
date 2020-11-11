Introduce
==========
여러 개의 스레드가 같은 레코드에 접근하고 수정하는 상황에서 스레드 간에 우선순위를 정하지 않는다면 올바른 정보가 유지되지 못할 수 있습니다.   
이를 해결하기 위해 Lock Table이라는 개념을 사용합니다.   
   
Lock이란 스레드가 레코드를 사용하기 위한 권한으로 볼 수 있습니다.   
어떤 스레드가 특정 레코드에 대하여 Lock을 보유했다면 해당 레코드에 접근 및 수정이 가능하고, 다른 스레드는 대기하게 됩니다.    
Lock을 보유하고 있는 스레드가 Lock을 release하게 된다면 순서에 맞춰 다른 스레드가 해당 Lock을 얻어 레코드에 접근하게 됩니다.   

Lock Table은 이러한 Lock을 관리하는 객체입니다.   
어떠한 테이블의 레코드에 대해서 스레드들이 어떠한 순서로 접근을 시도하였는지, 현재 어떤 스레드가 해당 레코드를 사용하는지를 관리합니다.  

lock_table.cpp에 구현되었습니다.     

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

Lock Table은 Chaining형식의 Hash Table입니다.   
그리하여 테이블의 구성요소인 table_entry는 다음 table_entry를 가리키는 next포인터를 가집니다.   

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
Lock Table 클래스입니다.   
테이블의 크기와 해쉬 테이블을 가리키는 포인터를 변수로 가졌습니다.   

* #### lock_table의 함수
  - find_pos : 인자로 받은 테이블과 key가 위치할 해쉬 테이블의 인덱스를 얻습니다.   
  - find_table_entry : find_pos로 찾은 인덱스에서부터 인자로 받은 table id, key에 해당하는 table_entry의 주소값을 얻습니다.   

> ### API

* #### 전역 변수 및 헤더파일
<pre>
<code>
#include <lock_table.h>
#include <pthread.h>

#define LOCK_TABLE_SIZE (15)

pthread_mutex_t lock_table_latch = PTHREAD_MUTEX_INITIALIZER;

lock_table* lock_table_ptr;
</code>
</pre>
API들의 선언부분이 위치하는 lock_table.h와 Thread 제어에 필요한 라이브러리인 pthread.h를 include하였습니다.
   
LOCK_TABLE_SIZE는 lock_table 객체의 생성자에서 사용됩니다. 테이블의 크기를 결정합니다.   
   
lock table은 여러 스레드의 lock 획득 및 해제에 대한 동시 요청을 순차적으로 처리하기 위해서 글로벌 mutex를 사용합니다.
이 글로벌 mutex를 lock_table_latch라고 정의하였습니다. lock_table_latch를 가진 스레드만 lock table에 접근할 수 있습니다.    
   
API 내에서는 lock_table을 사용하기 때문에 lock_table을 가리키는 포인터인 lock_table_ptr을 정의하였습니다.   
   
* #### int init_lock_table()
<pre>
<code>
int init_lock_table()
{
	lock_table_ptr = new lock_table;
	if(lock_table_ptr == NULL){
		return -1;
	}

	return 0;
}
</code>
</pre>
lock table을 초기화하는 함수입니다.   
lock_table을 동적할당하여 해당 주소를 lock_table_ptr이 가리키게 합니다.   
   
동적할당 실패 시 -1을 반환하고,   
성공하면 0을 반환합니다.   
   
* #### lock_t* lock_acquire(int table_id, int64_t key)
<pre>
<code>
lock_t* lock_acquire(int table_id, int64_t key)
{
	pthread_mutex_lock(&lock_table_latch);
	
	table_entry* target_entry = lock_table_ptr->find_table_entry(table_id, key);

	lock_t* new_lock = new lock_t;
	if(new_lock == NULL){
		pthread_mutex_unlock(&lock_table_latch);
		return NULL;
	}
	new_lock->sentinel_ptr = target_entry;
	new_lock->cond = PTHREAD_COND_INITIALIZER;

	if(target_entry->head == NULL){
		target_entry->head = new_lock;
		target_entry->tail = new_lock;
	}
	else{
		lock_t* prev = target_entry->tail;
		
		prev->next = new_lock;
		new_lock->prev = prev;

		target_entry->tail = new_lock;

		pthread_cond_wait(&(new_lock->cond), &lock_table_latch);
	}

	pthread_mutex_unlock(&lock_table_latch);
	
	return new_lock;
}
</code>
</pre>
1. lock_table_latch로 lock_table에 대한 mutex를 얻습니다. 만약 다른 스레드가 lock_table을 사용 중이라면 대기합니다.   
2. 찾고자 하는 테이블 id와 key가 위치하는 lock_table의 table_entry를 찾습니다.   
3. lock_t를 동적할당 받습니다. 실패한다면 lock_table_latch를 반납하고 NULL을 반환합니다.
4. 새로운 lock_t는 2번에서 찾은 table_entry를 sentinel_ptr로 가리키고, cond에 새로운 Conditional Variable을 할당받습니다. 
5. 새로운 lock을 table_entry에 입력합니다.  
5-1. 만약 table_entry에 lock이 하나도 없었다면 lock 리스트의 head와 tail을 새로운 lock으로 설정합니다.   
5-2. 만약 이미 lock이 있었다면 tail에 새로운 lock을 삽입하고 해당 lock의 cond를 사용하여 pthread_cond_wait함수를 호출하여 대기합니다.    
&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;이후 다른 스레드에서 해당 cond로 pthread_cond_signal을 호출하면 해당 스레드가 깨어나며 lock_table_latch를 얻습니다.     
6. lock_table_latch를 반납합니다.
7. 새로운 lock의 주소값을 반환합니다.   
   
* #### int lock_release(lock_t* lock_obj)
<pre>
<code>
int lock_release(lock_t* lock_obj)
{
	pthread_mutex_lock(&lock_table_latch);
	
	table_entry* target_entry = lock_obj->sentinel_ptr;

	if(lock_obj != target_entry->head){
		pthread_mutex_unlock(&lock_table_latch);
		return -1;
	}

	if (target_entry->head->next == NULL){
		target_entry->head = NULL;
		target_entry->tail = NULL;
	}
	else{
		target_entry->head = lock_obj->next;
		target_entry->head->prev = NULL;

		pthread_cond_signal(&(target_entry->head->cond));
	}

	delete lock_obj;
	
	pthread_mutex_unlock(&lock_table_latch);
	return 0;
}
</code>
</pre>
1. lock_table_latch를 얻습니다.
2. 인자로 받은 lock이 가리키는 table_entry를 찾습니다.
3. 해당 table_entry의 head와 인자로 받은 lock이 다르다면 -1을 반환합니다.
4. 인자로 받은 lock이 lock리스트의 head일 때, 해당 lock을 리스트에서 제거합니다.   
4-1. 인자로 받은 lock이 lock 리스트에 홀로 존재했다면 table_entry의 head와 tail을 NULL로 변경합니다.   
4-2. 인자로 받은 lock의 next가 존재한다면 table_entry의 head를 재조정하고 새로운 head가 가졌던 conditional variable로    
&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;head가 가리키는 lock을 가진 스레드를 깨웁니다.   
5. 인자로 받은 lock의 동적할당을 해제합니다.
6. lock_table_latch를 반납합니다.
7. 0을 반환합니다.