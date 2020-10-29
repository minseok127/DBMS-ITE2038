Buffer Manager   
==============
디스크 I/O를 최소화 하기위해 Index 계층과 Disk 계층 사이에 위치하게 되는 계층입니다.   
     
해당 계층의 역할은 다음과 같습니다.   
1. 메모리 상에 페이지를 저장하여 해당 페이지에 대한 read가 생길 시 디스크가 아니라 메모리에서 불러오는 역할을 해주는 caching   
2. 페이지에 대한 수정을 할 때 디스크를 바로 수정하지 않고 버퍼를 수정함으로써 Index 계층이 디스크 입력을 기다리지 않고 다음 행동 수행 가능
   
본 프로젝트에서는 c++언어를 사용하여 구현하였고 리눅스 환경에서 g++ 7.5.0로 컴파일되었습니다.
  
Features
========
[1. Buffer Manager API](#buffer-manager-api)
   
[2. File Manager API modification](#file-manager-api-modification)
   
[3. Index Manager Command modification](#index-manager-command-modification)
   
## Buffer Manager API
+ Introduce
+ Header File
+ API

> ### Introduce
Buffer Manager는 API의 작동을 위해 크게 4가지의 객체를 이용하게 됩니다.     
    
+ #### Buffer 배열   
페이지들을 메모리 상에 저장할 Buffer Structure들의 배열입니다.   
유저가 Buffer Manager의 API 중 하나인 init_db함수(링크걸기)를 통해 동적할당하여 생성하게 됩니다.   
   
그림 넣고   

+ #### LRU_HEAD, LRU_TAIL (LRU LIST)   
이번 디자인에서는 페이지 eviction을 위해 LRU policy를 사용합니다.      
이를 위해 Buffer 배열의 논리적인 구조를 Double Linked List 형태로 생각할 것이고 이를 제어하기 위해 사용되는 객체들입니다.   
   
기본 구조 그림 넣어주고   
   
버퍼에 새로운 페이지가 들어오게 된다면 LRU_HEAD의 next가 해당 페이지를 가리키게 됩니다.   
   
그림 넣고   
   
이미 버퍼에 존재하던 페이지에 재접근 시에는 해당 페이지를 LRU LIST에서 뽑아낸 뒤 LRU_HEAD의 next로 보냅니다.   
   
그림 넣고   
   
버퍼에 존재하지 않는 페이지를 읽어야 하는데 버퍼에 자리가 없다면 LRU policy에 따라서 LRU_TAIL의 prev를 eviction하게 됩니다.   
   
그림 넣고   
   
만약 해당 prev의 pin이 0이 아니라면 LRU LIST를 순회하면서 eviction할 버퍼의 위치를 찾습니다.   
   
그림 넣고   
   
그리고 이러한 LRU LIST의 제어들에는 insert_into_LRUList, remove_from_LRUList, get_from_LRUList(링크걸어주고 3개 다) 이라는 함수들이 사용됩니다.   

+ #### 해쉬 객체   
해쉬 객체는 버퍼 내에서 특정 페이지가 어느 위치에 있는 지 파악하기 위해 쓰이는 객체입니다.    
     
init_db(링크걸기)를 호출하면 테이블 id마다 해쉬 객체가 할당됩니다.    
내부에는 해쉬 테이블과 이 테이블 안에 어떤 페이지들이 있는 지 알 수 있는 리스트가 존재합니다.   
    
그림넣기
    
해쉬 테이블은 버퍼의 개수만큼 칸을 가지고 chaining hash 디자인을 사용합니다. 또한 해쉬 함수로 나머지를 이용합니다.   
예를 들어 버퍼의 개수가 100개라는 가정하에, 1058이라는 페이지 번호는 해쉬테이블에서 58번 자리에 위치하게 됩니다.   
   
해쉬 테이블에 대해서 해당 객체는 find, insert, delete (이것들도 다 링크 걸기)함수를 사용할 수 있습니다.   
   
find함수(링크)를 통해 해당 테이블에 대해서 유저가 원하는 페이지를 담은 버퍼의 위치를 파악합니다.   
   
그림 넣기   
   
insert함수(링크)를 통해서는 해쉬 객체에 새로운 페이지를 추가하게 됩니다. 
   
그림 넣기   
   
delete함수(링크)를 통해 해쉬 객체에 존재하는 페이지를 해쉬 테이블에서 제외시킵니다.   
   
그림 넣기
   
추가로 객체 내부에는 해쉬 테이블뿐만 아니라 테이블 안에 무슨 페이지들이 들어있는 지를 알 수 있도록 리스트가 존재합니다.   
이러한 리스트를 유지하는 이유는 특정 테이블 id만 flush시키는 것을 효율적으로 진행하기 위함입니다.   
   
만약 이러한 리스트가 없다면   
   
1. 버퍼 전체를 순회하며 해당 테이블 id인지 확인하며 flush시키는 것   
2. 해당 테이블 id를 담당하는 해쉬테이블 전체를 순회하며 버퍼를 flush시키는 것   
      
이러한 상황을 생각할 수 있습니다. 그러나 만약 사용자가 여럿이고 pin이 0이 아닌 버퍼들이 존재한다면   
한번의 순회 후에 어떤 페이지가 버퍼안에 추가로 생겼을 지 모르기 때문에 처음부터 다시 순회를 시작해야하고 이는 시간적인 비용이 커집니다.   
   
하지만 특정 테이블 id만 flush를 시키는 것이 흔히 발생하는 동작이 아니라서 리스트를 유지하는 비용이 더 커질 수 있다는 점을 고려해야합니다.    
그렇기에 리스트를 유지하는데 드는 시간적, 공간적 비용을 최소화하기 위해서 Double Linked List를 활용하였습니다.   
   
+ 시간적 비용   
insert 시에는 헤더의 next에 추가해주기만 하면 되기에 O(1)   
delete 시에는 해쉬 테이블 상의 노드로부터 주소값을 얻어 바로 해당 위치를 찾기 때문에 O(1)       
   
+ 공간적 비용    
노드 하나당 next포인터(4바이트) + prev포인터(4바이트) + 페이지 번호(8바이트) = 16바이트   
모든 해쉬 객체 속 리스트 노드의 개수를 다 합쳐도 최대 버퍼의 개수가 되기 떄문에(노드가 나타내는 것은 버퍼에 들어있는 페이지넘버)   
하나의 버퍼당 최대 16바이트를 추가 할당한 것으로 생각할 수 있고 flush에 걸리는 시간을 줄이는 것에 비해 크지 않은 비용이라고 판단했습니다.   
    
이러한 이유 때문에 해쉬 객체는 해당 디자인을 선택하여 해쉬 테이블에 존재하는 페이지번호를 관리하겠습니다.
   
위의 내용을 시각화한 이미지입니다.     
4번 디자인 그림   
     
+ #### 스택 객체
현재 어떠한 페이지도 올라오지 않은 버퍼의 주소값을 스택을 사용하여 가지고 있는 객체입니다.   
버퍼에 존재하지 않는 새로운 페이지를 버퍼에 올릴 때 스택을 활용하여 비어있는 버퍼를 찾습니다.   

> ### Header File
Buffer Manageer가 사용하는 구조체 및 클래스, 함수들을 선언하는 부분입니다.   
buffer.h로 작성되었습니다.   
   
<pre>
<code>
struct Buffer{
    FreePage frame;
    int table_id = -1;
    pagenum_t pagenum = 0;
    bool is_dirty = false;
    int pin_count = 0;
    Buffer* next = NULL;
    Buffer* prev = NULL;
};
</code>
</pre>
버퍼를 나타내는 구조체입니다.   
페이지를 담을 frame변수는 FreePage로 설정하였습니다. 바이트단위로 값을 복사할 것이기 때문에 페이지의 자료형은 중요하지 않습니다.   
table_id는 해당 버퍼가 어떤 테이블 id의 페이지를 담았는지를 나타내는 변수입니다. -1로 초기화하였습니다.   
pagenum은 해당 버퍼가 담고 있는 페이지의 번호입니다. 0으로 초기화하였습니다.   
is_dirty는 디스크와 차이가 생겼는지를 나타내는 변수입니다. false로 초기화하였습니다.   
pin_count는 해당 버퍼를 얼마나 많은 사용자가 읽고 있는 지 나타내는 변수입니다. 0으로 초기화하였습니다.   
next, prev는 LRU LIST 구조를 나타내기 위하여 사용하는 변수입니다. NULL로 초기화하였습니다.   
   
<pre>
<code>
struct DoubleListNode{
	DoubleListNode* prev = NULL;
	DoubleListNode* next = NULL;
	pagenum_t pagenum = 0;
};
</code>
</pre>
   
> ### API
   
## File Manager API modification
+ Introduce
+ Modification
## Index Manager Command modification
+ Introduce
+ Modification