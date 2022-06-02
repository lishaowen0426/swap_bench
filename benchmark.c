#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <stdint.h>
#include <sys/time.h>
#include <fcntl.h>
#define die(msg, args...) \
   do {                         \
      fprintf(stderr,"(%s,%d) " msg "\n", __FUNCTION__ , __LINE__, ##args); \
      fflush(stdout); \
      exit(-1); \
   } while(0)

#define declare_timer uint64_t elapsed; \
   struct timeval st, et;

#define start_timer gettimeofday(&st,NULL);

#define stop_timer(msg, args...) ;do { \
	   gettimeofday(&et,NULL); \
	   elapsed = ((et.tv_sec - st.tv_sec) * 1000000) + (et.tv_usec - st.tv_usec) + 1; \
	   printf("(%s,%d) [%6lums] " msg "\n", __FUNCTION__ , __LINE__, elapsed/1000, ##args); \
} while(0)

#define MULTIPLIER 30
#define NB_ACCESS 100000UL
#define PAGE_SIZE 4096



static __thread __uint128_t g_lehmer64_state;

static void init_seed() {
   g_lehmer64_state = rand();
}

static uint64_t lehmer64() {
   g_lehmer64_state *= 0xda942042e4dd58b5;
   return g_lehmer64_state >> 64;
}

int main(int argc, char** argv){
    declare_timer;    
    init_seed();
    size_t mem_size = MULTIPLIER * 1024UL * 1024UL * 1024UL;
    printf("Allocate %d GB memory\n", MULTIPLIER);

    char* buffer;
    buffer = malloc(mem_size);
    if(buffer == NULL){
        die("allocate memory failed");        
    }
    memset(buffer, 1, mem_size); 
    char* page = malloc(PAGE_SIZE);
    size_t granularity = 64;
    memset(page, 1, PAGE_SIZE); 
    
    start_timer{
        for(size_t i = 0; i < NB_ACCESS; i++){ 
            uint64_t loc_rand = (lehmer64()/granularity*granularity) % (mem_size - granularity);
            memcpy(page, &buffer[loc_rand], granularity);
        }
    } stop_timer("%ld memcpy %lu memcpy/s %lu MB/s",NB_ACCESS ,NB_ACCESS*1000000LU/elapsed, NB_ACCESS*granularity*1000000LU/elapsed/1024/1024);


}
