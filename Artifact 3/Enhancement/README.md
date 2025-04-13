# Enhanced Investment App

## Summary

This project enhancement implements an investment app with a database, authentication, and role-based access controls using oython and MongoDB. It allows users to login, and depending on role privileges, the can view client data, edits clients and their investment planns, and control users. 

## Setup Instructions
### Prerequisites
#### Software:
- Python 3.8 or Higher
- MongoDB (Local or Remote)
- MongoDB Command Line Database Tools
- pymongo Package
- bcrypt Package

## MongoDB Setup
Seed files with data in JSON format are provided. Utilize the "mongoimport" commmand to seed the data.
```bash
mongoimport --db=investmentApp --collection=users --file=users.json --jsonArray
mongoimport --db=investmentApp --collection=clients --file=clients.json --jsonArray
```
Also provided is a simple hash generator to create hashes for testing purposes. 

## Usage
In order to tun this program, start by cloning this repository (or downloading the program). Then open the enhancedInvestmentApp.py and configure the MongoDB connection details and save the file. Ensure the database is seeded correctly and then run the application. Login using a valid username and password. 

The default passwords are the same as the account names for each specific role (admin, manager, advisor).