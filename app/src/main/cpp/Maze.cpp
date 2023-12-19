#include "Maze.h"
#include <math.h>
#include <stdlib.h>
#include <list>

using namespace PlanarPhysics;

//--------------------------------- Maze ---------------------------------

Maze::Maze()
{
    this->widthCM = 0.0;
    this->heightCM = 0.0;
}

/*virtual*/ Maze::~Maze()
{
    this->Clear();
}

bool Maze::Generate(double widthCM, double heightCM, double densityCMPerCell)
{
    this->Clear();

    this->widthCM = widthCM;
    this->heightCM = heightCM;

    int rows = ::round(heightCM / densityCMPerCell);
    int cols = ::round(widthCM / densityCMPerCell);

    if(rows <= 0 || cols <= 0)
        return false;

    Node*** matrix = new Node**[rows];
    for(int i = 0; i < rows; i++)
    {
        matrix[i] = new Node*[cols];
        for(int j = 0; j < cols; j++)
        {
            Node* node = new Node();
            matrix[i][j] = node;
            this->nodeArray.push_back(node);
        }
    }

    for(int i = 0; i < rows; i++)
    {
        for(int j = 0; j < cols; j++)
        {
            if(i > 0)
                matrix[i][j]->adjacentNodeArray.push_back(matrix[i - 1][j]);
            else if(i < rows - 1)
                matrix[i][j]->adjacentNodeArray.push_back(matrix[i + 1][j]);
            if(j > 0)
                matrix[i][j]->adjacentNodeArray.push_back(matrix[i][j - 1]);
            else if(j < cols - 1)
                matrix[i][j]->adjacentNodeArray.push_back(matrix[i][j + 1]);
        }
    }

    for(int i = 0; i < rows; i++)
    {
        for(int j = 0; j < cols; j++)
        {
            Node* node = matrix[i][j];
            node->center.x = double(j) * densityCMPerCell + densityCMPerCell / 2.0;
            node->center.y = double(i) * densityCMPerCell + densityCMPerCell / 2.0;
        }
    }

    // Free the memory we used to build the graph.
    for(int i = 0; i < rows; i++)
        delete[] matrix[i];
    delete[] matrix;

    // Go generate the maze graph.
    std::list<Node*> nodeQueue;
    Node* node = this->RandomNode(this->nodeArray);
    nodeQueue.push_back(node);
    node->partOfMaze = true;
    while(nodeQueue.size() > 0)
    {
        // Pull a random node off the queue.  The queue is the periphery of a random BFS.
        node = this->RandomNode(nodeQueue, true);

        // Integrate the node with the rest of the growing maze.
        Node* adjacentNode = nullptr;
        int lastRandom = -1;
        for(int i = 0; i < node->adjacentNodeArray.size(); i++)
        {
            adjacentNode = this->RandomNode(node->adjacentNodeArray, &lastRandom);
            if(adjacentNode->partOfMaze)
            {
                node->connectedNodeArray.push_back(adjacentNode);
                adjacentNode->connectedNodeArray.push_back(node);
                break;
            }
        }

        // Queue up any adjacent nodes not yet part of the maze.
        for(Node* adjacentNode : node->adjacentNodeArray)
        {
            if(!adjacentNode->partOfMaze)
            {
                nodeQueue.push_back(adjacentNode);
                adjacentNode->partOfMaze = true;
            }
        }
    }

    return true;
}

int Maze::RandomNumber(int min, int max)
{
    double alpha = double(rand()) / double(RAND_MAX);
    double minFloat = double(min) - 0.5;
    double maxFloat = double(max) + 0.5;
    double lerp = minFloat + alpha * (maxFloat - minFloat);
    double numberFloat = ::round(lerp);
    int numberInt = (int)numberFloat;
    if(numberInt < min)
        numberInt = min;
    if(numberInt > max)
        numberInt = max;
    return numberInt;
}

Maze::Node* Maze::RandomNode(std::vector<Node*>& nodeArray, int* lastRandom /*= nullptr*/)
{
    if(lastRandom && *lastRandom >= 0)
    {
        *lastRandom = (*lastRandom + 1) % nodeArray.size();
        return nodeArray[*lastRandom];
    }

    int i = this->RandomNumber(0, nodeArray.size() - 1);
    Node* node = nodeArray[i];

    if(lastRandom)
        *lastRandom = i;

    return node;
}

Maze::Node* Maze::RandomNode(std::list<Node*>& nodeList, bool remove)
{
    int i = this->RandomNumber(0, nodeList.size() - 1);
    std::list<Node*>::iterator iter = nodeList.begin();
    while(i > 0)
    {
        iter++;
        i--;
    }

    Node* node = *iter;

    if(remove)
        nodeList.erase(iter);

    return node;
}

void Maze::PopulatePhysicsWorld(PlanarPhysics::Engine* engine) const
{
    // TODO: This is where we generate the walls of the maze in the physics world.
}

void Maze::Clear()
{
    for(Node* node : this->nodeArray)
        delete node;

    this->nodeArray.clear();
}

//--------------------------------- Maze::Node ---------------------------------

Maze::Node::Node()
{
    this->partOfMaze = false;
}

/*virtual*/ Maze::Node::~Node()
{
}