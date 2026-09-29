#pragma once

#include "common.hpp"

// A2: simple A-star node
struct Node {
	uint G, H, F;
	ivec2 position;
	ivec2 parent; // remember where we come from

	Node(){}; // default constructor

	Node(ivec2 position_, ivec2 parent_, uint g_, uint h_) : position(position_), parent(parent_), G(g_), H(h_) { F = G + H; };

	const uint get_score() const { return G + H; }
	
	bool operator==(const Node& rhs) { return this->position == rhs.position; }

	bool operator==(const ivec2& pos) { return this->position == pos; }

	// bool operator<(const Node& rhs) { return this->get_score() < rhs.get_score(); } 
};

