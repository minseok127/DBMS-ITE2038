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
   
 