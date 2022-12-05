//Project: optical mouse camera raw data read
//Homepage: www.HomoFaciens.de
//Author Norbert Heinz
//Version 0.1
//Creation date 05.06.2016

#include <stdio.h>
#include <termios.h>
#include <sys/ioctl.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <sys/time.h>
#include <dirent.h>
#include <math.h>
#include <unistd.h>
#include <fcntl.h>



#define BAUDRATE B115200
//#define ARDUINOPORT "/dev/ttyUSB0"
#define ARDUINOPORT "/dev/ttyACM0"
#define FALSE 0
#define TRUE 1


#define PI 3.1415927


int fd = 0;
struct termios TermOpt;
char PicturePath[1000];




//+++++++++++++++++++++++ Start readport ++++++++++++++++++++++++++
char readport(void){
  int n;
  char buff;
  
  n=0;
  while(n < 1){
    n = read(fd, &buff, 1);
  }
//  if(n > 0){
    return buff;
//  }
//  return 0;
}
//------------------------ End readport ----------------------------------

//+++++++++++++++++++++++ Start sendport ++++++++++++++++++++++++++
void sendport(unsigned char ValueToSend){
  int n;

  n = write(fd, &ValueToSend, 1);

  if (n < 1){
    //while(readport() != 'r');
    //usleep(AdvanceRateArduino * 1.1);
    //n = write(fd, &ValueToSend, 1);
    //if (n < 1){
      printf("write() of value failed!\n");
    //}
  }
//  else{
//    while(readport() != 'r');
//    while(readport() > 0);//flush read buffer
//  }

}
//------------------------ End sendport ----------------------------------

//+++++++++++++++++++++++ Start openport ++++++++++++++++++++++++++
void openport(void){

    fd = open(ARDUINOPORT, O_RDWR | O_NOCTTY | O_NDELAY);
    if (fd == -1)  {
      printf("init_serialport: Unable to open port \n");
    }

    if (tcgetattr(fd, &TermOpt) < 0) {
      printf("init_serialport: Couldn't get term attributes\n");
    }
    speed_t brate = BAUDRATE; // let you override switch below if needed

    cfsetispeed(&TermOpt, brate);
    cfsetospeed(&TermOpt, brate);

    // 8N1
    TermOpt.c_cflag &= ~PARENB;
    TermOpt.c_cflag &= ~CSTOPB;
    TermOpt.c_cflag &= ~CSIZE;
    TermOpt.c_cflag |= CS8;
    // no flow control
    TermOpt.c_cflag &= ~CRTSCTS;

    TermOpt.c_cflag |= CREAD | CLOCAL;  // turn on READ & ignore ctrl lines
    TermOpt.c_iflag &= ~(IXON | IXOFF | IXANY); // turn off s/w flow ctrl

    TermOpt.c_lflag &= ~(ICANON | ECHO | ECHOE | ISIG); // make raw
    TermOpt.c_oflag &= ~OPOST; // make raw

    // see: http://unixwiz.net/techtips/termios-vmin-vtime.html
    TermOpt.c_cc[VMIN]  = 0;
    TermOpt.c_cc[VTIME] = 20;

    if( tcsetattr(fd, TCSANOW, &TermOpt) < 0) {
      printf("init_serialport: Couldn't set term attributes\n");
    }

}
//------------------------ End openport ----------------------------------




//######################################################################
//################## Main ##############################################
//######################################################################

int main(int argc, char **argv){

  char FileName[1000] = "";
  char FullFileName[1000] = "";
  int i;
  char a;  
  int picNumber = 0;
  FILE *FileOut, *FileIn;
  
  char imageFileBuffer[2000000];
  long imageFileSize = 0;
  
  openport();

  printf("\n\nWaiting for 'X' from Arduino (Arduino pluged in?)...\n");

  //Wait for 'X' from Arduino
  while(readport() != 'X');

  printf("...connected.\n");
  
  sendport('r');
 

  getcwd(PicturePath, 1000);
  strcat(PicturePath, "/pictures/");
  printf("PicturePath=>%s<\n", PicturePath);
  sprintf(FileName, "%06d.jpg", picNumber);
  strcpy(FullFileName, PicturePath);
  strcat(FullFileName, FileName);

  if((FileIn=fopen("empty.bmp","rb"))==NULL){
	printf("Can't open file '%s'!\n", FileName);
	return(1);
  }
  while(!feof(FileIn) && imageFileSize < 2000000){
	fread(&a, 1, 1, FileIn);
	imageFileBuffer[imageFileSize] = a;
	imageFileSize++;
  }
  fclose(FileIn);
  printf("imageFileSize = %ld\n", imageFileSize);
  
  while(1==1){
    picNumber++;
    sprintf(FileName, "%06d.bmp", picNumber);
    strcpy(FullFileName, PicturePath);
    strcat(FullFileName, FileName);
	if((FileOut=fopen(FullFileName,"wb"))==NULL){
	  printf("Can't open file '%s'!\n", FileName);
	  return(1);
	}
	

	for(i=0; i<256; i++){
	  a = readport();
	  imageFileBuffer[imageFileSize - 256 -1 + i] = a;
	  //printf("%d, ", a);
    }
    
    fwrite(&imageFileBuffer, imageFileSize, 1, FileOut);
	fclose(FileOut);
	printf("%s written.\n", FullFileName);
  }

  return 0;
}
