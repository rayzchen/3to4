/**************************************************************************
 * 3to4++ - https://github.com/rayzchen/3to4++
 *-------------------------------------------------------------------------
 * Copyright 2024 Ray Chen
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 **************************************************************************/

#ifndef CONTROL_H
#define CONTROL_H

#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>
#include "render.h"
#include "puzzle.h"

class PuzzleController {
	public:
		PuzzleController(PuzzleRenderer* renderer);
		void updatePuzzle(GLFWwindow* window, double dt);
        bool checkMiddleGyro(GLFWwindow* window);
        bool checkDirectionalMove(GLFWwindow* window);
        void startGyro(CellLocation cell);
        bool checkCellKeys(GLFWwindow* window, CellLocation* cell);
        bool checkDirectionKeys(GLFWwindow* window, RotateDirection* direction);
        void startCellMove(CellLocation cell, RotateDirection direction);

	    static int cellKeys[];
    	static int directionKeys[];

	private:
		PuzzleRenderer *renderer;
		Puzzle *puzzle;
};

#endif // control.h
