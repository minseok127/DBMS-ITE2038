Introduce
=========
Recovery의 구현에 대한 프로젝트입니다. 로그의 시퀀스 번호는 로그의 마지막 오프셋을 기준으로 생성하였고 ARIES 방식으로 구현되었습니다.   
      
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
로그 구조체는 역할에 따라서 다양한 종류로 구성되어있습니다.   
   
* ### basic log
<pre>
<code>
struct basicLog{
    uint64_t seqNo;
    uint64_t prevSeqNo;
    int trx_id;
    int type;
    int size;
};
</code>
</pre>
begin, commit, rollback 시에 발행되는 로그입니다.    
각각 begin은 트랜잭션이 생성되었을 때 발행되고, commit은 트랜잭션이 커밋되었을 때 발행되고,    
rollback은 트랜잭션이 되돌림이 마무리되었을 떄 발행됩니다.   
   
해당 로그가 가지고 있는 정보는 로그의 시퀀스 넘버, 동일 트랜잭션 내에서 발행된 이전 로그의 번호, 트랜잭션 번호, 로그의 타입, 로그의 크기로
구성되어있습니다.     
   
> 만약 해당 로그가 트랜잭션의 첫 번째 로그라면 prevSeqNo는 0의 값을 가집니다.    
   
* ### update log
<pre>
<code>
struct updateLog{
    uint64_t seqNo;
    uint64_t prevSeqNo;
    int trx_id;
    int type;
    
    int table_id;
    uint64_t pagenum;
    int offset;
    int dataLength;
    char undo[120];
    char redo[120];

    int size;
};
</code>
</pre>
update 시에 발행되는 로그입니다. redo를 위해 basic 로그로부터 추가된 정보들이 존재합니다.   
해당 update의 대상이 되는 테이블 id, 오프셋, 데이터의 길이, undo(update 이전 레코드), redo(update 이후 레코드), 로그의 크기입니다.   
   
* ### compensate log
<pre>
<code>
struct compensateLog{
    uint64_t seqNo;
    uint64_t prevSeqNo;
    int trx_id;
    int type;

    int table_id;
    uint64_t pagenum;
    int offset;
    int dataLength;
    char undo[120];
    char redo[120];

    uint64_t nextUndoSeqNo;

    int size;
};
</code>
</pre>
update 동작을 되돌려줄 때 발행되는 로그입니다. 트랜잭션이 abort되거나 리커버리의 undo 단계에서 발행될 수 있습니다.    
   
update로그의 구성요소에 추가하여 nexUndoSeqNo라는 멤버가 추가되었습니다.      
해당 변수는 ARIES 방식에서 사용하는 next undo sequence number를 의미합니다.      
    
* ### Transaction Table
로그 매니저는 로그의 발행을 트랜잭션별로 관리하기 위해 트랜잭션 테이블이라는 구성요소를 가지고 있습니다.    
트랜잭션 테이블은 로그를 발행을 위한 정보를 관리합니다.      
<pre>
<code>
struct trxTable{
    trxTable* next = NULL;
    int trx_id;

    uint64_t lastSeqNo = 0;

    stack<uint64_t> updateSeqNoStack;
    uint64_t nextUndoSeqNo = 0;
};
</code>
</pre>
트랜잭션 테이블은 해쉬테이블의 형태로 구성되어있고 이를 위한 next 포인터를 가지고 해당 트랜잭션의 id 또한 저장되어있습니다.   
   
lastSeqNo은 해당 트랜잭션이 발행한 마지막 로그의 시퀀스 넘버를 의미합니다. 이후 새로 발행된 로그의 prevSeqNo에 사용됩니다.   
    
updateSeqNoStack은 해당 트랜잭션이 발행한 update로그들의 시퀀스 넘버를 스택으로 저장하는 데 사용됩니다. 가장 최근에 발행된 update로그가 스택의 top에 위치합니다. 이 스택은 이후에 compensate로그를 발행할 때 사용됩니다.   
   
nextUndoSeqNo 또한 이후 발생할 compensate로그의 발행에 사용되는 변수입니다.    
새로 발행되는 compensate의 nextUndoSeqNo를 위해 사용됩니다.    
   
* ### Meta Log
리커버리 이후에 발행되는 로그의 시퀀스 번호와 트랜잭션 id를 durable하게 관리하기 위해 사용되는 로그입니다.   
로그파일의 맨 앞에 위치하고 새로운 로그의 발행과 새로운 트랜잭션의 생성은 이러한 메타 로그를 기준으로 발생합니다.   
<pre>
<code>
struct MetaLog{
    uint64_t metaLSN;
    int metaTrxId;
};
</code>
</pre>
metaLSN은 새로운 로그 시퀀스 넘버의 기준이 되는 값입니다.   
예를 들어 어떠한 DBMS가 크래쉬가 나기 이전에 로그파일에 저장되었던 로그의 시퀀스 넘버가 100이라면 이를 리커버리하는 과정의 시작부터 새롭게 발행되는 로그의 시퀀스 번호 또한 100보다 큰 값부터 시작해야합니다. metaLSN은 이와같이 새로운 로그 시퀀스 넘버의 기준이 되는 값입니다.   
   
metaTrxId 또한 마찬가지로 크래쉬 당시 트랜잭션이 100까지 생성되었다면 리커버리 이후 새롭게 시작되는 트랜잭션의 id를 101부터 durable하게 맞춰주기 위해 사용되는 값입니다.   
   
* ### Log Manager
<pre>
<code>
class LogManager{
private:
    /* Managing trxTable with hash */
    trxTable* trxTableHash = NULL;

    /* Log Buffer */
    char buf[SIZE_LOG_BUF];

    /* Maintain offset about buffer and file */
    uint64_t tailBuf = 0;
    uint64_t tailFile = 0;

    /* For manage log file */
    int logFileFd = -1;

    /* For durable LSN, trx id */
    MetaLog* metaLog;
public:
    /* Initialize - destroy */
    LogManager(char* logfile_path);
    ~LogManager();

    /* Get log file's fd */
    int get_logFileFd();

    /* Publish log */
    uint64_t publish_beginLog(int trx_id);
    uint64_t publish_commitLog(int trx_id);
    uint64_t publish_rollbackLog(int trx_id);
    uint64_t publish_updateLog(int trx_id, int table_id, uint64_t pagenum, int offset, char* undo, char* redo);
    uint64_t publish_compensateLog(int trx_id, int table_id, uint64_t pagenum, int offset, char* undo, char* redo);

    /* Manager offset */
    void move_tailBuf(int size);
    void sync_tails();
    uint64_t get_tailBuf();

    /* Get trx table */
    trxTable* get_trxTable(int trx_id);
    /* Set trx table */
    void set_trxTable(int trx_id);
    /* Delete trx table */
    void delete_trxTable(int trx_id);
    /* Update trx table */
    void update_trxTable(int trx_id, uint64_t newSeqNo, int type);

    /* Flush */
    bool is_stuck(int size);
    void bufFlush();

    /* Get meta log */
    MetaLog* get_metaLog();
};
</code>
</pre>
로그 매니저의 선언부분입니다.   
   
로그 매니저는 트랜잭션 테이블을 해쉬테이블로 관리합니다. 또한 내부적으로 로그 버퍼를 가지고 있습니다.   
로그 버퍼를 관리하기 위해 tailBuf, tailFile이라는 변수를 사용합니다.   
   
tailBuf는 버퍼상에 존재하는 마지막 로그의 끝을 가리키는 변수입니다. tailFile은 로그 파일에 존재하는 마지막 로그의 끝을 가리키는 변수입니다.   
   
로그를 파일에 저장하려면 파일의 fd를 알아야합니다. 로그 매니저는 처음 할당될 때 인자로 로그 파일의 경로를 받고, 해당 파일을 open한 후 fd를 인자로 보유합니다.   
   
로그파일을 처음 open하면서 파일의 맨 앞에 있는 meta log의 정보를 읽어서 멤버 변수에 저장합니다.   
이를 기반으로 새로운 로그의 발행과 트랜잭션을 생성됩니다.      
   
> #### Log Publishing
로그의 발행은 로그 매니저의 멤버 함수를 이용합니다.   
   
* ### publish_beginLog
begin 로그를 발행할 때 사용하는 함수입니다.   
   
* ### publish_commitLog
commit 로그를 발행할 때 사용하는 함수입니다.   
내부적으로 로그 버퍼 플러쉬를 진행합니다.   
   
* ### publish_rollbackLog
rollback 로그를 발행할 때 사용하는 함수입니다.   
   
* ### publish_updateLog
update 로그를 발행할 때 사용하는 함수입니다.   
   
* ### publish_compensateLog
compensate 로그를 발행할 때 사용하는 함수입니다.   
   
위의 함수들은 내부적으로 tailBuf를 조절하며 발행됩니다.   
새로운 로그 객체가 생성된 이후에는 로그 매니저가 관리하는 로그 버퍼로 바이트단위 복사됩니다.   

또한 로그를 발행하면서 트랜잭션 테이블의 구성요소들도 최신화됩니다.   
   
> #### Flush
로그 매니저의 로그 버퍼가 디스크로 내려가는 경우는 3가지 경우가 존재합니다.   
   
1. commit
2. page write
3. Full buffer
   
1번의 경우는 commit 로그를 발행하면서 내부적으로 로그 버퍼의 flush가 일어납니다.   
2번의 경우는 page eviction을 해야할 때, 해당 페이지가 dirty여서 디스크와 동기화를 시켜줘야한다면 로그 버퍼 또한 flush합니다.   
3번의 경우는 로그 버퍼가 꽉차서 더이상 로그를 버퍼에 담을 수 없을 때 flush합니다.   
     
R
