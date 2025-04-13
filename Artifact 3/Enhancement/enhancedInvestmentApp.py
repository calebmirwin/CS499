"""
FILE: enhancedInvestmentApp.py
AUTHOR: Caleb Irwin
Date: 04/06/2025

Description:
    This is a secure investment application that allows users to view and manage client records
    and user accounts. It includes user authentication, password hashing, and role-based access control (RBAC).
    Once logged in, users can perform actions based on their roles: admin, manager, or advisor.
    This application includes features for creating, updating, and deleting clients and users. Data
    is stored in a MongoDB database in order to keep the application state persistent.
"""

import pymongo
import bcrypt
import getpass

# MongoDB Connection Setup
client = pymongo.MongoClient("mongodb://localhost:27017")
db = client["investmentApp"]
users_col = db["users"]
clients_col = db["clients"]

# Global Constants for RBAC
ROLE_ADMIN = "admin"
ROLE_MANAGER = "manager"
ROLE_ADVISOR = "advisor"

VALID_ROLES = {ROLE_ADMIN, ROLE_MANAGER, ROLE_ADVISOR}

def hash_password(password):
    """
    Hashes a plain-text password using bcrypt.

    Args:
        password (str): The plain-text password.

    Returns:
        bytes: The bcrypt hashed password.
    """
    return bcrypt.hashpw(password.encode(), bcrypt.gensalt())

def verify_password(password, hashed):
    """
    Verifies a password against its hashed version.

    Args:
        password (str): The user-provided password.
        hashed (str or bytes): The stored bcrypt hash.

    Returns:
        bool: True if the password matches, False otherwise.
    """
    if isinstance(hashed, str):
        hashed = hashed.encode("utf-8")
    return bcrypt.checkpw(password.encode(), hashed)

def authenticate():
    """
    Prompts for user login credentials and authenticates against the database.

    Returns:
        dict or None: The authenticated user's info or None if invalid.
    """
    username = input("Username: ")
    password = getpass.getpass("Password: ")
    user = users_col.find_one({"username": username})
    if user and verify_password(password, user["passwordHash"]):
        print(f"Login successful. Role: {user['role']}")
        return {"username": username, "role": user["role"]}
    print("Invalid credentials.")
    return None

def get_string(doc, field):
    """
    Safely retrieves a string field from a MongoDB document.

    Args:
        doc (dict): The document.
        field (str): The key to retrieve.

    Returns:
        str: The value or empty string.
    """
    return doc.get(field, "") if isinstance(doc.get(field, ""), str) else ""

def input_service(prompt="Enter service (1 = Brokerage, 2 = Retirement): "):
    """
    Prompts the user for a valid service selection.

    Args:
        prompt (str): The prompt message.

    Returns:
        int: 1 for Brokerage or 2 for Retirement.
    """
    while True:
        try:
            service = int(input(prompt))
            if service in [1, 2]:
                return service
            else:
                print("Invalid service. Must be 1 or 2.")
        except ValueError:
            print("Please enter a valid number (1 or 2).")

def input_role(prompt="Enter role (admin, manager, advisor): "):
    """
    Prompts the user for a valid user role.

    Args:
        prompt (str): The prompt message.

    Returns:
        str: Validated role string.
    """
    while True:
        role = input(prompt).strip().lower()
        if role in VALID_ROLES:
            return role
        print("Invalid role. Choose from: admin, manager, advisor.")

def confirm_action(prompt):
    """
    Prompts for confirmation before performing an action.

    Args:
        prompt (str): The confirmation message.

    Returns:
        bool: True if confirmed, False otherwise.
    """
    confirmation = input(f"{prompt} (y/n): ").strip().lower()
    return confirmation == "y"

def read_clients():
    """
    Displays all clients in the system.

    Returns:
        None
    """
    print("\nAll Clients:")
    for i, client in enumerate(clients_col.find(), start=1):
        print(f"{i}. {get_string(client, 'name')} - Service: {client.get('service')}")

def lookup_client():
    """
    Searches for a client by name.

    Returns:
        None
    """
    name = input("Enter the client's name to search: ")
    client = clients_col.find_one({"name": name})
    if client:
        print(f"Found: {get_string(client, 'name')} - Service: {client.get('service')}")
    else:
        print("Client not found.")

def create_client(user):
    """
    Creates a new client if the user has sufficient privileges.

    Args:
        user (dict): The authenticated user.

    Returns:
        None
    """
    if user["role"] not in [ROLE_ADMIN, ROLE_MANAGER]:
        print("Access denied.")
        return
    name = input("Enter client name: ")
    service = input_service()
    clients_col.insert_one({"name": name, "service": service})
    print("Client added.")

def update_client(user):
    """
    Updates a client's service selection.

    Args:
        user (dict): The authenticated user.

    Returns:
        None
    """
    if user["role"] not in [ROLE_ADMIN, ROLE_MANAGER]:
        print("Access denied.")
        return
    name = input("Enter client name to update: ")
    new_service = input_service("Enter new service: ")
    result = clients_col.update_one({"name": name}, {"$set": {"service": new_service}})
    print("Client updated." if result.modified_count else "Client not found.")

def delete_client(user):
    """
    Deletes a client after confirmation if user is admin.

    Args:
        user (dict): The authenticated user.

    Returns:
        None
    """
    if user["role"] != ROLE_ADMIN:
        print("Access denied.")
        return
    name = input("Enter client name to delete: ")
    if not confirm_action(f"Are you sure you want to delete client '{name}'?"):
        print("Deletion cancelled.")
        return
    result = clients_col.delete_one({"name": name})
    print("Client deleted." if result.deleted_count else "Client not found.")

def create_user():
    """
    Creates a new user account.

    Returns:
        None
    """
    username = input("Enter new username: ")
    if users_col.find_one({"username": username}):
        print("Username already exists.")
        return
    password = getpass.getpass("Enter password: ")
    role = input_role()
    users_col.insert_one({
        "username": username,
        "passwordHash": hash_password(password),
        "role": role
    })
    print("User created successfully.")

def view_users():
    """
    Displays all users in the system.

    Returns:
        None
    """
    print("\nRegistered Users:")
    for user in users_col.find():
        print(f"- {user['username']} ({user['role']})")

def update_user():
    """
    Updates a user's role.

    Returns:
        None
    """
    username = input("Enter the username to update: ")
    new_role = input_role("Enter new role (admin, manager, advisor): ")
    result = users_col.update_one({"username": username}, {"$set": {"role": new_role}})
    print("User updated." if result.modified_count else "User not found or no change applied.")

def delete_user(current_user):
    """
    Deletes a user account after confirmation (except own account).

    Args:
        current_user (dict): The authenticated user.

    Returns:
        None
    """
    username = input("Enter the username to delete: ")
    if username == current_user["username"]:
        print("You cannot delete your own account.")
        return
    if not confirm_action(f"Are you sure you want to delete user '{username}'?"):
        print("Deletion cancelled.")
        return
    result = users_col.delete_one({"username": username})
    print("User deleted." if result.deleted_count else "User not found.")

def manage_users(current_user):
    """
    Displays and handles the user management menu for admin users.

    Args:
        current_user (dict): The authenticated user.

    Returns:
        None
    """
    while True:
        print("\nUser Management:")
        print("1. Create User")
        print("2. View Users")
        print("3. Update User Role")
        print("4. Delete User")
        print("5. Back to Main Menu")
        choice = input("Enter Selection: ")
        if choice == "1": create_user()
        elif choice == "2": view_users()
        elif choice == "3": update_user()
        elif choice == "4": delete_user(current_user)
        elif choice == "5": break
        else: print("Invalid option.")

def main():
    """
    Entry point for the investment app.

    Handles login and displays role-specific menus.

    Returns:
        None
    """
    while True:
        print("\n1. Login\n2. Exit")
        menu_choice = input("Enter Selection: ")
        if menu_choice == "2":
            break
        user = authenticate()
        if not user:
            continue

        while True:
            # Role-based menu options, ALL ROLES
            print("\nMenu Options:")
            options = {}
            option_num = 1

            options[str(option_num)] = ("Lookup Client", lambda: lookup_client())
            print(f"{option_num}. Lookup Client")
            option_num += 1

            options[str(option_num)] = ("Display All Clients", lambda: read_clients())
            print(f"{option_num}. Display All Clients")
            option_num += 1
            
            # Role-based menu options, ADMIN & MANAGER
            if user["role"] in [ROLE_ADMIN, ROLE_MANAGER]:
                options[str(option_num)] = ("Add Client", lambda: create_client(user))
                print(f"{option_num}. Add Client")
                option_num += 1

                options[str(option_num)] = ("Update Client Service", lambda: update_client(user))
                print(f"{option_num}. Update Client Service")
                option_num += 1

            # Role-based menu options, ADMIN
            if user["role"] == ROLE_ADMIN:
                options[str(option_num)] = ("Delete Client", lambda: delete_client(user))
                print(f"{option_num}. Delete Client")
                option_num += 1

                options[str(option_num)] = ("Manage Users", lambda: manage_users(user))
                print(f"{option_num}. Manage Users")
                option_num += 1

            options[str(option_num)] = ("Logout", lambda: False)
            print(f"{option_num}. Logout")

            choice = input("Enter Selection: ")
            if choice in options:
                if options[choice][1]() is False:
                    break
            else:
                print("Invalid option.")

if __name__ == "__main__":
    main()
