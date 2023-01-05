#include "../include/E_Application_layer.h"



int fd;

int main(int argc, char *argv[]) {

  if ( (argc < 2) ||
  	     ((strcmp("/dev/ttyS10", argv[1])!=0) &&
  	      (strcmp("/dev/ttyS11", argv[1])!=0) )) {
      printf("Usage:SerialPort,ex:/dev/ttyS10\n");
      exit(-1);
  }

   /* if ( (argc < 2) ||
         ((strcmp("/dev/ttyS4", argv[1])!=0) &&
          (strcmp("/dev/ttyS0", argv[1])!=0) )) {
        printf("Usage:SerialPort,ex:/dev/ttyS4\n");
        exit(-1);
    }*/
  else if (argc < 3){
    printf("Numero de argumentos errado\n");
    exit(-1);
  }

  fd = open(argv[1], O_RDWR | O_NOCTTY );
  if (fd <0) {perror(argv[1]); exit(-1); }

  call_llopen(fd);

  ficheiro(argv[2]);

  emissor(fd);

  call_llclose(fd);

  return 0;
}
