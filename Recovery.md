Introduce
=========
Recovery의 구현에 대한 프로젝트입니다.   
   
Commit된 트랜잭션의 결과는 DB에 저장되어야하고, Commit되지 못한 트랜잭션의 결과는 DB에 반영되어있지 않도록 해야합니다.   
그러나 DBMS가 작동하는 과정에서는 예상치 못한 Crash가 발생할 수 있고 이로 인해 DB가 위의 성질을 만족시킬 수 없을 가능성이 존재합니다.   
   
이러한 Crash가 난 이후에도 DB가 그 직전의 상태로 돌아갈 수 있도록 하기위해 Recovery가 필요합니다.   
Recovery는 두 가지 매니저의 작동을 기반으로 구현되었습니다.
   
1. Log Manager   
2. Recovery Manager   
   
DB의 상태를 이전으로 되돌리기 위해서는 DBMS가 이전에 어떠한 동작들을 수행했었는지에 대한 기록이 필요합니다. 이 기록을 로그라는 명칭의 데이터로 저장을 하고 로그 매니저는 로그를 디스크에 저장하는 역할을 수행합니다.   
   
리커버리 매니저는 디스크에 저장되어있는 로그를 기반으로 DBMS를 크래쉬 이전으로 되돌리는 역할을 수행합니다.   
   
또한 Recoevry와는 별개로 project5에서 구현한 Concurrency Control을 좀 더 일반적인 디자인으로 구현하였습니다.     
   
Feature
=======
* Log Manager
* Recovery Manager
* General Locking
   
Log Manager
============
로그의 발행 및 로그파일과의 동기화를 담당하는 Log Manager 입니다.    
log.cpp 소스코드 및 logManager.h 헤더파일에 구현되었습니다.   

* Introduce
* Log Publishing
* Flush

> #### Introduce   

   

