#pragma once
#ifndef INDICES_H
#define INDICES_H

typedef unsigned short WORD;

WORD cubeIndices[] =
{
	3, 1, 0,
	2, 1, 3,

	6, 4, 5,
	7, 4, 6,

	11, 9, 8,
	10, 9, 11,

	14, 12, 13,
	15, 12, 14,

	19, 17, 16,
	18, 17, 19,

	22, 20, 21,
	23, 20, 22
};

WORD planeIndices[] =
{
	0, 3, 1,
	3, 2, 1,
};

#endif