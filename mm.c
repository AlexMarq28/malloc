
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <unistd.h>
#include <string.h>

#include "mm.h"
#include "memlib.h"


team_t team = {
    /* Team name */
    "105380202",
    /* First member's full name */
    "Alejandro Marquez Vera",
    /* First member's email address */
    "alexmarq28@gmail.com",
    /* Second member's full name (leave blank if none) */
    "",
    /* Second member's email address (leave blank if none) */
    ""
};

/* single word (4) or double word (8) alignment */
#define ALIGNMENT 8

/* rounds up to the nearest multiple of ALIGNMENT */
#define ALIGN(size) (((size) + (ALIGNMENT-1)) & ~0x7)


#define CHUNKSIZE (1<<6)

#define SIZE_T_SIZE (ALIGN(sizeof(size_t)))

#define LISTLIMIT 20
#define WSIZE 4
#define DSIZE 8

#define REALLOC_BUFFER 1<<7

#define GET(p)     (*(unsigned int *)(p))
#define GET_PTR(p) (*(void *)(p))
#define PUT(p,val) (*(unsigned int *)(p)=(val))
#define PACK(size, alloc) ((size) | (alloc))
#define SET_PTR(p,ptr) (*(void* *)(p)=(void*)(ptr))

#define HDRP(ptr) ((char *)(ptr) - WSIZE)
#define FTRP(ptr) ((char *)(ptr) + GET_SIZE(HDRP(ptr)) - DSIZE)



#define GET_SIZE(p)  (GET(p) & ~0x7)
#define GET_ALLOC(p) (GET(p) & 0x1)





#define PRED_PTR(ptr) ((char *)(ptr))
#define SUCC_PTR(ptr) ((char *)(ptr) + DSIZE)
#define NEXT_BLKP(ptr) ((char *)(ptr)+ GET_SIZE(HDRP(ptr )))
#define PREV_BLKP(ptr) ((char *)(ptr)-GET_SIZE((char*)(ptr)-DSIZE))

#define PRED(ptr) (*(char **)(ptr))
#define SUCC(ptr) (*(char **)(SUCC_PTR(ptr)))


void *segregated_free_lists[LISTLIMIT];




void * extend_heap(size_t size);
static void insert_node(void *ptr, size_t size) ;
static void * coalesce(void * ptr);
void* place(void* ptr, size_t nsize);
static void delete_node(void *ptr);
int mm_check();










/* 
 * mm_init - initialize the malloc package.
 */
int mm_init(void)
{
    int list;
    char *heap_start;
    
    for(list=0;list<LISTLIMIT;list++){
        segregated_free_lists[list]=NULL;
    }
    if((long)(heap_start=mem_sbrk(4*WSIZE))==-1){
        return -1;
    }
    
    PUT(heap_start,0);
    PUT(heap_start+(1*WSIZE),PACK(DSIZE,1));
    PUT(heap_start+(2*WSIZE),PACK(DSIZE,1));
    PUT(heap_start+(3*WSIZE),PACK(0,1));
    
    
     int prev_size= GET_SIZE((char*)(mem_heap_hi())-WSIZE);
    
    assert(GET_SIZE(heap_start+DSIZE)==DSIZE);
    if(extend_heap(CHUNKSIZE)==NULL)
        return -1;
//    mm_check();
    return 0;
    
    
}

/* 
 *  - Allocate a block by incrementing the brk pointer.
 *     Always allocate a block whose size is a multiple of the alignment.
 */
void *mm_malloc(size_t size)
{
    
    size_t nsize;
    size_t extendsize;
    
    void *ptr=NULL;
    
    if(size==0)return NULL;
    
    if(size<=2*DSIZE)nsize= 3*DSIZE;
    else nsize= ALIGN(size+DSIZE);
    size_t searchsize=nsize;
    int list;
    for(list=0;list<LISTLIMIT;list++){
        if(list==LISTLIMIT-1||(searchsize<=1&&segregated_free_lists[list]!=NULL)){
            ptr=segregated_free_lists[list];
            
            
            while((ptr !=NULL)&&((nsize>GET_SIZE(HDRP(ptr))) )){
                ptr= PRED(ptr);
                
            }
            if(ptr!=NULL) break;
        }
        searchsize>>=1;
        
    }
    if(ptr==NULL){
        extendsize=(nsize>CHUNKSIZE?nsize:CHUNKSIZE);
        ptr=extend_heap(extendsize);
        if(ptr==NULL) return NULL;
           
    }
    
           ptr=place(ptr,nsize);
            assert(GET_ALLOC(HDRP(ptr)));
//            mm_check();
           return ptr;
    


}

           
           
           
           

           
           
/*
 * mm_free - Freeing a block does nothing.
 */
void mm_free(void *ptr)
{
    size_t size = GET_SIZE(HDRP(ptr));
    PUT(HDRP(ptr),PACK(size,0));
    PUT(FTRP(ptr),PACK(size, 0));
    
    insert_node(ptr, size);
    coalesce(ptr);
//    mm_check();
    return;
}

/*
 * mm_realloc - Implemented simply in terms of mm_malloc and mm_free, the first implementation is for a slightly different implementation of the allocator
 */
/*
void *mm_realloc(void *ptr, size_t size)
{
    void *new_ptr=ptr;
    size_t new_size= size;
    int remainder;
    int block_buffer;
    int extendsize;


    if(size==0)
        return NULL;

    if(new_size<=2*DSIZE){
        new_size=3*DSIZE;
    }else{
        new_size= ALIGN(size+DSIZE);
    }

    new_size+=REALLOC_BUFFER;
    block_buffer= GET_SIZE(HDRP(ptr))-new_size;

    if(block_buffer<0){
        if(!GET_ALLOC(HDRP(NEXT_BLKP)) || !GET_SIZE(HDRP(NEXT_BLKP(ptr)))){
            remainder=GET_SIZE(HDRP(ptr))+GET_SIZE(HDRP(NEXT_BLKP(ptr))) - new_size;

            if(remainder<0){
                extendsize = MAX(-remainder, CHUNKSIZE);
                if(extend_heap(extendsize)==NULL) return NULL;
                remainder+= extendsize;
            }
            delete_node(NEXT_BLKP(ptr));

            PUT_NOTAG(HDRP(ptr), PACK(new_size+remainder, 1));
            PUT_NOTAG(FTRP(ptr), PACK(new_size + remainder, 1));

        }else{
                new_ptr=mm_malloc(new_size-DSIZE);
                memcpy(new_ptr, ptr, MIN(size,new_size));
                mm_free(ptr);
            }
            block_buffer = GET_SIZE(HDRP(new_ptr))-new_size;
        }
    if(block_buffer<2*REALLOC_BUFFER)
        SET_RATAG(HDRP(NEXT_BLKP(new_ptr)));

    return new_ptr;
    }
*/



    void * mm_realloc(void* ptr, size_t size){
            
            void *optr= ptr;
            void *nptr;
            void* nextptr;
            
            size_t csize, asize, nsize;
            
            
            if(size==0){
                mm_free(optr);
                return NULL;
            }
            if(optr==NULL)
                return mm_malloc(size);
            asize=ALIGN(size);
            csize=GET_SIZE(HDRP(ptr))-DSIZE;
            
            if(asize == csize)return optr;
            
            if(asize<csize){
                if(csize-asize<=3*DSIZE) return optr;
                PUT(HDRP(optr),PACK(asize+DSIZE,1));
                PUT(FTRP(optr),PACK(asize+DSIZE,1));
                
                nptr=NEXT_BLKP(optr);
                
                PUT(HDRP(nptr),PACK(csize-asize,0));
                PUT(FTRP(nptr), PACK(csize-asize,0));
                insert_node(nptr,csize-asize);
                coalesce(nptr);
                return optr;
                
            }
            
            nextptr= NEXT_BLKP(optr);
            
            if(nextptr!=NULL && !GET_ALLOC(HDRP(nextptr))){
                nsize=GET_SIZE(HDRP(nextptr));
                if(nsize+csize>=asize){
                    delete_node(nextptr);
                    if(nsize+csize-asize<3*DSIZE){
                        PUT(HDRP(optr),PACK(csize+nsize+DSIZE,1));
                        PUT(FTRP(optr),PACK(csize+nsize+DSIZE,1));
                        return optr;
                        
                    }else{
                        PUT(HDRP(optr),PACK(asize+DSIZE,1));
                        PUT(FTRP(optr),PACK(asize+DSIZE,1));
                        
                        nptr=NEXT_BLKP(optr);
                        
                        PUT(HDRP(nptr),PACK(csize-asize+nsize,0));
                        PUT(FTRP(nptr),PACK(csize-asize+nsize,0));
                        insert_node(nptr, GET_SIZE(HDRP(nptr)));
                        coalesce(nptr);
                        return optr;
                    }
                }
            }
            
            nptr= mm_malloc(size);
            if(nptr==NULL)
                return NULL;
            
            memcpy(nptr,optr,size);
            mm_free(optr);
            return nptr;
            
            
        }












void* place(void* ptr, size_t nsize){
            size_t remainder= GET_SIZE(HDRP(ptr))-nsize;
            size_t osize=GET_SIZE(HDRP(ptr));
            delete_node(ptr);
            if(remainder<=3*DSIZE){
                PUT(HDRP(ptr),PACK(osize,1));
                PUT(FTRP(ptr),PACK(osize,1));
            }
            else{
                PUT(HDRP(ptr),PACK(nsize,1));
                PUT(FTRP(ptr),PACK(nsize,1));
                PUT(HDRP(NEXT_BLKP(ptr)),PACK(remainder,0));
                PUT(FTRP(NEXT_BLKP(ptr)), PACK(remainder,0));
                insert_node(NEXT_BLKP(ptr),remainder);
                
            }
            return ptr;
            
}
           
           
static void delete_node(void *ptr) {
            int l=0;
            size_t size =GET_SIZE(HDRP(ptr));
            
            while((l<LISTLIMIT -1)&&(size>1)) {
                size>>=1;
                l++;
            }
            
            if(PRED(ptr)!=NULL){
                if(SUCC(ptr)!=NULL){
                    SET_PTR(PRED_PTR(SUCC(ptr)),PRED(ptr));
                    SET_PTR(SUCC_PTR(PRED(ptr)),SUCC(ptr));
                    
                }else{
                    SET_PTR(SUCC_PTR(PRED(ptr)),NULL);
                    segregated_free_lists[l]=PRED(ptr);
                }
            }else{
                if(SUCC(ptr)!= NULL){
                    SET_PTR(PRED_PTR(SUCC(ptr)), NULL);
                } else{
                    segregated_free_lists[l]= NULL;
                }
            }
            return;
        }
 
           
       static void * coalesce(void * ptr){
           
            
                     void* top = mem_heap_hi();
                     void* bottom = mem_heap_lo();
           
           
            void* nblk= NEXT_BLKP(ptr);
            void* pblk= PREV_BLKP(ptr);
           size_t nsize=GET_SIZE(HDRP(nblk));
           size_t psize=GET_SIZE(HDRP(pblk));
           size_t size= GET_SIZE(HDRP(ptr));
           
            
           
           int next_alloc=GET_ALLOC(HDRP(nblk));
            int prev_alloc=GET_ALLOC(HDRP(pblk));
           
            void* rptr = ptr;
          
           
                    if(!next_alloc&&!prev_alloc){
                        delete_node(ptr);
                        delete_node(pblk);
                        delete_node(nblk);
                        
                        
                        
                        PUT(HDRP(pblk),PACK(size + psize + nsize,0));
                        PUT(FTRP(nblk),PACK(size+psize+nsize,0));
                        
                        insert_node(pblk,GET_SIZE(HDRP(pblk)));
                        rptr=pblk;
                    }
                   else if(!next_alloc){
                        delete_node(ptr);
                        delete_node(nblk);
                       
                        PUT(HDRP(ptr),PACK(size+nsize,0));
                        PUT(FTRP(nblk),PACK(size+nsize,0));
                        
                          insert_node(ptr,GET_SIZE(HDRP(ptr)));
                        rptr=ptr;
                    }
                   else if(!prev_alloc){
                        delete_node(ptr);
                        delete_node(pblk);
                        PUT(HDRP(pblk),PACK(size+psize,0));
                        PUT(FTRP(ptr),PACK(size+psize,0));
                       
                        insert_node(pblk,GET_SIZE(HDRP(pblk)));
                        rptr=pblk;
                    }
                    
                    return rptr;
            
        }


static void insert_node(void *ptr, size_t size) {
    int list = 0;
    void *search_ptr = ptr;
    void *insert_ptr = NULL;
    
    /* Select segregated list */
    while ((list < LISTLIMIT - 1) && (size > 1)) {
        size >>= 1;
        list++;
    }
    
    /* Keep size ascending order and search */
    search_ptr = segregated_free_lists[list];
    while ((search_ptr != NULL) && (size > GET_SIZE(HDRP(search_ptr)))) {
        insert_ptr = search_ptr;
        search_ptr = PRED(search_ptr);
    }
    
    /*Set predecessor and successor */
    if (search_ptr != NULL) {
        if (insert_ptr != NULL) {
            SET_PTR(PRED_PTR(ptr), search_ptr);
            SET_PTR(SUCC_PTR(search_ptr), ptr);
            SET_PTR(SUCC_PTR(ptr), insert_ptr);
            SET_PTR(PRED_PTR(insert_ptr), ptr);
        } else {
            SET_PTR(PRED_PTR(ptr), search_ptr);
            SET_PTR(SUCC_PTR(search_ptr), ptr);
            SET_PTR(SUCC_PTR(ptr), NULL);
            segregated_free_lists[list] = ptr;
        }
    } else {
        if (insert_ptr != NULL) {
            SET_PTR(PRED_PTR(ptr), NULL);
            SET_PTR(SUCC_PTR(ptr), insert_ptr);
            SET_PTR(PRED_PTR(insert_ptr), ptr);
        } else {
            SET_PTR(PRED_PTR(ptr), NULL);
            SET_PTR(SUCC_PTR(ptr), NULL);
            segregated_free_lists[list] = ptr;
        }
    }
    
    return;
}

void * extend_heap(size_t size){
                    size= ALIGN(size);
                    void* ptr=mem_sbrk(size);
                    if(ptr==(void*)-1) return NULL;
                    
                    PUT(HDRP(ptr),PACK(size,0));
                    PUT(FTRP(ptr),PACK(size,0));
                    PUT(HDRP(NEXT_BLKP(ptr)),PACK(0,1));
    
                   
                    insert_node(ptr,size);
                    int prev_size= GET_SIZE((char*)(ptr)-DSIZE);
                    return coalesce(ptr);
                    
}


int mm_check(){
    char *p =mem_heap_lo();
    p+=DSIZE;
    
    while(p<mem_heap_hi()){
        printf("%s block at %p, size %d\n",GET_ALLOC(HDRP(p))?"allocated":"free",p,GET_SIZE(HDRP(p)));
        assert(GET_SIZE(HDRP(p))==GET_SIZE(FTRP(p)));
        p+=GET_SIZE(HDRP(p));
        
    }
    return 1;
}
