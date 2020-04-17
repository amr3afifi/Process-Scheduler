#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <limits.h>

struct memblock
{
	int value;
	int fill;
	struct memblock *left;
	struct memblock *right;
};

/*newSplit() allocates a new memblock with the given value and NULL left and  
   right pointers. */
struct memblock *newSplit(int value)
{
	struct memblock *memblock = (struct memblock *)malloc(sizeof(struct memblock));

	// Assign value to this memblock
	memblock->value = value;
	memblock->fill = 0;

	// Initialize left and right children as NULL
	memblock->left = NULL;
	memblock->right = NULL;
	return (memblock);
}

struct memblock *CreateMemTree()
{
	struct memblock *root = newSplit(1024);
	return root;
}

int roundUp(int in)
{
	int base2 = ceil(log(in) / log(2));
	base2 = pow(2, base2);
	return base2;
}

int createAlloc(int input, struct memblock *currentNode)
{
	if (!(input <= (currentNode->value) / 2))
	{
		currentNode->fill = currentNode->value; //find smallest area to split and insert
		return 0;
	}

	if (currentNode->left == NULL)
	{
		currentNode->left = newSplit(currentNode->value / 2);
		currentNode->right = newSplit(currentNode->value / 2);
		int retVal = createAlloc(input, currentNode->left);
		currentNode->fill += input;
		return retVal;
	}
	else
	{
		if (!(currentNode->left->fill + input > currentNode->left->value))
		{
			int retVal2 = createAlloc(input, currentNode->left);
			if (retVal2 != -1)
			{
				currentNode->fill += input;
				return retVal2;
			}
		}
	}

	if (!(currentNode->right->fill + input > currentNode->right->value))
	{
		int retVal4 = createAlloc(input, currentNode->right);
		if (retVal4 != -1)
		{
			currentNode->fill += input;
			return retVal4 + currentNode->value / 2;
		}
	}

	return -1;
}

void deallocatemem(int address, int size, struct memblock *currentNode)
{
	size = roundUp(size);
	if (size < currentNode->value / 2)
	{
		if (address < currentNode->value / 2)
		{
			deallocatemem(address, size, currentNode->left);
		}
		else
		{
			deallocatemem((address - currentNode->value / 2), size, currentNode->right);
		}
	}
	else
	{
		if (address == 0)
		{
			currentNode->left->fill = 0;
			if (currentNode->right->fill == 0)
			{
				free(currentNode->right);
				currentNode->right = NULL;
				free(currentNode->left);
				currentNode->left = NULL;
			}
		}
		else
		{
			currentNode->right->fill = 0;
			if (currentNode->left->fill == 0)
			{
				free(currentNode->right);
				currentNode->right = NULL;
				free(currentNode->left);
				currentNode->left = NULL;
			}
		}
	}

	currentNode->fill -= size;
}

int lookForInsert(int input, struct memblock *currentNode, int realInput)
{
	if (!(input <= (currentNode->value) / 2)) //exit condition, this memblock is of suitable size
	{

		return createAlloc(realInput, currentNode);
	}

	if (currentNode->left == NULL)
		return -1; //if there is no left child then there also isn't a right child, failed to insert
	else
	{
		if (!(currentNode->left->fill + input > currentNode->left->value)) //check if suitable amount of space is left in this segment
		{
			int retVal2 = lookForInsert(input, currentNode->left, realInput);
			if (retVal2 != -1)
			{
				currentNode->fill += realInput;
				return retVal2;
			}
		}
	}

	if (!(currentNode->right->fill + input > currentNode->right->value))
	{
		int retVal4 = lookForInsert(input, currentNode->right, realInput);
		if (retVal4 != -1)
		{
			currentNode->fill += realInput;
			return retVal4 + currentNode->value / 2;
		}
	}

	return -1;
}

int getmem(int input, struct memblock *currentNode)
{
	int address = 0;
	input = roundUp(input);
	int input2 = input;
	while (input2 <= 1024)
	{

		address = lookForInsert(input2, currentNode, input);
		if (address == -1)
		{
			//printf("1111-------------%d--------- p%d\n", input2, address);
			input2 = input2 * 2;
			//printf("22222------------%d--------- p%d\n", input2, address);
		}
		else
			break;
	}
	return address;
}
