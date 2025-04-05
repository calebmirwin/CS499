/**
  * @file EnhancedABCUAdvisingProgram.cpp
  * @author Caleb Irwin
  * @date 03/30/2025
  * @brief A simple course planning system for ABCU.
  *
  * This program loads, displays, and searches course information using an 
  * AVL-balanced binary search tree. It supports CSV file input, alphabetical 
  * traversal, duplicate detection, and structured exception handling.
  * 
  * CSV lines should be structured:
  * <courseId>,<courseName>,<prerequisite1>,<prerequisite2>,...
  */

#include <iostream>
#include <fstream>
#include <string>
#include <sstream>
#include <limits>
#include <algorithm>
#include <vector>
#include <memory>

using namespace std;

//============================================================================
// Global Definitions
//============================================================================

/**
 * @brief Structure to hold course information.
 * 
 * Each course includes a unique ID, course name, and list of prerequesite 
 * course IDs.
 * 
 */
struct Course {
    string courseId;                /// Unique Course Identifier
    string name;                    /// Full Course Name
    vector<string> prerequisites;   /// List of Course ID's that are prerequisites
};

/**
 * @brief Node structure to hold course information for the Binary Search Tree.
 *
 * Each node stores a Course, the node's height (for AVL balancing), and 
 * pointers to it's child nodes. 
 *
 */
struct Node {
    Course course;
    unique_ptr<Node> left;
    unique_ptr<Node> right;
    int height;

    // Custom Constructor
    Node(Course iCourse) :
        course(move(iCourse)),
        left(nullptr),
        right(nullptr),
        height(0) {}
};

//============================================================================
// Binary Search Tree Class Definition
//============================================================================

/**
 * @class BinarySearchTree
 * @brief AVL-based binary search tree that stores and manages course data.
 *
 * Supports node insertion, search, traversal, deletion, and self-balancing.
 */
class BinarySearchTree {

    private:
        unique_ptr<Node> root;

        // AVL Support & Recursive Helper Methods
        int height(const unique_ptr<Node>& node);
        void updateHeight(unique_ptr<Node>& node);
        unique_ptr<Node> leftRotate(unique_ptr<Node> node);
        unique_ptr<Node> rightRotate(unique_ptr<Node> node);
        unique_ptr<Node> rebalance(unique_ptr<Node> node);
        unique_ptr<Node> addNode(unique_ptr<Node> node, Course course);
        unique_ptr<Node> removeNode(unique_ptr<Node> node, const string& courseId);
        Node* searchNode(Node* node, const string& courseId);

        // Recursive Traversal Methods
        void inOrder(const unique_ptr<Node>& node);
        void postOrder(const unique_ptr<Node>& node);
        void preOrder(const unique_ptr<Node>& node);

    public:
        // Default Constructor
        BinarySearchTree();

        // Traversal Methods
        void InOrder();
        void PostOrder();
        void PreOrder();

        // Core Tree Operation Methods
        void Insert(Course course);
        void Remove(const string& courseId);
        Course Search(string courseId);

        /**
         * @class DuplicateCourseException
         * @brief Exception thrown when attempting to insert a course with a duplicate ID.
         */
        class DuplicateCourseException : public exception {
            string message;
        public:
            explicit DuplicateCourseException(const string& courseId) {
                message = "Duplicate course ID \"" + courseId + "\" was ignored.";
            }
            const char* what() const noexcept override {
                return message.c_str();
            }
        };
};

/**
 * @brief Default constructor for BinarySearchTree.
 */
BinarySearchTree::BinarySearchTree() {
    root = nullptr;
}

//============================================================================
// Private Class Methods
//============================================================================

/**
 * @brief Returns the height of a node in the tree.
 *
 * Used for AVL balancing. If the node is null, returns -1.
 *
 * @param node Unique pointer to the node.
 * @return Height of the node, or -1 if null.
 */
int BinarySearchTree::height(const unique_ptr<Node>& node) {
    return node ? node->height : -1;
}

/**
 * @brief Updates the height of a node based on its children.
 *
 * Called after modifications to the subtree (e.g., insert, remove).
 *
 * @param node Unique pointer to the node.
 */
void BinarySearchTree::updateHeight(unique_ptr<Node>& node) {
    if (node) {
        node->height = max(height(node->left), height(node->right)) + 1;
    }
}

/**
 * @brief Performs in-order traversal of the tree.
 *
 * Visits nodes in ascending order of course ID.
 *
 * @param node Pointer to the current node (recursive).
 */
void BinarySearchTree::inOrder(const unique_ptr<Node>& node) {
    if (node) {
        inOrder(node->left);
        cout << node->course.courseId << ", " << node->course.name << endl;
        inOrder(node->right);
    }
}

/**
 * @brief Performs post-order traversal of the tree.
 *
 * Visits left and right children before processing the node.
 *
 * @param node Pointer to the current node (recursive).
 */
void BinarySearchTree::postOrder(const unique_ptr<Node>& node) {
    if (node) {
        postOrder(node->left);
        postOrder(node->right);
        cout << node->course.courseId << ", " << node->course.name << endl;
    }
}

/**
 * @brief Performs pre-order traversal of the tree.
 *
 * Visits node before processing its left and right children.
 *
 * @param node Pointer to the current node (recursive).
 */
void BinarySearchTree::preOrder(const unique_ptr<Node>& node) {
    if (node) {
        cout << node->course.courseId << ", " << node->course.name << endl;
        preOrder(node->left);
        preOrder(node->right);
    }
}

/**
 * @brief Performs a left rotation on the subtree rooted at node.
 *
 * Used for rebalancing AVL trees when right-heavy.
 *
 * @param node Root of the unbalanced subtree.
 * @return New root after rotation.
 */
unique_ptr<Node> BinarySearchTree::leftRotate(unique_ptr<Node> node) {
    unique_ptr<Node> rightChild = move(node->right);
    node->right = move(rightChild->left);
    rightChild->left = move(node);

    updateHeight(rightChild->left);
    updateHeight(rightChild);

    return rightChild;
}

/**
 * @brief Performs a right rotation on the subtree rooted at node.
 *
 * Used for rebalancing AVL trees when left-heavy.
 *
 * @param node Root of the unbalanced subtree.
 * @return New root after rotation.
 */
unique_ptr<Node> BinarySearchTree::rightRotate(unique_ptr<Node> node) {
    unique_ptr<Node> leftChild = move(node->left);
    node->left = move(leftChild->right);
    leftChild->right = move(node);

    updateHeight(leftChild->right);
    updateHeight(leftChild);

    return leftChild;
}


/**
 * @brief Rebalances a node if it has become unbalanced.
 *
 * Handles left-left, left-right, right-right, and right-left cases.
 *
 * @param node The subtree root to check and rebalance.
 * @return Root of the balanced subtree.
 */
unique_ptr<Node> BinarySearchTree::rebalance(unique_ptr<Node> node) {
    if (!node) {
        return node;
    }

    updateHeight(node);
    int balance = height(node->left) - height(node->right);

    // Left Heavy
    if (balance > 1) {
        // Left-Left
        if (height(node->left->left) >= height(node->left->right)) {
            return rightRotate(move(node));
        }
        // Left-Right
        else {
            node->left = leftRotate(move(node->left));
            return rightRotate(move(node));
        }
    }
    // Right Heavy
    else if (balance < -1) {
        // Right-Right
        if (height(node->right->right) >= height(node->right->left)) {
            return leftRotate(move(node));
        }
        // Right-Left
        else {
            node->right = rightRotate(move(node->right));
            return leftRotate(move(node));
        }
    }

    return node;
} 

/**
 * @brief Recursively adds a course to the tree.
 *
 * Maintains BST ordering and AVL balance.
 *
 * @param node The current node.
 * @param course The course to add.
 * @return New root of the modified subtree.
 */
unique_ptr<Node> BinarySearchTree::addNode(unique_ptr<Node> node, Course course) {
    // If node is empty
    if (!node) {
        return make_unique<Node>(course);
    }
    // If courseId is smaller than course.courseId, add to left.
    if (course.courseId < node->course.courseId) {
        node->left = addNode(move(node->left), course);
    }
    // Else, add to right.
    else {
        node->right = addNode(move(node->right), course);
    }
    // Rebalance the tree
    return rebalance(move(node));
}

/**
 * @brief Recursively removes a course from the tree.
 *
 * Handles cases for nodes with 0, 1, or 2 children.
 *
 * @param node The current node.
 * @param courseId The ID of the course to remove.
 * @return New root of the modified subtree.
 */
unique_ptr<Node> BinarySearchTree::removeNode(unique_ptr<Node> node, const string& courseId) {
    if (!node) {
        return node;
    }
    if (courseId < node->course.courseId) {
        node->left = removeNode(move(node->left), courseId);
    }
    else if (courseId > node->course.courseId) {
        node->right = removeNode(move(node->right), courseId);
    }
    else {
        // If No Left Child Nodes
        if (!node->left) {
            return move(node->right);
        }

        // If No Right Child Nodes
        if (!node->right) {
            return move(node->left);
        }

        // Two Child Nodes
        Node* minLargerNode = node->right.get();
        while (minLargerNode->left) {
            minLargerNode = minLargerNode->left.get();
        }
        node->course = minLargerNode->course;
        node->right = removeNode(move(node->right), minLargerNode->course.courseId);
    }
    // Rebalance the tree
    return rebalance(move(node));
}

/**
 * @brief Recursively searches for a course in the tree.
 *
 * Performs binary search based on course ID.
 *
 * @param node The current node.
 * @param courseId The course ID to search for.
 * @return Pointer to the found node or nullptr.
 */
Node* BinarySearchTree::searchNode(Node* node, const string& courseId) {
    if (node == nullptr || node->course.courseId == courseId) {
        return node;
    }
    if (courseId < node->course.courseId) {
        return searchNode(node->left.get(), courseId);
    }
    else {
        return searchNode(node->right.get(), courseId);
    }
}

//============================================================================
// Public Class Methods
//============================================================================

/**
 * @brief Traverses the entire tree in in-order sequence.
 *
 * Calls the recursive inOrder() starting at the root.
 */
void BinarySearchTree::InOrder() {
    inOrder(root);
}

/**
 * @brief Traverses the entire tree in post-order sequence.
 *
 * Calls the recursive postOrder() starting at the root.
 */
void BinarySearchTree::PostOrder() {
    postOrder(root);
}

/**
 * @brief Traverses the entire tree in pre-order sequence.
 *
 * Calls the recursive preOrder() starting at the root.
 */
void BinarySearchTree::PreOrder() {
    preOrder(root);
}

/**
 * @brief Inserts a course into the tree.
 *
 * Throws a DuplicateCourseException if the course ID already exists.
 *
 * @param course The course to insert.
 * @throws DuplicateCourseException if the course ID is already in the tree.
 */
void BinarySearchTree::Insert(Course course) {
    if (Search(course.courseId).courseId.empty()) {
        root = addNode(move(root), course);
    }
    else {
        throw DuplicateCourseException(course.courseId);
    }
}

/**
 * @brief Removes a course from the tree.
 *
 * @param courseId The ID of the course to remove.
 */
void BinarySearchTree::Remove(const string& courseId) {
    root = removeNode(move(root), courseId);
}

/**
 * @brief Searches for a course in the tree.
 *
 * @param courseId The ID of the course to search for.
 * @return The course if found, or an empty Course object if not.
 */
Course BinarySearchTree::Search(string courseId) {
    Node* result = searchNode(root.get(), courseId);
    return result ? result->course : Course();
}


//============================================================================
// Static Methods for Testing
//============================================================================

/**
 * @brief Displays a single course and its prerequisites.
 *
 * Outputs the course ID, name, and all prerequisites to standard output.
 *
 * @param course The course to display.
 */
void displayCourse(Course course) {
    cout << course.courseId << ", " << course.name << endl;

    //If there are no prerequisites
    if (course.prerequisites.empty()) {
        cout << "Prerequisites: None";
    }
    //Else
    else {
        cout << "Prerequisites: ";
        //For Each Prerequisite
        for (int i = 0; i < course.prerequisites.size(); i++) {
            //Display Prerequisite
            cout << course.prerequisites.at(i);
            //If more than one prerequisite in the vector and not the last element
            if (course.prerequisites.size() > 1 && course.prerequisites.size() - 1 != i) {
                cout << ", ";
            }
        }
    }
    cout << endl;
}



/**
 * @brief Splits a CSV line into tokens based on a delimiter.
 *
 * @param line The line to split.
 * @param delimiter The delimiter character (e.g., ',').
 * @return A vector of split elements.
 */
vector<string> splitLine(const string& line, char delimiter) {
    vector<string> splitLine;
    stringstream ssLine(line);
    string lineElement;

    while (getline(ssLine, lineElement, delimiter)) {
        splitLine.push_back(lineElement);
    }
    return splitLine;
}

/**
 * @brief Loads course data from a CSV file into a binary search tree.
 *
 * Validates input, skips malformed lines, and detects duplicates.
 *
 * @param filePath Path to the CSV file.
 * @param courseList The tree to populate.
 * @return Number of invalid lines skipped, or -1 if a fatal error occurred.
 */
int loadCourses(string filePath, unique_ptr<BinarySearchTree>& courseList) {
    string line;
    vector<string> courseInfo;
    int duplicateCount = 0;
    int errorCount = 0;

    cout << "Loading file " << filePath << endl;

    try {
        ifstream inCourseFS(filePath);

        if (!inCourseFS.is_open()) {
            throw runtime_error("Unable to open file: " + filePath);
        }

        while (getline(inCourseFS, line)) {
            courseInfo = splitLine(line, ',');

            if (courseInfo.size() < 2) {
                ++errorCount;
                continue; // Skip malformed line
            }

            Course course;
            course.courseId = courseInfo[0];
            course.name = courseInfo[1];

            for (size_t i = 2; i < courseInfo.size(); ++i) {
                course.prerequisites.push_back(courseInfo[i]);
            }

            try {
                courseList->Insert(course);
            } catch (const BinarySearchTree::DuplicateCourseException& ex) {
                ++duplicateCount;
                cout << "Warning: " << ex.what() << endl;
            }
        }

        // Stream failed before EOF (e.g., corrupted input)
        if (!inCourseFS.eof() && inCourseFS.fail()) {
            throw runtime_error("Data input failure before reaching the end of file.");
        }

        inCourseFS.close();

        // Display non-fatal error summary
        if (errorCount > 0) {
            cout << "Warning: " << errorCount << " line(s) contained invalid course data and were skipped." << endl;
        }

        // Display number of duplicate courses ignored
        if (duplicateCount > 0)
            cout << "Ignored " << duplicateCount << " duplicate course(s)." << endl;

    } catch (const exception& ex) {
        cerr << "Fatal error while loading courses: " << ex.what() << endl;
        return -1;
    }

    return errorCount;
}

//============================================================================
// Main Method
//============================================================================

/**
 * @brief Main entry point for ABCU Course Planner.
 *
 * Supports interactive command-line interface menu for loading, viewing, and 
 * searching course data.
 *
 * @param argc Argument count.
 * @param argv Argument vector (supports file path and course ID).
 * @return Exit status code.
 */
int main(int argc, char* argv[]) {

    // Initialize Variables
    string filePath, courseId;

    // Process command line arguments
    switch (argc) {
    case 2:
        filePath = argv[1];
        courseId = "CSCI400";
        break;
    case 3:
        filePath = argv[1];
        courseId = argv[2];
        break;
    default:
        filePath = "ABCU_Advising_Program_Input_Extended.csv";
    }

    // Define a binary search tree to hold all courses
    unique_ptr<BinarySearchTree> courseList = make_unique<BinarySearchTree>();
    Course course;

    cout << "Welcome to the course planner." << endl;

    int choice = 0;
    while (choice != 9) {
        cout << endl;
        cout << "  1. Load Courses" << endl;
        cout << "  2. Display All Courses" << endl;
        cout << "  3. Find Course" << endl;
        cout << "  9. Exit" << endl;
        cout << endl;
        cout << "What would you like to do? ";
        cin >> choice;


        switch (choice) {
        case 1:
            cout << endl; // Empty line for readability

            // Complete the method call to load the courses
            loadCourses(filePath, courseList);

            break;

        case 2:
            cout << endl; // Empty line for readability
            cout << "Course list:" << endl;
            cout << endl; // Empty line for readability

            courseList->InOrder();

            break;

        case 3:
            cout << endl; // Empty line for readability
            cout << "What course do you want to know about? ";
            cin.ignore();

            getline(cin, courseId);

            transform(courseId.begin(), courseId.end(), courseId.begin(), ::toupper);

            cout << endl; // Empty line for readability

            course = courseList->Search(courseId);

            // If course is found
            if (!course.courseId.empty()) {
                displayCourse(course);// Print Course Information
            }
            //Else
            else {
                cout << "Course ID " << courseId << " not found." << endl;
            }

            break;

        case 9:
            break;

        default:
            cout << "That entry is not a valid option." << endl;
            cin.clear(); // Clear cin error flag
            cin.ignore(numeric_limits<streamsize>::max(), '\n');// Ignore Additional Input
        }
    }

    cout << "Thank you for using the course planner!" << endl;

    return 0;
}
