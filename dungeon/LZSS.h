#pragma once
#include "BitIO.h"
#include <cstring>
#include <sstream>
#include <vector>

constexpr int INDEX_BIT_COUNT = 12; //search buffer
constexpr int LENGTH_BIT_COUNT = 4; //look ahead buffer
constexpr int WINDOW_SIZE = (1 << INDEX_BIT_COUNT);
constexpr int LOOK_AHEAD_SIZE = (1 << LENGTH_BIT_COUNT);
constexpr int BYTE = 8;
constexpr int TREE_ROOT = WINDOW_SIZE;
constexpr int UNUSED = -1;
constexpr int END_OF_STREAM = 0;
constexpr int BREAK_EVEN = (1 + INDEX_BIT_COUNT + LENGTH_BIT_COUNT) / (1 + BYTE);

#define MOD_WINDOW(value) ((value) & (WINDOW_SIZE - 1))

struct Tree {
	int parent{ UNUSED };
	int largerChild{ UNUSED };
	int smallerChild{ UNUSED };
};

std::vector<unsigned char> window(WINDOW_SIZE);
std::vector<Tree> tree(WINDOW_SIZE + 1);

void contractNode(int oldNode, int newNode) {
	tree[newNode].parent = tree[oldNode].parent;
	if (tree[tree[oldNode].parent].largerChild == oldNode)
		tree[tree[oldNode].parent].largerChild = newNode;
	else
		tree[tree[oldNode].parent].smallerChild = newNode;
	tree[oldNode].parent = UNUSED;
}

int findNextNode(int node) {
	int next = tree[node].smallerChild;
	while (tree[next].largerChild != UNUSED)
		next = tree[next].largerChild;
	return next;
}

void replaceNode(int oldNode, int newNode) {
	int parent = tree[oldNode].parent;
	if (tree[parent].smallerChild == oldNode)
		tree[parent].smallerChild = newNode;
	else
		tree[parent].largerChild = newNode;
	tree[newNode] = tree[oldNode];
	tree[tree[newNode].smallerChild].parent = newNode;
	tree[tree[newNode].largerChild].parent = newNode;
	tree[oldNode].parent = UNUSED;
}

void deleteString(int position) {
	if (tree[position].parent == UNUSED)
		return;
	if (tree[position].largerChild == UNUSED)
		contractNode(position, tree[position].smallerChild);
	else if (tree[position].smallerChild == UNUSED)
		contractNode(position, tree[position].largerChild);
	else {
		int replacementPosition = findNextNode(position);
		deleteString(replacementPosition);
		replaceNode(position, replacementPosition);
	}
}

void addString(int stringPosition) {
	//printf("%c", window[stringPosition]);
	int i{ 0 }, testNode{ 0 }, delta{ 0 }, * child{ nullptr };
	if (tree[TREE_ROOT].largerChild == UNUSED) {
		tree[TREE_ROOT].largerChild = stringPosition;
		tree[stringPosition].parent = TREE_ROOT;
		tree[stringPosition].largerChild = UNUSED;
		tree[stringPosition].smallerChild = UNUSED;
	}
	else {
		testNode = tree[TREE_ROOT].largerChild;
		for (;;) {
			for (i = 0; i < LOOK_AHEAD_SIZE; ++i) {
				delta = window[MOD_WINDOW(stringPosition + i)] - window[MOD_WINDOW(testNode + i)];
				if (delta != 0)
					break;
			}
			if (delta == 0) {
				replaceNode(testNode, stringPosition);
				break;
			}
			else if (delta > 0)
				child = &tree[testNode].largerChild;
			else
				child = &tree[testNode].smallerChild;
			if (*child == UNUSED) {
				*child = stringPosition;
				tree[stringPosition].parent = testNode;
				tree[stringPosition].largerChild = UNUSED;
				tree[stringPosition].smallerChild = UNUSED;
				break;
			}
			testNode = *child;
		}
	}
}

int getMatchLength(int currentPosition, int* matchPosition) {
	*matchPosition = 0;
	int i{ 0 }, testNode{ 0 }, delta{ 0 }, matchLength{ 0 }, * child{ nullptr };
	testNode = tree[TREE_ROOT].largerChild;
	for (;;) {
		for (i = 0; i < LOOK_AHEAD_SIZE; i++) {
			delta = window[MOD_WINDOW(currentPosition + i)] - window[MOD_WINDOW(testNode + i)];
			if (delta != 0)break;
		}
		if (i > matchLength) {
			matchLength = i;
			*matchPosition = testNode;
		}
		if (delta == 0)
			break;
		else if (delta > 0)
			child = &tree[testNode].largerChild;
		else
			child = &tree[testNode].smallerChild;
		if (*child == UNUSED)
			break;
		testNode = *child;
	}
	return matchLength;
}

void compressFile(std::stringstream& input, std::unique_ptr<stl::BitFile>& output) {
	int i{ 0 }, c{ 0 }, lookAheadBytes{ 0 }, currentPosition{ 0 }, replaceCount{ 0 },
		matchLength{ 0 }, matchPosition{ 0 };
	for (i = 0; i < LOOK_AHEAD_SIZE; i++) {
		c = input.get();
		if (input.eof())
			break;
		window[currentPosition + i] = (unsigned char)c;
	}
	lookAheadBytes = i;
	while (lookAheadBytes > 0) {
		if (matchLength >= lookAheadBytes)
			matchLength = lookAheadBytes - 1;
		if (matchLength <= BREAK_EVEN) {
			matchLength = 1;
			stl::outputBit(output, 0);
			stl::outputBits(output, (uint32_t)window[currentPosition], BYTE);
		}
		else {
			stl::outputBit(output, 1);
			stl::outputBits(output, (uint32_t)matchPosition, INDEX_BIT_COUNT);
			stl::outputBits(output, (uint32_t)(matchLength), LENGTH_BIT_COUNT);
		}
		replaceCount = matchLength;
		for (i = 0; i < replaceCount; ++i) {
			deleteString(MOD_WINDOW(currentPosition + LOOK_AHEAD_SIZE));
			c = input.get();
			if (input.eof())
				--lookAheadBytes;
			else
				window[MOD_WINDOW(currentPosition + LOOK_AHEAD_SIZE)] = (unsigned char)c;
			addString(currentPosition);
			currentPosition = MOD_WINDOW(currentPosition + 1);
			//std::cout << matchPosition++ << ":" << matchLength << "\n";
		}
		if (lookAheadBytes)
			matchLength = getMatchLength(currentPosition, &matchPosition);
	}
	stl::outputBit(output, 1);
	stl::outputBits(output, (uint32_t)END_OF_STREAM, INDEX_BIT_COUNT + LENGTH_BIT_COUNT);
}

void expandFile(std::unique_ptr<stl::BitFile>& input, std::stringstream& output) {
	int i{ 0 }, currentPosition{ 0 }, c{ 0 }, matchLength{ 0 }, matchPosition{ 0 };
	currentPosition = 0;
	for (;;) {
		if (stl::inputBit(input) == 0) {
			c = (int)stl::inputBits(input, BYTE);
			output.put(c);
			window[currentPosition] = (unsigned char)c;
			currentPosition = MOD_WINDOW(currentPosition + 1);
		}
		else {
			matchPosition = (int)stl::inputBits(input, INDEX_BIT_COUNT);
			matchLength = (int)stl::inputBits(input, LENGTH_BIT_COUNT);
			if (matchLength == END_OF_STREAM)
				break;
			for (i = 0; i < matchLength; i++) {
				c = window[MOD_WINDOW(matchPosition + i)];
				output.put(c);
				window[currentPosition] = (unsigned char)c;
				currentPosition = MOD_WINDOW(currentPosition + 1);
			}
		}
	}
}
