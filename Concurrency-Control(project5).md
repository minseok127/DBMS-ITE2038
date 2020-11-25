Introduce
==========
Concurrency Control의 구현에 대한 설명입니다.   
   
이번 프로젝트부터는 여러 Thread들마다 transaction을 사용하여 DB를 사용하게 되고,   
각각의 transaction은 trx_begin함수의 호출로 시작하여 trx_commit함수의 호출로 끝나게 됩니다.      
   
begin과 commit 사이에서는 db_find와 db_update API를 사용하여 원하는 테이블의 레코드를 찾거나 변경하게 됩니다.   
그러나 db_find와 db_update 사용 중 데드락이 발생하면 이 함수들의 내부에서 abort됩니다.   
   
