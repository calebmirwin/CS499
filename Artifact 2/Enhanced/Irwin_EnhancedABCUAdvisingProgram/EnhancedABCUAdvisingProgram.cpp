/******************************************************************************
 *  FILE: EnhancedABCUAdvisingProgram.cpp
 *  AUTHOR: Caleb Irwin
 *  DATE: 03/30/2025
 *
 *  DESCRIPTION:
 *      Simple class planning program for ABCU. Can load in a comma delimited
 *      file, with one course per line, each containing at least an id and
 *      name, but also could contain prerequesite course information. Can
 *      display entire course list in alphabetical order or search for a
 *      specific course and its information.
 ******************************************************************************/

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

//Structure to hold course information
struct Course {
    string courseId; //Unique Identifier
    string name;
    vector<string> prerequisites;
};

//Structure for tree node
struct Node {
    Course course;
    unique_ptr<Node> left;
    unique_ptr<Node> right;
    int height;

    //Custom Constructor
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
 * ======== BinarySearchTree ========
 * Class containing data members and methods to implement a binary search tree.
 */
class BinarySearchTree {

    private:
        unique_ptr<Node> root;

        int height(const unique_ptr<Node>& node);
        void updateHeight(unique_ptr<Node>& node);
        void inOrder(const unique_ptr<Node>& node);
        void postOrder(const unique_ptr<Node>& node);
        void preOrder(const unique_ptr<Node>& node);
        unique_ptr<Node> leftRotate(unique_ptr<Node> node);
        unique_ptr<Node> rightRotate(unique_ptr<Node> node);
        unique_ptr<Node> rebalance(unique_ptr<Node> node);
        unique_ptr<Node> addNode(unique_ptr<Node> node, Course course);
        unique_ptr<Node> removeNode(unique_ptr<Node> node, const string& courseId);
        Node* searchNode(Node* node, const string& courseId);

    public:
        BinarySearchTree();
        void InOrder();
        void PostOrder();
        void PreOrder();
        void Insert(Course course);
        void Remove(const string& courseId);
        Course Search(string courseId);
};

/**
 * ======== Default Constructor ========
 */
BinarySearchTree::BinarySearchTree() {
    root = nullptr;
}


//============================================================================
// Private Class Methods
//============================================================================

/**
 * ======== height ========
 * returns the height of current node. If node is null, returns -1.
 *
 * @param node Current node in tree
 * @return node->height or -1
 */
int BinarySearchTree::height(const unique_ptr<Node>& node) {
    return node ? node->height : -1;
}


/**
 * ======== updateHeight ========
 * Traverse the BST in pre-order, outputing the nodes in pre-order (recursive).
 *
 * @param node Current node in tree
 */
void BinarySearchTree::updateHeight(unique_ptr<Node>& node) {
    if (node) {
        node->height = max(height(node->left), height(node->right)) + 1;
    }
}


/**
 * ======== inOrder ========
 * Traverse the BST in order, outputing the nodes in order (recursive).
 *
 * @param node Current node in tree
 */
void BinarySearchTree::inOrder(const unique_ptr<Node>& node) {
    if (node) {
        inOrder(node->left);
        cout << node->course.courseId << ", " << node->course.name << endl;
        inOrder(node->right);
    }
}


/**
 * ======== postOrder ========
 * Traverse the BST in post-order, outputing the nodes in post-order (recursive).
 *
 * @param node Current node in tree
 */
void BinarySearchTree::postOrder(const unique_ptr<Node>& node) {
    if (node) {
        postOrder(node->left);
        postOrder(node->right);
        cout << node->course.courseId << ", " << node->course.name << endl;
    }
}


/**
 * ======== preOrder ========
 * Traverse the BST in pre-order, outputing the nodes in pre-order (recursive).
 *
 * @param node Current node in tree
 */
void BinarySearchTree::preOrder(const unique_ptr<Node>& node) {
    if (node) {
        cout << node->course.courseId << ", " << node->course.name << endl;
        preOrder(node->left);
        preOrder(node->right);
    }
}


/**
 * ======== leftRotate ========
 * 
 *
 * @param node Current node in tree
 * @return 
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
 * ======== rightRotate ========
 * 
 *
 * @param node Current node in tree
 * @return 
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
 * ======== rebalance ========
 *
 *
 * @param node Current node in tree
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
 * ======== addNode ========
 * Add a course to some node (recursive).
 *
 * @param node Current node in tree
 * @param course Course Structure to be added
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
 * ======== removeNode ========
 * Remove a course node from the BST (recursive).
 *
 * @param node Current node in tree
 * @param course Course to be removed
 *
 * @return
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
 * ======== searchNode ========
 * Remove a course node from the BST (recursive).
 *
 * @param node Current node in tree
 * @param course Course to be removed
 *
 * @return
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
 * ======== InOrder ========
 * Traverse the BST in order, starting at the root, outputing the nodes in order.
 */
void BinarySearchTree::InOrder() {
    inOrder(root);
}


/**
 * ======== PostOrder ========
 * Traverse the BST in post-order, starting at the root, outputing the nodes in post-order.
 */
void BinarySearchTree::PostOrder() {
    postOrder(root);
}


/**
 * ======== PreOrder ========
 * Traverse the BST in pre-order, starting at the root, outputing the nodes in pre-order.
 */
void BinarySearchTree::PreOrder() {
    preOrder(root);
}


/**
 * ======== Insert ========
 * Insert a course to the BST.
 *
 * @param course Course Structure to be added.
 */
void BinarySearchTree::Insert(Course course) {
    if (Search(course.courseId).courseId.empty()) {
        root = addNode(move(root), course);
    }
    else{
        cout << "Duplicate course ID \"" << course.courseId << "\" ignored." << endl; // FIXME Change to return Value/Object?
    }
}


/**
 * ======== Remove ========
 * Remove a course from the BST.
 *
 * @param courseId Course to be removed.
 */
void BinarySearchTree::Remove(const string& courseId) {
    root = removeNode(move(root), courseId);
}


/**
 * ======== Search ========
 * Search for a course in the BST.
 *
 * @param courseId Course to be found
 */
Course BinarySearchTree::Search(string courseId) {
    Node* result = searchNode(root.get(), courseId);
    return result ? result->course : Course();
}


//============================================================================
// Static Methods for Testing
//============================================================================

/**
 * ======== displayCourse ========
 * Display a single course's information to the console (std::out)
 *
 * @param course Structure containing the course info
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
 * ======== splitLine ========
 * Function to split file lines into appropriate sections deliminated by a character.
 *
 * @param line Line read from file
 * @param delimiter Delimiter seperating data
 * @return splitLine Tokenized data to return
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
 * ======== loadCourses ========
 * Load a file containing courses into a container
 *
 * @param filePath the path to the file to load
 * @param courseList BST to load the course data to
 * @return errorCount number of errors processed while inputing file data
 */
int loadCourses(string filePath, BinarySearchTree* courseList) {
    //Declare Function Variables
    string line;
    vector<string> courseInfo;
    int errorCount = 0;

    cout << "Loading file " << filePath << endl;

    //Attempt to Open File
    ifstream inCourseFS(filePath);

    //If file isn't able to be opened
    if (!inCourseFS.is_open()) {
        cout << "Unable to open file " << filePath << "." << endl;
    }
    // Else
    else {
        // While there are no errors and not edn of file
        while (!inCourseFS.fail() && !inCourseFS.eof()) {
            // Read line from file
            getline(inCourseFS, line);

            // Spilt line into token based on delimiter
            courseInfo = splitLine(line, ',');

            // If line has minimum amount of course information
            if (courseInfo.size() < 2) {
                errorCount += 1;
            }
            // Else, create and store course in structure
            else {
                Course course;
                course.courseId = courseInfo[0];
                course.name = courseInfo[1];

                for (int i = 2; courseInfo.size() > i; i++) {
                    course.prerequisites.push_back(courseInfo[i]);
                }

                courseList->Insert(course);
            }
        }

        // If there were errors while importing the data, but the file was read.
        if (errorCount > 0) {
            cout << "There were " << errorCount << " error(s) while loading the data." << endl;
        }

        // Displays Error Message if Failed to Read Data Before the End of File.
        if (!inCourseFS.eof() || inCourseFS.fail()) {
            cout << "Data input failure before reaching the end of the file." << endl;
        }

        // Closing Input File
        inCourseFS.close();
    }

    return errorCount;
}


/**
 * ======== Main Method ========
 * The one and only main() method
 */
int main(int argc, char* argv[]) {

    //InitializeVariables
    string filePath, courseId;

    //Process command line arguments
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

    //Define a binary search tree to hold all courses
    BinarySearchTree* courseList;
    courseList = new BinarySearchTree();
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
            cout << endl; //Empty line for readability

            // Complete the method call to load the courses
            loadCourses(filePath, courseList);

            break;

        case 2:
            cout << "Course list:" << endl;
            cout << endl; //Empty line for readability

            courseList->InOrder();
            //courseList->PreOrder();
            //courseList->PostOrder();

            break;

        case 3:
            cout << "What course do you want to know about? ";
            cin.ignore();

            getline(cin, courseId);

            transform(courseId.begin(), courseId.end(), courseId.begin(), ::toupper);

            cout << endl; //Empty line for readability

            course = courseList->Search(courseId);

            //If course is found
            if (!course.courseId.empty()) {
                displayCourse(course);//Print Course Information
            }
            //Else
            else {
                cout << "Course Id " << courseId << " not found." << endl;
            }

            break;

        case 9:
            break;

        default:
            cout << "That entry is not a valid option." << endl;
            cin.clear(); //Clear cin error flag
            cin.ignore(numeric_limits<streamsize>::max(), '\n');//Ignore Additional Input
        }
    }

    cout << "Thank you for using the course planner!" << endl;

    return 0;
}
