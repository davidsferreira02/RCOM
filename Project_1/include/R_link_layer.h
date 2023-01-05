#ifndef RLIGACAODADOS
#define RLIGACAODADOS

#define FLAG_RCV 0x7E
#define A_RCV 0x03

/*Tramas: Campo C*/
/*Tramas de supervisão*/
#define SET 0x03
#define DISC 0x0B
#define UA 0x07
#define RR0 0x05
#define RR1 0x85
#define REJ0 0x01
#define REJ1 0x81
/*Tramas de informação*/
#define C0 0x00
#define C1 0x40

/*Campo de proteção
#define BCC1
#define BCC2
*/

/*Transparência*/
#define SETEE 0x7E
#define SETED 0x7D
#define CINCOE 0x5E
#define CINCOD 0x5D

int llopen(int fd);
int verificarTramaS(int fd, unsigned char comando);
void TramaSupervisor(unsigned char* trama, unsigned char comando);
int llclose(int fd);
int llread(int fd, unsigned char* buffer);
unsigned char* destuffing(unsigned char* tramaI, int *sizeTrama);
unsigned char* lerTrama(int fd);
int BCC2(unsigned char* pacote, int sizePacote, unsigned char bcc);
unsigned char* Headers(unsigned char* tramaI, int sizeTramaI, int *sizePacote);

#endif
