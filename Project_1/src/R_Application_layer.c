#include "../include/R_Application_layer.h"
#include "../include/R_link_layer.h"

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <termios.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define BAUDRATE B38400
#define _POSIX_SOURCE 1
#define FALSE 0
#define TRUE 1

#define MAXSIZE 1024

/*Mensagem recebida*/
unsigned char* mensage;
int bytes=0; //bytes do ficheiro
int packagesReceive=0;//pacotes que receberam

/*Informação recebida do ficheiro*/
char *nameFile;
long int sizeFile;

int nFlag=0;

struct termios oldtio;
struct termios newtio;

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
  if(llclose(fd)==1) {
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

void pacotesReceber(){ // numero de pacotes a receber
  long int tmp = (long int) sizeFile;
  int r=1;
  while(tmp-256 > 0){
    r++;
    tmp-=256;
  }
  packagesReceive=r;
  return;
}

void recetor(int fd,char *file){
  unsigned char* pacote = (unsigned char*) malloc (MAXSIZE);
  int sizePacote = llread(fd, pacote);
  /*Receção da trama Start*/
    TipoPacote(pacote, sizePacote);

  bytes=0;
  pacotesReceber();

  printf("%d pacotes a receber\n", packagesReceive);
  while(packagesReceive>0){
    //printf("%d pacotes em falta\n",packagesReceive);
    free(pacote);
    pacote = (unsigned char*) malloc (MAXSIZE);
    sizePacote = llread(fd, pacote);
    //printf("%d -> sizePacote\n", sizePacote);
    if (sizePacote>0){
        if (TipoPacote(pacote, sizePacote)) packagesReceive--;
    }
    else{
        //printf("Entrei aqui no 0\n");
        continue;
    }
  }
  printf("%d bytes do ficheiro\n", bytes);
  ficheiro(file);

  free(pacote);
  pacote = (unsigned char*) malloc (MAXSIZE);
  sizePacote = llread(fd, pacote);
  /*Receção trama END*/
  TipoPacote(pacote, sizePacote);
}

int TipoPacote(unsigned char* pacote, int sizePacote){ //verifica se é Start END DADOS
  unsigned char c = pacote[0];
  switch (c) {
    case START:
      pacoteStart(pacote, sizePacote);
      printf("Trama START bem recebida\n");
      return 1;
      break;
    case END:
      printf("Trama END bem recebida\n");
      return 1;
      break;
    case DADOS:
      if(nFlag==(int)pacote[1]){
        nFlag++;
        if(nFlag==256){nFlag=0;}
        pacoteDados(pacote,sizePacote);
        printf("Trama I bem recebida com %d bytes, pacote %d\n", sizePacote, (int)pacote[1]);
        return 1;}
    break;
  }
    return 0;
}

void pacoteStart(unsigned char* pacote, int sizePacote){ // se o pacote tiver o comando Start
  int tamanho = 0x00;
  int index=0;
  int packages;
  if(pacote[1]==T1){ //Tamaho do ficheiro
    packages = (int)pacote[2];
    for(int i=3; i<3+packages; i++){
      tamanho+=pacote[i];
    }
    sizeFile = (long int) tamanho;
    index=3+packages;
  }

  int lengthName, j=0 ;
  if(pacote[index++]==T2){//Nome do ficheiro;
    lengthName = (int)pacote[index++];
    nameFile= (char*)malloc(lengthName);
    for(int i=index; i<index+lengthName; i++){
      nameFile[j++]=(char)pacote[i];
    }
  }

  printf("tamanho ficheiro: %ld\n", sizeFile);
  printf("nome do ficheiro: %s\n", nameFile);

  mensage=(unsigned char*)malloc(sizeFile);

}

void pacoteDados(unsigned char* pacote, int sizePacote){
  int end = (256 * (int)pacote[2]) + (int)pacote[3];
  int j=4;

  for(int i=bytes; i<bytes+end; i++){
    mensage[i]=pacote[j++];
  }
  bytes = bytes + end;
  return;
}

void ficheiro(char *file){
  FILE *fp;
  fp=fopen(file,"wb");

  if(fp==NULL){
    printf("Erro na criação de ficheiro\n");
    exit(-1);
  }
  fwrite(mensage, bytes, sizeof(mensage),fp);

}
