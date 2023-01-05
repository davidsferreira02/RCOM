#include "../include/E_Application_layer.h"
#include "../include/E_link_layer.h"



#define M 0xFF //255 bites

#define BAUDRATE B38400


 struct termios oldtio;
 struct termios newtio;

unsigned char N = 0x00; //numero sequencia do pactote de dados 255

/*Variáveis para os ficheiros*/
unsigned char* nameFile; int sizefile;
int sizeNameFile;
unsigned char* mensage_content;

void ficheiro(char *file){
  sizeNameFile = strlen(file);
  nameFile = (unsigned char*) malloc(sizeNameFile);
  nameFile = (unsigned char*) file;

  struct stat sfile; // struct informaçoes sobre o file

  if (stat(file, &sfile) == -1) { //se informaçao sobre o file nao passou da erro
    perror("stat");
    exit(EXIT_FAILURE);
  }

  sizefile = sfile.st_size;
  int tamanho = sizefile;

    mensage_content  = (unsigned char*) malloc(tamanho);

  FILE *file_Emissor;
  file_Emissor = fopen(file, "rb");

  if(file_Emissor==NULL){
    printf("File not found existe\n");
    exit(-1);
  }

  printf("%ld tamanho do ficheiro\n", (long int)sizefile);

  fread(mensage_content, sizefile+1, sizeof(mensage_content), file_Emissor);

}

void call_llopen(int fd){
  if ( tcgetattr(fd,&oldtio) == -1) { /* save current port settings */
      perror("tcgetattr");
      exit(-1);
    }

  bzero(&newtio, sizeof(newtio));
  newtio.c_cflag = BAUDRATE | CS8 | CLOCAL | CREAD;
  newtio.c_iflag = IGNPAR;
  newtio.c_oflag = 0;
  newtio.c_lflag = 0;
  newtio.c_cc[VTIME] = 1;
  newtio.c_cc[VMIN] = 0;

  tcflush(fd, TCIOFLUSH);
  if ( tcsetattr(fd,TCSANOW,&newtio) == -1) {
    perror("tcsetattr");
    exit(-1);
  }
  printf("New termios structure set\n");


  if(llopen(fd)==1) printf("Ligação estabelecidda com sucesso\n");
  else{
    printf("Ligação falhada\n");
    sleep(1);
    if ( tcsetattr(fd,TCSANOW,&oldtio) == -1) {
      perror("tcsetattr");
      exit(-1);
    }
    close(fd);
    exit(-1);
  }
}

void call_llclose(int fd){
  if(llclose(fd)==1){
   printf("Terminada com sucesso\n");
   sleep(1);
   if ( tcsetattr(fd,TCSANOW,&oldtio) == -1) {
     perror("tcsetattr");
     exit(-1);
   }
   close(fd);
  }
  else{
    printf("Terminada sem sucesso\n");
    sleep(1);
    if ( tcsetattr(fd,TCSANOW,&oldtio) == -1) {
      perror("tcsetattr");
      exit(-1);
    }
    close(fd);
    exit(-1);
  }
}

int blocos(int size, int *resto){ // Quantos blocos faltam a ocupar maximo de um bloco 255 bites
  long int tamanho = (long int) size;
  int i=1;
  while(tamanho-255 > 0){
    i++;
    tamanho-=255;
  }
  *resto=tamanho;
  return i;
}


int V1(unsigned char* pacote, int start, int l1, int rest){ //V1 numero de octetos indicado em L
  int i;
  for(i=start; i<(start+l1-1); i++){
    pacote[i]=M;
  }
  pacote[i++]=(unsigned char)rest;
  return i;
}

unsigned char* PacoteControlo(unsigned char controlo, int *sizePackage){ //guarda o tamanho do ficheiro
  unsigned char* package;
  int i=0, resto;
  int l1 = blocos(sizefile,&resto); //tamanho de octetos

  package = (unsigned char*)malloc(5+l1+sizeNameFile);

  package[i++]=controlo;
  package[i++]=T1;
  package[i++]=(unsigned char)l1;
  i = V1(package,i,l1,resto);
  package[i++]=T2;
  package[i++]=(unsigned char)sizeNameFile;
  memcpy(package+i, nameFile, sizeNameFile);
  i=i+sizeNameFile;

  *sizePackage=i;
  return package;
}

int PacoteEnviar(int *rest){ //numero de pacotes a enviar
  long int  tamanho= (long int) sizefile;
  int i=1;
  while(tamanho-256 > 0){
    i++;
    tamanho-=256;
  }
  *rest = tamanho;
  return i;
}

void emissor(int fd){
  unsigned char* package;
  int sizePackage;

  package =PacoteControlo(START, &sizePackage);



  int res=llwrite(fd,package,sizePackage);
  if(res>0){ //START
    printf("Trama START enviada com sucesso, com %d bits\n", res);
  }
  else{
    printf("Trama START enviada sem sucesso\n");
    sleep(1);
    if ( tcsetattr(fd,TCSANOW,&oldtio) == -1) {
      perror("tcsetattr");
      exit(-1);
    }
    close(fd);
    exit(-1);
  }

  int pacoteEnviar, l1;
  pacoteEnviar = PacoteEnviar(&l1);

  int start=0, end=256;
  while(pacoteEnviar>0){
    if(pacoteEnviar==1){
      package= PacoteDados(mensage_content, start, (long int)sizefile, 0x00, (unsigned char)l1, &sizePackage);
      res=llwrite(fd,package,sizePackage);
      if(res>0){
        printf("Trama I %d enviada com sucesso, com %d bits\n",(int)package[1], res);
      }
      else{
        printf("Trama I recebida sem sucesso\n");
        sleep(1);
        if ( tcsetattr(fd,TCSANOW,&oldtio) == -1) {
          perror("tcsetattr");
          exit(-1);
        }
        close(fd);
        exit(-1);
      }
    }
    else{
      package= PacoteDados(mensage_content, start, end, 0x01, 0x00, &sizePackage);
      res=llwrite(fd,package,sizePackage);
      if(res>0){
        printf("Trama %d enviada com sucesso, com %d bits\n",(int)package[1] ,res);
      }
      else{
        printf("Trama  recebida sem sucesso\n");
        sleep(1);
    if ( tcsetattr(fd,TCSANOW,&oldtio) == -1) {
      perror("tcsetattr");
      exit(-1);
    }
    close(fd);
    exit(-1);
      }
      start=end;
      end+=256;
    }
    pacoteEnviar--;
  }

  package = PacoteControlo(END, &sizePackage);
  res=llwrite(fd,package,sizePackage);
  if(res>0){ //START
    printf("Trama END enviada com sucesso, com %d bits\n", res);
  }
  else{
    printf("Trama END enviada sem sucesso\n");
    sleep(1);
    if ( tcsetattr(fd,TCSANOW,&oldtio) == -1) {
      perror("tcsetattr");
      exit(-1);
    }
    close(fd);
    exit(-1);
  }
}

unsigned char* PacoteDados(unsigned char* mensagem, int start,long int end, unsigned char l2, unsigned char l1, int *sizePacote){
  unsigned char* pacote;
  int k = (256 * (int)l2) + (int)l1;
  pacote = (unsigned char*) malloc (4+k);

  int i =0;
  pacote[i++]=DADOS;
  pacote[i++]=N;
  pacote[i++]=l2;
  pacote[i++]=l1;

  for(int j=start; j<end; j++){
    pacote[i++]=mensagem[j];
  }

  *sizePacote=i;

  N+=0x01;
  return pacote;
}
