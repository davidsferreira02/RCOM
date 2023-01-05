#include "../include/E_link_layer.h"


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>

#define FALSE 0
#define TRUE 1

/*flags para o alarme*/
int flag_alarme=0, conta_alarme=0;

/*flags para controlar o que é lido*/
volatile int STOP=FALSE;
int res;
unsigned char buf[256];

/*flags para controlar os estados em tramas supervisão*/
int stateInicio, stateFim, stateMedio;

/*bits de controlo em envio*/
int Ns=0;

void alarme(){
  printf("alarme #%d\n", conta_alarme+1);
  flag_alarme = 1;
  conta_alarme++;
}

int llopen(int fd){
  (void) signal(SIGALRM, alarme);


    unsigned char set[5];
    TramaSupervisor(set, SET);

    conta_alarme=0;

    while (conta_alarme<3) {
      fflush(stdout);
      res=write(fd,set,5);

      flag_alarme=0;
      alarm(3);

      if(verificarTramaS(fd,UA)==1) return 1;
    }

    return -1;
}

void TramaSupervisor(unsigned char* trama, unsigned char comando){
  trama[0]=F;
  trama[1]=A;
  trama[2]=comando;
  trama[3]=(A^comando);
  trama[4]=F;
}

unsigned char* lerTrama(int fd){
  unsigned char* rec = (unsigned char*)malloc(5);
  int i=0;
  STOP=FALSE;
  stateInicio=1, stateFim=0, stateMedio=0;

  while (STOP==FALSE) {
    res=read(fd,buf,1); //leitura byte a byte
    if(res>0){
      if(buf[0]==F && stateInicio){ //inicio da leitura
        rec[i]=buf[0];
        stateInicio=0;
        stateMedio=1;
        i++;
      }
      else if(buf[0]==F && stateMedio){ //fim da leitura
        rec[i]=buf[0];
        stateMedio=0;
        stateFim=1;
        i++;
      }
      else if(stateMedio){ //estado no meio
        rec[i]=buf[0];
        i++;
      }
    }
    else if(stateFim || flag_alarme){
      STOP=TRUE;
    }
  }
  fflush(stdout); //buffer fica limpo
  return rec;
}

int verificarTramaS(int fd, unsigned char comando){ //verifica o seu bbc1 e se a trama esta correta
  unsigned char* rec=lerTrama(fd);
  alarm(0);
  if(rec[3]==(rec[1]^rec[2]) && rec[2]==comando) return 1;
  else return -1;
}

int llclose(int fd){
  unsigned char disc[5];
  TramaSupervisor(disc, DISC);
  unsigned char ua[5];
  TramaSupervisor(ua,UA);

  conta_alarme=0;

  while (conta_alarme<3) {
    flag_alarme=0;
    alarm(3);
    fflush(stdout);
    res=write(fd,disc,5);


    if(verificarTramaS(fd,DISC)==1) {
      fflush(stdout);
      write(fd,ua,5);
      return 1;
    }
  }

  return -1;
}

unsigned char BCC2(unsigned char* package, int sizePackage){ //bbc2 campo de proteçao de dados
  unsigned char r = package[0];
  for(int i=1; i<sizePackage; i++){
    r=r^package[i];
  }
  return r;
}

unsigned char* TramaI(unsigned char* package, int sizeP, int *sizeI){
  unsigned char* trama;
  int i=0;
  trama=(unsigned char*)malloc(sizeP+6);
  trama[i++]=F;
  trama[i++]=A;

  switch (Ns) {
    case 0:
      trama[i++]=C0;
      break;
    case 1:
      trama[i++]=C1; //64
      break;
    default:
      printf("Erro no bit\n");
      exit(-1);
      break;
  }

  trama[i++] = (A^trama[2]);
  memcpy(trama+i, package, sizeP);
  i = i+sizeP;
  trama[i++] = BCC2(package, sizeP);
  trama[i++] = F;

  *sizeI = i;

  return trama;
}

int llwrite(int fd, unsigned char* package, int sizePackage){
  if(conta_alarme==3) return -1;
  int length;
  unsigned char* trama = TramaI(package, sizePackage, &length);

  conta_alarme=0;
  int reject=0,sizeTramaInformacao;

  unsigned char* tramaI = stuffing(trama, length, &sizeTramaInformacao);

  while(conta_alarme<3 || reject){
    //printf("%d conta_alarme || %d rejeitar\n", conta_alarme, rejeitar);
    if(conta_alarme==3) return -1;
    fflush(stdout);
    int res1=write(fd,tramaI,sizeTramaInformacao);
    //printf("%d envidos\n",res1);
    flag_alarme=0;
    alarm(3);

    switch (Ns) {
      case 0:
        if(verificarTramaS(fd,RR1)==1){
          Ns=1;
          return res1;
        }
        else if(verificarTramaS(fd,REJ0)==1){
          printf("REJ0 recebido\n");
          reject=1;
        }
        break;
      case 1:
        if(verificarTramaS(fd,RR0)==1){
          Ns=0;
          return res1;
        }
        else if(verificarTramaS(fd,REJ1)==1){
          printf("REJ1 recebido\n");
          reject=1;
        }
        break;
      default:
        printf("Erro\n");
        exit(-1);
        break;
    }
  }
  return -1;
}

unsigned char* stuffing(unsigned char* trama, int length, int *sizeTramaI){
  unsigned char* tramaStuff;
  tramaStuff = (unsigned char*) malloc(2*(length+1)+5);
  int i=0;

  tramaStuff[i++]=F;
  tramaStuff[i++]=A;

  switch (Ns) {
    case 0:
      tramaStuff[i++]=C0;
      break;
    case 1:
      tramaStuff[i++]=C1;
      break;
    default:
      printf("Erro no bit\n");
      exit(-1);
      break;
  }
  tramaStuff[i++] = (A^tramaStuff[2]);

  for(int j=0; j<length; j++){
    if(trama[j]==SETEE){
      tramaStuff[i++]=SETED;
      tramaStuff[i++]=CINCOE;
    }
    else if(trama[j]==SETED){
      tramaStuff[i++]=SETED;
      tramaStuff[i++]=CINCOD;
    }
    else{
      tramaStuff[i++]=trama[j];
    }
  }

  tramaStuff[i++] = F;
  *sizeTramaI = i;
  return tramaStuff;
}
