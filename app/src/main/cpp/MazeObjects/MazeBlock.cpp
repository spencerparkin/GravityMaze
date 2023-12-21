#include "MazeBlock.h"

MazeBlock::MazeBlock()
{
}

/*virtual*/ MazeBlock::~MazeBlock()
{
}

/*static*/ MazeBlock* MazeBlock::Create()
{
    return new MazeBlock();
}

/*virtual*/ void MazeBlock::Render(DrawHelper& drawHelper) const
{

}