#include "arl.h"

screen_coord screen_size = { 20, 20 };
screen_coord charPos = { 9, 9 };

internal
bool isOutOfBounds(screen_coord* pos)
{
	if (pos->x < 0)
	{
		pos->x = 0;
		return true;
	}

	if (pos->y < 0)
	{
		pos->y = 0;
		return true;
	}
	
	if (pos->y > screen_size.y)
	{
		pos->y = screen_size.y;
		return true;
	}

	if (pos->x > screen_size.x)
	{
		pos->x = screen_size.x;
		return true;
	}

	return false;
}

internal
screen_coord toScreenCoord(screen_coord pos)
{
	short xOffset = (80 - screen_size.x) / 2;
	short yOffset = (25 - screen_size.y) / 2;

	return { pos.x + xOffset, pos.y + yOffset };
}

void
updateAndRender()
{
	if (isOutOfBounds(&charPos))
	{
		drawToBuffer("You bumped into a wall");
	}

	drawCharAt(toScreenCoord(charPos), '@');
}

void
processInput(const game_input input)
{
	charPos.x += input.xOffset;
	charPos.y += input.yOffset;
}