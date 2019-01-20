#include "gpiolib_addr.h"
#include "gpiolib_reg.h"

#include <stdint.h>
#include <stdio.h>		//for the printf() function
#include <fcntl.h>
#include <linux/watchdog.h> 	//needed for the watchdog specific constants
#include <unistd.h> 		//needed for sleep
#include <sys/ioctl.h> 		//needed for the ioctl function
#include <stdlib.h> 		//for atoi
#include <time.h> 		//for time_t and the time() function
#include <sys/time.h>           //for gettimeofday()
#include <signal.h>
#include <errno.h>

//Below is a macro that had been defined to output appropriate logging messages
//You can think of it as being similar to a function
//file        - will be the file pointer to the log file
//time        - will be the current time at which the message is being printed
//programName - will be the name of the program, in this case it will be Lab4Sample
//str         - will be a string that contains the message that will be printed to the file.

#define PRINT_MSG(file, time, programName, str) \ //Prints any messages to the log file
	do{ \
			fprintf(logFile, "%s : %s : %s", time, programName, str); \
			fflush(logFile); \
	}while(0)

#define PRINT_LASERSTATS(file, time, programName, str) \ //Prints when either laser has been broken to the stats file
	do{ \
			fprintf(statsFile, "%s : %s : %d%s", time, programName, laserCount, str); \
			fflush(logFile); \
	}while(0)

#define PRINT_PERSONSTATS(file, time, programName, str) \ //Prints when someone has either entered or left the room to the stats file
	do{ \
			fprintf(statsFile, "%s : %s : %s%d%s", time, programName, str, personCount, newLn); \
			fflush(logFile); \
	}while(0)


//In the macro, we use the fflush() function to make sure that the messages get printed
//to the file. If we did not use fflush() then it is possible some messages would not
//get printed if the program ended unexpectedly.

//Most of the following functions are the same as the ones from the Lab 3
//sample code.

//This function will initialize the GPIO pins and handle any error checking
//for the initialization
GPIO_Handle initializeGPIO()
{
	//This is the same initialization that was done in Lab 2
	GPIO_Handle gpio;
	gpio = gpiolib_init_gpio();
	if(gpio == NULL)
	{
		perror("Could not initialize GPIO");
	}
	return gpio;
}

//This function will change the appropriate pins value in the select register
//so that the pin can function as an output
void setToOutput(GPIO_Handle gpio, int pinNumber)
{
	//Check that the gpio is functional
	if(gpio == NULL)
	{
		printf("The GPIO has not been intitialized properly \n");
		return;
	}

	//Check that we are trying to set a valid pin number
	if(pinNumber < 2 || pinNumber > 27)
	{
		printf("Not a valid pinNumer \n");
		return;
	}

	//This will create a variable that has the appropriate select
	//register number. For more information about the registers
	//look up BCM 2835.
	int registerNum = pinNumber / 10;

	//This will create a variable that is the appropriate amount that 
	//the 1 will need to be shifted by to set the pin to be an output
	int bitShift = (pinNumber % 10) * 3;

	//This is the same code that was used in Lab 2, except that it uses
	//variables for the register number and the bit shift
	uint32_t sel_reg = gpiolib_read_reg(gpio, GPFSEL(registerNum));
	sel_reg |= 1  << bitShift;
	gpiolib_write_reg(gpio, GPFSEL(1), sel_reg);
}

//This function will make an output pin output 3.3V. It is the same
//as what was done in Lab 2 to make the pin output 3.3V
void outputOn(GPIO_Handle gpio, int pinNumber)
{
	gpiolib_write_reg(gpio, GPSET(0), 1 << pinNumber);
}

//This function will make an output pin turn off. It is the same
//as what was done in Lab 2 to make the pin turn off
void outputOff(GPIO_Handle gpio, int pinNumber)
{
	gpiolib_write_reg(gpio, GPCLR(0), 1 << pinNumber);
}

//This is a function used to read from the config file. It is not implemented very
//well, so when you create your own you should try to create a more effective version
void readConfig(FILE* configFile, int* timeout, char* logFileName, char* statsFileName)
{
	//Loop counter
	int i = 0;
	
	//A char array to act as a buffer for the file
	char buffer[255];

	//The value of the timeout variable is set to zero at the start
	*timeout = 0;

	//This is a variable used to track which input we are currently looking
	//for (timeout, logFileName or statsFileName)
	int input = 0;

	//This will 
	//fgets(buffer, 255, configFile);
	//This will check that the file can still be read from and if it can,
	//then the loop will check to see if the line may have any useful 
	//information.
	while(fgets(buffer, 255, configFile) != NULL)
	{
		i = 0;
		//If the starting character of the string is a '#', 
		//then we can ignore that line
		if(buffer[i] != '#')
		{
			while(buffer[i] != 0)
			{
				//This if will check the value of timeout
				if(buffer[i] == '=' && input == 0)
				{
					//The loop runs while the character is not null
					while(buffer[i] != 0)
					{
						//If the character is a number from 0 to 9
						if(buffer[i] >= '0' && buffer[i] <= '9')
						{
							//Move the previous digits up one position and add the
							//new digit
							*timeout = (*timeout *10) + (buffer[i] - '0');
						}
						i++;
					}
					input++;
				}
				else if(buffer[i] == '=' && input == 1) //This will find the name of the log file
				{
					int j = 0;
					//Loop runs while the character is not a newline or null
					while(buffer[i] != 0  && buffer[i] != '\n')
					{
						//If the characters after the equal sign are not spaces or
						//equal signs, then it will add that character to the string
						if(buffer[i] != ' ' && buffer[i] != '=')
						{
							logFileName[j] = buffer[i];
							j++;
						}
						i++;
					}
					//Add a null terminator at the end
					logFileName[j] = 0;
					input++;
				}
				else if(buffer[i] == '=' && input == 2) //This will find the name of the stats file
				{
					int k = 0;
					//Loop runs while the character is not a newline or null
					while(buffer[i] != 0  && buffer[i] != '\n')
					{
						//If the characters after the equal sign are not spaces or
						//equal signs, then it will add that character to the string
						if(buffer[i] != ' ' && buffer[i] != '=')
						{
							statsFileName[k] = buffer[i];
							j++;
						}
						i++;
					}
					//Add a null terminator at the end
					statsFileName[k] = 0;
					input++;
				}
				else
				{
					i++;
				}
			}
		}
	}
}


#define LASER1_PIN_NUM 4 //This will replace LASER1_PIN_NUM with 4 when compiled
#define LASER2_PIN_NUM 10 //Laser 1 is entry, laser 2 is exit
int laserDiodeStatus(GPIO_Handle gpio, int diodeNumber)
{
	if(gpio == NULL)
	{
		return -1;
	}

	if(diodeNumber == 1)
	{
		uint32_t level_reg = gpiolib_read_reg(gpio, GPLEV(0));

		if(level_reg & (1 << LASER1_PIN_NUM))
		{
			return 1;
		}
		else
		{
			return 0;
		}
	}
	else if(diodeNumber == 2)
	{
		uint32_t level_reg = gpiolib_read_reg(gpio, GPLEV(0));

		if(level_reg & (1 << LASER2_PIN_NUM))
		{
			return 1;
		}
		else
		{
			return 0;
		}
	}
	else
	{
		return -1;
	}
	
}


//This function will get the current time using the gettimeofday function
void getTime(char* buffer)
{
	//Create a timeval struct named tv
  	struct timeval tv;

	//Create a time_t variable named curtime
  	time_t curtime;


	//Get the current time and store it in the tv struct
  	gettimeofday(&tv, NULL); 

	//Set curtime to be equal to the number of seconds in tv
  	curtime=tv.tv_sec;

	//This will set buffer to be equal to a string that in
	//equivalent to the current date, in a month, day, year and
	//the current time in 24 hour notation.
  	strftime(buffer,30,"%m-%d-%Y  %T.",localtime(&curtime));

} 

int main(const int argc, const char* const argv[])
{
	//Create a string that contains the program name
	const char* argName = argv[0];

	//These variables will be used to count how long the name of the program is
	int i = 0;
	int namelength = 0;

	while(argName[i] != 0)
	{
		namelength++;
		i++;
	} 

	char programName[namelength];

	i = 0;

	//Copy the name of the program without the ./ at the start
	//of argv[0]
	while(argName[i + 2] != 0)
	{
		programName[i] = argName[i + 2];
		i++;
	} 	

	//Create a file pointer named configFile
	FILE* configFile;
	//Set configFile to point to the Lab4Sample.cfg file. It is
	//set to read the file.
	configFile = fopen("/home/pi/Lab4Sample.cfg", "r");

	//Output a warning message if the file cannot be openned
	if(!configFile)
	{
		perror("The config file could not be opened");
		return -1;
	}

	//Declare the variables that will be passed to the readConfig function
	int timeout;
	char logFileName[50];

	//Call the readConfig function to read from the config file
	readConfig(configFile, &timeout, logFileName, statsFileName);

	//Close the configFile now that we have finished reading from it
	fclose(configFile);

	//Create a new file pointer to point to the log file
	FILE* logFile;
	//Set it to point to the file from the config file and make it append to
	//the file when it writes to it.
	logFile = fopen(logFileName, "a");

	FILE* statsFile;

	statsFile = fopen(statsFileName, "a");

	//Check that the file opens properly.
	if(!configFile)
	{
		perror("The log file could not be opened");
		perror("The stats file coud not be opened");
		return -1;
	}

	//Create a char array that will be used to hold the time values
	char time[30];
	getTime(time);

	//Initialize the GPIO pins
	GPIO_Handle gpio = initializeGPIO();	
	
	//Get the current time
	getTime(time);
	
	//Log that the GPIO pins have been initialized
	PRINT_MSG(logFile, time, programName, "The GPIO pins have been initialized\n\n");


	
	//This variable will be used to access the /dev/watchdog file, similar to how
	//the GPIO_Handle works
	int watchdog;

	//We use the open function here to open the /dev/watchdog file. If it does
	//not open, then we output an error message. We do not use fopen() because we
	//do not want to create a file if it doesn't exist
	if ((watchdog = open("/dev/watchdog", O_RDWR | O_NOCTTY)) < 0) {
		printf("Error: Couldn't open watchdog device! %d\n", watchdog);
		return -1;
	} 
	//Get the current time
	getTime(time);
	//Log that the watchdog file has been opened
	PRINT_MSG(logFile, time, programName, "The Watchdog file has been opened\n\n");

	//This line uses the ioctl function to set the time limit of the watchdog
	//timer to 15 seconds. The time limit can not be set higher that 15 seconds
	//so please make a note of that when creating your own programs.
	//If we try to set it to any value greater than 15, then it will reject that
	//value and continue to use the previously set time limit
	ioctl(watchdog, WDIOC_SETTIMEOUT, &timeout);
	
	//Get the current time
	getTime(time);
	//Log that the Watchdog time limit has been set
	PRINT_MSG(logFile, time, programName, "The Watchdog time limit has been set\n\n");


	//The value of timeout will be changed to whatever the current time limit of the
	//watchdog timer is
	ioctl(watchdog, WDIOC_GETTIMEOUT, &timeout);

	//This print statement will confirm to us if the time limit has been properly
	//changed. The \n will create a newline character similar to what endl does.
	printf("The watchdog timeout is %d seconds.\n\n", timeout);

	//A counter used to keep track of how many blinks have occurred.
	int counter = 0;

	enum State {CLOSED,ENTRY,OPENENTRY,ENTRYFINISHED,EXIT,OPENEXIT,EXITFINISHED};    //Closed => both lasers = 1, entry => laser 1 = 0, laser 2 = 1
	               																	 //open entry => both lasers = 0, entryfinished = laser 2 = 0, laser 1 = 1
	enum State entry = CLOSED; 
	int laser1count = 0;
	int laser2count = 0;      

	while(1)
	{
		//The code in this while loop is identical to the code from buttonBlink
		//in lab 2. You can think of the laser/photodiode setup as a type of switch

		int laser1 = laserDiodeStatus(gpio, 1);
		int laser2 = laserDiodeStatus(gpio, 2);    //1 = laser detected, 0 = laser blocked
		printf("%d %d %d ",laser1,laser2,counter);
	
		printf ("%d \n", entry);
		
		usleep(10000); 
		getTime(time);

		switch (entry){                                                 //Anything strange happens, will reset back to CLOSED (such as if lasers disconnect) 
			case CLOSED:
				if (laser1 == 0){      //Starts to enter
					entry = ENTRY;
					laser1count++;
					//Log that laser 1 has been broken
					PRINT_LASERSTATS(statsFile, time, programName, laser1count, " times in total. Laser 1 has been broken\n\n");
				else if (laser2 == 0)  {    //Starts to leave
					entry = EXIT;
					laser2count++;
					//Log that laser 2 has been broken
					PRINT_LASERSTATS(statsFile, time, programName, laser2count, " times in total. Laser 2 has been broken\n\n");
				break;
				
			case ENTRY:
				if (laser1 == 1)        //Backs away 
					entry = CLOSED;
				else if (laser2 == 0){
					entry = OPENENTRY;   //Enters more
					laser2count++;
					//Log that laser 2 has been broken
					PRINT_LASERSTATS(statsFile, time, programName, laser2count, " times in total. Laser 2 has been broken\n\n");
				break;
				
			case OPENENTRY:
				if (laser2 == 1)
					entry = ENTRY;      //Backs away
				else if (laser1 == 1)
					entry = ENTRYFINISHED;   //Enters more
				break;
				
			case ENTRYFINISHED:
				if (laser1 == 0) {
					entry = OPENENTRY;  //Backs away
					laser1count++;
					//Log that laser 1 has been broken
					PRINT_LASERSTATS(statsFile, time, programName, laser1count, " times in total. Laser 1 has been broken\n\n");
				else if (laser2 == 1){
					counter++;
					entry = CLOSED;      //Fully enters 
					//Log that person has entered the room
					PRINT_PERSONSTATS(statsFile, time, programName, "Person has entered the room. The current number is: ", counter, "\n\n");
				}
				break;
				
			case EXIT:
				if (laser2 == 1)        //Backs away 
					entry = CLOSED;
				else if (laser1 == 0){
					entry = OPENEXIT;   //Exits more
					laser1count++;
					//Log that laser 1 has been broken
					PRINT_LASERSTATS(statsFile, time, programName, laser1count, " times in total. Laser 1 has been broken\n\n");
				break;
				
			case OPENEXIT:
				if (laser1 == 1)
					entry = EXIT;      //Backs away
				else if (laser2 == 1)
					entry = EXITFINISHED;   //Exits more
				break;
				
			case EXITFINISHED:
				if (laser2 == 0) {
					entry = OPENEXIT;   //Backs away
					laser2count++;
					//Log that laser 2 has been broken
					PRINT_LASERSTATS(statsFile, time, programName, laser2count, " times in total. Laser 2 has been broken\n\n");
				else if (laser1 == 1){
					counter--;
					entry = CLOSED;      //Fully exits
					//Log that person has left the room

					PRINT_PERSONSTATS(statsFile, time, programName, "Person has left the room. The current number is: ", counter, "\n\n");
				}
				break;
		}
		
		ioctl(watchdog, WDIOC_KEEPALIVE, 0);
		getTime(time);
		//Log that the Watchdog was kicked
		PRINT_MSG(logFile, time, programName, "The Watchdog was kicked\n\n");
	
	}

	//Writing a V to the watchdog file will disable to watchdog and prevent it from
	//resetting the system
	write(watchdog, "V", 1);
	getTime(time);
	//Log that the Watchdog was disabled
	PRINT_MSG(logFile, time, programName, "The Watchdog was disabled\n\n"); 

	//Close the watchdog file so that it is not accidentally tampered with
	close(watchdog);
	getTime(time);
	//Log that the Watchdog was closed
	PRINT_MSG(logFile, time, programName, "The Watchdog was closed\n\n");


	//Free the gpio pins
	gpiolib_free_gpio(gpio);
	getTime(time);
	//Log that the GPIO pins were freed
	PRINT_MSG(logFile, time, programName, "The GPIO pins have been freed\n\n");

	//Return to end the program
	return 0;
}