#pragma once

#include "Engine.h"
#include "Math/GeometricAlgebra/Vector2D.h"
#include "Math/Utilities/LineSegment.h"
#include <vector>

// Mazes can very in size in terms of rows and columns, but the cell
// size should always remain the same so that the physics can work
// properly as it has been calibrated.  No matter how big or small
// the maze is, we can always render it to fit the screen.
#define MAZE_CELL_SIZE       40.0

// TODO: It wouldn't be too hard to make mazes with different geometries; e.g., those
//       that aren't necessarily rectangular, but, say, honey-comb shapes or circular.
class Maze
{
    friend class Node;

public:
    Maze();
    virtual ~Maze();

    bool Generate(int rows, int cols);
    void PopulatePhysicsWorld(PlanarPhysics::Engine* engine, int touches) const;
    void Clear();

private:
    class Node
    {
    public:
        Node();
        virtual ~Node();

        std::vector<Node*> adjacentNodeArray;
        std::vector<Node*> connectedNodeArray;

        bool IsConnectedTo(const Node* node) const;
        void GenerateWalls(PlanarPhysics::Engine* engine, const Maze* maze) const;
        bool WallAlreadyExists(const PlanarPhysics::LineSegment& wallSegment, PlanarPhysics::Engine* engine) const;

        PlanarPhysics::Vector2D center;
        bool queued;
        bool integrated;
        char debugName[128];
    };

    Node* RandomNode(std::vector<Node*>& nodeArray, int* lastRandom = nullptr);
    Node* RandomNode(std::list<Node*>& nodeList, bool remove);

    std::vector<Node*> nodeArray;

    int rows, cols;
};