// Program Header Information ////////////////////////////////////////
/**
* @file Sim03.c
*
* @brief program for SIM03
* 
* @details Simulation of several programs OS simulator
*
* @version 3.00
*          C.S. Student (22 April 2016)
*          Initial development of SIM03
*
* @note None
*/
// Program Description/Support /////////////////////////////////////
/*
This phase will require the creation of a multiprogramming OS simulator
named Sim03, using five states (Enter/Start, Ready, Runing, Blocked/
Waiting, and Exit). It will accept the meta-data for several programs
(i.e., potentially unlimited number, with a potentially unlimited number
of operations in each), run the programs concurrently using a multi-
programming strategy, and then end the simulation.
*/
// Precompiler Directives //////////////////////////////////////////
//
/////  NONE
//
// Header Files ///////////////////////////////////////////////////
//
   #include <stdio.h>
   #include <stdlib.h>
   #include <string.h>
   #include <unistd.h>
   #include <time.h>
   #include <stdbool.h>
   #include <pthread.h>
//
// Global Constant Definitions ////////////////////////////////////
//
   #define BILLION   1E9 
//
// Class Definitions //////////////////////////////////////////////
//
   struct meta
      {
       // struct that records meta data
       int proc_num;
       char component;
       char operation[10];
       int cyc_time;
       struct meta *next;
      };

   struct pcb_table
      {
       // struct that records pcb information read from config file
       int processNum;
       int quantumTime;
       int processorCycleTime;
       int monitorCycleTime;
       int hardDriveCycleTime;
       int printerCycleTime;
       int keyboardCycleTime;
       int logMode;
       char scheduling[10];
       char dataFile[15];
       char outputFile[15];
       struct meta* metaHead;
       struct meta* metaTail;
       struct meta* current;
       int currentLeft;
       struct pcb_table* next;
      };

   struct logLine
      {
       // struct that records log to print and write to file
       double time;
       char comment[60];
       struct logLine *next;
      };
//
// Global Valuables ///////////////////////////////////////////////
//
    struct pcb_table *pcbList = NULL;   // pcb linked list
    struct pcb_table *blockQueue = NULL;   // blocked pcb queue
    struct logLine *logList = NULL;   // log linked list
    struct logLine *currentLog = NULL;   // pointer to current log
    struct timespec startTime, endTime;   // timer
    int processCounter = 0;   // number of processes
    bool toMonitor = false;   // check if the log needs to print on screen
    double totalTime;   // time range
    char logComment[60];   // comment inside log
//
// Free Function Prototypes ///////////////////////////////////////
//
   void readConfig( char* fileName, struct pcb_table* pcb );
   void readMeta( char* fileName, struct pcb_table** pcbList, struct pcb_table myPcb, 
                  int* processCounter );
   void* thread_perform( void* argument );
   int calcTime( struct meta metaData, struct pcb_table pcb );
   double timeLap( struct timespec startTime, struct timespec endTime );
   void delay( clock_t wait );
   void recordLog( struct logLine** list, struct logLine** currentLog, 
                  double time, char memo[60] );
   void printLog( struct logLine* currentLog );
   void outputToFile( struct logLine* list, struct pcb_table pcb );
   void readString( char* des, char* line );
   void readNum( int* des, char* line );
   void runProcess( struct meta* current, struct pcb_table** pcbPtr );
   void metaStartLog( struct meta* currentMeta, struct pcb_table* pcbPtr, char comment[60] );
   void blockProcess(struct pcb_table** pcbList, struct pcb_table* pcbPtr, 
                     struct pcb_table** blockQueue);
   void unblockProcess(int procNum);
   void interruptLog( struct meta* currentMeta, char comment[60] );
   void blockLog(struct meta* currentMeta, char comment[60] );
//
// Main Function Implementation ///////////////////////////////////
//
   int main( int argc, char* argv[] )
      {
       struct pcb_table myPCB = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };   // pcb table
       struct pcb_table *pcbPtr = NULL;   //pcb pointer
       bool isThread = false;   // check if the meta a thread
       bool toFile = false;   // check if the log needs to output to file
       int currentProcess = 0;   // current running process
       pthread_t myThread;   // thread

       currentLog = logList;

       // read config file
       readConfig( argv[1], &myPCB );

       // check if need to print to monitor or file
       if(myPCB.logMode == 1 || myPCB.logMode == 2 )
          toMonitor = true;
       if(myPCB.logMode == 1 || myPCB.logMode == 3 )
          toFile = true;

       // read meta data file, update pointer
       readMeta( myPCB.dataFile, &pcbList, myPCB, &processCounter);
       pcbPtr = pcbList;

       // start timer
       clock_gettime( CLOCK_REALTIME, &startTime ); 

       // print start log
       clock_gettime( CLOCK_REALTIME, &endTime );
       totalTime = timeLap( startTime, endTime );
       recordLog( &logList, &currentLog, totalTime, "Simulator program starting" );
       if( toMonitor )
          printLog( currentLog );

       // print log
       clock_gettime( CLOCK_REALTIME, &endTime );
       totalTime = timeLap( startTime, endTime );
       recordLog( &logList, &currentLog, totalTime, "OS: preparing all processes" );
       if( toMonitor )
          printLog( currentLog );

       // loop around processes
       while( processCounter > 0 )
          {
          // if the process hasn't finished
          if( pcbPtr -> current != NULL )
             {
              // print log
              clock_gettime( CLOCK_REALTIME, &endTime );
              totalTime = timeLap( startTime, endTime );
              recordLog( &logList, &currentLog, totalTime, "OS: seleting next process" );
              if( toMonitor )
                 printLog( currentLog );

              // print log
              clock_gettime( CLOCK_REALTIME, &endTime );
              totalTime = timeLap( startTime, endTime );
              metaStartLog( pcbPtr -> current, pcbPtr, logComment );
              recordLog( &logList, &currentLog, totalTime, logComment );
              if( toMonitor )
                 printLog( currentLog );

              // if the current meta is process
              if( pcbPtr -> current -> component == 'P' )
                 {
                  runProcess( pcbPtr -> current, &pcbPtr );
                  if( pcbPtr -> currentLeft == 0 )
                     {
                      clock_gettime( CLOCK_REALTIME, &endTime );
                      totalTime = timeLap( startTime, endTime );
                      interruptLog(pcbPtr -> current, logComment);
                      recordLog( &logList, &currentLog, totalTime, logComment );
                      if( toMonitor )
                         printLog( currentLog );

                      pcbPtr -> current = pcbPtr -> current -> next;
                      if( pcbPtr -> current != NULL && pcbPtr -> current -> component == 'P' )
                         pcbPtr -> currentLeft = pcbPtr -> current -> cyc_time;
                      if(pcbPtr -> current == NULL)
                         processCounter --;
                     }
                 }

              // if the current meta needs spawn thread
              else
                 {
                  // spawn thread
                  pthread_create( &myThread, NULL, thread_perform, (void*) pcbPtr );

                  // print log
                  clock_gettime( CLOCK_REALTIME, &endTime );
                  totalTime = timeLap( startTime, endTime );
                  blockLog(pcbPtr->current, logComment);
                  recordLog( &logList, &currentLog, totalTime, logComment );
                  if( toMonitor )
                     printLog( currentLog );

                  // block process
                  blockProcess(&pcbList, pcbPtr, &blockQueue);
                 }
             }

          // select next process in queue
          if(pcbPtr -> next == NULL)
             pcbPtr = pcbList;
          else
             pcbPtr = pcbPtr -> next;
          }

       // print ending log
       clock_gettime( CLOCK_REALTIME, &endTime );
       totalTime = timeLap( startTime, endTime );
       recordLog( &logList, &currentLog, totalTime, "Simulator program ending" );
       if( toMonitor )
          printLog( currentLog );

       // output to file
       if( toFile )
          outputToFile( logList, myPCB );

       return 0;
      }   // end of main

//
// Free Function Implementation ///////////////////////////////////
/**
* @brief Function creates a thread
*
* @details Function creates a thread according its cycle time, then
*          unblock the process
*
* @pre void* argument contains argument
*
* @post thread run for its time
*
* @return None
*
*/
   void* thread_perform( void* argument )
      {
       struct pcb_table* temp;   // temp argument
       int time = 0;   // init. time

       temp = ( ( struct pcb_table * ) argument );

       // calculate running time
       time = calcTime( *( temp->current ), *temp );

       // delay that long time
       delay( ( clock_t ) time );

       // print log
       clock_gettime( CLOCK_REALTIME, &endTime );
       totalTime = timeLap( startTime, endTime );
       interruptLog( temp->current, logComment );
       recordLog( &logList, &currentLog, totalTime, logComment );
       if( toMonitor )
          printLog( currentLog );

       // unblock process
       unblockProcess(temp->processNum);
      }   // end of func

/**
* @brief Function unblock process 
*
* @details Function reads process number and unblock process
*
* @pre int procNum contains the rank of process
*
* @post move the process from block queue to ready queue
*
* @return None
*
*/
   void unblockProcess(int procNum)
      {
       struct pcb_table *blockPtr = blockQueue;   // pointer to block queue
       struct pcb_table *pcbListTemp = pcbList;   // pointer to pcb list
       struct pcb_table *temp = NULL;   // pointer temp

       // update pcb list
       while( blockPtr->processNum != procNum)
          blockPtr = blockPtr -> next;
       blockPtr -> current = blockPtr -> current -> next;
       if( blockPtr -> current != NULL && blockPtr -> current -> component == 'P' )
          blockPtr -> currentLeft = blockPtr -> current -> cyc_time;
       if( blockPtr -> current == NULL )
          processCounter --;

       // move pcb from block queue to pcb list
       while(pcbListTemp -> next -> next != NULL)
          pcbListTemp = pcbListTemp -> next;
       temp = blockPtr -> next;
       blockPtr -> next = pcbListTemp -> next;
       pcbListTemp -> next = blockPtr;

       // update block queue
       if( blockPtr != blockQueue)
          {
           blockPtr = blockQueue;
           while( blockPtr-> next != pcbListTemp -> next )
              blockPtr = blockPtr -> next;
           blockPtr -> next = temp;
          }
       else
          blockQueue = temp;
      }   // end of func

/**
* @brief Function blocks process
*
* @details Function blocks process (move to block queue)
*
* @pre struct** pcbList contains the pcb list
*
* @pre struct* pcb contains the pcb table
*
* @pre struct** blockQueue contains the block queue
*
* @post move the process from ready queue to block queue
*
* @return None
*
*/
    void blockProcess(struct pcb_table** pcbList, struct pcb_table* pcbPtr, 
                      struct pcb_table** blockQueue)
       {
        struct pcb_table *blockPtr = *blockQueue;   // pointer to block queue
        struct pcb_table *pcbListTemp = *pcbList;   // pointer to pcb list

        if( blockPtr == NULL )
           {
            *blockQueue = pcbPtr;
            blockPtr = *blockQueue;
           }
        else
           {
            while( ( blockPtr -> next ) != NULL )
            blockPtr = blockPtr -> next;
            blockPtr -> next = pcbPtr;
            blockPtr = blockPtr -> next;
           }
// remove pcbptr out out pcbList
        while( (pcbListTemp -> next) != pcbPtr )
           pcbListTemp = pcbListTemp -> next;
        pcbListTemp -> next = pcbPtr -> next;

// update block next
        blockPtr -> next = NULL;
       }   // end of func

/**
* @brief Function runs process
*
* @details Function delays certain time for process run
*
* @pre struct* current contains current meta
*
* @pre struct* pcb contains the pcb table
*
* @post certain time delayd for process
*
* @return None
*
*/
   void runProcess( struct meta* current, struct pcb_table** pcbPtr )
      {
       int time = 0;   // init. time

       time = calcTime( *current, **pcbPtr );

       // delay that long time
       delay( ( clock_t ) time );

       ( *pcbPtr ) -> currentLeft -= ( *pcbPtr ) -> quantumTime;
       if( ( *pcbPtr ) -> currentLeft < 0 )
       ( *pcbPtr ) -> currentLeft = 0;
      }   // end of func

/**
* @brief Function reads config file 
*
* @details Function reads config file and record to pcb table
*
* @pre char* fileName contains the name of config file
*
* @pre struct* pcb contains the pcb table
*
* @post if the config file doesn't exist, end the program
*
* @post all pcb information recorded in pcb table
*
* @return None
*
*/
   void readConfig( char* fileName, struct pcb_table* pcb )
      {
       FILE* filePtr;   // file pointer
       char line[60];   // string holds each line of file
       char temp[15];   // temp string

       // open file and read
       filePtr = fopen( fileName, "r" );

       // if the file doesn't exist
       if( filePtr == NULL )
          {
           printf( "CONFIGURATION FILE NOT FOUND!\n" );
           exit( 1 );
          }

       // otherwise
       else
          {
           // loop to each line
           while( fgets( line, sizeof(line), filePtr ) )
              {
               // ingore these lines
               if( strncmp( line, "Start Simulator Configuration File", 15 ) == 0
                || strncmp( line, "Version/Phase: ", 10 ) == 0
                || strncmp( line, "End Simulator Configuration File", 15 ) == 0 )
                  continue;

               // read and record meta file name
               if( strncmp( line, "File Path: ", 10 ) == 0 )
                  readString( pcb -> dataFile, line );

               // read and record scheduling mode
               if( strncmp( line, "CPU Scheduling Code: ", 10 ) == 0 )
                  {
                   readString( pcb-> scheduling, line);

                   // check if the scheduling mode supported
                   if( strcmp( pcb-> scheduling, "FIFO-P") == 0 )
                      continue;
                   else
                      {
                       printf("Scheduling Mode Does Not Support!\n");
                       exit( 1 );
                      }
                  }

               if( strncmp( line, "Quantum Time (cycles): ", 10 ) == 0 )
                  readNum( &( pcb -> quantumTime ), line );

               // read and record processor cycle time
               if( strncmp( line, "Processor cycle time (msec): ", 10 ) == 0 )
                  readNum( &( pcb -> processorCycleTime ), line );

               // read and record monitor cycle time
               if( strncmp( line, "Monitor display time (msec): ", 10 ) == 0 )
                  readNum( &( pcb -> monitorCycleTime ), line );

               // read and record hard drive cycle time
               if( strncmp( line, "Hard drive cycle time (msec): ", 10 ) == 0 )
                  readNum( &( pcb -> hardDriveCycleTime ), line );

               // read and record printer cycle time
               if( strncmp( line, "Printer cycle time (msec): ", 10 ) == 0 )
                  readNum( &( pcb -> printerCycleTime ), line );

               // read and record keyboard cycle time
               if( strncmp( line, "Keyboard cycle time (msec): ", 10 ) == 0 )
                  readNum( &( pcb -> keyboardCycleTime ), line );

               // read and record log mode
               if( strncmp( line, "Log: ", 4 ) == 0 )
                  {
                   readString( temp, line );

                   if( strncmp( temp, "Log to Both", 11 ) == 0 )
                      pcb -> logMode = 1;
                   if( strncmp( temp, "Log to Monitor", 14 ) == 0 )
                      pcb -> logMode = 2;
                   if( strncmp( temp, "Log to File", 11 ) == 0 )
                      pcb -> logMode = 3;
                  }

               // read and record output file name
               if( strncmp( line, "Log File Path: ", 10 ) == 0 )
                   readString( pcb -> outputFile, line );
              }   // end of loop
          }

      // close file
      fclose(filePtr);
      }   // end of func

/**
* @brief Function reads string in a line
*
* @details Function reads string in a line and copy
*
* @pre char* des contains the string to store
*
* @pre char* line contains the line
*
* @post the certain part of string is read and saved
*
* @return None
*
*/
    void readString( char* des, char* line )
       {
        int charIndex;   // index of each character
        int stringIndex = 0;   // index of string

        // loop through the stirng 
        for( charIndex = 0; line[charIndex] != '\n'; charIndex ++ )
           {
            if( line[charIndex] == ':' )
               {
                do
                   {
                    charIndex ++;
                   }
                while( line[charIndex] == ' ' );
                while( line[charIndex] != '\n' )
                   {
                    des[stringIndex] = line[charIndex];
                    charIndex ++;
                    stringIndex ++;
                   }
                des[stringIndex] = '\0';
               }
           }   // end of loop
       }   // end of func

/**
* @brief Function reads a number in a line
*
* @details Function reads a number in a line and copy
*
* @pre int* des contains the number to store
*
* @pre char* line contains the line
*
* @post the certain part of number is read and saved
*
* @return None
*
*/
    void readNum( int* des, char* line )
       {
        int charIndex;   // index of each character

        // loop through the string
        for( charIndex = 0; line[charIndex] != '\n'; charIndex ++ )
           {
            if( line[charIndex] == ':' )
               {
                do
                   {
                    charIndex ++;
                   }
                while( line[charIndex] == ' ' );
                while( line[charIndex] != ' ' && line[charIndex] != '\n' )
                   {
                    *des = *des * 10 + line[charIndex] - '0';
                    charIndex ++;
                   }
               }
           }   // end of loop
       }   // end of func

/**
* @brief Function reads meta data file 
*
* @details Function reads meta data and record
*
* @pre char* fileName contains the name of data file
*
* @pre struct** pcbList contains the pcb linked list
*
* @pre struct myPcb contains the pcb table
*
* @pre int* processCounter contains the number of process
*
* @post if the data file doesn't exist, end the program
*
* @post all meta data information recorded
*
* @return None
*
*/
void readMeta( char* fileName, struct pcb_table** pcbList, struct pcb_table myPcb, int* processCounter )
      {
       FILE* filePtr;   // file pointer
       char line[100];   // string holds each line of file
       int charIndex;   // index to string
       int processNum = 0;   // the rank of process
       struct meta tempMeta;   // temp meta
       //struct meta* metaListEnd = *metaList;   // meta pointer to end of linked list
       struct pcb_table* pcbListEnd = *pcbList;   // pcb pointer to end of linked list

       // open file
       filePtr = fopen( fileName, "r" );

       // if the file doesn't exist
       if( filePtr == NULL )
          {
           printf( "META DATA FILE NOT FOUND!\n" );
           exit( 1 );
          }

       // otherwise
       else
          {
           // loop to get each line
           while( fgets( line, sizeof( line ), filePtr ) )
              {
               // ignore these lines
               if( strncmp( line, "Start Program Meta-Data Code:", 15 ) == 0
                || strncmp( line, "End Program Meta-Data Code.", 15 ) == 0 )
                  continue;

               // loop to record each meta data
               for( charIndex = 0; charIndex < sizeof( line ); charIndex ++ )
                  {
                   while( line[charIndex] == ' ' )
                      charIndex ++;
                   if( line[charIndex] == '\n' )
                      break;

                   // read component
                   tempMeta.component = line[charIndex]; 

                   charIndex ++;
                   if( line[charIndex] == '(' ) 
                      charIndex ++;
                      int k;

                   // read operation
                   for( k=0; line[charIndex] != ')' ; charIndex ++, k ++ )
                      tempMeta.operation[k] = line[charIndex];
                   tempMeta.operation[k] = '\0';
                   charIndex ++;

                   // read cycle time
                   tempMeta.cyc_time = 0;
                   for( ; line[charIndex] != ';' && line[charIndex] != '.'; charIndex ++ )
                      tempMeta.cyc_time = tempMeta.cyc_time * 10 + line[charIndex] - '0';


                   // decide the rank the process
                   if( tempMeta.component == 'A' && strcmp( tempMeta.operation, "start" ) == 0 )
                      processNum ++;
                   if( tempMeta.component != 'S' )
                      tempMeta.proc_num = processNum;
                   else
                      tempMeta.proc_num = 0;

                   // set up meta and copy data above
                   struct meta* metaPtr = ( struct meta* ) malloc ( sizeof( struct meta ) );
                   metaPtr -> component = tempMeta.component;
                   strcpy( metaPtr -> operation, tempMeta.operation );
                   metaPtr -> cyc_time = tempMeta.cyc_time;
                   metaPtr -> proc_num = tempMeta.proc_num;
                   metaPtr -> next = NULL;

                   // if the meta represent a begin of process, set up a pcb for it
                   if( tempMeta.component == 'S' || strcmp( tempMeta.operation, "start" ) == 0 )
                      {
                       struct pcb_table* pcbPtr = ( struct pcb_table* ) malloc 
                                                                      ( sizeof ( struct pcb_table) );
                       pcbPtr -> processNum = tempMeta.proc_num;
                       strcpy( pcbPtr -> dataFile, myPcb.dataFile);
                       strcpy( pcbPtr -> scheduling, myPcb.scheduling);
                       pcbPtr -> quantumTime = myPcb.quantumTime;
                       pcbPtr -> processorCycleTime = myPcb.processorCycleTime;
                       pcbPtr -> monitorCycleTime = myPcb.monitorCycleTime;
                       pcbPtr -> hardDriveCycleTime = myPcb.hardDriveCycleTime;
                       pcbPtr -> printerCycleTime = myPcb.printerCycleTime;
                       pcbPtr -> keyboardCycleTime = myPcb.keyboardCycleTime;
                       pcbPtr -> logMode = myPcb.logMode;
                       strcpy( pcbPtr -> outputFile, myPcb.outputFile );
                       pcbPtr -> next = NULL;
                       pcbPtr -> metaHead = NULL;
                       pcbPtr -> metaTail = NULL;
                       pcbPtr -> current = NULL;
                       pcbPtr -> currentLeft = 0;

                       if( *pcbList == NULL )
                          *pcbList = pcbPtr;
                       else
                          pcbListEnd -> next = pcbPtr;
                       pcbListEnd = pcbPtr;
                      }
                   else if( tempMeta.component == 'P' || tempMeta.component == 'I' 
                         || tempMeta.component == 'O' )
                      {
                       if( pcbListEnd -> metaHead == NULL )
                          {
                           pcbListEnd -> metaHead = metaPtr;
                           pcbListEnd -> metaTail = metaPtr;
                           pcbListEnd -> current = metaPtr;
                           pcbListEnd -> currentLeft = metaPtr -> cyc_time;
                          }
                       else
                          {
                           pcbListEnd -> metaTail -> next = metaPtr;
                           pcbListEnd -> metaTail = pcbListEnd -> metaTail -> next;
                          }
                      }
                  }   // end of loop
              }
          }   // end of loop

       *processCounter = processNum;
       // close file
       fclose(filePtr);
      }   // end of func

/**
* @brief Function print a certain log
*
* @details Function prints a certain log
*
* @pre struct* currentMeta contains the current meta
*
* @pre struct* pcbPtr contains the process
*
* @pre char comment contains log
*
* @post copys log to comment
*
* @return None
*
*/
   void metaStartLog( struct meta* currentMeta, struct pcb_table* pcbPtr, char comment[60] )
      {
       // generate log for process
       if( currentMeta -> component == 'P')
          {
           sprintf( comment, "Process %d: processing action - ", currentMeta -> proc_num );
           if( currentMeta -> cyc_time == pcbPtr -> currentLeft )
              strcat( comment, "start" );
           else
              strcat( comment, "continue" );
          }

       // generate log for I/O
       else 
          {
           sprintf( comment, "Process %d: ", currentMeta -> proc_num );
           strcat( comment, currentMeta -> operation );
           if( currentMeta -> component == 'I' )
              strcat( comment, " input - start" );
           if( currentMeta -> component == 'O' )
              strcat( comment, " output - start");
          }
      }   // end of func

/**
* @brief Function print a certain log
*
* @details Function prints a certain log when interrupts
*
* @pre struct* currentMeta contains the current meta
*
* @pre char comment contains log
*
* @post copys log to comment
*
* @return None
*
*/
   void interruptLog( struct meta* currentMeta, char comment[60] )
      {
       sprintf( comment, "Interrupt: Process %d - ", currentMeta -> proc_num );

       // generate log for process
       if( currentMeta -> component == 'P' )
          strcat( comment, "quantum time out" );

       // generate log for I/O
       else
          {
           strcat( comment, currentMeta -> operation );
           if( currentMeta -> component == 'I' )
              strcat( comment, " input completed" );
           else
              strcat( comment, " output completed");
          }
      }   // end of func

/**
* @brief Function print a certain log
*
* @details Function prints a certain log when block
*
* @pre struct* currentMeta contains the current meta
*
* @pre char comment contains log
*
* @post copys log to comment
*
* @return None
*
*/
   void blockLog( struct meta* currentMeta, char comment[60] )
      {
       sprintf( comment, "Process %d : block for ", currentMeta -> proc_num );
       strcat( comment, currentMeta -> operation );
       if( currentMeta -> component == 'I' )
          strcat( comment, " input" );
       else
          strcat( comment, " output" );
      }   // end of func

/**
* @brief Function delays some time
*
* @details Function delays according to the time
*
* @pre clock_t time contains the time to delay
*
* @post delays for a certain time
*
* @return None
*
*/
   void delay( clock_t wait )
      {
       clock_t goal, now;   // time to stop and current time
       now = clock();

       goal = wait + now;

       while( goal > now )
          now = clock();
      }   // end of func

/**
* @brief Function calculates the time
*
* @details Function calculates the time according to cycle time
*
* @pre struct metaData contains meta data
*
* @pre struct pcb contains the pcb table
*
* @post the time that needs to delay created
*
* @return int time that the delay time
*
*/
   int calcTime( struct meta metaData, struct pcb_table pcb )
      {
       int time = 0;   // init. time
       char temp[10] = { 0 };   // string temp
       int numberOfCycle = 0;   // number of cycles
       int cycleTime = 0;  // cycle time

       // get number of cycles
       strcpy( temp, metaData.operation );
       if( metaData.component == 'P' )
          numberOfCycle = pcb.quantumTime;
       else
          numberOfCycle = metaData.cyc_time;

       // get cycle time
       if( strcmp( temp, "run" ) == 0 )
          cycleTime = pcb.processorCycleTime;
       if( strcmp( temp, "hard drive" ) == 0 )
          cycleTime = pcb.hardDriveCycleTime;
       if( strcmp( temp, "keyboard" ) == 0 )
          cycleTime = pcb.keyboardCycleTime;
       if( strcmp( temp, "monitor" ) == 0 )
          cycleTime = pcb.monitorCycleTime;
       if( strcmp( temp, "printer" ) == 0 )
          cycleTime = pcb.printerCycleTime;

       time = cycleTime * numberOfCycle * 1000;

       return time;
      }   // end of func

/**
* @brief Function gets the time range it elapses
*
* @details Function returns the real time range
*
* @pre struct startTime contains start time point
*
* @pre struct endTime contains end time point
*
* @post time range calculated and returned
*
* @return double the time range
*
*/
    double timeLap( struct timespec startTime, struct timespec endTime )
       {
        return ( endTime.tv_sec - startTime.tv_sec ) + 
               ( endTime.tv_nsec - startTime.tv_nsec ) / BILLION;
       }   // end of func

/**
* @brief Function records log 
*
* @details Function records log's time and comment
*
* @pre struct** list contains the log linked list
*
* @pre struct** currentLog contains the current log
*
* @pre double time contains the lap time
*
* @pre char memo contains commment of log
*
* @post records the log
*
* @return None
*
*/
    void recordLog( struct logLine** list, struct logLine** currentLog, double time, char memo[60] )
       {
        // allocate a new log
        struct logLine* ptr = ( struct logLine* ) malloc ( sizeof( struct logLine ) );

        // copy all data
        if( time != 0 )
           ptr -> time = time;
        if( memo != 0 )
           strcpy( ptr -> comment, memo );
        ptr -> next = NULL;

        // connect the linked list
        if( *list == NULL)
           *list = ptr;
        else
           (*currentLog) -> next = ptr;
        *currentLog = ptr;
       }   // end of func

/**
* @brief Function prints each log 
*
* @details Function prints each log to screen
*
* @pre struct currentLog contains the log
*
* @post the log printed on screen
*
* @return None
*
*/
   void printLog( struct logLine* currentLog )
      {
       printf( "%f - %s\n", currentLog -> time, currentLog -> comment );
      }   // end of func

/**
* @brief Function output to file
*
* @details Function output logs to file
*
* @pre struct currentLog contains the log
*
* @pre int numberOfLog contains the number of logs
*
* @pre struct pcb contains the pcb table
*
* @post output logs to file
*
* @return None
*
*/
    void outputToFile( struct logLine* list, struct pcb_table pcb )
       {
        FILE *filePtr;   // file pointer

        // open file and write
        filePtr = fopen( pcb.outputFile, "w" );

        // output each log
        while( list != NULL )
           {
            fprintf( filePtr, "%f", list -> time );
            fputs( " - ", filePtr );
            fprintf( filePtr, "%s", list -> comment );
            fputs( "\n", filePtr );
            list = list -> next;
           }

        // close file
        fclose( filePtr );
       }   // end of func

