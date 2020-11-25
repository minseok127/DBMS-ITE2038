Introduce
==========
Concurrency Control의 구현에 대한 설명입니다.   
   
이번 프로젝트부터는 여러 Thread들마다 transaction을 사용하여 DB를 사용하게 되고,   
각각의 transaction은 trx_begin함수의 호출로 시작하여 trx_commit함수의 호출로 끝나게 됩니다.      
   
begin과 commit 사이에서는 db_find와 db_update API를 사용하여 원하는 테이블의 레코드를 찾거나 변경하게 됩니다.   
그러나 db_find와 db_update 사용 중 데드락이 발생하면 이 함수들의 내부에서 abort됩니다.   
   
시스템 내부적으로는 Concurrency Control을 위해서 Strict 2 Phase Locking을 사용하여 트랜잭션들의 lock을 관리하고,   
트랜잭션이 새로운 lock을 추가할 때마다 deadlock 여부를 판단하여 abort할 지, operation을 진행할 지 결정합니다.   
또한 lock은 record단위로 생성하게 됩니다.   
   
Features
========
* Transaction Manager
* Lock
* Other Modification

Transaction Manager
====================
트랜잭션들을 관리하고, 트랜잭션과 관련된 API를 제공하는 Transaction Manager입니다.   
   
각 트랜잭션들의 정보를 설정하거나 불러오는데 쓰이는 trxManager객체와   
트랜잭션을 사용하기 위해 쓰이는 trx_begin, trx_commit 등과 같은 함수들이 정의되어있습니다.   
   
* Introduce
* Functions of Transaction Manager
* Functions for API
* API

> #### Introduce
트랜잭션이 새롭게 생성될 때마다 해당 트랜잭션은 id를 부여받습니다.    
트랜잭션의 id는 1부터 시작하며 새로운 트랜잭션은 마지막으로 생성되었던 트랜잭션 id에 1을 더한 값을 id로 할당받습니다.    
   
트랜잭션이 생성되면 그에 대응하는 trxNode가 생성됩니다. trxNode는 해당 트랜잭션에 대한 다양한 정보를 가지고 있습니다.  
   
![trxNode](uploads/3a1a60fbc5f3ac4a8e27f8dca544dadf/trxNode.png)
   
트랜잭션이 lock을 생성하고 lock manager에 걸릴 떄마다 해당 lock은 trxNode의 lock head에 추가되게 됩니다.   
   
또한 트랜잭션이 db_find나 db_update 도중 lock을 얻지 못하고 대기 상태가 되는 confilct이 일어나면 해당 트랜잭션은 잠들게 되고,   
confilct이 풀리게 되어야 트랜잭션은 잠에서 깨어나 일하게 됩니다.   
      
즉 하나의 트랜잭션당 confilct이 발생한 lock은 존재하거나, 존재하지 않거나 두가지 상태를 유지하게 됩니다.   
confilct lock은 confilct이 발생한 lock을 가리킵니다. lock head로 표현되는 lock list와는 별개의 포인터입니다.   
   
next trxNode는 trxManager가 trxNode를 해쉬테이블 형태로 관리하기 위해 사용되는 포인터입니다.    
해쉬 테이블 상 next trxNode를 가리팁니다.   
   
record log는 해당 트랜잭션이 현재까지 변경해온 레코드들의 기록을 담고 있습니다.   
trxNode는 레코드들의 record log 객체들을 해쉬테이블 형태로 관리합니다.   
   
![record_log](uploads/b74a5778a62c9cfeb831977dcec6374d/record_log.png)
   
record_log 객체는 어떤 table id의 무슨 key에 대한 record인지를 구별하기 위한 정보를 가지고,   
trxNode가 record_log 객체를 해쉬 테이블 형태로 관리하기 때문에 이를 위한 next record_log를 가집니다.   
   
또한 record_log 객체는 original_value라는 변수를 가지는데   
이 변수는 트랜잭션이 변경하기 이전의 레코드를 담고 있습니다. 즉, 트랜잭션이 호출되는 시점에 저장되어있던 레코드를 가집니다.   
   
* ### trx_manager_latch
trxManager에는 동시에 여러 스레드들이 접근하여 정보가 제대로 반영되지 못하거나 잘못된 정보를 가져갈 위험이 있습니다.   
그렇기 때문에 trxManager의 정보를 업데이트할 때에는 trx_manager_latch라는 mutex를 통해 관리하겠습니다.   
    
> #### Functions of Transaction Manager
   
trxManager가 사용하는 함수들에 대한 설명입니다.   
      
* ### int trxManager::set_new_trxNode()
새로운 trxNode 객체를 생성하고 trxManager의 해쉬 테이블에 추가합니다.   
   
해당 trxNode 객체는 trxManager가 내부적으로 저장하고 있던 trx id를 할당받습니다.   
id의 할당이 끝나면 trxManager는 다음 트랜잭션을 위해 저장하고 있던 id를 1 증가시킵니다.   
   
trxNode가 할당받은 trx id를 반환합니다.   
   
* ### trxNode* trxManager::get_trxNode(int trx_id)
인자로 받은 trx_id에 해당하는 trxNode의 주소값을 반환하는 함수입니다.   
   
* ### int trxManager::delete_trxNode(int trx_id)   
인자로 받은 trx_id에 해당하는 trxNode를 trxManager가 관리하는 해쉬 테이블에서 제거하는 함수입니다.   
   
* ### void trxManager::store_original_log(int trx_id, int table_id, int64_t key, char* original)
인자로 받은 trx_id에 해당하는 trxNode에 orignal한 record를 저장하기 위한 함수입니다.   
table_id와 key, orignal record를 담은 record_log를 만들고 trxNode가 관리하는 record_log 해쉬 테이블에 저장합니다.   
   
만약 해당하는 record_log가 이미 trxNode의 record_log 해쉬 테이블에 존재한다면 중복된 저장을 하지 않고 함수를 종료하고,   
해당 record_log가 아예 처음 생긴 record_log일 떄만 해쉬테이블에 저장합니다.   
    
* ### record_log* trxManager::load_log(int trx_id, int table_id, int64_t key)
인자로 받은 trx_id의 trxNode가 관리하는 record_log의 주소값을 반환합니다.   
   
> #### Functions for API
libbpt는 아니지만 
* ### int init_trxManager()
