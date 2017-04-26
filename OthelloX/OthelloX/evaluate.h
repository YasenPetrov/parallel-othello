#pragma once
#ifndef EVALUATE_H
#define EVALUATE_H
#endif // !EVALUATE_H

#include "stdafx.h"
#include "general.h"
#include "board.h"

// Evaluates a board in an intermediate state
// max - it is MAX's turn?
int evalBoard(const board &state);

// Generate coefficient matrix
void fillWeightsMatrix(board &matrix);