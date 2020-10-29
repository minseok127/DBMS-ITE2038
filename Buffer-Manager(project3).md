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
    
+ Buffer 배열 

> ### Header File
   
> ### API
   
## File Manager API modification
+ Introduce
+ Modification
## Index Manager Command modification
+ Introduce
+ Modification