#define _GNU_SOURCE
#include <sched.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <stdint.h>
#include <sys/time.h>
#include <fcntl.h>
#include <pthread.h>
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

#define foreach(item, array) \
    for(int keep = 1, \
            count = 0,\
            size = sizeof (array) / sizeof *(array); \
        keep && count != size; \
        keep = !keep, count++) \
      for(item = *((array) + count); keep; keep = !keep)

#define MULTIPLIER 32
#define NB_ACCESS 100000UL
#define PAGE_SIZE 4096

char* buffer;
size_t granularity;
size_t mem_size;

void pin_me_on(int core) {
   cpu_set_t cpuset;
   pthread_t thread = pthread_self();

   CPU_ZERO(&cpuset);
   CPU_SET(core, &cpuset);

   int s = pthread_setaffinity_np(thread, sizeof(cpu_set_t), &cpuset);
   if (s != 0)
      printf("Cannot pin thread on core %d\n", core);
}

static __thread __uint128_t g_lehmer64_state;

static void init_seed() {
   g_lehmer64_state = rand();
}

static uint64_t lehmer64() {
   g_lehmer64_state *= 0xda942042e4dd58b5;
   return g_lehmer64_state >> 64;
}

static double bandwith(long data_size, long time) {
   return (((double)data_size)/1024./1024.)/(((double)time)/1000000.);
}

void* func (void* arg){
    int core = (int)arg;
    pin_me_on(core);

    init_seed();
    char* page = malloc(PAGE_SIZE);
    memset(page, 1, PAGE_SIZE);
    for(size_t i = 0; i < NB_ACCESS; i++){ 
        uint64_t loc_rand = (lehmer64()/granularity*granularity) % (mem_size - granularity);
        memcpy(page, &buffer[loc_rand], granularity);
    }
    return NULL;
} 
int main(int argc, char** argv){
    //declare_timer;
    
    init_seed();
    mem_size = MULTIPLIER * 1024UL * 1024UL * 1024UL;
    printf("Allocate %d GB memory\n", MULTIPLIER);

    buffer = malloc(mem_size);
    if(buffer == NULL){
        die("allocate memory failed");        
    }
    memset(buffer, 1, mem_size); 
    char* page = malloc(PAGE_SIZE);
    memset(page, 1, PAGE_SIZE); 
/*    
    for( granularity = 64; granularity <= 4096; granularity *= 2){
    
        start_timer{
            for(size_t i = 0; i < NB_ACCESS; i++){ 
                uint64_t loc_rand = (lehmer64()/granularity*granularity) % (mem_size - granularity);
                memcpy(page, &buffer[loc_rand], granularity);
            }
        } stop_timer("%ld memcpy %lu memcpy/s %lu MB/s, granularity %lu",NB_ACCESS ,NB_ACCESS*1000000LU/elapsed, NB_ACCESS*granularity*1000000LU/elapsed/1024/1024, granularity);
    }
*/

    for(int num_threads = 2; num_threads <= 20; num_threads += 2){
         for( granularity = 64; granularity <= 4096; granularity *= 2){
             declare_timer;
             start_timer{
                     pthread_t threads[num_threads];
                     for(int i = 0; i < num_threads; i++){
                         pthread_create(&threads[i],NULL,func,(void*)i);
                     }

                     for(int i = 0; i < num_threads; i++){
                         pthread_join(threads[i],NULL);
                     }
            }stop_timer("%d threads doing %ld memcpy of %ld bytes (%f MB/s)", num_threads, num_threads*NB_ACCESS, granularity, bandwith(num_threads*NB_ACCESS*granularity, elapsed));
         }
     }

}
