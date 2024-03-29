

/* passportoffice.c
 *      Passport office as a user program
 */

#include "syscall.h"

#define CLERK_NUMBER 20
#define CUSTOMER_NUMBER 60
#define CLERK_TYPES 4
#define SENATOR_NUMBER 10
#define false 0
#define true 1

void ApplicationClerk(); /* used to have int myLine in parameter */
void PictureClerk();
void PassportClerk();
void Cashier();
void Customer(); /* used to have int custNumber in parameter */
void Senator();
void Manager();

struct CustomerAttribute {
    int SSN;
    int likesPicture;
    int applicationIsFiled;
    int hasCertification;
    int isDone;
    int clerkMessedUp;
    int money;
    int currentLine;
};

int allCustomersAreDone = 0;
char threadNames[250][80];
int testChosen = 1; /* CL: indicate test number (1-7) or full program (0)*/
int clerkCount = 0;  /* CL: number of total clerks of the 4 types that can be modified*/
int customerCount = 0; /* CL: number of customers that can be modified*/
int senatorCount = 0; /* CL: number of senators that can be modified*/
int senatorLineCount = 0; /* CL: number of senators in a line at any given time*/
int senatorDone = false;

int prevTotalBoolCount = 0;
int currentTotalBoolCount = 0;

/* initialize locks and arrays for linecount, clerk, customer, manager, senator information*/
int clerkLineLock;
int clerkLineCount[CLERK_NUMBER]; /* CL: number of customers in a clerk's regular line*/
int clerkBribeLineCount[CLERK_NUMBER]; /* CL: number of customers in a clerk's bribe line*/
typedef enum X {AVAILABLE, BUSY, ONBREAK} ClerkState; /* CL: enum for clerk's conditions*/
ClerkState clerkStates[CLERK_NUMBER] = {AVAILABLE}; /*state of each clerk is available in the beginning*/
int clerkLineCV[CLERK_NUMBER];
int clerkBribeLineCV[CLERK_NUMBER];
int clerkSenatorLineCV;
int clerkSenatorCVLock[CLERK_NUMBER];

int outsideLineCV;
int outsideLock;
int outsideLineCount = 0; /* CL: an outside line for customers to line up if senator is here, or other rare situations*/

int breakCV[CLERK_NUMBER];
int senatorLock;
/* Condition* senatorCV = new Condition("SenatorCV");*/

struct CustomerAttribute customerAttributes[CUSTOMER_NUMBER]; /* CL: customer attributes, accessed by custNumber*/
int clerkMoney[CLERK_NUMBER] = {0}; /* CL: every clerk has no bribe money in the beginning*/
/*Senator control variables*/
int senatorLineCV;
int clerkSenatorCV[CLERK_NUMBER];
/*Second monitor*/
int clerkLock[CLERK_NUMBER];
int clerkCV[CLERK_NUMBER];
int customerData[CLERK_NUMBER]; /*HUNG: Every clerk will use this to get the customer's customerAttribute index*/
char clerkTypesStatic[CLERK_TYPES][30] = { "ApplicationClerks : ", "PictureClerks     : ", "PassportClerks    : ", "Cashiers          : " };
char clerkTypes[CLERK_NUMBER][30];
int clerkTypesLengths[CLERK_NUMBER];
int clerkArray[CLERK_TYPES];

int breakLock[CLERK_NUMBER];

typedef void (*VoidFunctionPtr)(int arg);

struct CustomerAttribute initCustAttr(int ssn) {
    int moneyArray[4] = {100, 600, 1100, 1600};
    int randomIndex = Rand(4, 0); /*0 to 3*/

    struct CustomerAttribute ca;
    ca.SSN = ssn;
    ca.applicationIsFiled = false;
    ca.likesPicture = false;
    ca.hasCertification = false;
    ca.isDone = false;
    ca.clerkMessedUp = false;

    ca.money = moneyArray[randomIndex];
    return ca;
}

int my_strcmp(char s1[], const char s2[], int len) {
    int i = 0;
    for(i = 0; i < len; ++i) {
        if(s1[i] == '\0') {
            return false;
        }

        if(s2[i] == '\0') {
            return false;
        }

        if(s1[i] != s2[i]) {
            return false;
        }
    }

    return true;
}

char* my_strcpy(char s1[], const char s2[], int len) {
    int i = 0;
    for(i = 0; i < len - 1; ++i) {
        s1[i] = s2[i];
    }
    s1[len] = '\0';
    return (s1);
}

/* CL: parameter: an int array that contains numbers of each clerk type
     Summary: gets input from user or test, initialize and print out clerk numbers
     return value: void */

void clerkFactory(int countOfEachClerkType[]) {
    int tempClerkCount = 0, i = 0;
    for(i = 0; i < CLERK_TYPES; ++i) {
        if(testChosen == 0) { /* gets input from user if running full program, does not get input if test */
            do {
                PrintString(clerkTypesStatic[i], 20);
                tempClerkCount = 4;  /* Technically would read user input */
                if(tempClerkCount <= 0 || tempClerkCount > 5) {
                    PrintString("    The number of clerks must be between 1 and 5 inclusive!\n", 60);
                }
            } while(tempClerkCount <= 0 || tempClerkCount > 5);
            clerkCount += tempClerkCount;
            clerkArray[i] = tempClerkCount;
        } else {
            clerkArray[i] = countOfEachClerkType[i];
        }
    }
    /* CL: print statements */
    PrintString("Number of ApplicationClerks = ", 30); PrintNum(clerkArray[0]); PrintNl();
    PrintString("Number of PictureClerks = ", 26); PrintNum(clerkArray[1]); PrintNl();
    PrintString("Number of PassportClerks = ", 27); PrintNum(clerkArray[2]); PrintNl();
    PrintString("Number of CashiersClerks = ", 27); PrintNum(clerkArray[3]); PrintNl();
    PrintString("Number of Senators = ", 21); PrintNum(senatorCount); PrintNl();
}

void createClerkThreads() {
    int clerkType = 0, j = 0;
    int clerkNumber = 0, clerkTypeLength;
    for(clerkType = 0; clerkType < 4; ++clerkType) {
        if(clerkType == 0) {
            for(j = 0; j < clerkArray[clerkType]; ++j) {
                clerkTypeLength = 16;
                my_strcpy(clerkTypes[clerkNumber], "ApplicationClerk", clerkTypeLength + 1); /* plus one for null character */
                clerkTypesLengths[clerkNumber] = clerkTypeLength;
                Fork((VoidFunctionPtr)ApplicationClerk, clerkNumber);
                ++clerkNumber;
            }
        } else if(clerkType == 1) {
            for(j = 0; j < clerkArray[clerkType]; ++j) {
                clerkTypeLength = 12;
                my_strcpy(clerkTypes[clerkNumber], "PictureClerk", clerkTypeLength + 1);
                clerkTypesLengths[clerkNumber] = clerkTypeLength;
                Fork((VoidFunctionPtr)PictureClerk,clerkNumber);
                ++clerkNumber;
            }
        } else if(clerkType == 2) {
            for(j = 0; j < clerkArray[clerkType]; ++j) {
                clerkTypeLength = 13;
                my_strcpy(clerkTypes[clerkNumber], "PassportClerk", clerkTypeLength + 1);
                clerkTypesLengths[clerkNumber] = clerkTypeLength;
                Fork((VoidFunctionPtr)PassportClerk,clerkNumber);
                ++clerkNumber;
            }
        } else { /* i == 3 */
            for(j = 0; j < clerkArray[clerkType]; ++j) {
                clerkTypeLength = 7;
                my_strcpy(clerkTypes[clerkNumber], "Cashier", clerkTypeLength + 1);
                clerkTypesLengths[clerkNumber] = clerkTypeLength;
                Fork((VoidFunctionPtr)Cashier,clerkNumber);
                ++clerkNumber;
            }
        }
    }
}

void createClerkLocksAndConditions() {
    int i;
    char name[20];
    int cpyLen;
    for(i = 0; i < clerkCount; ++i) {
        if(i / 10 == 0) {
            cpyLen = 1;
        } else {
            cpyLen = 2;
        }

        clerkLock[i] = CreateLock("ClerkLock", 9 + cpyLen, i);

        clerkCV[i] = CreateCondition("ClerkCV", 7 + cpyLen, i);

        clerkLineCV[i] = CreateCondition("ClerkLineCV", 11 + cpyLen, i);

        clerkBribeLineCV[i] = CreateCondition("ClerkBribeLineCV", 16 + cpyLen, i);

        breakLock[i] = CreateLock("BreakLock", 9 + cpyLen, i);

        breakCV[i] = CreateCondition("BreakCV", 7 + cpyLen, i);

        clerkSenatorCV[i] = CreateCondition("ClerkSenatorCV", 14 + cpyLen, i);

        clerkSenatorCVLock[i] = CreateLock("ClerkSenatorCVLock", 18 + cpyLen, i);
    }
}

/*CL: Parameter: Thread*
    Summary: create and fire off customer threads with designated names
    Return value: void*/
void createCustomerThreads() {
    int i;
    for(i = 0; i < customerCount; i++){
      PrintString("+++++Customer created with number: ", 35); PrintNum(i); PrintNl();
        Fork((VoidFunctionPtr)Customer, i);
    }
}

/*CL: Parameter: Thread*
    Summary: create and fire off senator threads with designated names
    Return value: void*/
void createSenatorThreads(){
    int i;
    for(i = 0; i < senatorCount; i++){
      PrintString("+++++Senator Created\n", 21); PrintNum(i + 50); PrintNl();
        Fork((VoidFunctionPtr)Senator, i + 50);
    }
}

/*CL: Parameter: Thread*, int []
    Summary: group all create thread/ lock/ condition functions together and fire off manager
    Return value: void*/
void createTestVariables(int countOfEachClerkType[]) {
    clerkFactory(countOfEachClerkType);
    createClerkLocksAndConditions();
    createClerkThreads();
    createCustomerThreads();
    createSenatorThreads();
    Fork((VoidFunctionPtr)Manager, 0);
}

void Part2() {
    int countOfEachClerkType[CLERK_TYPES] = {0,0,0,0};

    PrintString("Starting Part 2\n", 16);
    PrintString("Test to run (put 0 for full program): ", 38);
    testChosen = 1; /* technically cin >> */
    PrintNum(testChosen); PrintNl();

    if(testChosen == 1) {
        PrintString("Starting Test 1\n", 16); /*Customers always take the shortest line, but no 2 customers ever choose the same shortest line at the same time*/
        customerCount = 20;
        clerkCount = 6;
        senatorCount = 0;
        countOfEachClerkType[0] = 2; countOfEachClerkType[1] = 1; countOfEachClerkType[2] = 1; countOfEachClerkType[3] = 2;

        createTestVariables(countOfEachClerkType);
    } else if(testChosen == 2) {
        PrintString("Starting Test 2\n", 16); /*Managers only read one from one Clerk's total money received, at a time*/
        customerCount = 5;
        clerkCount = 4;
        senatorCount = 0;
        countOfEachClerkType[0] = 1; countOfEachClerkType[1] = 1; countOfEachClerkType[2] = 1; countOfEachClerkType[3] = 1;

        createTestVariables(countOfEachClerkType);
    } else if(testChosen == 3) {
        PrintString("Starting Test 3\n", 16); /*Customers do not leave until they are given their passport by the Cashier.
                                     The Cashier does not start on another customer until they know that the last Customer has left their area*/
        customerCount = 5;
        clerkCount = 4;
        senatorCount = 0;
        countOfEachClerkType[0] = 1; countOfEachClerkType[1] = 1; countOfEachClerkType[2] = 1; countOfEachClerkType[3] = 1;

        createTestVariables(countOfEachClerkType);
    } else if(testChosen == 4) {
        PrintString("Starting Test 4\n", 16); /*Clerks go on break when they have no one waiting in their line*/
        customerCount = 5;
        clerkCount = 4;
        senatorCount = 0;
        countOfEachClerkType[0] = 1; countOfEachClerkType[1] = 1; countOfEachClerkType[2] = 1; countOfEachClerkType[3] = 1;

        createTestVariables(countOfEachClerkType);
    } else if(testChosen == 5) {
        PrintString("Starting Test 5\n", 16); /*Managers get Clerks off their break when lines get too long*/
        customerCount = 7;
        clerkCount = 4;
        senatorCount = 0;
        countOfEachClerkType[0] = 1; countOfEachClerkType[1] = 1; countOfEachClerkType[2] = 1; countOfEachClerkType[3] = 1;

        createTestVariables(countOfEachClerkType);
    } else if(testChosen == 6) {
        PrintString("Starting Test 6\n", 16); /*Total sales never suffers from a race condition*/
        customerCount = 25;
        clerkCount = 4;
        senatorCount = 0;
        countOfEachClerkType[0] = 1; countOfEachClerkType[1] = 1; countOfEachClerkType[2] = 1; countOfEachClerkType[3] = 1;

        createTestVariables(countOfEachClerkType);
    } else if(testChosen == 7) {
        PrintString("Starting Test 7\n", 16); /*Total sales never suffers from a race condition*/
        customerCount = 7;
        clerkCount = 4;
        senatorCount = 1;
        countOfEachClerkType[0] = 1; countOfEachClerkType[1] = 1; countOfEachClerkType[2] = 1; countOfEachClerkType[3] = 1;

        createTestVariables(countOfEachClerkType);
    } else if(testChosen == 0) {
        do {
            PrintString("Number of customers: ", 21);
            customerCount = 10; /* technically cin >> */
            if(customerCount <= 0 || customerCount > 50) {
                PrintString("    The number of customers must be between 1 and 50 inclusive!\n", 64);
            }
        } while(customerCount <= 0 || customerCount > 50);

        do {
            PrintString("Number of Senators: ", 20);
            senatorCount = 1; /* technically cin >> */
            if(senatorCount < 0 || senatorCount > 10) {
                PrintString("    The number of senators must be between 1 and 10 inclusive!\n", 63);
            }
        } while(senatorCount < 0 || senatorCount > 10);

        createTestVariables(countOfEachClerkType);
    }
}

/* CL: Parameter: int myLine (line number of current clerk)
    Summary: chooses customer from the line, or decides if clerks go on break
    Return value: int (the customer number) */

int chooseCustomerFromLine(int myLine, char* clerkName, int clerkNameLength) {
    int testFlag = false;
    do {
        testFlag = false;
        /* TODO: -1 used to be NULL.  Hung needs to figure this out */
        if((senatorLineCount > 0 && clerkSenatorCVLock[myLine] != -1) || (senatorLineCount > 0 && senatorDone)) {
            /* CL: chooses senator line first */

            Acquire(clerkSenatorCVLock[myLine]);
            Signal(clerkSenatorCVLock[myLine], clerkSenatorCV[myLine]);
            /*Wait for senator here, if they need me*/
            Wait(clerkSenatorCV[myLine], clerkSenatorCVLock[myLine]);

            if(senatorLineCount == 0){
              Acquire(clerkLineLock);
              clerkStates[myLine] = AVAILABLE;
              Release(clerkSenatorCVLock[myLine]);
            }else if(senatorLineCount > 0 && senatorDone){
              clerkStates[myLine] = AVAILABLE;
              Release(clerkSenatorCVLock[myLine]);

            }else{
              clerkStates[myLine] = BUSY;
              testFlag = true;
            }
        }else{
            Acquire(clerkLineLock);
            if(clerkBribeLineCount[myLine] > 0) {
                PrintString(clerkName, clerkNameLength); PrintNum(myLine); PrintString(" is servicing a customer from bribe line\n", 41);
                Signal(clerkLineLock, clerkBribeLineCV[myLine]);
                clerkStates[myLine] = BUSY; /*redundant setting*/
            } else if(clerkLineCount[myLine] > 0) {
                PrintString(clerkName, clerkNameLength); PrintNum(myLine); PrintString(" is servicing a customer from regular line\n", 43);
                Signal(clerkLineLock, clerkLineCV[myLine]);
                clerkStates[myLine] = BUSY; /*redundant setting*/
            }else{
                Acquire(breakLock[myLine]);
                PrintString(clerkName, clerkNameLength); PrintNum(myLine); PrintString(" is going on break\n", 19);
                clerkStates[myLine] = ONBREAK;
                Release(clerkLineLock);
                Wait(breakLock[myLine], breakCV[myLine]);
                PrintNum(breakLock[myLine]); PrintNl();
                clerkStates[myLine] = AVAILABLE;
                Release(breakLock[myLine]);
                if(allCustomersAreDone) {
                    Exit(0);
                }
            }
        }
    } while(clerkStates[myLine] != BUSY);

    Acquire(clerkLock[myLine]);
    Release(clerkLineLock);
    /*wait for customer*/
    if(testFlag){
      Signal(clerkSenatorCVLock[myLine], clerkSenatorCV[myLine]);
      Release(clerkSenatorCVLock[myLine]);
    }
    Wait(clerkLock[myLine], clerkCV[myLine]);
    /*Do my job -> customer waiting*/
    return customerData[myLine];
}

/* CL: Parameter: int myLine (line number of clerk)
    Summary: completes the final signal wait communication after doing all the logic in choosing customer
    Return value: void */

void clerkSignalsNextCustomer(int myLine) {
    Signal(clerkLock[myLine], clerkCV[myLine]);
    /* If there is a senator, here is where the clerk waits, after senator is done with them */
    Wait(clerkLock[myLine],clerkCV[myLine]);
    Release(clerkLock[myLine]);
}

/* CL: Parameter: int myLine (line number of clerk)
    Summary: logics for application clerk
    Return value: void */

void PrintCust(int isCustomer) {
    if(isCustomer) {
        PrintString("Customer_", 9);
    } else {
        PrintString("Senator_", 8);
    }
}

void hasSignaledString(int isCustomer, char* threadName, int threadNameLength, int clerkNum, int custNumber) {
    PrintString(threadName, threadNameLength); PrintNum(clerkNum); PrintString(" has signalled ", 15);
    PrintCust(isCustomer); PrintNum(custNumber); PrintString(" to come to their counter. (", 28);
    PrintCust(isCustomer); PrintNum(custNumber); PrintString(")", 1); PrintNl();
}

void givenSSNString(int isCustomer, int custNumber, char* threadName, int threadNameLength, int clerkNum) {
    PrintCust(isCustomer); PrintNum(custNumber); PrintString(" has given SSN ", 15);
    PrintNum(custNumber); PrintString(" to ", 4); PrintString(threadName, threadNameLength); PrintNum(clerkNum); PrintNl();
}

void recievedSSNString(int isCustomer, char* threadName, int threadNameLength, int clerkNum, int custNumber) {
    PrintString(threadName, threadNameLength); PrintNum(clerkNum); PrintString(" has received SSN ", 18); PrintNum(custNumber);
    PrintString(" from ", 6); PrintCust(isCustomer); PrintNum(custNumber); PrintNl();
}

void ApplicationClerk() {
    int myLine = GetThreadArgs();
    int i, numYields;
    char personName[50];
    int isCustomer = 1, custNumber;

    while(!allCustomersAreDone) {
        custNumber = chooseCustomerFromLine(myLine, "ApplicationClerk_", 17);
        if(custNumber >= 50) {
            isCustomer = 0;
        } else {
            isCustomer = 1;
        }

        clerkStates[myLine] = BUSY;
        hasSignaledString(isCustomer, "ApplicationClerk_", 17, myLine, custNumber);
        Yield();
        givenSSNString(isCustomer, custNumber, "ApplicationClerk_", 17, myLine);
        Yield();
        recievedSSNString(isCustomer, "ApplicationClerk_", 17, myLine, custNumber);

/* CL: random time for applicationclerk to process data */
        numYields = Rand(80, 20);
        for(i = 0; i < numYields; ++i) {
            Yield();
        }

        customerAttributes[custNumber].applicationIsFiled = true;
        PrintString("ApplicationClerk_", 17); PrintNum(myLine);
        PrintString(" has recorded a completed application for ", 42);
        PrintCust(isCustomer); PrintNum(custNumber); PrintNl();


        clerkSignalsNextCustomer(myLine);
    }
    Exit(0);
}

/* CL: Parameter: int myLine (line number of clerk)
    Summary: logics for picture clerk
    Return value: void */

void PictureClerk() {
    int myLine = GetThreadArgs();
    int i = 0, numYields, probability, isCustomer = 1;
    char personName[50];

    while(!allCustomersAreDone) {
        int custNumber = chooseCustomerFromLine(myLine, "PictureClerk_", 13);
        if(custNumber >= 50) {
            isCustomer = 0;
        } else {
            isCustomer = 1;
        }

        clerkStates[myLine] = BUSY;
        hasSignaledString(isCustomer, "PictureClerk_", 13, myLine, custNumber);
        Yield();
        givenSSNString(isCustomer, custNumber, "PictureClerk_", 13, myLine);
        Yield();
        recievedSSNString(isCustomer, "PictureClerk_", 13, myLine, custNumber);

        numYields = Rand(80, 20);

        while(!customerAttributes[custNumber].likesPicture) {
            PrintString("PictureClerk_", 13); PrintNum(myLine); PrintString(" has taken a picture of ", 24);
            PrintCust(isCustomer); PrintNum(custNumber); PrintNl();

            probability = Rand(100, 0);
            if(probability >= 25) {
                customerAttributes[custNumber].likesPicture = true;
                PrintCust(isCustomer); PrintNum(custNumber);  PrintString(" likes their picture from ", 26); PrintString("PictureClerk_", 13); PrintNum(myLine); PrintNl();
                Yield();
                PrintString("PictureClerk_", 13); PrintNum(myLine); PrintString(" has been told that ", 20); PrintCust(isCustomer); PrintNum(custNumber); PrintString(" does like their picture\n", 25);
                /* CL: random time for pictureclerk to process data */

                for(i = 0; i < numYields; ++i) {
                    Yield();
                }
            }else{
                PrintCust(isCustomer); PrintNum(custNumber); PrintString(" does not like their picture from ", 34);
                PrintString("PictureClerk_", 13); PrintNum(myLine); PrintNl();

                PrintString("PictureClerk_", 13); PrintNum(myLine); PrintString(" has been told that ", 20);
                PrintCust(isCustomer); PrintNum(custNumber); PrintString(" does not like their picture\n", 29);
            }
        }
        clerkSignalsNextCustomer(myLine);
    }
    Exit(0);
}

/* CL: Parameter: int myLine (line number of clerk)
    Summary: logics for passport clerk
    Return value: void */

void PassportClerk() {
    int myLine = GetThreadArgs();
    int numYields, clerkMessedUp, i, isCustomer = 1;
    char personName[50];

    while(!allCustomersAreDone) {
        int custNumber = chooseCustomerFromLine(myLine, "PassportClerk_", 14);
        if(custNumber >= 50) {
            isCustomer = 0;
        } else {
            isCustomer = 1;
        }
        hasSignaledString(isCustomer, "PassportClerk_", 14, myLine, custNumber);
        Yield();
        givenSSNString(isCustomer, custNumber, "PassportClerk_", 14, myLine);
        Yield();
        recievedSSNString(isCustomer, "PassportClerk_", 14, myLine, custNumber);

        if(customerAttributes[custNumber].likesPicture && customerAttributes[custNumber].applicationIsFiled) {
            /* CL: only determine that customer has app and pic completed by the boolean */
            PrintString("PassportClerk_", 14); PrintNum(myLine); PrintString(" has determined that ", 21);
            PrintCust(isCustomer); PrintNum(custNumber); PrintString(" has both their application and picture completed\n",50);
            clerkStates[myLine] = BUSY;

            numYields = Rand(80, 20);

                clerkMessedUp = Rand(100, 0);
                        if(custNumber > 49){
              clerkMessedUp = 100;
            }
            if(clerkMessedUp <= 5) { /* Send to back of line */
                PrintString("PassportClerk_", 14); PrintNum(myLine); PrintString(": Messed up for ", 16);
                PrintCust(isCustomer); PrintNum(custNumber); PrintString(". Sending customer to back of line.\n", 36);
                customerAttributes[custNumber].clerkMessedUp = true; /*TODO: customer uses this to know which back line to go to*/
            } else {
                PrintString("PassportClerk_", 14); PrintNum(myLine); PrintString(" has recorded ", 14);
                PrintCust(isCustomer); PrintNum(custNumber); PrintString(" passport documentation\n", 24);
                for(i = 0; i < numYields; ++i) {
                    Yield();
                }
                customerAttributes[custNumber].clerkMessedUp = false;
                customerAttributes[custNumber].hasCertification = true;
            }
        } else {
            PrintString("PassportClerk_", 14); PrintNum(myLine); PrintString(" has determined that ", 21);
            PrintCust(isCustomer); PrintNum(custNumber); PrintString(" does not have both their application and picture completed\n", 60);
        }
        clerkSignalsNextCustomer(myLine);
    }
    Exit(0);
}

/*CL: Parameter: int myLine (line number of clerk)
    Summary: logics for cashier
    Return value: void*/

void Cashier() {
    int myLine = GetThreadArgs();
    int numYields, clerkMessedUp, i;
    char personName[50], isCustomer = 1;

    while(!allCustomersAreDone) {
        int custNumber = chooseCustomerFromLine(myLine, "Cashier_", 8);
        if(custNumber >= 50) {
            isCustomer = 0;
        } else {
            isCustomer = 1;
        }
        hasSignaledString(isCustomer, "Cashier_", 8, myLine, custNumber);
        Yield();
        givenSSNString(isCustomer, custNumber, "Cashier_", 8, myLine);
        Yield();
        recievedSSNString(isCustomer, "Cashier_", 8, myLine, custNumber);

        if(customerAttributes[custNumber].hasCertification) {
            PrintString("Cashier_", 8); PrintNum(myLine); PrintString(" has verified that ", 19);
            PrintCust(isCustomer); PrintNum(custNumber); PrintString("has been certified by a PassportClerk\n", 38);
            customerAttributes[custNumber].money -= 100;/* CL: cashier takes money from customer */
            PrintString("Cashier_", 8); PrintNum(myLine); PrintString(" has received the $100 from ", 28);
            PrintCust(isCustomer); PrintNum(custNumber); PrintString("after certification\n", 20);
            PrintCust(isCustomer); PrintNum(custNumber); PrintString(" has given ", 11); PrintString("Cashier_", 8); PrintNum(myLine); PrintString(" $100\n", 6);

            clerkMoney[myLine] += 100;
            clerkStates[myLine] = BUSY;
            numYields = Rand(80, 20);
            /* CL: yields after processing money*/
            for(i = 0; i < numYields; ++i) {
                Yield();
            }
            clerkMessedUp = Rand(100, 0);
            if(custNumber > 49){
                clerkMessedUp = 100;
            }
            if(clerkMessedUp <= 5) { /* Send to back of line*/
                PrintString("Cashier_", 8); PrintNum(myLine); PrintString(": Messed up for ", 16);
                PrintCust(isCustomer); PrintNum(custNumber);
                PrintString(". Sending customer to back of line.\n", 36);
                customerAttributes[custNumber].clerkMessedUp = true; /* TODO: customer uses this to know which back line to go to*/
            } else {
                PrintString("Cashier_", 8); PrintNum(myLine); PrintString(" has provided ", 14);
                PrintCust(isCustomer); PrintNum(custNumber); PrintString(" their completed passport\n", 26);
                Yield();
                PrintString("Cashier_", 8); PrintNum(myLine); PrintString(" has recorded that ", 19);
                PrintCust(isCustomer); PrintNum(custNumber); PrintString(" has been given their completed passport\n", 41);
                customerAttributes[custNumber].clerkMessedUp = false;
                customerAttributes[custNumber].isDone = true;
            }
        }
        clerkSignalsNextCustomer(myLine);
    }
    Exit(0);
}

/*CL: Parameter: int custNumber
    Summary: logics for customer, includes logic to choose lines to go to and to bribe or not
    Return value: void*/

void Customer() {
    int custNumber = GetThreadArgs();
    int yieldTime, i;
    int bribe = false; /*HUNG: flag to know whether the customer has paid the bribe, can't be arsed to think of a more elegant way of doing this*/
    int myLine = -1;
    int lineSize = 1000;
    int pickedApplication;
    int pickedPicture;
    int totalLineCount;
    struct CustomerAttribute myCustAtt = initCustAttr(custNumber); /*Hung: Creating a CustomerAttribute for each new customer*/

    customerAttributes[custNumber] = myCustAtt;
    while(!customerAttributes[custNumber].isDone) {
        if(senatorLineCount > 0){
            Acquire(outsideLock);
            Wait(outsideLock, outsideLineCV);
            Release(outsideLock);
        }
        Acquire(clerkLineLock); /* CL: acquire lock so that only this customer can access and get into the lines*/

        bribe = false;
        myLine = -1;
        lineSize = 1000;

        if(!customerAttributes[custNumber].applicationIsFiled && !customerAttributes[custNumber].likesPicture) { /* check conditions if application and picture are done*/
            pickedApplication = Rand(2, 0);
            pickedPicture = !pickedApplication;
        } else {
            pickedApplication = true;
            pickedPicture = true;
        }
        for(i = 0; i < clerkCount; i++) {
            totalLineCount = clerkLineCount[i] + clerkBribeLineCount[i];

            /* CL: if else pairs for customer to choose clerk based on their attributes*/
            if(pickedApplication &&
                !customerAttributes[custNumber].applicationIsFiled &&
                !customerAttributes[custNumber].hasCertification &&
                !customerAttributes[custNumber].isDone &&
                my_strcmp(clerkTypes[i], "ApplicationClerk", clerkTypesLengths[i])) {
                if(totalLineCount < lineSize) {
                    myLine = i;
                    lineSize = totalLineCount;
                }
            } else if(pickedPicture &&
                      !customerAttributes[custNumber].likesPicture &&
                      !customerAttributes[custNumber].hasCertification &&
                      !customerAttributes[custNumber].isDone &&
                      my_strcmp(clerkTypes[i], "PictureClerk", clerkTypesLengths[i])) {
                if(totalLineCount < lineSize) {
                    myLine = i;
                    lineSize = totalLineCount;
                }
            } else if(customerAttributes[custNumber].applicationIsFiled &&
                      customerAttributes[custNumber].likesPicture &&
                      !customerAttributes[custNumber].hasCertification &&
                      !customerAttributes[custNumber].isDone &&
                      my_strcmp(clerkTypes[i], "PassportClerk", clerkTypesLengths[i])) {
                if(totalLineCount < lineSize) {
                    myLine = i;
                    lineSize = totalLineCount;
                }
            } else if(customerAttributes[custNumber].applicationIsFiled &&
                      customerAttributes[custNumber].likesPicture &&
                      customerAttributes[custNumber].hasCertification &&
                      !customerAttributes[custNumber].isDone &&
                      my_strcmp(clerkTypes[i], "Cashier", clerkTypesLengths[i])) {
                if(totalLineCount < lineSize) {
                    myLine = i;
                    lineSize = totalLineCount;
                }
            }
        }

        PrintString("------------------myLine: ", 26); PrintNum(myLine); PrintNl();
        if(clerkStates[myLine] != AVAILABLE ) { /*clerkStates[myLine] == BUSY*/
            /*I must wait in line*/
            if(customerAttributes[custNumber].money > 100){
                PrintString("Customer_", 9); PrintNum(custNumber); PrintString(" has gotten in bribe line for ", 30);
                PrintString(clerkTypes[myLine], clerkTypesLengths[myLine]); PrintString("_", 1); PrintNum(myLine); PrintNl();
                /* CL: takes bribe money*/
                customerAttributes[custNumber].money -= 500;
                clerkMoney[myLine] += 500;
                clerkBribeLineCount[myLine]++;
                bribe = true;
                Wait(clerkLineLock, clerkBribeLineCV[myLine]);
            } else {
                PrintString("Customer_", 9); PrintNum(custNumber); PrintString(" has gotten in regular line for ", 32);
                PrintString(clerkTypes[myLine], clerkTypesLengths[myLine]); PrintString("_", 1); PrintNum(myLine); PrintNl();
                clerkLineCount[myLine]++;
                Wait(clerkLineLock, clerkLineCV[myLine]);
            }

            totalLineCount = 0;
            for(i = 0; i < clerkCount; ++i) {
                totalLineCount = totalLineCount + clerkBribeLineCount[i] + clerkLineCount[i];
            }

            if(bribe) {
                clerkBribeLineCount[myLine]--;
            } else {
                clerkLineCount[myLine]--;
            }

        } else {
            clerkStates[myLine] = BUSY;
        }
        PrintString("Customer_", 9); PrintNum(custNumber); PrintString(" is trying to release clerkLineLock\n", 36); 
        Release(clerkLineLock);
        PrintString("Customer_", 9); PrintNum(custNumber); PrintString(" released clerkLineLock\n", 24); 

        Acquire(clerkLock[myLine]);
        /*Give my data to my clerk*/
        customerData[myLine] = custNumber;

        Signal(clerkLock[myLine], clerkCV[myLine]);
        /*wait for clerk to do their job*/
        Wait(clerkLock[myLine], clerkCV[myLine]);
       /*Read my data*/
        Signal(clerkLock[myLine], clerkCV[myLine]);
        Release(clerkLock[myLine]);

        if(customerAttributes[custNumber].clerkMessedUp) {
            PrintString("Clerk messed up.  Customer is going to the back of the line.\n", 61);
            yieldTime = Rand(900, 100);
            for(i = 0; i < yieldTime; ++i) {
                Yield();
            }
            customerAttributes[custNumber].clerkMessedUp = false;
        }
    }
    /* CL: CUSTOMER IS DONE! YAY!*/
    PrintString("Customer_", 9); PrintNum(custNumber); PrintString(" is leaving the Passport Office.\n", 33);
    Exit(0);
}

/*CL: Parameter: int custNumber (because we treat senators as customers)
    Summary: logics for senators, includes logic to choose lines to go to, very similar to customer but a lot more conditions and locks
    Return value: void*/

void Senator(){
    int custNumber = GetThreadArgs();

    struct CustomerAttribute myCustAtt = initCustAttr(custNumber); /*Hung: custNumber == 50 to 59*/
    int i, myLine;
    customerAttributes[custNumber] = myCustAtt;

    Acquire(senatorLock);
    if(senatorLineCount > 0){
        senatorLineCount++;
        Wait(senatorLock, senatorLineCV);
        senatorDone = true;
        for(i = 0; i < clerkCount; i++){
            Acquire(clerkLock[i]);
            Acquire(clerkSenatorCVLock[i]);
        }
        for(i = 0; i < clerkCount; i++){
            Signal(clerkLock[i], clerkCV[i]);
            Release(clerkLock[i]);
        }
        for(i = 0; i < clerkCount; i++){

            PrintString("Waiting for clerk ", 18); PrintNum(i); PrintNl();
            Signal(clerkSenatorCVLock[i], clerkSenatorCV[i]);

            Wait(clerkSenatorCVLock[i], clerkSenatorCV[i]);
            PrintString("Getting confirmation from clerk ", 32); PrintNum(i); PrintNl();
        }
        senatorDone=false;
    }else{
        for(i = 0; i < clerkCount; i++){
            Acquire(clerkSenatorCVLock[i]);
        }

        senatorLineCount++;
        Release(senatorLock);
        senatorDone = false;

        for(i = 0; i < clerkCount; i++){
            PrintString("Waiting for clerk ", 18); PrintNum(i); PrintNl();
            Wait(clerkSenatorCVLock[i], clerkSenatorCV[i]);
            PrintString("Getting confirmation from clerk ", 32); PrintNum(i); PrintNl();
        }
    }

    myLine = 0;
    while(!customerAttributes[custNumber].isDone) {
        for(i = 0; i < clerkCount; i++) {
            if(!customerAttributes[custNumber].applicationIsFiled &&
                /*customerAttributes[custNumber].likesPicture &&*/
                !customerAttributes[custNumber].hasCertification &&
                !customerAttributes[custNumber].isDone &&
                my_strcmp(clerkTypes[i], "ApplicationClerk", clerkTypesLengths[i])) {
                PrintString("    ", 4); PrintString("Senator_", 8); PrintNum(custNumber); PrintString("::: ApplicationClerk chosen\n", 28);
                myLine = i;
            } else if(/*customerAttributes[custNumber].applicationIsFiled &&*/
                      !customerAttributes[custNumber].likesPicture &&
                      !customerAttributes[custNumber].hasCertification &&
                      !customerAttributes[custNumber].isDone &&
                      my_strcmp(clerkTypes[i], "PictureClerk", clerkTypesLengths[i])) {
                PrintString("    ", 4); PrintString("Senator_", 8); PrintNum(custNumber); PrintString("::: PictureClerk chosen\n", 24);
                myLine = i;
            } else if(customerAttributes[custNumber].applicationIsFiled &&
                      customerAttributes[custNumber].likesPicture &&
                      !customerAttributes[custNumber].hasCertification &&
                      !customerAttributes[custNumber].isDone &&
                      my_strcmp(clerkTypes[i], "PassportClerk", clerkTypesLengths[i])) {
                PrintString("    ", 4); PrintString("Senator_", 8); PrintNum(custNumber); PrintString("::: PassportClerk chosen\n", 25);
                myLine = i;
            } else if(customerAttributes[custNumber].applicationIsFiled &&
                      customerAttributes[custNumber].likesPicture &&
                      customerAttributes[custNumber].hasCertification &&
                      !customerAttributes[custNumber].isDone &&
                      my_strcmp(clerkTypes[i], "Cashier", clerkTypesLengths[i])) {
                PrintString("    ", 4); PrintString("Senator_", 8); PrintNum(custNumber); PrintString("::: Cashier chosen\n", 19);
                myLine = i;
            }
        }
        Signal(clerkSenatorCVLock[myLine], clerkSenatorCV[myLine]);
        PrintString("Senator_", 8); PrintNum(custNumber); PrintString("has gotten in regular line for ", 31);
        PrintString(clerkTypes[myLine], clerkTypesLengths[myLine]); PrintString("_", 1); PrintNum(myLine); PrintNl();
        Wait(clerkSenatorCVLock[myLine], clerkSenatorCV[myLine]);

        Release(clerkSenatorCVLock[myLine]);
        Acquire(clerkLock[myLine]);
        /*Give my data to my clerk*/
        customerData[myLine] = custNumber;
        Signal(clerkLock[myLine], clerkCV[myLine]);

        /*wait for clerk to do their job*/
        Wait(clerkLock[myLine], clerkCV[myLine]);
        /*Read my data*/
    }

    Acquire(senatorLock);
    senatorLineCount--;
    if(senatorLineCount == 0){
        for(i = 0 ; i < clerkCount ; i++){
            /*Free up clerks that worked with me*/
            Signal(clerkLock[i],clerkCV[i]);
            /*Frees all locks I hold*/
            Release(clerkLock[i]);
            /*Free up clerks that I didn't work with*/
            Signal(clerkSenatorCVLock[i], clerkSenatorCV[i]);
            /*Frees up all clerkSenatorCVLocks*/
            Release(clerkSenatorCVLock[i]);
        }
        Acquire(outsideLock);
        Broadcast(outsideLock, outsideLineCV);
        Release(outsideLock);
        Release(senatorLock);
    }else{
        for(i = 0 ; i < clerkCount ; i++){
            /*Free up clerkLocks for other senator*/
            Release(clerkLock[i]);
            /*Frees up clerkSenatorCVLocks for other senator*/
            Release(clerkSenatorCVLock[i]);
        }
        Signal(senatorLock, senatorLineCV);
        Release(senatorLock);
    }
    PrintString("Senator_", 8); PrintNum(custNumber); PrintString(" is leaving the Passport Office.\n", 33);
    Exit(0);
}

void wakeUpClerks() {
    int i;
    for(i = 0; i < clerkCount; ++i) {
        if(clerkStates[i] == ONBREAK) {
            PrintString("Manager has woken up a ", 23); PrintString(clerkTypes[i], clerkTypesLengths[i]); PrintString("_", 1); PrintNum(i); PrintNl();
            Acquire(breakLock[i]);
            Signal(breakLock[i],breakCV[i]);
            Release(breakLock[i]);
            PrintString(clerkTypes[i], clerkTypesLengths[i]); PrintString("_", 1); PrintNum(i); PrintString(" is coming off break\n", 21);
        }
    }
}

/* CL: Parameter: -
    Summary: print all the money as manager checks the money earned by each clerk
    Return value: void */

void printMoney() {
    int totalMoney = 0;
    int applicationMoney = 0;
    int pictureMoney = 0;
    int passportMoney = 0;
    int cashierMoney = 0;
    int i;
    for(i = 0; i < clerkCount; ++i) {
        if (i < clerkArray[0]){ /*ApplicationClerk index*/
            applicationMoney += clerkMoney[i];
        } else if (i < clerkArray[0] + clerkArray[1]){ /*PictureClerk index*/
            pictureMoney += clerkMoney[i];
        } else if (i < clerkArray[0] + clerkArray[1] + clerkArray[2]){ /*PassportClerk index*/
            passportMoney += clerkMoney[i];
        } else if (i < clerkArray[0] + clerkArray[1] + clerkArray[2] + clerkArray[3]){ /*Cashier index*/
            cashierMoney += clerkMoney[i];
        }
        totalMoney += clerkMoney[i];
    }


    PrintString("Manager has counted a total of ", 31); PrintNum(applicationMoney); PrintString(" for ApplicationClerks\n", 23);
    PrintString("Manager has counted a total of ", 31); PrintNum(pictureMoney); PrintString(" for PictureClerks\n", 19);
    PrintString("Manager has counted a total of ", 31); PrintNum(passportMoney); PrintString(" for PassportClerks\n", 20);
    PrintString("Manager has counted a total of ", 31); PrintNum(cashierMoney); PrintString(" for Cashiers\n", 14);
    PrintString("Manager has counted a total of ", 31); PrintNum(totalMoney); PrintString(" for passport office\n", 21);
}

/*CL: Parameter: -
    Summary: manager code, interrupts are disabled
    Return value: void*/

void Manager() {
    int totalLineCount, i, waitTime;

    do {
        /* IntStatus oldLevel = interrupt->SetLevel(IntOff); disable interrupts*/
        Acquire(outsideLock);
        totalLineCount = 0;
        for(i = 0; i < clerkCount; ++i) {
            totalLineCount += clerkLineCount[i] + clerkBribeLineCount[i];
            if(totalLineCount > 2 || senatorLineCount > 0 ) {
                wakeUpClerks();
                break;
            }
        }
        printMoney();
        /* (void) interrupt->SetLevel(oldLevel); /*restore interrupts*/
        Release(outsideLock);
        waitTime = 100;
        for(i = 0; i < waitTime; ++i) {
            Yield();
        }
    } while(!customersAreAllDone());
    allCustomersAreDone = true;
    wakeUpClerks();
    Exit(0);
}

/*CL: Parameter: -
    Summary: check if customers are done, i.e. are all attributes set to true
    Return value: void*/

int customersAreAllDone() {
    int boolCount = 0, i;
    for(i = 0; i < customerCount; ++i) {

        boolCount += customerAttributes[i].isDone;
    }

    for(i = 0; i < senatorCount; ++i) {
        i += 50;
        boolCount += customerAttributes[i].isDone;
        i -= 50;
    }

    prevTotalBoolCount = currentTotalBoolCount;
    currentTotalBoolCount = 0;
    for(i = 0; i < customerCount; ++i) {
        currentTotalBoolCount += customerAttributes[i].applicationIsFiled + customerAttributes[i].likesPicture + customerAttributes[i].hasCertification + customerAttributes[i].isDone;
    }
    if(prevTotalBoolCount == currentTotalBoolCount) {
        wakeUpClerks();
    }

    if(boolCount == customerCount + senatorCount) {
        return true;
    }
    return false;
}

int main() {
    OpenFileId fd;
    int bytesread, lockNum, condNum;
    char buf[20];

    clerkLineLock = CreateLock("ClerkLineLock", 14, 0);
    clerkSenatorLineCV = CreateCondition("ClerkSenatorLineCV", 19, 0);
    outsideLineCV = CreateCondition("OutsideLineCV", 14, 0);
    outsideLock = CreateLock("OutsideLock", 12, 0);
    senatorLock = CreateLock("SenatorLock", 12, 0);
    senatorLineCV = CreateCondition("SenatorLineCV", 13, 0);    
    Part2();

    Exit(0);
}
