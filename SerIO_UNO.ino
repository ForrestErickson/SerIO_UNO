//SerIO
//SparkFun Electronics
//Written by Ryan Owens
//All code released under the Creative Commons Attribution Liscense 
// http://creativecommons.org/licenses/by/3.0/

// SerIO board is equivalant of the Arduino Pro (3.3V, 8MHz) w/ ATmega328
// so make sure to select this in the Tools->Board menu when programming
//Run on UNO by Forrest Erickson. Set baud to 115200.
//Implement "T,n" command to call "test_sequence()" because could not get <Ctrl>t to work with built in serial window.
//Implement " " as delimiter.  Delimiter is ',' or ' '.  

//Load the libraries for the program
#include <EEPROM.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <ctype.h>
#include <avr/io.h>
#include <avr/interrupt.h>
#include <avr/pgmspace.h>

//The maximum number of characters allowed in a command string on the command prompt
#define MAX_COMMAND_LENGTH	9

//Function Definitions
//Function: get_command(char *)
//Inputs: char * command - the string to store an entered command
//Return: 0 - Command was too long
//        1 - Successfully loaded command to 'command' string
//usage: get_command(string_name);
char get_command(char * command);

//Function: process_command(char *)
//Inputs: char * command - the string which contains the commandto be checked for validity and executed
//Outputs: 0 - Invalid command was sent to function
//         1 - Command executed
//usage: process_command(string_name);
//Notes:
//Command should always be in format C,P,(P),(V)(V)(V)
//Sample Commands:
//Reading digital pins(13 and 2) - D,13 or D,2
//Reading analog pins (1 and 5) - A,1 or A,5
//Writing to digital or PWM pins - W,2,1 (write pin 3 high) W,13,0 (write pin 10 low) W,3,150 (write analog 150 to pin 3)
//Configure pins to Input or Output (4 and 11) C,4,0 to initialize pin 4 as an input or C,11,1 to initialize pin 11 as an output
//Read all pins - R,A (Read all analog pins) R,D (Read all digital pins that are configured as inputs) R,P (List the pin configuration of the digital pins)
//Control command echo echo - E,0 (Turn off Echo) or E,1 (Turn on echo)
//T,n for "test_sequence()"
char process_command(char * command);

//Function Purpose: Verifies that the command contains appropriate numbers of fields and values for the specified command
//Inputs: char * command - The command supplied from the user
//        char type - The type of command that's been entered (R, D, A, or C or T)
//Outputs: 0 - Failed the test.
//         Other - Test passes. Returns the number of commas in the command, which indicates the number of fields.
char verifyCommandFields(char * command, char type, char * comma_index);

//Function: test_sequence()
//Inputs: None
//Outputs: 0 - Test Failed
//         1 - Test Passed
//Usage: test_sequence()
//Notes: Function is called when the user enters "CTRL+t" in the command prompt
char test_sequence(void);

#define PIN_MEMORY  2
char command_buffer[MAX_COMMAND_LENGTH];  //This buffer will be treated as a string which holds the command
unsigned char pin_direction[14];          //An array to keep track of how the pins are configured (inputs or outputs)
char run_count=0;                         //A variable to test if the program has been run before. Used to load default settings to the pin directions
char echo_command=1;
unsigned char i=0;                        //General variable for looping iterations
int average=0;                            //Keeps track of the analog voltage reading average

void setup()
{
//  Serial.begin(9600);  //Initialize the UART to 9600 for long cables on real serial port.
  Serial.begin(115200);  //Initialize the UART to 115200 
  
  //Find out if this is the first time the code is being executed
  run_count=EEPROM.read(0);    //Read the value from the first location in EEPROM memory
  Serial.print("Run: ");
  Serial.println(run_count, DEC);
  //If the code has never run before, set all the pins to inputs
  if(run_count !=1)
  {
    pinMode(0, OUTPUT);  //Set Tx pin as output
    pin_direction[0]=1;  //Set the Tx pin state in the array
    EEPROM.write(PIN_MEMORY, 1);  //Set Tx Pin state in memory
    pinMode(1, INPUT);  //Set Rx pin as an input
    pin_direction[1]=0;
    EEPROM.write(PIN_MEMORY+1, 0);  //Set Rx Pin state in memory
    //Loop through all of the digital pins and set them as inputs
    for(i=2; i < 14; i++){
      pinMode(i, OUTPUT);      //Set the current pin as an input
      digitalWrite(i, LOW);  //Enable the pull-up resistor on the current pin
      pin_direction[i]=1;     //Set the value in the direction array for later reference
      EEPROM.write(i+PIN_MEMORY, 1);   //Write the pin direction to memory so it can be retrieved the next time the board is turned on.
      delay(1);
    }

    delay(1);
    EEPROM.write(0, 1);  //Set the 'run_count' in memory.
    EEPROM.write(1,echo_command);  //Set the echo_command 
  }
  //If the code has been run before, retrieve all of the pin direction values from memory
  else
  {
    //Retrieve all of the digital pin directions from memory
    for(i=0; i < 14; i++)
    {      
      pin_direction[i]=EEPROM.read(i+PIN_MEMORY); //Read the current pin direction from EEPROM memory
      //Serial.print("Pin: ");             //Print the current pin and direction to the terminal
     // Serial.print(i, DEC);
      //Serial.print("\tInput/Output: ");
      //Serial.println(pin_direction[i], DEC);
      //Initialize the pin directions
      if(pin_direction[i]==0)  //If the value in memory is '0', set the pin as an input
      {
        pinMode(i, INPUT);
        digitalWrite(i, HIGH);
      }
      else pinMode(i, OUTPUT);  //If the value in memory is '1', set the pin as an output
    }
    
    //Retrieve the Echo setting
    echo_command = EEPROM.read(1);
  }
//  Serial.println("\fReady for Commands:");  // Form feed, \f did not clear the Sketch terminal.
  Serial.println("\nReady for Commands:");  // 
}

void loop()
{ 
  if(echo_command==1)Serial.print("Commands: {C,p,0/1; D,p; A,p; R,a/d; W,p,0/1 E,0/1 T,x}> ");  //Print the command prompt character
  get_command(command_buffer);  //Wait for the user to enter a command and store it in the command_buffer string
  if(echo_command==1)Serial.println("");  //Send a new-line to the terminal
  //Check for a special "test" character (CTRL+t) and run the test sequency if entered
  if(command_buffer[0] == 20)
  {
    //Run Test Sequence
    i=test_sequence();
    if(i > 0)
    {
      Serial.print("Test Failed on Pin ");
      Serial.println(i, DEC);
    }
    else Serial.println("PASS!");
    
  }
  else if(!process_command(command_buffer)) Serial.println("Invalid Command"); //Check the command for validity and execute the command in the command_buffer string
}

//Function Definitions
//Function: get_command(char *)
//Inputs: char * command - the string to store an entered command
//Return: 0 - Command was too long
//        1 - Successfully loaded command to 'command' string
//usage: get_command(string_name);
char get_command(char * command)
{
  char receive_char=0, command_buffer_count=0;
  	
  //Get a command from the prompt (A command can have a maximum of MAX_COMMAND_LENGTH characters). Command is ended with a carriage return ('Enter' key)
  while(Serial.available() <= 0);  //Wait for a character to come into the UART
  receive_char = Serial.read();  //Get the character from the UART and put it into the receive_char variable
  //Keep adding characters to the command string until the carriage return character is received
  //Commands terminated with CR only.  Set terminal program accordintly.

  while(receive_char != '\r'){
    *command=receive_char;      //Add the current character to the command string
    if(echo_command==1)Serial.print(*command++);  //Print the current character, then move the end of the command sting.
    //Don't echo the character!
    else *command++;
    command_buffer_count++;    //Increase the character count variable
    if(command_buffer_count == (2*MAX_COMMAND_LENGTH))return 0;  //If the command is too long, exit the function and return an error
    
    while(Serial.available() <= 0);  //Wait for a character to come into the UART
    receive_char = Serial.read();  //Get the character from the UART and put it into the receive_char variable
  }
  *command='\0';		//Terminate the command string with a NULL character. This is so command_buffer[] can be treated as a string.
  
  return 1;
}

//Function: process_command(char *)
//Inputs: char * command - the string which contains the command to be checked for validity and executed
//Outputs: 0 - Invalid command was sent to function
//         1 - Command executed
//usage: process_command(string_name);
//Notes:
//Command should always be in format C,P,(P),(V)(V)(V)
//Sample Commands:
//Reading digital pins(13 and 2) - D,13 or D,2
//Reading analog pins (1 and 5) - A,1 or A,5
//Writing to digital or PWM pins - W,2,1 (write pin 3 high) W,13,0 (write pin 10 low) W,3,150 (write analog 150 to pin 3)
//Configure pins to Input or Output (4 and 11) C,4,0 to initialize pin 4 as an input or C,11,1 to initialize pin 11 as an output
//Read all pins - R,A (Read all analog pins) R,D (Read all digital pins that are configured as inputs) R,P (List the pin configuration of the digital pins)
//Configure Echoing - E,0 (Turn off Echo) E,1 (Tun on Echo)
char process_command(char * command)
{
  char command_length=strlen(command);  //Find out how many characters are in the command string
  char pin=0, pin_string[3]="0"; 
  unsigned char value=0, value_string[3]="0";
  char command_type=command[0];
  char comma_count=0, comma_index[2];

	
  //First make sure we have enough characters for a valid command
  if(command_length < 3 || command_length > MAX_COMMAND_LENGTH)
  {
    return 0;
  }

  //Verify that command has a correct format and valid field entries
  comma_count=verifyCommandFields(command, command_type, comma_index);
  if(!comma_count)return 0;  //If there were no zeroes, the command is invalide.

  //The command has been verified. Now extract the data from the fields.
  //Get the pin number
  if(comma_count==1)	//Here we only have one data field
  {
    //Concatonate the number characters in the first field until we reach the end of the string
    strcat(pin_string, &command[2]);
  }
  else	//If we run this set if means we have two data fields. We need to get the pin number and the value
  {
    //Get the pin field into the pin_string 
    for(int i=comma_index[0]+1; i<comma_index[1]; i++)
    {
      //Concatonate the number characters in the first field until we reach the end of the field
      strcat(pin_string, &command[i]);
    }
    //Get the value field into the value string
    value=strtol(&command[comma_index[1]+1], NULL, 10);  //Convert the pin number to an interger rather than a string

    *value_string='\0';
    
  }		

  pin=(char)strtol(pin_string, NULL, 10);  //Convert the pin number to an integer rather than a string
  *pin_string='\0';
  
  //Check to make sure the pin number and the value number are in the acceptable range
  //Valid pin number are 0-13
  if(pin < 0 || pin > 13)
  {
    return 0;		//Check to make sure we parsed a valid pin number
  }
  
  //Make sure the value is between 0 and 255
  if(value < 0 || value > 255)return 0;
		
  //Determine and execute the proper command. If an invalid command character was sent then exit the function
  switch(command_type)
  {
    //Read an analog pin
    case 'a':
    case 'A': 
      average=0;
      for(i=0; i<10; i++)
      {
        average+=analogRead(pin);
      }
      average/=10;
      //Print the pin number that was read and its voltage
      Serial.print("Pin: ");
      Serial.print(pin, DEC);
      Serial.print("\tValue: ");
      Serial.println(average, DEC);   
      break;
    //Configure a pin to an input or an output
    case 'C':
    case 'c':      
      if(value == 0){
        pinMode(pin, INPUT);
        digitalWrite(pin, HIGH);  //Set the pullup resistor to high if the pin is an input
      }
      else pinMode(pin, OUTPUT);
      EEPROM.write(pin+PIN_MEMORY, value);  //Save the new pin setting in memory
      pin_direction[pin]=value;    //Keep track of the pin directions
      
      //Print the pin number that was configured, and its new setting
      Serial.print("Pin: ");
      Serial.print(pin, DEC);
      Serial.print("\tInput/Output: ");
      Serial.println(pin_direction[pin], DEC);
      break;
    //Write a value to a digital output pin
    case 'w':
    case 'W':
      
    //If a PWM pin was entered, the value can be between 0 and 255
    if(pin == 3 || pin == 5 || pin == 6 || pin == 9 || pin == 10 || pin == 11)
    {
      //Write the analog voltage to the pin
      analogWrite(pin, value);
    }
    //If a normal pin value was entered, the value can only be 0 or 1.
    else
    {
      //Make sure the value is either a 0 or a 1.
      digitalWrite(pin, value);
    }
      
      //Print the pin number that was set, and the value that was sent
      Serial.print("Pin: ");
      Serial.print(pin, DEC);
      Serial.print("\tValue: ");
      Serial.println(value, DEC);
      
      break;
    //Read a digital pin
    case 'd':
    case 'D': 
      //Print the pin number and value that was read
      Serial.print("Pin: ");
      Serial.print(pin, DEC);
      Serial.print("\tValue: ");
      Serial.println(digitalRead(pin), DEC);
      break;
    //Read all of the digital or analog pins
    case 'r':
    case 'R':
      if(command[2]=='d' || command[2]=='D')
      {
        for(i=0; i<14; i++)
        {
          //Go through all the digital pins and read them if they are set as inputs
          if(pin_direction[i]==0)
          {
            Serial.print("Pin: ");
            Serial.print(i, DEC);
            Serial.print("\tValue: ");
            Serial.println(digitalRead(i), DEC);		
          }
        }
      }
      //Read and display all of the analog pin values
      else if(command[2]=='a' || command[2]=='A')
      {
        for(i=0; i<8; i++)
        {
          Serial.print("Pin: ");
          Serial.print(i, DEC);
          Serial.print("\tValue: ");
          Serial.println(analogRead(i), DEC);          
        }
      }
      else if(command[2]=='p' || command[2]=='P')
      {
        for(i=0; i<14; i++)
        {
          //Go through all the digital pins and read them if they are set as inputs
            Serial.print("Pin: ");
            Serial.print(i, DEC);
            Serial.print("\tInput/Output: ");
            Serial.println(pin_direction[i], DEC);		
        }
      }      
      else
      {
        Serial.println("Invalid Command");
        return 0;
      }
      break;	
    case 'e':
    case 'E':
      echo_command=pin;
      Serial.println("OK");
      EEPROM.write(1, echo_command);
      break;
    case 't':
    case 'T':
      Serial.print("Test Sequence Return = ");
      Serial.println (test_sequence());
      command_buffer[0] = 20;
      break;
    default:
      //An invalid command was entered
      return 0;
      //break;
  }
		
  return 1;
}

//Function: test_sequence()
//Inputs: None
//Outputs: 0 - Test Failed
//         1 - Test Passed
//Usage: test_sequence()
//Notes: Function is called when the user enters "CTRL+t" in the command prompt
char test_sequence(void)
{
  Serial.println("Testing...");
  //Set all of the pins to outputs
  for(int i=2; i < 20; i++)
  {
    pinMode(i, OUTPUT);
  }
  //Turn on the first set of LEDs
  for(i=2; i<20; i+=2)
  {
    digitalWrite(i, HIGH);
    digitalWrite(i+1, LOW);
  }
  delay(1000);
  //Turn on the seconds set of LEDs
  for(i=3; i<20; i+=2)
  {
    digitalWrite(i, HIGH);
    digitalWrite(i-1, LOW);
  }
  delay(1000);    
  
  average=0;
  //Test voltage on ADC6
  for(i=0; i<10; i++)
  {
    average+=analogRead(6);
  }
  average/=10;   
  //make sure ADC6 pin is reading between 1.45V and 1.65V. If it is, check voltage on ADC7
  if((average < 450) || (average > 574))return 6;
  
  //Now test voltage on ADC7
  average=0;
  for(i=0; i<10; i++)
  {
    average+=analogRead(7);
  }
  average/=10;
  //If the average is within range on pin 7, the test passes
  if((average < 496) || (average > 527))
  {
    return 7;   
  }

  //Now load the correct pin directions back
  for(i=0; i<14; i++)
  {
    pinMode(i, pin_direction[i]);
  }
  //Reconfigure all the Analog pins as inputs
  for(i=14; i<20; i++)
  {
    pinMode(i, INPUT);
  }  
  return 0;
}

//Function Purpose: Verifies that the command contains appropriate numbers of fields and values for the specified command
//Inputs: char * command - The command supplied from the user
//        char type - The type of command that's been entered (R, D, A, or C)
//Outputs: 0 - Failed the test.
//         Other - Test passes. Returns the number of commas in the command, which indicates the number of fields.
char verifyCommandFields(char * command, char type, char * comma_index)
{
  char field_count=0;
  int recording_index=0;
  char command_length=strlen(command);
  type=toupper(type);
  
   //Count the commas and record their indices to find out how many fields are in the command
  for(int index=0; index < command_length; index++)
  {
    if(command[index]==',' || command[index]==' ')  //FLE let's use space as delimiter too.
    {
      field_count+=1;
      comma_index[recording_index++]=index;
    }
  }
	
  //Compare how many fields are in the command with the type of command to make sure the proper field length has been received.
  switch(field_count)
  {
    //Every command has at least two fields, so if there were no commas entered then the command is invalid
    case 0:
      return 0;
      break;
    //If there's only 1 comma, the command can only be digitalRead, analogRead, readAll or Echo. If it's not, then this is an invalid command
    case 1:
      //If we only found 1 comma, but a 'write' or 'initialize' command was received, than we don't have enough fields
      if(type=='W' || type=='C')
      {
        //Serial.println("Not enough fields for the command\n");
        return 0;
      }
      break;
    //If there are 2 commas then the command can only be configurePin or writePin. If it's something else then the command is invalid
    case 2:
      //If we found 2 commas but a 'digitalRead', 'analogRead' or 'readAll' command was received than we have too many fields for the command.
      if(type == 'D' || type == 'A' || type == 'R' || type == 'E' || type =='T')
      {
        return 0;
      }
      break;
    default:
      //Serial.println("Too many fields in the command string\n");
      return 0;
      break;
  }
	
  //Make sure the commas are in the correct places
  if(comma_index[0] != 1)  //If there's not a comma in the first command position there is an incorrect command format
  {
    //Serial.println("Missing first comma\n");
    return 0;
  }
  
  //If there are 2 fields, we need to make sure commas follow each field appropriately
  if(field_count >1)
  {
    if(comma_index[1] != 3 && comma_index[1] != 4) //A comma must follow either 1 or 2 digits, which would put the comma in position 3 or 4 of the command
    {
      //We have two fields, but the second comma isn't in the right place
      return 0;
    }
    if(command[command_length-1]==',' || command[command_length-1]==' ') //If there is a delimiter at the end of the command, the last field wasn't given a value.
    {
      return 0;
    }
  }
  
  if(field_count > 1)
  {
    if(comma_index[1]-comma_index[0] < 2) //Make sure a value was entered between the first and second comma (second field)
    {
      //This condition means there is the second field was left blank
      return 0;
    }
    if(command_length-comma_index[1] > 4)
    {
      //This condition means a four digit value was entered (this isn't allowed!)
      //Serial.println("Invalid Value entered\n");
      return 0;
    }
  }
	
  //Now we know the command is in the correct format, we need to make sure the characters are acceptable
  for(int index=1; index < command_length; index++)
  {

    //Make sure all characters are numbers, commas (or spaces), or acceptable command parameters (A,D and P)
    //For testing against invalid delimiter characters remember DeMorgan, if not (a or b), = not a and not b.
if(!isdigit(command[index]) && ((command[index]!=',') && (command[index]!=' ')) && (toupper(command[index]))!='A' && (toupper(command[index]))!='D' && (toupper(command[index]))!='P' && toupper(command[index])!='E')
    {
      //There is a character in the command that is not a comma or a number (other than the command parameter). This is bad!
      return 0;
    }
  }
  
  //If we made it past all the tests, return the number of commas that were found with the function
  return field_count;
}
