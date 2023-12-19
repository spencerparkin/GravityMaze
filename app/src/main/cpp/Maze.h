#pragma once

#include "Engine.h"
#include "Math/GeometricAlgebra/Vector2D.h"
#include <vector>

class Maze
{
public:
    Maze();
    virtual ~Maze();

    bool Generate(double widthCM, double heightCM, double densityCMPerCell);
    void PopulatePhysicsWorld(PlanarPhysics::Engine* engine) const;
    void Clear();

private:
    class Node
    {
    public:
        Node();
        virtual ~Node();

        std::vector<Node*> adjacentNodeArray;
        std::vector<Node*> connectedNodeArray;

        PlanarPhysics::Vector2D center;
        bool partOfMaze;
    };

    int RandomNumber(int min, int max);
    Node* RandomNode(std::vector<Node*>& nodeArray, int* lastRandom = nullptr);
    Node* RandomNode(std::list<Node*>& nodeList, bool remove);

    std::vector<Node*> nodeArray;

    double widthCM;
    double heightCM;
};
