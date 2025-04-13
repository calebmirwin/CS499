"""
FILE: hashGenerator.py
AUTHOR: Caleb Irwin
Date: 04/06/2025

Description:
    Simple program for generating a bcrypt hash of a password for testing and database creation.
"""

import bcrypt

def generate_bcrypt_hash():
    password = input("Enter password to hash: ").encode()
    hashed = bcrypt.hashpw(password, bcrypt.gensalt())
    print(f"Hashed password: {hashed.decode()}")

if __name__ == "__main__":
    generate_bcrypt_hash()
