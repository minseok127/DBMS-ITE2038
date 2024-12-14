DBMS [HYU-ITE2038, 2nd semester 2020]
===============================================================
DBMS는 여러개의 계층으로 구성되어 있습니다.   
해당 프로젝트에서는 리눅스 환경에서 c++언어 및 g++ 7.5.0버전을 사용하여 구현합니다.   
   
Features
========
### 1. Disk based b+tree   
디스크에 직접적으로 접근하여 데이터를 저장하고 불러오는 역할을 하는 Disk Space Manager & File Manager에 대한 구현과    
데이터를 효율적으로 다루기 위한 Index Manager에 대한 구현입니다.    
   
b+tree의 작동방식에 대한 위키와 이를 디스크 상에서 어떻게 구현하는 지에 대한 위키로 구별하여 작성되었습니다.     
+ [Basic Operations of B+Tree](https://github.com/minseok127/DBMS-ITE2038/wiki/Basic-Operations-of-B-Tree(Milestone1))
+ [Disk based B+Tree](https://github.com/minseok127/DBMS-ITE2038/wiki/Disk-based-b-tree(Milestone2))

### 2. Buffer Manager   
디스크 I/O를 빈번하게 사용하는 행위는 프로그램의 퍼포먼스를 해치게 됩니다.   
이를 해결하기 위하여 Index 계층과 Disk 계층 사이에 Buffer 계층을 형성하여 디스크 I/O의 횟수를 최소한으로 줄입니다.   
+ [Buffer Manager](https://github.com/minseok127/DBMS-ITE2038/wiki/Buffer-Manager(project3))

### 3. Lock Table
여러 스레드가 하나의 레코드에 동시 접근한다면 레코드의 변경이 올바르게 이뤄지지 않을 수 있습니다.   
이러한 동시 접근을 보호하기 위하여 Lock Table을 사용합니다.
+ [Lock Table](https://github.com/minseok127/DBMS-ITE2038/wiki/Lock_table(project4))
   
### 4. Concurrency Control   
여러 transaction들을 동시에 제어하기 위한 작업입니다.   
multi thread를 고려해야하기 때문에 매니저들에 크고 작은 변화가 생겼고, transaction을 위한 매니저가 생겼습니다.   
+ [Concurrency Control](https://github.com/minseok127/DBMS-ITE2038/wiki/Concurrency-Control(project5))   
   
#### 5. Recovery    
DBMS는 사용자가 commit한 Transaction에 대해서 Atomicity와 Durability를 만족시켜야 합니다.    
즉 Transaction의 변경 사항이 모두 적용되거나 아예 적용되지 않아야 하고, commit에 성공했다면 DB에 해당 내용이 반드시 저장되어야 합니다.   
하지만 예상치 못한 시스템의 Crash에 대한 대비가 없다면 위의 성질들을 만족시킬 수 없습니다.   
DBMS에서는 Recovery를 통해 시스템의 Crash를 대비합니다.   
+ [Recovery](https://github.com/minseok127/DBMS-ITE2038/wiki/Recovery(project6))
