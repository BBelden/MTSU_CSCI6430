typedef int MPI_Datatype;
#define MPI_CHAR           ((MPI_Datatype)0x4c000101)
#define MPI_INT            ((MPI_Datatype)0x4c000405)

typedef int MPI_Comm;
#define MPI_COMM_WORLD ((MPI_Comm)0x44000000)

typedef int MPI_Request;
typedef int MPI_Aint;
typedef int MPI_Op;

#define MPI_MAX     (MPI_Op)(0x58000001)
#define MPI_MIN     (MPI_Op)(0x58000002)
#define MPI_SUM     (MPI_Op)(0x58000003)
#define MPI_PROD    (MPI_Op)(0x58000004)
#define MPI_LAND    (MPI_Op)(0x58000005)
#define MPI_BAND    (MPI_Op)(0x58000006)
#define MPI_LOR     (MPI_Op)(0x58000007)
#define MPI_BOR     (MPI_Op)(0x58000008)
#define MPI_LXOR    (MPI_Op)(0x58000009)
#define MPI_BXOR    (MPI_Op)(0x5800000a)
#define MPI_MINLOC  (MPI_Op)(0x5800000b)
#define MPI_MAXLOC  (MPI_Op)(0x5800000c)
#define MPI_REPLACE (MPI_Op)(0x5800000d)

typedef struct MPI_Status {
    int count;
    int cancelled;
    int MPI_SOURCE;
    int MPI_TAG;
    int MPI_ERROR;
} MPI_Status;

typedef int MPI_Win;
#define MPI_WIN_NULL ((MPI_Win)0x20000000)

typedef int MPI_Info;
#define MPI_INFO_NULL         ((MPI_Info)0x1c000000)
#define MPI_MAX_INFO_KEY       255
#define MPI_MAX_INFO_VAL      1024

#define MPI_SUCCESS          0      
#define MPI_ERR_BUFFER       1     
#define MPI_ERR_COUNT        2    
#define MPI_ERR_TYPE         3   
#define MPI_ERR_TAG          4  
#define MPI_ERR_COMM         5 
#define MPI_ERR_RANK         6
#define MPI_ERR_ROOT         7   
#define MPI_ERR_TRUNCATE    14  
#define MPI_ANY_SOURCE     ((int)5000)
#define MPI_ANY_TAG        ((int)5500)


/* We require that the C compiler support prototypes */
/* Begin Prototypes */
int MPI_Init(int *, char ***);
int MPI_Finalize(void);
//int MPI_Abort(MPI_Comm, int);
//int MPI_Attr_put(MPI_Comm, int, void*);
//int MPI_Attr_get(MPI_Comm, int, void *, int *);
double MPI_Wtime(void);
//double MPI_Wtick(void);
int MPI_Gather(void* , int, MPI_Datatype, void*, int, MPI_Datatype, int, MPI_Comm); 
int MPI_Barrier(MPI_Comm );
int MPI_Bcast(void*, int, MPI_Datatype, int, MPI_Comm );
int MPI_Comm_dup(MPI_Comm, MPI_Comm *);
int MPI_Comm_size(MPI_Comm, int *);
int MPI_Comm_rank(MPI_Comm, int *);
int MPI_Isend(void*, int, MPI_Datatype, int, int, MPI_Comm, MPI_Request *);
int MPI_Irecv(void*, int, MPI_Datatype, int, int, MPI_Comm, MPI_Request *);
//int MPI_Probe(int, int, MPI_Comm, MPI_Status *);
int MPI_Recv(void*, int, MPI_Datatype, int, int, MPI_Comm, MPI_Status *);
//int MPI_Reduce(void* , void*, int, MPI_Datatype, MPI_Op, int, MPI_Comm);
//int MPI_Rsend(void*, int, MPI_Datatype, int, int, MPI_Comm);
int MPI_Send(void*, int, MPI_Datatype, int, int, MPI_Comm);
int MPI_Ssend(void*, int, MPI_Datatype, int, int, MPI_Comm);
int MPI_Test(MPI_Request *, int *, MPI_Status *);
int MPI_Wait(MPI_Request *, MPI_Status *);
//int MPI_Get(void *, int, MPI_Datatype, int, MPI_Aint, int, MPI_Datatype, MPI_Win);
//int MPI_Put(void *, int, MPI_Datatype, int, MPI_Aint, int, MPI_Datatype, MPI_Win);
//int MPI_Win_create(void *, MPI_Aint, int, MPI_Info, MPI_Comm, MPI_Win *);
//int MPI_Win_lock(int, int, int, MPI_Win);
//int MPI_Win_unlock(int, MPI_Win);

