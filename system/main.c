
#include <xinu.h>

process test1(ppid32 pipe) { 
  pjoin(pipe); 
  char * message = "test1"; 
  pwrite(pipe, message, 5); 
  return OK;
}

process test2(ppid32 pipe) {
  pjoin(pipe);
  char b[6]; 
  memset(&b , 0, 6);
  pread(pipe, &b, 5); 
  if (!strcmp(b, "test2")) {
    kprintf("test2 passed\n");
  } else 
     kprintf("test2 failed\n");
  return OK;
}


process test3(ppid32 pipe) {
  kprintf("test 3 pid is %d\n", currpid );  
 pjoin(pipe); 
  char b[7]; 
  memset(&b, 0, 7); 
  pread(pipe, &b, 6); 
  kprintf("returns from first read\n"); 
  char c[11];
  memset(&c, 0, 11); 
  pread(pipe, &c, 10);

  char d[10];
  memset(&d, 0, 10); 
  pread(pipe, &d, 9);

  if ( (strcmp(b, "tst31t") == 0) && (strcmp(c, "st32tst33t") == 0) 
    && (strcmp(d, "st34tst35") == 0)) 
    kprintf("test 3 passed\n"); 
  else 
    kprintf("test 3 failed\n"); 


}

process main(void) {
  //kprintf("Main Process %d\n" , currpid); 
  ppid32 pipe = popen("r"); 
  resume( create(test1, 8192, 20,"p1", 1, pipe));

  char b[6];
  memset(&b , 0, 6);
  pread(pipe, &b, 5);
  if (!strcmp(b, "test1")) {
    kprintf("test1 passed\n");
  } 
  else 
    kprintf("test1 failed\n");
  pclose(pipe); 


  ppid32 pipe2 = popen("w"); 
  resume( create(test2, 8192, 20,"p2", 1, pipe2));
  char * message = "test2"; 
  pwrite(pipe2, message, 5); 

  pipe = popen("w"); 
  resume( create(test3, 8192, 20,"p3", 1, pipe));
  char * test3 = "tst31tst32tst33tst34tst35"; 

  pwrite(pipe, test3, 25);

  return OK;
}
