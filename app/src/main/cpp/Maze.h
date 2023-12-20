#pragma once

#include "Engine.h"
#include "Math/GeometricAlgebra/Vector2D.h"
#include "Math/Utilities/LineSegment.h"
#include <vector>

class Maze
{
    friend class Node;

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

        bool IsConnectedTo(const Node* node) const;
        void GenerateWalls(PlanarPhysics::Engine* engine, const Maze* maze) const;
        bool WallAlreadyExists(const PlanarPhysics::LineSegment& wallSegment, PlanarPhysics::Engine* engine) const;

        PlanarPhysics::Vector2D center;
        bool queued;
        bool integrated;
        char debugName[128];
    };

    int RandomNumber(int min, int max);
    Node* RandomNode(std::vector<Node*>& nodeArray, int* lastRandom = nullptr);
    Node* RandomNode(std::list<Node*>& nodeList, bool remove);

    std::vector<Node*> nodeArray;

    double widthCM;
    double heightCM;
    double cellWidthCM;
    double cellHeightCM;
};