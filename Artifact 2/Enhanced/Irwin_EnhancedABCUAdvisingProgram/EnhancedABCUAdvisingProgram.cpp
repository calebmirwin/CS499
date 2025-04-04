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
    Node* left;
    Node* right;
    int height;

    //Default constructor
    Node() {
        height = -1;
        left = nullptr;
        right = nullptr;
    }

    //Initialize with a course
    Node(Course iCourse) :
        Node() {
        course = iCourse;
    }
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
        Node* root;

        int height(Node* node);
        void inOrder(Node* node);
        void postOrder(Node* node);
        void preOrder(Node* node);
        void updateHeight(Node* node);
        void destroyTree(Node* node);
        Node* addNode(Node* node, Course course);
        Node* removeNode(Node* node, string courseId);
        Node* leftRotate(Node* node);
        Node* rightRotate(Node* node);
        Node* rebalance(Node* node);

    public:
        BinarySearchTree();
        virtual ~BinarySearchTree();
        void InOrder();
        void PostOrder();
        void PreOrder();
        void Insert(Course course);
        void Remove(string courseId);
        Course Search(string courseId);
};

/**
 * ======== Default Constructor ========
 */
BinarySearchTree::BinarySearchTree() {
    root = nullptr;
}

/**
 * ======== Destructor ========
 */
BinarySearchTree::~BinarySearchTree() {
    destroyTree(root);
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
int BinarySearchTree::height(Node* node) {
    return node ? node->height : -1;
}


/**
 * ======== inOrder ========
 * Traverse the BST in order, outputing the nodes in order (recursive).
 *
 * @param node Current node in tree
 */
void BinarySearchTree::inOrder(Node* node) {
    //If node is not equal to null ptr
    if (node != nullptr) {
        //InOrder left
        inOrder(node->left);
        //Output courseID, name, amount, fund
        cout << node->course.courseId << ", " << node->course.name << endl;
        //InOrder right
        inOrder(node->right);
    }
}


/**
 * ======== postOrder ========
 * Traverse the BST in post-order, outputing the nodes in post-order (recursive).
 *
 * @param node Current node in tree
 */
void BinarySearchTree::postOrder(Node* node) {
    //If node is not equal to null ptr
    if (node != nullptr) {
        //postOrder left
        postOrder(node->left);
        //postOrder right
        postOrder(node->right);
        //Output courseID, name, amount, fund
        cout << node->course.courseId << ", " << node->course.name << endl;
    }

}


/**
 * ======== preOrder ========
 * Traverse the BST in pre-order, outputing the nodes in pre-order (recursive).
 *
 * @param node Current node in tree
 */
void BinarySearchTree::preOrder(Node* node) {
    //If node is not equal to null ptr
    if (node != nullptr) {
        //Output courseID, name, amount, fund
        cout << node->course.courseId << ", " << node->course.name << endl;
        //postOrder left
        preOrder(node->left);
        //postOrder right
        preOrder(node->right);
    }
}


/**
 * ======== updateHeight ========
 * Traverse the BST in pre-order, outputing the nodes in pre-order (recursive).
 *
 * @param node Current node in tree
 */
void BinarySearchTree::updateHeight(Node* node) {
    if (node != nullptr) {
        node->height = max(height(node->left), height(node->right)) + 1;
    }
}


/**
 * ======== destroyTree ========
 * Traverse the BST in pre-order, outputing the nodes in pre-order (recursive).
 *
 * @param node Current node in tree
 */
void BinarySearchTree::destroyTree(Node* node) {
    if (node != nullptr) {
        destroyTree(node->left);
        destroyTree(node->right);
        delete node;
    }
}


/**
 * ======== addNode ========
 * Add a course to some node (recursive).
 *
 * @param node Current node in tree
 * @param course Course Structure to be added
 */
Node* BinarySearchTree::addNode(Node* node, Course course) {
    // If node is is empty
    if (node == nullptr) {
        return new Node(course);
    }
    // If courseId is smaller than course.courseId, add to left.
    if (course.courseId < node->course.courseId) {
        node->left = addNode(node->left, course);
    }
    // Else, add to right.
    else {
        node->right = addNode(node->right, course);
    }
    // Rebalance the tree
    return rebalance(node);
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
Node* BinarySearchTree::removeNode(Node* node, string courseId) {
    if (node == nullptr) {
        return node;
    }
    //Go left
    if (node->course.courseId > courseId) {
        node->left = removeNode(node->left, courseId);
    }
    //Go right
    else if (node->course.courseId < courseId) {
        node->right = removeNode(node->right, courseId);
    }
    else {
        //If Node is Leaf Node
        if (node->left == nullptr && node->right == nullptr) {
            delete node;
            node = nullptr;
        }
        //Node has 1 child node to the left
        else if (node->left != nullptr && node->right == nullptr) {
            Node* tempNode = node;
            node = node->left;
            delete tempNode;
        }
        //Node has 1 child node to the right
        else if (node->left == nullptr && node->right != nullptr) {
            Node* tempNode = node;
            node = node->right;
            delete tempNode;
        }
        //Node has 2 child nodes
        else {
            Node* tempNode = node->right;
            while (tempNode->left != nullptr) {
                tempNode = tempNode->left;
            }
            node->course = tempNode->course;
            node->right = removeNode(node->right, tempNode->course.courseId);
        }
    }
    node = rebalance(node);

    return node;
}


/**
 * ======== leftRotate ========
 * 
 *
 * @param node Current node in tree
 * @return 
 */
Node* BinarySearchTree::leftRotate(Node* node) {
    Node* rightChild = node->right;
    Node* temp = rightChild->left;

    rightChild->left = node;
    node->right = temp;

    updateHeight(node);
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
Node* BinarySearchTree::rightRotate(Node* node) {
    Node* leftChild = node->left;
    Node* temp = leftChild->right;

    leftChild->right = node;
    node->left = temp;

    updateHeight(node);
    updateHeight(leftChild);

    return leftChild;
}



/**
 * ======== rebalance ========
 *
 *
 * @param node Current node in tree
 */
Node* BinarySearchTree::rebalance(Node* node) {
    if (node == nullptr) {
        return node;
    }

    updateHeight(node);
    int balance = height(node->left) - height(node->right);

    // Left Heavy
    if (balance > 1) {
        // Left-Left
        if (height(node->left->left) >= height(node->left->right)) {
            return rightRotate(node);
        }
        // Left-Right
        else {
            node->left = leftRotate(node->left);
            return rightRotate(node);
        }
    }
    // Right Heavy
    else if (balance < -1) {
        // Right-Right
        if (height(node->right->right) >= height(node->right->left)) {
            return leftRotate(node);
        }
        // Right-Left
        else {
            node->right = rightRotate(node->right);
            return leftRotate(node);
        }
    }

    return node;
} 


//============================================================================
// Public Class Methods
//============================================================================

/**
 * ======== InOrder ========
 * Traverse the BST in order, starting at the root, outputing the nodes in order.
 */
void BinarySearchTree::InOrder() {
    //Call inOrder fuction and pass root
    inOrder(root);
}


/**
 * ======== PostOrder ========
 * Traverse the BST in post-order, starting at the root, outputing the nodes in post-order.
 */
void BinarySearchTree::PostOrder() {
    //Call postOrder fuction and pass root
    postOrder(root);
}


/**
 * ======== PreOrder ========
 * Traverse the BST in pre-order, starting at the root, outputing the nodes in pre-order.
 */
void BinarySearchTree::PreOrder() {
    //Call preOrder fuction and pass root
    preOrder(root);
}


/**
 * ======== Insert ========
 * Insert a course to the BST.
 *
 * @param course Course Structure to be added.
 */
void BinarySearchTree::Insert(Course course) {
    root = addNode(root, course);
}


/**
 * ======== Remove ========
 * Remove a course from the BST.
 *
 * @param courseId Course to be removed.
 */
void BinarySearchTree::Remove(string courseId) {
    //
    root = removeNode(root, courseId);
}


/**
 * ======== Search ========
 * Search for a course in the BST.
 *
 * @param courseId Course to be found
 */
Course BinarySearchTree::Search(string courseId) {
    //Set current node equal to root
    Node* currNode = root;

    //Keep looping downwards until bottom reached or matching courseId found
    while (currNode != nullptr) {
        //If current node matches courseId, return current node course
        if (currNode->course.courseId == courseId) {
            return currNode->course;
        }

        //Else If 
        //Current node courseId is greater than input courseId, go left
        else if (currNode->course.courseId > courseId) {
            currNode = currNode->left;
        }

        //Else
        //Current node courseId is smaller than input courseId, go right
        else {
            currNode = currNode->right;
        }
    }

    //No match found, return empty course
    Course course;
    return course;
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
        cout << endl; //Empty line for readability
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
