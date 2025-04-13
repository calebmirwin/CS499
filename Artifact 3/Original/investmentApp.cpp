#include <string>
#include <limits>
#include <sstream>
#include <iostream>
using namespace std;

//Added global constants
const int MAX_CLIENTS = 5;
const int MIN_CLIENTS = 1;
const int MAX_SERVICE = 2;
const int MIN_SERVICE = 1;
const int MIN_MENU_OPTIONS = 1;
const int MAX_MENU_OPTIONS = 3;


/* - Security Vulnerability -
 * All of the customer data is stored in global variables.
 * This is a security vulnerability because it allows
 * anyone with access to the program to view and modify
 * the data. This could lead to data breaches and
 * unauthorized access to sensitive information.
 * This could also lead to data corruption and
 * other unexpected behavior.
 * 
 * Recommendation:
 * Use a database or other secure storage system to store
 * customer data. This will help to protect the data
 * from unauthorized access and corruption.
 * It will also help to ensure that the data is
 * consistent and accurate. 
 * 
 * - Security Vulnerability -
 * The password was originally stored in plain text.
 * Although its been adjusted to a hash, this method isn't
 * considered cryptogrtaphically secure.abort
 * 
 * Recommendation:abort
 * Implement Secure cryptographic hashing and salting of
 * passwords.
 */
string name1 = "Bob Jones";
string name2 = "Sarah Davis";
string name3 = "Amy Friendly";
string name4 = "Johnny Smith";
string name5 = "Carol Spears";
int num1 = 1;
int num2 = 2;
int num3 = 1;
int num4 = 1;
int num5 = 2;
string passwordHash = "10089081994332581363"; // ADDED Hashed Password, FIXES SECURITY VULNERABILITY


/* - Security Vulnerability -
 * All of the program variables are stored in global variables.
 * This is a security vulnerability because it allows
 * anyone with access to the program to view and modify
 * the data. This could lead to data breaches and
 * unauthorized access to sensitive information.
 * This could also lead to data corruption and
 * other unexpected behavior.
 *
 * Recommendation:
 * Use local variables or class members to store
 * program data. This will help to protect the data
 * from unauthorized access and corruption.
 * It will also help to ensure that the data is
 * consistent and accurate.
 */

 // Moved Global Variables Inside Functions

/* Added Function to check if Valid Integer
 * Returns true if valid. 
 */
bool checkValidInt(int& num) {
  string input;
  getline(cin, input);
  stringstream ss(input);

  if (ss >> num && ss.eof()) {
    return true;
  } else {
    cout << "Invalid input. Please enter a valid integer." << endl;
    return false;
  }
}


/* Added Function to hash password
 * Returns hashed password 
 */
string hashPassword(const string &password) {
  hash<string> hasher;
  size_t hashedValue = hasher(password);
  return to_string(hashedValue);
}


/* Function to print customer info.
 * This function uses the hard-coded customer data and isnt't
 * scalable.
 */
void DisplayInfo(){
  cout << "  Client's Name    Service Selected (1 = Brokerage, 2 = Retirement)" << endl;
  cout << "1. " << name1 << " selected option " << num1 << endl;
  cout << "2. " << name2 << " selected option " << num2 << endl;
  cout << "3. " << name3 << " selected option " << num3 << endl;
  cout << "4. " << name4 << " selected option " << num4 << endl;
  cout << "5. " << name5 << " selected option " << num5 << endl;
}


/* - Security Vulnerability -
 * The program does not have any input validation.
 * This is a security vulnerability because it allows
 * the user to enter invalid data. This could lead to
 * data corruption , exploitation, and other unexpected 
 * behavior.
 *
 * Recommendation:
 * Add input validation to the program. This will help
 * to ensure that the data entered by the user is valid
 * and consistent. It will also help to prevent
 * data corruption and other unexpected behavior.
 */
void ChangeCustomerChoice(){
  int changechoice; // Moved Global Variable, FIXES SECURITY VULNERABILITY
  int newservice; // Moved Global Variable, FIXES SECURITY VULNERABILITY

  do {
    cout << "Enter the number of the client that you wish to change\n";
  } while (!checkValidInt(changechoice) || changechoice < MIN_CLIENTS || changechoice > MAX_CLIENTS); // ADDED Logic for checking client selection, FIXES SECURITY VULNERABILITY
  
  do {
    cout << "Please enter the client's new service choice (1 = Brokerage, 2 = Retirement)\n";
  } while (!checkValidInt(newservice) | newservice < MIN_SERVICE || newservice > MAX_SERVICE); // ADDED Logic for checking sevice selection, FIXES SECURITY VULNERABILITY

  if (changechoice == 1) {
    num1 = newservice;
  }
  else if (changechoice == 2) {
    num2 = newservice;
  }
  else if (changechoice == 3) {
    num3 = newservice;
  }
  else if (changechoice == 4) {
    num4 = newservice;
  }
  else if (changechoice == 5) {
    num5 = newservice;
  }
}


/* - Security Vulnerability -
 * The username is not used to authenticate login.
 * This is a security vulnerability because it allows
 * the user to enter any username and provided they know 
 * the password they can access the program.
 * 
 * Recommendation:
 * Use the username to authenticate login. This will help
 * to ensure that only authorized users can access the
 * program.
 * 
 * - Security Vulnerability -
 * The password is hard-coded and not encrypted.
 * This is a security vulnerability because it allows anyone
 * with access to the program to know the password.
 * 
 * Recommendation:
 * Hash (and salt) the password and store it securely in the 
 * same database recommended for customer data.
 * 
 * - Security Vulnerability -
 * The username and password are not validated on input.
 * This is a security vulnerability because it allows
 * the user to enter invalid data. This could lead to
 * data corruption , exploitation, and other unexpected 
 * behavior.
 * 
 * Recommendation:
 * Add input validation to the program. This will help
 * to ensure that the data entered by the user is valid
 * and consistent. It will also help to prevent
 * data corruption and other unexpected behavior.
 */
int CheckUserPermissionAccess(){
  string username; // Moved Global Variable, FIXES SECURITY VULNERABILITY
  string password;
  cout << "Enter your username: \n";
  cin >> username;
  cin.ignore(numeric_limits<streamsize>::max(), '\n');
  cout << "Enter your password: \n";
  cin >> password;
  cin.ignore(numeric_limits<streamsize>::max(), '\n');
    
  if (hashPassword(password) == passwordHash) { // ADDED Password Hashing, FIXES SECURITY VULNERABILITY
    return 1;
  } else {
    return 2;
  }
}


int main(){
  int choice; // Moved Global Variable, FIXES SECURITY VULNERABILITY
  int answer; // Moved Global Variable, FIXES SECURITY VULNERABILITY

  cout << "Irwin, Caleb | CS410 | Project 2" << endl;

  cout << "Hello! Welcome to our Investment Company\n";
  do {
    answer = CheckUserPermissionAccess();
    if (answer != 1) {
      cout << "Invalid Password. Please try again\n";
    }
  } while (answer != 1);
    
  do {
    cout << "What would you like to do?\n";
    cout << "DISPLAY the client list (enter 1)\n";
    cout << "CHANGE a client's choice (enter 2)\n";
    cout << "Exit the program.. (enter 3)\n";

    do {
    } while (!checkValidInt(choice) | choice < MIN_MENU_OPTIONS || choice >  MAX_MENU_OPTIONS); // ADDED Logic for checking sevice selection, FIXES SECURITY VULNERABILITY

    cout << "You chose " << choice << endl;
        
    if (choice == 1) {
      DisplayInfo();
    } else if (choice == 2) {
      ChangeCustomerChoice();
    }
  } while (choice != 3);
    
    return 0;
}
