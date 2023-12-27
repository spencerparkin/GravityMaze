#include "Maze.h"
#include "MazeObjects/MazeWall.h"
#include "MazeObjects/MazeBall.h"
#include "MazeObjects/MazeBlock.h"
#include "Engine.h"
#include "Math/Utilities/BoundingBox.h"
#include "Math/GeometricAlgebra/PScalar2D.h"
#include "Math/Utilities/Random.h"
#include <math.h>
#include <stdlib.h>
#include <list>
#include <set>

using namespace PlanarPhysics;

//--------------------------------- Maze ---------------------------------

Maze::Maze()
{
    this->rows = 0;
    this->cols = 0;
}

/*virtual*/ Maze::~Maze()
{
    this->Clear();
}

bool Maze::Generate(int rows, int cols)
{
    this->Clear();

    this->rows = rows;
    this->cols = cols;

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
            node->center.x = double(j) * MAZE_CELL_SIZE + MAZE_CELL_SIZE / 2.0;
            node->center.y = double(i) * MAZE_CELL_SIZE + MAZE_CELL_SIZE / 2.0;
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

Maze::Node* Maze::RandomNode(std::vector<Node*>& nodeArray, int* lastRandom /*= nullptr*/)
{
    if(lastRandom && *lastRandom >= 0)
    {
        *lastRandom = (*lastRandom + 1) % nodeArray.size();
        return nodeArray[*lastRandom];
    }

    int i = Random::Integer(0, nodeArray.size() - 1);
    Node* node = nodeArray[i];

    if(lastRandom)
        *lastRandom = i;

    return node;
}

Maze::Node* Maze::RandomNode(std::list<Node*>& nodeList, bool remove)
{
    int i = Random::Integer(0, nodeList.size() - 1);
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

    PlanarPhysics::BoundingBox mazeBox;
    mazeBox.min = Vector2D(0.0, 0.0);
    mazeBox.max = Vector2D(0.0, 0.0);
    for(const Node* node : this->nodeArray)
    {
        mazeBox.ExpandToIncludePoint(node->center + Vector2D(MAZE_CELL_SIZE / 2.0, MAZE_CELL_SIZE / 2.0));
        mazeBox.ExpandToIncludePoint(node->center - Vector2D(MAZE_CELL_SIZE / 2.0, MAZE_CELL_SIZE / 2.0));
    }

    mazeBox.min.x -= 5.0;
    mazeBox.max.x += 5.0;
    mazeBox.min.y -= 5.0;
    mazeBox.max.y += 5.0;

    engine->SetWorldBox(mazeBox);

    for(const Node* node : this->nodeArray)
        node->GenerateWalls(engine, this);

    double mazeWidth = MAZE_CELL_SIZE * this->cols;
    double mazeHeight = MAZE_CELL_SIZE * this->rows;

    MazeWall* mazeWallLeft = engine->AddPlanarObject<MazeWall>();
    mazeWallLeft->lineSeg.vertexA = Vector2D(0.0, 0.0);
    mazeWallLeft->lineSeg.vertexB = Vector2D(0.0, mazeHeight);

    MazeWall* mazeWallRight = engine->AddPlanarObject<MazeWall>();
    mazeWallRight->lineSeg.vertexA = Vector2D(mazeWidth, 0.0);
    mazeWallRight->lineSeg.vertexB = Vector2D(mazeWidth, mazeHeight);

    MazeWall* mazeWallBottom = engine->AddPlanarObject<MazeWall>();
    mazeWallBottom->lineSeg.vertexA = Vector2D(0.0, 0.0);
    mazeWallBottom->lineSeg.vertexB = Vector2D(mazeWidth, 0.0);

    MazeWall* mazeWallTop = engine->AddPlanarObject<MazeWall>();
    mazeWallTop->lineSeg.vertexA = Vector2D(0.0, mazeHeight);
    mazeWallTop->lineSeg.vertexB = Vector2D(mazeWidth, mazeHeight);

    MazeBall* mazeBall = engine->AddPlanarObject<MazeBall>();
    mazeBall->position = this->nodeArray[0]->center;
    mazeBall->radius = MAZE_CELL_SIZE / 3.0;
    mazeBall->color = Color(0.0, 1.0, 0.0);
    mazeBall->SetFlags(PLNR_OBJ_FLAG_INFLUENCED_BY_GRAVITY | PLNR_OBJ_FLAG_GEN_COLLISION_EVENTS);

    int numMazeBlocks = this->cols - 1;
    std::unordered_set<int> occupiedCellsSet;
    occupiedCellsSet.insert(0);
    for(int i = 0; i < numMazeBlocks; i++)
    {
        MazeBlock* mazeBlock = engine->AddPlanarObject<MazeBlock>();
        int j = 0;
        while(occupiedCellsSet.find(j) != occupiedCellsSet.end())
            j = Random::Integer(1, this->nodeArray.size() - 1);
        occupiedCellsSet.insert(j);
        mazeBlock->position = this->nodeArray[j]->center;
        mazeBlock->color = Color(1.0, 0.0, 0.0);
        mazeBlock->SetFlags(PLNR_OBJ_FLAG_INFLUENCED_BY_GRAVITY | PLNR_OBJ_FLAG_GEN_COLLISION_EVENTS);

        std::vector<Vector2D> pointArray;
        double radius = MAZE_CELL_SIZE / 6.0;
        int k = Random::Integer(3, 5);
        for(j = 0; j < k; j++)
        {
            double angle = (double(j) / double(k)) * 2.0 * PLNR_PHY_PI;
            Vector2D vertex(radius * ::cos(angle), radius * ::sin(angle));
            pointArray.push_back(vertex);
        }

        mazeBlock->MakeShape(pointArray, 1.0);
    }

    engine->ConsolidateWalls();
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

        LineSegment wallSegment(wallCenter + wallTangent * MAZE_CELL_SIZE / 2.0, wallCenter - wallTangent * MAZE_CELL_SIZE / 2.0);
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