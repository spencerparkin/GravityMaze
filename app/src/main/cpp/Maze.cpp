#include "Maze.h"
#include "MazeObjects/MazeWall.h"
#include "MazeObjects/MazeBall.h"
#include "Engine.h"
#include "Math/Utilities/BoundingBox.h"
#include "Math/GeometricAlgebra/PScalar2D.h"
#include <math.h>
#include <stdlib.h>
#include <list>
#include <set>

using namespace PlanarPhysics;

//--------------------------------- Maze ---------------------------------

Maze::Maze()
{
    this->widthCM = 0.0;
    this->heightCM = 0.0;
    this->cellWidthCM = 0.0;
    this->cellHeightCM = 0.0;
}

/*virtual*/ Maze::~Maze()
{
    this->Clear();
}

bool Maze::Generate(double widthCM, double heightCM, double densityCellsPerCM)
{
    this->Clear();

    this->widthCM = widthCM;
    this->heightCM = heightCM;

    int rows = ::round(heightCM * densityCellsPerCM);
    int cols = ::round(widthCM * densityCellsPerCM);

    if(rows <= 0 || cols <= 0)
        return false;

    this->cellWidthCM = this->widthCM / double(cols);
    this->cellHeightCM = this->heightCM / double(rows);

    Node*** matrix = new Node**[rows];
    for(int i = 0; i < rows; i++)
    {
        matrix[i] = new Node*[cols];
        for(int j = 0; j < cols; j++)
        {
            Node* node = new Node();
            matrix[i][j] = node;
            this->nodeArray.push_back(node);
            sprintf(node->debugName, "%d, %d", i, j);
        }
    }

    for(int i = 0; i < rows; i++)
    {
        for(int j = 0; j < cols; j++)
        {
            if(i > 0)
                matrix[i][j]->adjacentNodeArray.push_back(matrix[i - 1][j]);
            if(i < rows - 1)
                matrix[i][j]->adjacentNodeArray.push_back(matrix[i + 1][j]);
            if(j > 0)
                matrix[i][j]->adjacentNodeArray.push_back(matrix[i][j - 1]);
            if(j < cols - 1)
                matrix[i][j]->adjacentNodeArray.push_back(matrix[i][j + 1]);
        }
    }

    for(int i = 0; i < rows; i++)
    {
        for(int j = 0; j < cols; j++)
        {
            Node* node = matrix[i][j];
            node->center.x = double(j) * this->cellWidthCM + this->cellWidthCM / 2.0;
            node->center.y = double(i) * this->cellHeightCM + this->cellHeightCM / 2.0;
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
    node->queued = true;
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
            if(adjacentNode->integrated)
            {
                node->connectedNodeArray.push_back(adjacentNode);
                adjacentNode->connectedNodeArray.push_back(node);
                break;
            }
        }
        node->integrated = true;

        // Queue up any adjacent nodes not yet part of the maze.
        for(Node* adjacentNode : node->adjacentNodeArray)
        {
            if(!adjacentNode->queued && !adjacentNode->integrated)
            {
                nodeQueue.push_back(adjacentNode);
                adjacentNode->queued = true;
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
    engine->Clear();

    PlanarPhysics::BoundingBox worldBox;
    worldBox.min = Vector2D(-this->cellWidthCM / 2.0, -this->cellHeightCM / 2.0);
    worldBox.max = Vector2D(this->widthCM + this->cellWidthCM / 2.0, this->heightCM + this->cellHeightCM / 2.0);
    engine->SetWorldBox(worldBox);

    for(const Node* node : this->nodeArray)
    {
        node->GenerateWalls(engine, this);
    }

    MazeWall* mazeWallLeft = engine->AddPlanarObject<MazeWall>();
    mazeWallLeft->lineSeg.vertexA = Vector2D(0.0, 0.0);
    mazeWallLeft->lineSeg.vertexB = Vector2D(0.0, this->heightCM);

    MazeWall* mazeWallRight = engine->AddPlanarObject<MazeWall>();
    mazeWallRight->lineSeg.vertexA = Vector2D(this->widthCM, 0.0);
    mazeWallRight->lineSeg.vertexB = Vector2D(this->widthCM, this->heightCM);

    MazeWall* mazeWallBottom = engine->AddPlanarObject<MazeWall>();
    mazeWallBottom->lineSeg.vertexA = Vector2D(0.0, 0.0);
    mazeWallBottom->lineSeg.vertexB = Vector2D(this->widthCM, 0.0);

    MazeWall* mazeWallTop = engine->AddPlanarObject<MazeWall>();
    mazeWallTop->lineSeg.vertexA = Vector2D(0.0, this->heightCM);
    mazeWallTop->lineSeg.vertexB = Vector2D(this->widthCM, this->heightCM);
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
    this->queued = false;
    this->integrated = false;
    this->debugName[0] = '\0';
}

/*virtual*/ Maze::Node::~Node()
{
}

bool Maze::Node::IsConnectedTo(const Node* node) const
{
    for(const Node* connectedNode : this->connectedNodeArray)
        if(connectedNode == node)
            return true;

    return false;
}

void Maze::Node::GenerateWalls(PlanarPhysics::Engine* engine, const Maze* maze) const
{
    for(const Node* adjacentNode : this->adjacentNodeArray)
    {
        if(this->IsConnectedTo(adjacentNode))
            continue;

        Vector2D wallCenter = (this->center + adjacentNode->center) / 2.0;
        Vector2D wallNormal = (adjacentNode->center - this->center).Normalized();
        Vector2D wallTangent = wallNormal * PScalar2D(1.0);
        double wallSize = 0.0;
        if(::abs(wallNormal.x) > ::abs(wallNormal.y))
            wallSize = maze->cellHeightCM;
        else
            wallSize = maze->cellWidthCM;

        LineSegment wallSegment(wallCenter + wallTangent * wallSize / 2.0, wallCenter - wallTangent * wallSize / 2.0);
        if(this->WallAlreadyExists(wallSegment, engine))
            continue;

        MazeWall* mazeWall = engine->AddPlanarObject<MazeWall>();
        mazeWall->lineSeg = wallSegment;
    }
}

bool Maze::Node::WallAlreadyExists(const PlanarPhysics::LineSegment& wallSegment, PlanarPhysics::Engine* engine) const
{
    const std::vector<PlanarObject*>& planarObjectArray = engine->GetPlanarObjectArray();

    for(const PlanarObject* planarObject : planarObjectArray)
    {
        auto wall = dynamic_cast<const Wall*>(planarObject);
        if(wall)
        {
            if(wall->lineSeg.SameGeometryAs(wallSegment))
                return true;
        }
    }

    return false;
}