// Final example with condition variables
// Compile with: gcc -O2 -Wall -pthread cycle_buffer.c -o cbuffer

#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>

#define N 1000000
#define CUTOFF 150
#define MESSAGES 10000

// ---- globals ----

// global integer buffer
int global_buffer;

// global avail messages count (0 or 1)
int global_availmsg = 0;	// empty

// conditional variable, signals a put operation (receiver waits on this)
pthread_cond_t msg_in = PTHREAD_COND_INITIALIZER;
// conditional variable, signals a get operation (sender waits on this)
pthread_cond_t msg_out = PTHREAD_COND_INITIALIZER;

// mutex protecting common resources
pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;



typedef struct {
	
	int begin;
	int end;
	int sorted;
	int finish;
	double * a;
}Content;

typedef struct {
	Content array[MESSAGES];
	int qin, qout;
	int count;

}Circus_Buffer;

void send_msg(Content msg);
Content recv_msg();
void *worker_thread(void *args);
void inssort(double *a,int n);
int partition(double *a,int n);

Circus_Buffer circus_buffer;

// gcc -O2 -Wall -pthread cycle_bufferPanos.c -o test


int main() {
  	double *a;
  	int i;

	a = (double *)malloc(N*sizeof(double));
	if (a==NULL) {
		printf("error in malloc\n");
		exit(1);
	}

  	// fill array with random numbers
  	srand(time(NULL));
	for (i=0;i<N;i++) {
    	a[i] = (double)rand()/RAND_MAX;
  	}

  	// arxikopoioume ta buffer
	circus_buffer.qin = 0;
  	circus_buffer.qout = 0;
  	circus_buffer.count = 0;
  	
  	Content c;
  	c.begin = 0;
	c.end = N;
	c.sorted = 0;
	c.finish = 0;
	c.a = a;
	send_msg(c);
  	
	pthread_t t1, t2, t3, t4;
	pthread_create(&t1,NULL,worker_thread,NULL);
	pthread_create(&t2,NULL,worker_thread,NULL);
	pthread_create(&t3,NULL,worker_thread,NULL);
	pthread_create(&t4,NULL,worker_thread,NULL);

	int numbers = 0;
	for(;;){
		if(numbers == N) break;
		c = recv_msg();
		if(c.sorted == 1){
			numbers = numbers + c.end - c.begin;

		}
		else{
			send_msg(c);
		}
	}

	c.finish = 1;
	send_msg(c);
	send_msg(c);
	send_msg(c);
	send_msg(c);

	pthread_join(t1,NULL);
	pthread_join(t2,NULL);
	pthread_join(t3,NULL);
	pthread_join(t4,NULL);

	// destroy mutex - should be unlocked
	pthread_mutex_destroy(&mutex);

	// destroy cvs - no process should be waiting on these
	pthread_cond_destroy(&msg_out);
	pthread_cond_destroy(&msg_in);
	 // check sorting
  	for (i=0;i<(N-1);i++) {
    	if (a[i]>a[i+1]) {
      		printf("Sort failed!\n");
      		break;
    	}	
  	}  
	free(a);
	return 0;
}






// ---- send/receive functions ----

void send_msg(Content msg) {

    pthread_mutex_lock(&mutex);
    while (circus_buffer.count == MESSAGES) { // NOTE: we use while instead of if! more than one thread may wake up
    				// cf. 'mesa' vs 'hoare' semantics
      pthread_cond_wait(&msg_out,&mutex);  // wait until a msg is received - NOTE: mutex MUST be locked here.
      					   // If thread is going to wait, mutex is unlocked automatically.
      					   // When we wake up, mutex will be locked by us again. 
    }
    
    // send message
    circus_buffer.array[circus_buffer.qin] = msg;
    circus_buffer.qin++;
    if(circus_buffer.qin == MESSAGES)
    	circus_buffer.qin = 0;
    circus_buffer.count++;
    
    // signal the receiver that something was put in buffer
    pthread_cond_signal(&msg_in);
    
    pthread_mutex_unlock(&mutex);

}


Content recv_msg() {

    // lock mutex
    pthread_mutex_lock(&mutex);
    while (circus_buffer.count == 0) {	// NOTE: we use while instead of if! see above in producer code
    
      pthread_cond_wait(&msg_in,&mutex);  
    
    }
    
    // receive message
    Content c = circus_buffer.array[circus_buffer.qout];
    circus_buffer.qout++;
    if(circus_buffer.qout == MESSAGES)
    	circus_buffer.qout = 0;
    circus_buffer.count--;
    
    // signal the sender that something was removed from buffer
    pthread_cond_signal(&msg_out);
    
    pthread_mutex_unlock(&mutex);

    return(c);
}



// producer thread function
void *worker_thread(void *args) {
	int pivot;
	Content c1, c2;
	for(;;){
		Content c = recv_msg();
		if(c.finish == 1){
			pthread_exit(NULL);
		}
		else if(c.sorted == 1){// edw to c.sorted einai 1
			send_msg(c);

		}
		else if((c.end - c.begin) <= CUTOFF){
			inssort(c.a + c.begin, c.end - c.begin);
			c.sorted = 1;
			send_msg(c);

		}
		else if(c.sorted == 0){
			pivot = partition(c.a+c.begin, c.end - c.begin);
			c1.begin = c.begin;
			c1.end = c.begin + pivot;
			c1.a = c.a;
			c1.finish = 0;
			c1.sorted = 0;
			c2.begin = c.begin + pivot;
			c2.end = c.end;
			c2.a = c.a;
			c2.finish = 0;
			c2.sorted = 0;
			send_msg(c1);
			send_msg(c2);
		}



		
	}
 
  pthread_exit(NULL); 
}

void inssort(double *a,int n) {
int i,j;
double t;
  
  for (i=1;i<n;i++) {
    j = i;
    while ((j>0) && (a[j-1]>a[j])) {
      t = a[j-1];  a[j-1] = a[j];  a[j] = t;
      j--;
    }
  }

}
int partition(double *a,int n) {
int first,last,middle;
double t,p;
int i,j;

  // take first, last and middle positions
  first = 0;
  middle = n/2;
  last = n-1;  
  
  // put median-of-3 in the middle
  if (a[middle]<a[first]) { t = a[middle]; a[middle] = a[first]; a[first] = t; }
  if (a[last]<a[middle]) { t = a[last]; a[last] = a[middle]; a[middle] = t; }
  if (a[middle]<a[first]) { t = a[middle]; a[middle] = a[first]; a[first] = t; }
    
  // partition (first and last are already in correct half)
  p = a[middle]; // pivot
  for (i=1,j=n-2;;i++,j--) {
    while (a[i]<p) i++;
    while (p<a[j]) j--;
    if (i>=j) break;

    t = a[i]; a[i] = a[j]; a[j] = t;      
  }
  
  // return position of pivot
  return i;
}


  







