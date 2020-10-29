Data Base Management System
===========================
DBMS는 여러개의 계층으로 구성되어 있습니다.   
해당 프로젝트에서는 리눅스 환경에서 c++언어 및 g++ 7.5.0버전을 사용하여 구현합니다.   
   
Features
========
### 1. Disk based b+tree   
디스크에 직접적으로 접근하여 데이터를 저장하고 불러오는 역할을 하는 Disk Space Manager & File Manager에 대한 구현과    
데이터를 효율적으로 다루기 위한 Index Manager에 대한 구현입니다.    
   
b+tree의 작동방식에 대한 위키와 이를 디스크 상에서 어떻게 구현하는 지에 대한 위키로 구별하여 작성되었습니다.     
+ [Basic Operations of B+Tree](https://hconnect.hanyang.ac.kr/2020_ite2038_11800/2020_ite2038_2016025650/-/wikis/Basic-Operations-of-B-Tree(Milestone1))
+ [Disk based B+Tree](https://hconnect.hanyang.ac.kr/2020_ite2038_11800/2020_ite2038_2016025650/-/wikis/Disk-based-b-tree(Milestone2))

### 2. Buffer Manager   
디스크 I/O를 빈번하게 사용하는 행위는 프로그램의 퍼포먼스를 해치게 됩니다.   
이를 해결하기 위하여 Index 계층과 Disk 계층 사이에 Buffer 계층을 형성하여 디스크 I/O의 횟수를 최소한으로 줄입니다.   
+ [Buffer Manager](https://hconnect.hanyang.ac.kr/2020_ite2038_11800/2020_ite2038_2016025650/-/wikis/Buffer-Manager(project3))